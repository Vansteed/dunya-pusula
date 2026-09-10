// DiskHealth.h - Get-DiskSaglik (moduller\10-disk.ps1) esdegeri.
// IOCTL_STORAGE_QUERY_PROPERTY / SMART yonetici GEREKTIRIR - PowerShell'deki
// Get-StorageReliabilityCounter kisitiyla AYNI.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct DiskSaglikKaydi {
    std::string ad;
    std::string tur;              // MediaType (SSD/HDD/bilinmiyor)
    double boyutGB = 0.0;
    std::string saglikDurumu;     // "Healthy" / "Warning" / "Unhealthy" / "Unknown"
    std::optional<double> asinmaYuzde;
    std::optional<int64_t> okumaHatasi;
    std::optional<int64_t> yazmaHatasi;
    std::optional<double> sicaklikC;
};

namespace DiskHealth {

// Yonetici degilse SMART sayaclari okunamaz; bu durumda bos liste doner
// (UI "yonetici gerekli" gostersin - 00-altyapi.ps1/RamTemizleyici.ps1
// satir 1416 ile ayni davranis).
bool yoneticiMi();

std::vector<DiskSaglikKaydi> diskSagligi();

}
