#include "SysInfo.h"

#include <windows.h>
#include <cmath>
#include <cstdlib>

namespace {

double yuvarla(double deger, int basamak)
{
    const double carpan = std::pow(10.0, basamak);
    return std::round(deger * carpan) / carpan;
}

std::string sistemSurucusu()
{
    char buf[16] = {0};
    size_t len = 0;
    if (getenv_s(&len, buf, sizeof(buf), "SystemDrive") != 0 || len == 0) {
        return "C:\\";
    }
    std::string s(buf);
    if (s.empty() || s.back() != '\\') s += '\\';
    return s;
}

}

namespace SysInfo {

RamBilgisi ramBilgisi()
{
    RamBilgisi r;
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        const double totalB = static_cast<double>(ms.ullTotalPhys);
        const double freeB = static_cast<double>(ms.ullAvailPhys);
        const double usedB = totalB - freeB;
        const double gib = 1024.0 * 1024.0 * 1024.0;
        r.totalGB = yuvarla(totalB / gib, 2);
        r.usedGB = yuvarla(usedB / gib, 2);
        r.freeGB = yuvarla(freeB / gib, 2);
        r.percent = totalB > 0 ? yuvarla((usedB / totalB) * 100.0, 1) : 0.0;
    }
    return r;
}

DiskBilgisi diskKullanim(const std::string &surucuKoku)
{
    DiskBilgisi d;
    const std::string kok = surucuKoku.empty() ? sistemSurucusu() : surucuKoku;
    d.harf = kok.substr(0, 2); // "C:"

    ULARGE_INTEGER bosBayt{}, toplamBayt{}, tumBosBayt{};
    if (GetDiskFreeSpaceExW(std::wstring(kok.begin(), kok.end()).c_str(),
                             &bosBayt, &toplamBayt, &tumBosBayt)) {
        const double toplam = static_cast<double>(toplamBayt.QuadPart);
        const double bos = static_cast<double>(tumBosBayt.QuadPart);
        const double kullanilan = toplam - bos;
        const double gib = 1024.0 * 1024.0 * 1024.0;
        d.toplamGB = yuvarla(toplam / gib, 2);
        d.kullanilanGB = yuvarla(kullanilan / gib, 2);
        d.bosGB = yuvarla(bos / gib, 2);
        d.yuzde = toplam > 0 ? yuvarla((kullanilan / toplam) * 100.0, 1) : 0.0;
    }
    return d;
}

int baslangicOgeSayisi()
{
    int toplam = 0;
    auto sayHkeyDegerleri = [&toplam](HKEY kok, const wchar_t *yol) {
        HKEY h;
        if (RegOpenKeyExW(kok, yol, 0, KEY_READ, &h) == ERROR_SUCCESS) {
            DWORD degerSayisi = 0;
            if (RegQueryInfoKeyW(h, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                  &degerSayisi, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                toplam += static_cast<int>(degerSayisi);
            }
            RegCloseKey(h);
        }
    };
    sayHkeyDegerleri(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    sayHkeyDegerleri(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    return toplam;
}

}
