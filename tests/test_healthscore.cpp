// test_healthscore.cpp - moduller\20-skor.ps1 test-disk.ps1 senaryolarinin
// C++ birebir cevirisi. Framework gerekmez, assert() tabanli.
// Baslangic bileseni kaldirildi (uc bilesen: RAM 33 + disk 33 + diskSaglik 34 = 100).
#include "core/HealthScore.h"

#include <cassert>
#include <cstdio>
#include <optional>

int main()
{
    // En iyi durum -> 100 puan (33+33+34)
    {
        auto s = hesaplaSaglikSkoru(30.0, 40.0, std::optional<double>(10.0), true);
        assert(s.kirilim.ram == 33);
        assert(s.kirilim.disk == 33);
        assert(s.kirilim.diskSaglik == 34);
        assert(s.puan == 100);
        assert(s.yorum == "Sisteminiz saglikli durumda.");
    }

    // En kotu durum -> 7 puan (0+0+7, disk saglikli degil)
    {
        auto s = hesaplaSaglikSkoru(96.0, 3.0, std::optional<double>(60.0), false);
        assert(s.kirilim.ram == 0);
        assert(s.kirilim.disk == 0);
        assert(s.kirilim.diskSaglik == 7);
        assert(s.puan == 7);
        assert(s.yorum == "Sisteminiz dikkat gerektiriyor.");
    }

    // Disk verisi yok (nullopt) -> o bilesen 24 puan
    {
        auto s = hesaplaSaglikSkoru(50.0, 30.0, std::nullopt, true);
        assert(s.kirilim.diskSaglik == 24);
    }

    // Sinir degerleri: RAM tam 60 -> 24 (60 < 60 false, sonraki dal)
    {
        auto s = hesaplaSaglikSkoru(60.0, 30.0, std::nullopt, true);
        assert(s.kirilim.ram == 24);
    }
    // RAM tam 59.9 -> 33
    {
        auto s = hesaplaSaglikSkoru(59.9, 30.0, std::nullopt, true);
        assert(s.kirilim.ram == 33);
    }

    // Disk bos yuzde tam 25 -> 24 (25 > 25 false)
    {
        auto s = hesaplaSaglikSkoru(30.0, 25.0, std::nullopt, true);
        assert(s.kirilim.disk == 24);
    }

    // Disk asinma tam 50 -> hala 20 (>50 degil ama diskSaglikli true; asinma>=20 true -> 20)
    {
        auto s = hesaplaSaglikSkoru(30.0, 40.0, std::optional<double>(50.0), true);
        assert(s.kirilim.diskSaglik == 20);
    }
    // Asinma 50.1 -> 7
    {
        auto s = hesaplaSaglikSkoru(30.0, 40.0, std::optional<double>(50.1), true);
        assert(s.kirilim.diskSaglik == 7);
    }
    // Asinma tam 20 -> 20, 19.9 -> 34
    {
        auto s20 = hesaplaSaglikSkoru(30.0, 40.0, std::optional<double>(20.0), true);
        assert(s20.kirilim.diskSaglik == 20);
        auto s19 = hesaplaSaglikSkoru(30.0, 40.0, std::optional<double>(19.9), true);
        assert(s19.kirilim.diskSaglik == 34);
    }

    // Toplam sinir yorumlar: 80 -> saglikli, 60 -> iyilestirilebilir, altinda dikkat
    {
        // 33+33+20 = 86
        auto s = hesaplaSaglikSkoru(30.0, 40.0, std::optional<double>(60.0), true);
        assert(s.puan == 86);
        assert(s.yorum == "Sisteminiz saglikli durumda.");
    }
    {
        // 24+24+7 = 55 -> dikkat gerektiriyor
        auto s = hesaplaSaglikSkoru(70.0, 20.0, std::optional<double>(60.0), true);
        assert(s.puan == 55);
        assert(s.yorum == "Sisteminiz dikkat gerektiriyor.");
    }

    std::printf("test_healthscore: TUM TESTLER GECTI\n");
    return 0;
}
