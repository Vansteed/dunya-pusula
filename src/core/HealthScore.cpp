#include "HealthScore.h"

SaglikSonuc hesaplaSaglikSkoru(double ramYuzde, double diskBosYuzde,
                                std::optional<double> diskAsinma,
                                bool diskSaglikli)
{
    SaglikSonuc sonuc;

    // Baslangic bileseni kaldirildi, 25 puani diger uc bilesene dagitildi
    // (RAM +8, disk doluluk +8, disk sagligi +9) toplam yine 100.

    // RAM (kullanim yuzdesi, max 33)
    int pRam;
    if (ramYuzde < 60) pRam = 33;
    else if (ramYuzde < 75) pRam = 24;
    else if (ramYuzde < 85) pRam = 13;
    else if (ramYuzde < 95) pRam = 5;
    else pRam = 0;

    // Disk doluluk (bos yuzdesi, max 33)
    int pDisk;
    if (diskBosYuzde > 25) pDisk = 33;
    else if (diskBosYuzde >= 15) pDisk = 24;
    else if (diskBosYuzde >= 10) pDisk = 13;
    else if (diskBosYuzde >= 5) pDisk = 5;
    else pDisk = 0;

    // Disk sagligi (max 34)
    int pDiskSaglik;
    if (!diskAsinma.has_value()) {
        pDiskSaglik = 24;
    } else if (!diskSaglikli || *diskAsinma > 50) {
        pDiskSaglik = 7;
    } else if (*diskAsinma >= 20) {
        pDiskSaglik = 20;
    } else {
        pDiskSaglik = 34;
    }

    const int toplam = pRam + pDisk + pDiskSaglik;

    std::string yorum;
    if (toplam >= 80) yorum = "Sisteminiz sağlıklı durumda.";
    else if (toplam >= 60) yorum = "Sisteminizde iyileştirilebilecek noktalar var.";
    else yorum = "Sisteminiz dikkat gerektiriyor.";

    sonuc.puan = toplam;
    sonuc.yorum = yorum;
    sonuc.kirilim = SaglikKirilim{pRam, pDisk, pDiskSaglik};
    return sonuc;
}
