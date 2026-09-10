#include "FolderScanner.h"

// windows.h min/max makrolari std::min/std::max ile cakisiyor.
#define NOMINMAX
#include <windows.h>
#include <cctype>
#include <algorithm>
#include <unordered_map>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>
#include <string>

namespace fs = std::filesystem;

namespace {

// Win32 dogrudan dizin gezintisi. Eskiden klasor basina robocopy alt sureci
// aciliyordu; robocopy TUM dosya listesini metin olarak boruya yaziyor ve biz
// onu regex ile ayristiriyorduk - surec baslatma + metin I/O yuku taramayi
// kat kat yavaslatiyordu. FindFirstFileExW + LARGE_FETCH ile ayni bilgi
// dogrudan alinir.
// UTF-16 yolu UTF-8'e cevirir. fs::path::string() cevrilemeyen karakterde
// istisna firlatiyor ve bir dosya butun klasorun olcumunu iptal ediyordu;
// bu surum kayipsiz ve firlatmasiz.
std::string yolMetni(const std::wstring &genis)
{
    if (genis.empty()) return {};
    const int uzunluk = WideCharToMultiByte(CP_UTF8, 0, genis.data(),
                                            static_cast<int>(genis.size()),
                                            nullptr, 0, nullptr, nullptr);
    if (uzunluk <= 0) return {};
    std::string dar(static_cast<size_t>(uzunluk), '\0');
    WideCharToMultiByte(CP_UTF8, 0, genis.data(), static_cast<int>(genis.size()),
                        dar.data(), uzunluk, nullptr, nullptr);
    return dar;
}

// Ozyinelemeli: dugum verilirse (nullptr degilse) her alt dizin icin
// dugum->cocuklar'a bir FolderScanner::DizinDugumu eklenir - tek taramada
// TAM ALT AGAC kurulur, UI'da dugum acmak yeniden tarama gerektirmez.
// dugum nullptr ise davranis eskisiyle AYNI (sadece toplam bayt).
int64_t nativeOlc(const std::wstring &dizin, FolderScanner::BuyukDosyaToplayici *buyukDosyalar,
                  FolderScanner::DizinDugumu *dugum)
{
    int64_t toplam = 0;
    int64_t dogrudanDosyaBayt = 0;

    WIN32_FIND_DATAW veri{};
    // LARGE_FETCH toplu okuma - cok girdili dizinlerde belirgin hizlanma.
    const std::wstring desen = dizin + L"\\*";
    HANDLE h = FindFirstFileExW(desen.c_str(), FindExInfoBasic, &veri,
                                FindExSearchNameMatch, nullptr,
                                FIND_FIRST_EX_LARGE_FETCH);
    if (h == INVALID_HANDLE_VALUE) {
        if (dugum) dugum->dosyaBayt = 0;
        return 0;   // erisim yok - bu dizin 0 boyutlu sayilir, tarama devam eder
    }

    do {
        // "." ve ".." atla
        const wchar_t *ad = veri.cFileName;
        if (ad[0] == L'.' && (ad[1] == 0 || (ad[1] == L'.' && ad[2] == 0)))
            continue;

        if (veri.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            const std::wstring altYol = dizin + L"\\" + veri.cFileName;
            // Reparse point (symlink/junction) izlenmez - sonsuz donguye ve
            // ayni verinin iki kez sayilmasina yol acar. Yine de agacta
            // gorunsun diye 0 boyutlu, baglanti=true bir cocuk eklenir.
            const bool baglanti = (veri.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            if (baglanti) {
                if (dugum) {
                    FolderScanner::DizinDugumu c;
                    c.ad = yolMetni(veri.cFileName);
                    c.tamYol = yolMetni(altYol);
                    c.baglanti = true;
                    dugum->cocuklar.push_back(std::move(c));
                }
                continue;
            }

            if (dugum) {
                FolderScanner::DizinDugumu c;
                c.ad = yolMetni(veri.cFileName);
                c.tamYol = yolMetni(altYol);
                c.boyut = nativeOlc(altYol, buyukDosyalar, &c);
                toplam += c.boyut;
                dugum->cocuklar.push_back(std::move(c));
            } else {
                toplam += nativeOlc(altYol, buyukDosyalar, nullptr);
            }
        } else {
            const int64_t dosyaBoyutu = (static_cast<int64_t>(veri.nFileSizeHigh) << 32) |
                                         static_cast<int64_t>(veri.nFileSizeLow);
            toplam += dosyaBoyutu;
            dogrudanDosyaBayt += dosyaBoyutu;
            if (buyukDosyalar) {
                const std::wstring tamYol = dizin + L"\\" + veri.cFileName;
                buyukDosyalar->ekle(yolMetni(tamYol), dosyaBoyutu);
            }
        }
    } while (FindNextFileW(h, &veri));

    FindClose(h);

    if (dugum) dugum->dosyaBayt = dogrudanDosyaBayt;
    return toplam;
}

}

namespace FolderScanner {

const char *dosyaKategorisi(const std::string &yol)
{
    const size_t nokta = yol.find_last_of('.');
    if (nokta == std::string::npos || yol.size() - nokta > 6) return nullptr;
    std::string uz = yol.substr(nokta + 1);
    for (char &c : uz) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));

    static const std::unordered_map<std::string, const char *> harita = {
        { "mp4", "video" },  { "mkv", "video" },  { "avi", "video" },
        { "mov", "video" },  { "wmv", "video" },  { "flv", "video" },
        { "webm", "video" }, { "m4v", "video" },  { "mpg", "video" },
        { "mpeg", "video" }, { "ts", "video" },   { "m2ts", "video" },
        { "vob", "video" },  { "3gp", "video" },  { "rmvb", "video" },
        { "jpg", "resim" },  { "jpeg", "resim" }, { "png", "resim" },
        { "gif", "resim" },  { "bmp", "resim" },  { "tif", "resim" },
        { "tiff", "resim" }, { "webp", "resim" }, { "heic", "resim" },
        { "psd", "resim" },  { "raw", "resim" },  { "cr2", "resim" },
        { "nef", "resim" },  { "arw", "resim" },  { "svg", "resim" },
        { "mp3", "ses" },    { "flac", "ses" },   { "wav", "ses" },
        { "aac", "ses" },    { "ogg", "ses" },    { "m4a", "ses" },
        { "wma", "ses" },
        { "zip", "arsiv" },  { "rar", "arsiv" },  { "7z", "arsiv" },
        { "tar", "arsiv" },  { "gz", "arsiv" },   { "bz2", "arsiv" },
        { "xz", "arsiv" },   { "iso", "arsiv" },  { "img", "arsiv" },
        { "vhd", "arsiv" },  { "vhdx", "arsiv" }, { "vmdk", "arsiv" },
        { "exe", "kurulum" },  { "msi", "kurulum" },  { "msix", "kurulum" },
        { "appx", "kurulum" }, { "cab", "kurulum" },
    };
    const auto it = harita.find(uz);
    return it == harita.end() ? nullptr : it->second;
}

int64_t kategoriTabani(const char *kategori)
{
    if (!kategori) return 0;
    const std::string k = kategori;
    if (k == "resim") return 256LL * 1024;          // ekran goruntusu / fotograf
    if (k == "ses") return 1LL * 1024 * 1024;       // sarki ~3-10 MB
    return 5LL * 1024 * 1024;                       // video / arsiv / kurulum
}

// Kategori tabanlarinin en kucugu - ucuz on eleme icin.
static constexpr int64_t kEnKucukTurTabani = 256LL * 1024;

bool sistemYolu(const std::string &yol)
{
    std::string kucuk = yol;
    for (char &c : kucuk) {
        if (c == '\\') c = '/';
        else c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
    }
    // Yol PARCASI olarak aranir ("/windows/"): "d:/oyunlarim/windows7.iso"
    // gibi bir dosya yanlislikla sistem sayilmasin.
    static const char *desenler[] = {
        "/windows/", "/program files/", "/program files (x86)/", "/programdata/",
        "/appdata/", "/$recycle.bin/", "/system volume information/",
        "/node_modules/", "/.git/", "/.cache/", "/.gradle/", "/.nuget/",
        "/perflogs/", "/$winreagent/", "/recovery/", "/msocache/",
    };
    for (const char *d : desenler) {
        if (kucuk.find(d) != std::string::npos) return true;
    }
    return false;
}

void BuyukDosyaToplayici::sifirla()
{
    std::lock_guard<std::mutex> k(m_kilit);
    m_liste.clear();
    m_turHavuzlari.clear();
}

namespace {
// En kucuk eleman kokte duran min-heap: dolunca en kucugu O(log N) atar.
// Havuz buyudukce (2000) her dosya icin linear min_element taramasi
// taramayi yavaslatiyordu.
void heapEkle(std::vector<BuyukDosya> &liste, size_t maks, const BuyukDosya &oge)
{
    const auto kucukOnce = [](const BuyukDosya &a, const BuyukDosya &b) {
        return a.boyut > b.boyut;
    };
    if (liste.size() < maks) {
        liste.push_back(oge);
        std::push_heap(liste.begin(), liste.end(), kucukOnce);
        return;
    }
    if (liste.front().boyut >= oge.boyut) return;
    std::pop_heap(liste.begin(), liste.end(), kucukOnce);
    liste.back() = oge;
    std::push_heap(liste.begin(), liste.end(), kucukOnce);
}

std::vector<BuyukDosya> buyuktenKucuge(std::vector<BuyukDosya> kopya)
{
    std::sort(kopya.begin(), kopya.end(),
              [](const BuyukDosya &a, const BuyukDosya &b) { return a.boyut > b.boyut; });
    return kopya;
}
}  // namespace

void BuyukDosyaToplayici::ekle(const std::string &tamYol, int64_t boyut)
{
    const int64_t genelEsik = m_esik.load();
    // Once ucuz boyut kapisi: dosyalarin buyuk cogunlugu burada elenir,
    // uzanti/kategori isi sadece aday dosyalar icin yapilir.
    if (boyut < genelEsik && boyut < kEnKucukTurTabani) return;

    const char *kat = dosyaKategorisi(tamYol);
    const bool genel = boyut >= genelEsik;
    const bool turlu = kat != nullptr && boyut >= kategoriTabani(kat);
    if (!genel && !turlu) return;

    std::lock_guard<std::mutex> k(m_kilit);
    const BuyukDosya oge{ tamYol, boyut, kat,
                          turlu ? sistemYolu(tamYol) : false };
    if (genel) heapEkle(m_liste, m_maks, oge);
    if (turlu) heapEkle(m_turHavuzlari[kat], m_maks, oge);
}

std::vector<BuyukDosya> BuyukDosyaToplayici::listeAl() const
{
    std::lock_guard<std::mutex> k(m_kilit);
    return buyuktenKucuge(m_liste);
}

std::vector<BuyukDosya> BuyukDosyaToplayici::turListesiAl(const std::string &kategori) const
{
    std::lock_guard<std::mutex> k(m_kilit);
    const auto it = m_turHavuzlari.find(kategori);
    if (it == m_turHavuzlari.end()) return {};
    return buyuktenKucuge(it->second);
}

int64_t klasorBoyutuFallback(const std::string &yol, BuyukDosyaToplayici *buyukDosyalar)
{
    if (yol.empty() || !fs::exists(yol)) return 0;
    int64_t toplam = 0;
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(
             yol, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        std::error_code fileEc;
        if (it->is_regular_file(fileEc) && !fileEc) {
            const auto boyut = it->file_size(fileEc);
            if (!fileEc) {
                toplam += static_cast<int64_t>(boyut);
                if (buyukDosyalar) buyukDosyalar->ekle(yolMetni(it->path().wstring()), static_cast<int64_t>(boyut));
            }
        }
    }
    return toplam;
}

int64_t klasorBoyutuOlc(const std::string &yol, BuyukDosyaToplayici *buyukDosyalar, DizinDugumu *agac)
{
    if (yol.empty()) return 0;
    try {
        if (agac) {
            agac->ad = yolMetni(fs::path(yol).filename().wstring());
            agac->tamYol = yol;
        }
        return nativeOlc(fs::path(yol).wstring(), buyukDosyalar, agac);
    } catch (const std::exception &) {
        // Geri donus yolu da firlarsa 0 don - tek bir klasor yuzunden tarama
        // yarim kalmasin (worker catch'i o klasoru komple olculmemis birakir).
        try {
            return klasorBoyutuFallback(yol, buyukDosyalar);
        } catch (const std::exception &) {
            return 0;
        }
    }
}

std::vector<AltKlasorBilgisi> altKlasorleriTara(
    const std::string &yol,
    const std::function<void(const std::string &)> &ilerleme,
    const std::function<void(const AltKlasorBilgisi &)> &ogeBulundu,
    const std::function<void(const AltKlasorBilgisi &)> &ogeGuncellendi,
    BuyukDosyaToplayici *buyukDosyalar,
    const std::atomic<bool> *iptal,
    int64_t *dosyaBaytlari)
{
    if (dosyaBaytlari) *dosyaBaytlari = 0;
    std::vector<AltKlasorBilgisi> sonuc;
    std::error_code ec;
    fs::directory_iterator it(yol, fs::directory_options::skip_permission_denied, ec);
    if (ec) return sonuc;

    // 1. gecis: sadece dizin gezintisi, boyut olcumu YOK - agac hemen dolsun.
    for (; it != fs::directory_iterator(); it.increment(ec)) {
        if (iptal && iptal->load()) return sonuc;
        if (ec) { ec.clear(); continue; }
        try {
            const auto &girdi = *it;
            std::error_code isDirEc;
            if (!girdi.is_directory(isDirEc) || isDirEc) {
                // Dizinin kendi dosyalari: agacta satiri yok ama toplama ve
                // buyuk dosya havuzuna girmeli.
                std::error_code boyutEc;
                const auto db = girdi.file_size(boyutEc);
                if (boyutEc) continue;
                if (dosyaBaytlari) *dosyaBaytlari += static_cast<int64_t>(db);
                if (buyukDosyalar)
                    buyukDosyalar->ekle(yolMetni(girdi.path().wstring()), static_cast<int64_t>(db));
                continue;
            }

            AltKlasorBilgisi bilgi;
            bilgi.ad = yolMetni(girdi.path().filename().wstring());
            bilgi.tamYol = yolMetni(girdi.path().wstring());
            bilgi.boyut = -1; // henuz olculmedi

            std::error_code linkEc;
            bilgi.baglanti = fs::is_symlink(girdi.symlink_status(linkEc)) && !linkEc;

            if (!bilgi.baglanti) {
                // Hizli "alt klasor var mi" kontrolu - tam tarama yapma,
                // ilk is_directory bulununca dur.
                std::error_code alt_ec;
                for (auto alt = fs::directory_iterator(
                         bilgi.tamYol, fs::directory_options::skip_permission_denied, alt_ec);
                     !alt_ec && alt != fs::directory_iterator(); alt.increment(alt_ec)) {
                    std::error_code altDirEc;
                    if (alt->is_directory(altDirEc) && !altDirEc) {
                        bilgi.altKlasoruVar = true;
                        break;
                    }
                }
            } else {
                bilgi.boyut = 0; // baglanti - olculmez
            }

            if (ogeBulundu) ogeBulundu(bilgi);
            sonuc.push_back(std::move(bilgi));
        } catch (const std::exception &) {
            // Erisim hatasi vs. - bu klasoru atla, tarama YARIM KALMASIN
            // (DiskCleaner.cpp'deki genis catch pattern'iyle AYNI, tek
            // istisna QThread worker'da butun uygulamayi cokertmemeli).
            continue;
        }
    }

    // 2. gecis: boyutlar. Disk I/O bekleme agirlikli oldugu icin cekirdek
    // sayisi kadar is parcacigi ile paralel olculur; callback'ler tek bir
    // mutex arkasindan cagrilir (cagiran taraf thread-safe olmak zorunda
    // kalmasin).
    {
        const unsigned cekirdek = std::max(2u, std::thread::hardware_concurrency());
        const unsigned isciSayisi = std::min<unsigned>(cekirdek, static_cast<unsigned>(sonuc.size()));
        std::atomic<size_t> sonraki{ 0 };
        std::mutex bildirimKilit;

        auto isci = [&]() {
            for (;;) {
                if (iptal && iptal->load()) return;
                const size_t i = sonraki.fetch_add(1);
                if (i >= sonuc.size()) return;
                AltKlasorBilgisi &bilgi = sonuc[i];
                if (bilgi.baglanti) continue;
                try {
                    {
                        std::lock_guard<std::mutex> k(bildirimKilit);
                        if (ilerleme) ilerleme(bilgi.tamYol);
                    }
                    const int64_t olculen = klasorBoyutuOlc(bilgi.tamYol, buyukDosyalar, &bilgi.agac);
                    bilgi.boyut = olculen;
                    std::lock_guard<std::mutex> k(bildirimKilit);
                    if (ogeGuncellendi) ogeGuncellendi(bilgi);
                } catch (const std::exception &) {
                    continue;
                }
            }
        };

        std::vector<std::thread> isciler;
        for (unsigned i = 1; i < isciSayisi; ++i) isciler.emplace_back(isci);
        isci();   // cagiran thread de calissin
        for (auto &t : isciler) t.join();
    }

    return sonuc;
}

}
