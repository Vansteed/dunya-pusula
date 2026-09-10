// FolderScanner.h - $global:DiskScanScript (RamTemizleyici.ps1 satir 1480+)
// esdegeri. robocopy /L ile hizli boyut olcumu, basarisiz olursa
// std::filesystem ozyinelemeli fallback (Get-DirSizeNet esdegeri).
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace FolderScanner {

// nativeOlc ozyinelemesi sirasinda kurulan TAM ALT AGAC - tek taramada
// butun alt klasorler kaydedilir, dugum acilinca yeniden taranmaz.
struct DizinDugumu {
    std::string ad;
    std::string tamYol;
    int64_t boyut = 0;        // alt agac dahil toplam
    int64_t dosyaBayt = 0;    // bu dizinin DOGRUDAN dosyalari
    bool baglanti = false;
    std::vector<DizinDugumu> cocuklar;
};

// path'in dogrudan bir alt klasoru hakkinda ozet bilgi.
struct AltKlasorBilgisi {
    std::string ad;         // klasor adi (yol degil)
    std::string tamYol;
    int64_t boyut = 0;      // klasorBoyutuOlc ile olculur
    bool altKlasoruVar = false;
    bool baglanti = false;  // reparse point / symlink - icine girilmez
    // klasorBoyutuOlc ile birlikte kurulan TAM ALT AGAC - tek taramada butun
    // alt klasorler bir kere gezilsin, dugum acilinca yeniden taranmasin.
    DizinDugumu agac;
};

// Tarama sirasinda karsilasilan dosyalardan esigi asan en buyuk N tanesini
// tutar (thread-safe - birden fazla olcum is parcaciginda paralel doldurulur).
// Sabit boyutlu liste + en-kucugu-degistir: N kucuk sabit oldugu icin tam
// siralama/heap yerine O(N) linear scan yeterli, tarama yavaslamaz.
struct BuyukDosya {
    std::string tamYol;
    int64_t boyut = 0;
    // Statik literal ("video"/"resim"/...) ya da bilinmeyen tur icin nullptr.
    const char *kategori = nullptr;
    // Windows/Program Files/AppData gibi bir yolun altinda mi -
    // kullanicinin kendi dosyalarini ayirmak icin.
    bool sistem = false;
};

// Uzantidan kaba tur: "video", "resim", "ses", "arsiv", "kurulum" ya da
// nullptr. Donen isaretci statik literaldir, sahiplik yoktur.
const char *dosyaKategorisi(const std::string &yol);

// Yol sistem/program klasorlerinden birinin altinda mi.
bool sistemYolu(const std::string &yol);

// Kategorinin toplama tabani - resim/ses dosyalari videolardan cok
// daha kucuk, ortak taban ikisini birden dogru filtreleyemiyor.
int64_t kategoriTabani(const char *kategori);

class BuyukDosyaToplayici {
public:
    explicit BuyukDosyaToplayici(size_t maksimum = 2000) : m_maks(maksimum) {}

    void esikAyarla(int64_t esikBayt) { m_esik.store(esikBayt); }
    int64_t esikBaytAl() const { return m_esik.load(); }
    void sifirla();
    void ekle(const std::string &tamYol, int64_t boyut);
    // Boyut havuzu: her turden, m_esik ustu en buyuk N dosya.
    std::vector<BuyukDosya> listeAl() const;
    // Tur havuzu: KATEGORI BASINA ayri tutulur - ortak havuzda dev arsiv ve
    // kurulum dosyalari videolari disari itiyordu.
    std::vector<BuyukDosya> turListesiAl(const std::string &kategori) const;

private:
    mutable std::mutex m_kilit;
    std::vector<BuyukDosya> m_liste;
    std::unordered_map<std::string, std::vector<BuyukDosya>> m_turHavuzlari;
    std::atomic<int64_t> m_esik{ 500LL * 1024 * 1024 };
    size_t m_maks;
};

// robocopy.exe /L /S /NJH /BYTES ile klasor boyutunu olcer (alt process,
// stdout'tan "Bytes :" satiri regex ile okunur). Basarisiz olursa
// std::filesystem::recursive_directory_iterator fallback'e duser.
// Bu fonksiyon UZUN SURER (Phase 4'te QThread icine sarilacak).
// buyukDosyalar verilirse (nullptr degilse) yol altinda gezilen her DOSYA
// esigi asiyorsa toplayiciya eklenir.
// agac verilirse (nullptr degilse) yol'un TAM ALT AGACI icine kurulur -
// dugum acmak icin yeniden tarama gerekmesin diye.
int64_t klasorBoyutuOlc(const std::string &yol, BuyukDosyaToplayici *buyukDosyalar = nullptr,
                        DizinDugumu *agac = nullptr);

// std::filesystem tabanli dogrudan fallback (Get-DirSizeNet esdegeri).
// NOT: agac parametresi burada kurulmaz (fallback nadiren tetiklenir, agac
// eksik kalirsa dugum acilinca eski davranisa (yeniden tarama) duser).
int64_t klasorBoyutuFallback(const std::string &yol, BuyukDosyaToplayici *buyukDosyalar = nullptr);

// yol'un DOGRUDAN alt klasorlerini listeler, her biri icin klasorBoyutuOlc
// ile boyutunu olcer (sirali, UZUN SUREBILIR - QThread'de calistirilmali).
// Baglanti (symlink/reparse point) olan alt klasorlere GIRILMEZ.
// ilerleme: her alt klasor olculmeden hemen once cagirilir (tam yol) - UI'da
// "Taraniyor: X" gostermek icin, taramanin donmadigini belli eder.
// ogeBulundu: alt klasor DIZIN GEZINTISI sirasinda (boyut olculmeden) HEMEN
// cagirilir (bilgi.boyut = -1, "henuz olculmedi") - UI agaci aninda dolsun.
// ogeGuncellendi: ikinci gecis - alt klasorun boyutu olculunce cagirilir
// (bilgi.boyut dolu).
// buyukDosyalar: verilirse taranan dosyalar bu toplayiciya eklenir.
// iptal: verilirse ve true olursa dongulerden erken cikilir (yarim sonuc
// donulur, cagiran taraf zaten kismi agaci gosteriyor).
std::vector<AltKlasorBilgisi> altKlasorleriTara(
    const std::string &yol,
    const std::function<void(const std::string &)> &ilerleme = nullptr,
    const std::function<void(const AltKlasorBilgisi &)> &ogeBulundu = nullptr,
    const std::function<void(const AltKlasorBilgisi &)> &ogeGuncellendi = nullptr,
    BuyukDosyaToplayici *buyukDosyalar = nullptr,
    const std::atomic<bool> *iptal = nullptr,
    // Taranan dizinin DOGRUDAN icindeki dosyalarin toplami. Bunlar agacta
    // satir olmadigi icin klasor boyutlarina eklenmiyor, ayri doner.
    int64_t *dosyaBaytlari = nullptr);

}
