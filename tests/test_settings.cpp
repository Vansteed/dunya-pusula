// test_settings.cpp - ayarlar.json yaz-oku round-trip testi, ayrica
// PowerShell surumunun urettigi dosya formatinin okunabildigini dogrular.
#include "core/Settings.h"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

int main()
{
    fs::path dosya = fs::temp_directory_path() / "ram_temizleyici_test_ayarlar.json";
    std::error_code ec;
    fs::remove(dosya, ec);

    // Round-trip: yaz -> oku
    {
        AyarVerisi yazilan;
        yazilan.autoClean = true;
        yazilan.threshold = 77;
        assert(Settings::kaydet(dosya.string(), yazilan));

        AyarVerisi okunan = Settings::yukle(dosya.string());
        assert(okunan.autoClean == true);
        assert(okunan.threshold == 77);
    }

    // PowerShell'in Save-Ayar ile uretecegi format (ConvertTo-Json cikisi,
    // iki bosluklu ":" araligi) okunabiliyor mu
    {
        std::ofstream f(dosya, std::ios::binary | std::ios::trunc);
        f << "{\n    \"AutoClean\":  false,\n    \"Threshold\":  85\n}";
        f.close();

        AyarVerisi okunan = Settings::yukle(dosya.string());
        assert(okunan.autoClean == false);
        assert(okunan.threshold == 85);
    }

    // Dosya yok -> varsayilan
    {
        fs::remove(dosya, ec);
        AyarVerisi okunan = Settings::yukle(dosya.string());
        assert(okunan.autoClean == false);
        assert(okunan.threshold == 85);
    }

    fs::remove(dosya, ec);
    std::printf("test_settings: TUM TESTLER GECTI\n");
    return 0;
}
