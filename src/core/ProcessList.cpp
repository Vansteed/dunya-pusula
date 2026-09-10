#include "ProcessList.h"
#include "MemoryCleaner.h"

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <algorithm>
#include <unordered_map>

namespace ProcessList {

std::vector<ProcKaydi> yenile()
{
    struct Ham { std::string ad; uint32_t pid; int64_t bayt; std::string tamYol; };
    std::vector<Ham> hamListe;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return {};

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            // GetProcessMemoryInfo, PROCESS_QUERY_LIMITED_INFORMATION tek basina
            // yetmiyor - PROCESS_VM_READ da gerekli, yoksa cagri sessizce
            // basarisiz olur ve neredeyse tum islemler atlanir.
            HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                                   FALSE, pe.th32ProcessID);
            if (!h) continue;

            PROCESS_MEMORY_COUNTERS pmc;
            int64_t ws = 0;
            if (GetProcessMemoryInfo(h, &pmc, sizeof(pmc))) {
                ws = static_cast<int64_t>(pmc.WorkingSetSize);
            }
            if (ws <= 0) { CloseHandle(h); continue; }

            std::string tamYol;
            WCHAR yolBuf[MAX_PATH];
            DWORD yolBoyut = MAX_PATH;
            if (QueryFullProcessImageNameW(h, 0, yolBuf, &yolBoyut)) {
                std::wstring wyol(yolBuf, yolBoyut);
                tamYol.assign(wyol.begin(), wyol.end());
            }
            CloseHandle(h);

            std::wstring wname(pe.szExeFile);
            std::string name(wname.begin(), wname.end());
            const auto dot = name.rfind('.');
            if (dot != std::string::npos) name = name.substr(0, dot);

            hamListe.push_back({name, pe.th32ProcessID, ws, tamYol});
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    struct Grup {
        std::string ad;       // ilk gorulen orijinal buyuk/kucuk hal
        int64_t toplamBayt = 0;
        std::vector<ProcAlt> altlar;
        std::string tamYol;   // ilk bulunan
    };
    std::unordered_map<std::string, Grup> gruplar;
    for (const auto &h : hamListe) {
        std::string kucuk = h.ad;
        std::transform(kucuk.begin(), kucuk.end(), kucuk.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        auto it = gruplar.find(kucuk);
        if (it == gruplar.end()) {
            Grup g;
            g.ad = h.ad;
            g.toplamBayt = h.bayt;
            g.altlar.push_back({h.pid, h.bayt / (1024 * 1024), h.tamYol});
            g.tamYol = h.tamYol;
            gruplar.emplace(kucuk, std::move(g));
        } else {
            it->second.toplamBayt += h.bayt;
            it->second.altlar.push_back({h.pid, h.bayt / (1024 * 1024), h.tamYol});
            if (it->second.tamYol.empty()) it->second.tamYol = h.tamYol;
        }
    }

    std::vector<Grup> siraliGruplar;
    siraliGruplar.reserve(gruplar.size());
    for (auto &kv : gruplar) siraliGruplar.push_back(std::move(kv.second));

    std::sort(siraliGruplar.begin(), siraliGruplar.end(),
              [](const Grup &a, const Grup &b) { return a.toplamBayt > b.toplamBayt; });

    if (siraliGruplar.size() > 25) siraliGruplar.resize(25);

    std::vector<ProcKaydi> sonuc;
    sonuc.reserve(siraliGruplar.size());
    for (const auto &g : siraliGruplar) {
        ProcKaydi k;
        k.ad = g.ad;
        k.mb = g.toplamBayt / (1024 * 1024);
        k.altlar = g.altlar;
        std::sort(k.altlar.begin(), k.altlar.end(),
                  [](const ProcAlt &a, const ProcAlt &b) { return a.mb > b.mb; });
        k.tamYol = g.tamYol;
        std::string kucuk = g.ad;
        std::transform(kucuk.begin(), kucuk.end(), kucuk.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        k.korumali = MemoryCleaner::korumaliMi(kucuk);
        k.durum = k.korumali ? "Sistem - Korumalı" : "Kapatılabilir";
        sonuc.push_back(std::move(k));
    }
    return sonuc;
}

namespace {

DWORD sinifaCevir(int sinif)
{
    switch (sinif) {
    case 0: return IDLE_PRIORITY_CLASS;
    case 1: return BELOW_NORMAL_PRIORITY_CLASS;
    case 2: return NORMAL_PRIORITY_CLASS;
    case 3: return ABOVE_NORMAL_PRIORITY_CLASS;
    case 4: return HIGH_PRIORITY_CLASS;
    default: return 0; // gecersiz - REALTIME dahil hicbiri desteklenmez
    }
}

// PID'in olusturma zamani - cocuk sureci PID yeniden kullanimindan ayirt
// etmek icin kullanilir. Alinamazsa sifir FILETIME doner.
FILETIME olusturmaZamani(uint32_t pid)
{
    FILETIME olusturma{}, cikis{}, cekirdek{}, kullanici{};
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return olusturma;
    GetProcessTimes(h, &olusturma, &cikis, &cekirdek, &kullanici);
    CloseHandle(h);
    return olusturma;
}

bool sifirMi(const FILETIME &f)
{
    return f.dwLowDateTime == 0 && f.dwHighDateTime == 0;
}

bool oncekiMi(const FILETIME &a, const FILETIME &b) // a, b'den ONCE mi basladi?
{
    ULARGE_INTEGER ua; ua.LowPart = a.dwLowDateTime; ua.HighPart = a.dwHighDateTime;
    ULARGE_INTEGER ub; ub.LowPart = b.dwLowDateTime; ub.HighPart = b.dwHighDateTime;
    return ua.QuadPart < ub.QuadPart;
}

// Verilen ebeveynin TUM alt sureclerini (ozyinelemeli, once cocuklar) sonuc'a
// ekler. Ebeveynden ONCE baslamis "cocuklar" (PID yeniden kullanimi) atlanir.
void altSurecleriTopla(uint32_t ebeveynPid, const FILETIME &ebeveynZamani,
                        std::vector<uint32_t> &sonuc)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;

    std::vector<uint32_t> cocuklar;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (pe.th32ParentProcessID != ebeveynPid) continue;
            const FILETIME cocukZamani = olusturmaZamani(pe.th32ProcessID);
            if (sifirMi(cocukZamani)) continue;              // artik yasamiyor
            if (oncekiMi(cocukZamani, ebeveynZamani)) continue; // PID yeniden kullanimi
            cocuklar.push_back(pe.th32ProcessID);
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    for (uint32_t cocukPid : cocuklar) {
        altSurecleriTopla(cocukPid, olusturmaZamani(cocukPid), sonuc);
        sonuc.push_back(cocukPid);
    }
}

}

bool oncelikAta(uint32_t pid, int sinif)
{
    const DWORD win32Sinif = sinifaCevir(sinif);
    if (win32Sinif == 0) return false;

    HANDLE h = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
    if (!h) return false;
    const bool basarili = SetPriorityClass(h, win32Sinif);
    CloseHandle(h);
    return basarili;
}

int oncelikOku(uint32_t pid)
{
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return -1;
    const DWORD sinif = GetPriorityClass(h);
    CloseHandle(h);

    switch (sinif) {
    case IDLE_PRIORITY_CLASS: return 0;
    case BELOW_NORMAL_PRIORITY_CLASS: return 1;
    case NORMAL_PRIORITY_CLASS: return 2;
    case ABOVE_NORMAL_PRIORITY_CLASS: return 3;
    case HIGH_PRIORITY_CLASS: return 4;
    default: return -1; // REALTIME dahil bilinmeyenler - "-" gosterilir
    }
}

bool agaciKapat(uint32_t pid)
{
    const FILETIME ebeveynZamani = olusturmaZamani(pid);
    std::vector<uint32_t> hedefler;
    altSurecleriTopla(pid, ebeveynZamani, hedefler);
    hedefler.push_back(pid); // ebeveyn EN SON kapatilir

    bool hepsiBasarili = true;
    for (uint32_t hedefPid : hedefler) {
        HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, hedefPid);
        if (h) {
            if (!TerminateProcess(h, 0)) hepsiBasarili = false;
            CloseHandle(h);
        } else {
            hepsiBasarili = false;
        }
    }
    return hepsiBasarili;
}

int agacUyeSayisi(uint32_t pid)
{
    std::vector<uint32_t> altlar;
    altSurecleriTopla(pid, olusturmaZamani(pid), altlar);
    return static_cast<int>(altlar.size()) + 1; // +1 kendisi
}

}
