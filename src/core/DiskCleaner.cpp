#include "DiskCleaner.h"

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

std::string ortamDegiskeni(const char *ad)
{
    char *buf = nullptr;
    size_t len = 0;
    if (_dupenv_s(&buf, &len, ad) != 0 || buf == nullptr) return "";
    std::string s(buf);
    free(buf);
    return s;
}

std::string yolBirlestir(const std::string &kok, const std::string &alt)
{
    return (fs::path(kok) / alt).string();
}

// Basit tek '*' destekli desen eslesme (ornegin "thumbcache_*.db").
bool desenEslesir(const std::string &ad, const std::string &desen)
{
    if (desen == "*") return true;
    const auto yildiz = desen.find('*');
    if (yildiz == std::string::npos) return ad == desen;

    const std::string onEk = desen.substr(0, yildiz);
    const std::string sonEk = desen.substr(yildiz + 1);
    if (ad.size() < onEk.size() + sonEk.size()) return false;
    if (ad.compare(0, onEk.size(), onEk) != 0) return false;
    if (ad.compare(ad.size() - sonEk.size(), sonEk.size(), sonEk) != 0) return false;
    return true;
}

}

namespace DiskCleaner {

std::vector<TemizlikHedefi> temizlikHedefleri()
{
    const std::string temp = ortamDegiskeni("TEMP");
    const std::string sistemKok = ortamDegiskeni("SystemRoot");
    const std::string localAppData = ortamDegiskeni("LOCALAPPDATA");

    return {
        { "Kullanıcı geçici dosyaları", temp, HedefTuru::Klasor, "" },
        { "Windows geçici dosyaları", yolBirlestir(sistemKok, "Temp"), HedefTuru::Klasor, "" },
        { "Prefetch", yolBirlestir(sistemKok, "Prefetch"), HedefTuru::Klasor, "" },
        { "Windows Update artıkları", yolBirlestir(sistemKok, "SoftwareDistribution\\Download"), HedefTuru::Klasor, "" },
        { "Teslimat iyileştirme önbelleği",
          yolBirlestir(sistemKok, "ServiceProfiles\\NetworkService\\AppData\\Local\\Microsoft\\Windows\\DeliveryOptimization"),
          HedefTuru::Klasor, "" },
        { "Küçük resim önbelleği", yolBirlestir(localAppData, "Microsoft\\Windows\\Explorer"),
          HedefTuru::Desen, "thumbcache_*.db" },
        { "Geri dönüşüm kutusu", "", HedefTuru::GeriDonusum, "" },
    };
}

KlasorBoyutSonuc klasorBoyutu(const std::string &yol, const std::string &desen)
{
    if (yol.empty() || !fs::exists(yol)) {
        return { 0, 0 };
    }

    int64_t boyut = 0;
    int64_t sayi = 0;
    // Erisim engellenirse recursive_directory_iterator filesystem_error
    // firlatir; bu, PowerShell surumundeki "erisim engellendi" istisna
    // davranisiyla ayni (bkz. moduller\10-disk.ps1 yorumu).
    for (const auto &girdi : fs::recursive_directory_iterator(
             yol, fs::directory_options::skip_permission_denied)) {
        if (!girdi.is_regular_file()) continue;
        const std::string ad = girdi.path().filename().string();
        if (!desenEslesir(ad, desen)) continue;
        boyut += static_cast<int64_t>(girdi.file_size());
        ++sayi;
    }
    return { boyut, sayi };
}

std::vector<TemizlikSonucSatiri> tara()
{
    std::vector<TemizlikSonucSatiri> sonuclar;
    for (const auto &h : temizlikHedefleri()) {
        TemizlikSonucSatiri satir;
        satir.konum = h.ad;

        if (h.tur == HedefTuru::GeriDonusum) {
            // Geri Donusum Kutusu COM/Shell erisimi gerektirir, bu STL-only
            // katmanin kapsami disinda (Qt/UI katmaninda ele alinacak).
            satir.boyut = 0;
            satir.sayi = 0;
            satir.durum = "Yok";
            sonuclar.push_back(satir);
            continue;
        }

        const std::string desen = h.tur == HedefTuru::Desen ? h.desen : "*";
        try {
            const auto r = klasorBoyutu(h.yol, desen);
            satir.boyut = r.boyut;
            satir.sayi = r.sayi;
            satir.durum = fs::exists(h.yol) ? "Bulundu" : "Yok";
        } catch (const std::exception &) {
            // skip_permission_denied cogu erisim hatasini onluyor ama tum
            // durumlari kapsamiyor (ornegin file_size() bir kilitli dosyada
            // hala firlatabilir) - fs::filesystem_error'a ozel yakalamak
            // yerine genis catch: bir QThread worker slot'unda yakalanmayan
            // istisna sessizce process'i sonlandirir (Qt event loop'u
            // sarmalamaz), tek satirlik hedef atlamak tum uygulamayi
            // kapatmaktan iyidir.
            satir.boyut = 0;
            satir.sayi = 0;
            satir.durum = "Erişim engellendi - yönetici gerekli";
        }
        sonuclar.push_back(satir);
    }
    return sonuclar;
}

}
