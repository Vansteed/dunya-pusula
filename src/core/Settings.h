// Settings.h - ayarlar.json esdegeri. RamTemizleyici.ps1 Save-Ayar (satir
// 1115-1122) ve acilis okuma (satir 2083-2089) ile BIREBIR ayni sema:
// { "AutoClean": bool, "Threshold": int }
#pragma once

#include <string>

struct AyarVerisi {
    bool autoClean = false;
    int threshold = 85; // XAML SliderThreshold varsayilani ile ayni
};

namespace Settings {

// Dosya yoksa ya da bozuksa varsayilan AyarVerisi doner (PowerShell
// surumundeki try/catch ile ayni: sessizce yut, mevcut degerleri koru).
AyarVerisi yukle(const std::string &dosyaYolu);

// { "AutoClean": ..., "Threshold": ... } formatinda UTF-8 JSON yazar.
// Bu katman Qt icermedigi icin QJsonDocument yerine minimal el yazimi
// JSON kullanir (sema tek duz obje, kutuphane gerektirmez).
bool kaydet(const std::string &dosyaYolu, const AyarVerisi &veri);

}
