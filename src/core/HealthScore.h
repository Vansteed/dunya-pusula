// HealthScore.h - Get-SaglikSkoru (moduller\20-skor.ps1) BIREBIR karsiligi.
// Saf fonksiyon, GUI/WinAPI baglantisi yok.
#pragma once

#include <optional>
#include <string>

struct SaglikKirilim {
    int ram = 0;
    int disk = 0;
    int diskSaglik = 0;
};

struct SaglikSonuc {
    int puan = 0;
    std::string yorum;
    SaglikKirilim kirilim;
};

// ramYuzde: RAM kullanim yuzdesi
// diskBosYuzde: disk BOS alan yuzdesi
// diskAsinma: SMART asinma yuzdesi, veri yoksa nullopt
// diskSaglikli: disk HealthStatus == "Healthy" mi
SaglikSonuc hesaplaSaglikSkoru(double ramYuzde, double diskBosYuzde,
                                std::optional<double> diskAsinma,
                                bool diskSaglikli);
