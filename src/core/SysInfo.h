// SysInfo.h - Get-RamInfo / Get-DiskKullanim esdegeri (RamTemizleyici.ps1 ~87,
// moduller\10-disk.ps1). Sadece WinAPI, Qt yok.
#pragma once

#include <string>

struct RamBilgisi {
    double totalGB = 0.0;
    double usedGB = 0.0;
    double freeGB = 0.0;
    double percent = 0.0;
};

struct DiskBilgisi {
    std::string harf;
    double toplamGB = 0.0;
    double kullanilanGB = 0.0;
    double bosGB = 0.0;
    double yuzde = 0.0;
};

namespace SysInfo {

// GlobalMemoryStatusEx ile RAM bilgisi.
RamBilgisi ramBilgisi();

// GetDiskFreeSpaceExW ile sistem surucusunun (ornegin "C:") disk kullanimi.
// surucuKoku "C:\\" gibi ters slash ile bitmelidir; verilmezse %SystemDrive%
// kullanilir.
DiskBilgisi diskKullanim(const std::string &surucuKoku = "");

// HKCU + HKLM Software\Microsoft\Windows\CurrentVersion\Run anahtarlarindaki
// deger sayisi - Get-CimInstance Win32_StartupCommand (RamTemizleyici.ps1
// ~1338) esdegeri, WMI/COM'a girmeden native registry karsiligi.
int baslangicOgeSayisi();

}
