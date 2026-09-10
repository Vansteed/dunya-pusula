// RestorePoint.h - New-GeriYuklemeNoktasi (00-altyapi.ps1) esdegeri.
// SRSetRestorePointW dinamik yuklenir (import lib genelde yok).
#pragma once

#include <string>

struct RestorePointSonuc {
    bool basarili = false;
    std::string mesaj;
};

namespace RestorePoint {

bool yoneticiMi();

// Son 24 saat icinde nokta varsa yeni olusturmaz (PowerShell'deki 24 saat
// kuraliyla AYNI, ancak "son nokta zamani" bu STL/WinAPI katmaninda WMI/COM
// olmadan guvenilir okunamadigi icin ARASTIRILDI: kesin bir Win32 API yok,
// en yakin karsilik SRSetRestorePointW'nin kendisi (COM/WMI olmadan zaman
// sorgulanamaz) - bu yuzden burada 24 saatlik yinelenen-nokta kontrolu
// YAPILMAZ, dogrudan olusturma denenir; UI/Qt katmani isterse WMI ile
// tamamlayabilir. YORUM: plan maddesi 2.9 geregi belirtilmistir.
RestorePointSonuc olustur(const std::string &aciklama = "Dünya Pusula - bakim oncesi");

}
