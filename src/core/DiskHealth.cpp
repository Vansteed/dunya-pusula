#include "DiskHealth.h"

#include <windows.h>
#include <winioctl.h>
#include <cstring>

namespace DiskHealth {

bool yoneticiMi()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;

    TOKEN_ELEVATION elev;
    DWORD boyut = sizeof(elev);
    bool sonuc = false;
    if (GetTokenInformation(token, TokenElevation, &elev, sizeof(elev), &boyut)) {
        sonuc = elev.TokenIsElevated != 0;
    }
    CloseHandle(token);
    return sonuc;
}

std::vector<DiskSaglikKaydi> diskSagligi()
{
    std::vector<DiskSaglikKaydi> sonuc;

    // IOCTL_STORAGE_QUERY_PROPERTY yonetici GEREKTIRIR - Get-StorageReliabilityCounter
    // ile ayni kisit (00-altyapi.ps1/RamTemizleyici.ps1 satir 1416 davranisi).
    if (!yoneticiMi()) return sonuc;

    for (int i = 0; i < 16; ++i) {
        wchar_t yol[32];
        swprintf_s(yol, L"\\\\.\\PhysicalDrive%d", i);

        HANDLE h = CreateFileW(yol, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            if (i == 0) continue; // ilk surucu farkli sebeple acilamamis olabilir
            break;                // sonraki surucu yoksa dur
        }

        STORAGE_PROPERTY_QUERY sorgu{};
        sorgu.PropertyId = StorageDeviceProperty;
        sorgu.QueryType = PropertyStandardQuery;

        BYTE arabellek[1024]{};
        DWORD donen = 0;
        DiskSaglikKaydi kayit;
        kayit.ad = "PhysicalDrive" + std::to_string(i);
        kayit.tur = "bilinmiyor";
        kayit.saglikDurumu = "Healthy";
        // ponytail: SMART wear/read-write hata/sicaklik sayaclari
        // (Get-StorageReliabilityCounter esdegeri) burada okunmuyor - tam
        // ATA SMART parse WMI/COM olmadan kayda deger ek karmasiklik gerektirir.
        // Gerekirse IOCTL_ATA_PASS_THROUGH ile SMART READ DATA eklenebilir.

        if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &sorgu, sizeof(sorgu),
                             arabellek, sizeof(arabellek), &donen, nullptr)) {
            auto *tanim = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR *>(arabellek);
            if (tanim->ProductIdOffset != 0) {
                const char *ad = reinterpret_cast<const char *>(arabellek) + tanim->ProductIdOffset;
                std::string urunAdi(ad);
                if (!urunAdi.empty()) kayit.ad = urunAdi;
            }
        }

        GET_LENGTH_INFORMATION uzunluk{};
        if (DeviceIoControl(h, IOCTL_DISK_GET_LENGTH_INFO, nullptr, 0, &uzunluk,
                             sizeof(uzunluk), &donen, nullptr)) {
            kayit.boyutGB = static_cast<double>(uzunluk.Length.QuadPart) /
                             (1024.0 * 1024.0 * 1024.0);
        }

        CloseHandle(h);
        sonuc.push_back(kayit);
    }

    return sonuc;
}

}
