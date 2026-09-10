// ProcessList.h - Refresh-ProcList esdegeri (RamTemizleyici.ps1 ~1078).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ProcAlt {
    uint32_t pid = 0;
    int64_t mb = 0;   // bu PID'in KENDI RAM'i
    std::string exeYolu;   // bu PID'in tam exe yolu (alinamazsa bos)
};

struct ProcKaydi {
    std::string ad;
    int64_t mb = 0;                    // TOPLAM RAM (grup ise tum uyeler toplami)
    std::vector<ProcAlt> altlar;       // grup uyeleri - pid + kendi RAM'i (>=1 eleman)
    std::string tamYol;                // temsili exe tam yolu (ikon icin, ilk bulunan)
    std::string durum;    // "Sistem - Korumali" / "Kapatilabilir"
    bool korumali = false;
};

namespace ProcessList {

// Ayni isimli islemler ISME gore gruplanir (toplam RAM'e gore siralanir).
// En cok RAM (WorkingSet) yiyen 25 GRUP, buyukten kucuge sirali.
// PowerShell'deki gibi WorkingSet64 > 0 filtrelenir.
std::vector<ProcKaydi> yenile();

// PROCESS_SET_INFORMATION + SetPriorityClass. sinif: 0=dusuk(IDLE),
// 1=normalin altinda(BELOW_NORMAL), 2=normal(NORMAL), 3=normalin
// ustunde(ABOVE_NORMAL), 4=yuksek(HIGH). REALTIME kasitli olarak yok -
// sistemi kilitleyebilir.
bool oncelikAta(uint32_t pid, int sinif);

// GetPriorityClass, yukaridaki olcege cevirir. Hata / bilinmeyen sinif -1.
int oncelikOku(uint32_t pid);

// Verilen PID ve TUM alt sureclerini kapatir (once cocuklar, sonra
// ebeveyn). PID yeniden kullanimina karsi: cocuk, ebeveynin olusturma
// zamanindan ONCE baslamissa gercek cocuk sayilmaz, atlanir.
bool agaciKapat(uint32_t pid);

// agaciKapat ile AYNI agac uyelerini sayar (kendisi dahil), hicbirini
// kapatmaz - onay diyalogunda "N surec kapanacak" yazisi icin.
int agacUyeSayisi(uint32_t pid);

}
