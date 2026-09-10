#include "Settings.h"

#include <fstream>
#include <regex>
#include <sstream>

namespace Settings {

AyarVerisi yukle(const std::string &dosyaYolu)
{
    AyarVerisi veri; // varsayilanlar

    std::ifstream dosya(dosyaYolu, std::ios::binary);
    if (!dosya.is_open()) return veri;

    std::ostringstream tampon;
    tampon << dosya.rdbuf();
    const std::string icerik = tampon.str();
    if (icerik.empty()) return veri;

    try {
        std::smatch eslesme;

        std::regex autoCleanDesen(R"("AutoClean"\s*:\s*(true|false))");
        if (std::regex_search(icerik, eslesme, autoCleanDesen)) {
            veri.autoClean = (eslesme[1].str() == "true");
        }

        std::regex thresholdDesen(R"("Threshold"\s*:\s*(-?\d+))");
        if (std::regex_search(icerik, eslesme, thresholdDesen)) {
            veri.threshold = std::stoi(eslesme[1].str());
        }
    } catch (...) {
        return AyarVerisi{};
    }

    return veri;
}

bool kaydet(const std::string &dosyaYolu, const AyarVerisi &veri)
{
    std::ofstream dosya(dosyaYolu, std::ios::binary | std::ios::trunc);
    if (!dosya.is_open()) return false;

    dosya << "{\n"
          << "  \"AutoClean\": " << (veri.autoClean ? "true" : "false") << ",\n"
          << "  \"Threshold\": " << veri.threshold << "\n"
          << "}";
    return true;
}

}
