#include "HealthBackend.h"

#include "core/DiskHealth.h"
#include "core/HealthScore.h"
#include "core/SysInfo.h"

#include <QVariantMap>

namespace {

// Ajanin donanim adiyla DiskHealth'in urun adi ayni disk icin farkli
// bicimlerde gelebilir - biri digerini iceriyorsa ayni disk kabul edilir.
bool ayniDiskMi(const QString &a, const QString &b)
{
    if (a.isEmpty() || b.isEmpty()) return false;
    return a.contains(b, Qt::CaseInsensitive) || b.contains(a, Qt::CaseInsensitive);
}

QString sayiYaziciM(std::optional<double> v, const QString &birim)
{
    return v ? QString::number(*v, 'f', 0) + " " + birim : QStringLiteral("-");
}

QString yuzdeYazici(std::optional<double> v)
{
    return v ? "%" + QString::number(*v, 'f', 0) : QStringLiteral("-");
}

}

HealthBackend::HealthBackend(SensorAjaniIstemcisi *sensorAjani, QObject *parent)
    : QObject(parent), m_sensorAjani(sensorAjani)
{
    connect(m_sensorAjani, &SensorAjaniIstemcisi::veriGeldi, this, &HealthBackend::yenile);

    m_timer.setInterval(2000);
    connect(&m_timer, &QTimer::timeout, this, &HealthBackend::yenile);
    m_timer.start();

    yenile();
}

void HealthBackend::yenile()
{
    const RamBilgisi ram = SysInfo::ramBilgisi();
    const DiskBilgisi disk = SysInfo::diskKullanim();

    m_yonetici = DiskHealth::yoneticiMi();
    const auto kayitlar = DiskHealth::diskSagligi();
    const auto &ajanDiskleri = m_sensorAjani->son().diskler;

    // Saglik skoru icin ajanin ilk diskinden asinma/saglik bilgisi kullanilir
    // - DiskHealth'in ham IOCTL verisi SMART asinma/omur icermiyor.
    std::optional<double> asinma;
    bool saglikli = true;
    if (!ajanDiskleri.isEmpty()) {
        const auto &ilk = ajanDiskleri.front();
        asinma = ilk.kullanilanYuzde ? ilk.kullanilanYuzde
                                      : (ilk.omurYuzde ? std::optional<double>(100.0 - *ilk.omurYuzde) : std::nullopt);
        saglikli = ilk.omurYuzde ? (*ilk.omurYuzde >= 50.0) : true;
    } else if (!kayitlar.empty()) {
        asinma = kayitlar.front().asinmaYuzde;
        saglikli = (kayitlar.front().saglikDurumu == "Healthy");
    }

    const double diskBosYuzde = disk.toplamGB > 0.0
                                    ? (disk.bosGB / disk.toplamGB) * 100.0
                                    : 0.0;

    const SaglikSonuc sonuc =
        hesaplaSaglikSkoru(ram.percent, diskBosYuzde, asinma, saglikli);

    m_puan = sonuc.puan;
    m_yorum = QString::fromStdString(sonuc.yorum);
    m_ramPuan = sonuc.kirilim.ram;
    m_diskPuan = sonuc.kirilim.disk;
    m_diskSaglikPuan = sonuc.kirilim.diskSaglik;

    // Ajan diskleri ile DiskHealth kayitlarini ada gore eslestir. Ajan daha
    // guvenilir (Total Space/omur/sicaklik veriyor), DiskHealth sadece
    // urun adi + boyut + genel saglik durumu veriyor.
    QList<bool> diskHealthKullanildi(int(kayitlar.size()), false);

    m_diskler.clear();
    for (const auto &ad : ajanDiskleri) {
        const DiskSaglikKaydi *eslesen = nullptr;
        for (size_t i = 0; i < kayitlar.size(); ++i) {
            if (diskHealthKullanildi[int(i)]) continue;
            if (ayniDiskMi(ad.ad, QString::fromStdString(kayitlar[i].ad))) {
                eslesen = &kayitlar[i];
                diskHealthKullanildi[int(i)] = true;
                break;
            }
        }

        QString tur = eslesen ? QString::fromStdString(eslesen->tur) : QString();
        if (tur.isEmpty() || tur.compare("bilinmiyor", Qt::CaseInsensitive) == 0) {
            tur = (ad.kullanilanYuzde || ad.yedekYuzde) ? QStringLiteral("NVMe") : QStringLiteral("-");
        }

        QVariantMap m;
        m["ad"] = ad.ad;
        m["tur"] = tur;
        m["toplamGB"] = ad.toplamGB.value_or(eslesen ? eslesen->boyutGB : 0.0);
        m["bosGB"] = ad.bosGB.value_or(0.0);
        m["sicaklik"] = ad.sicaklikC ? QString::number(*ad.sicaklikC, 'f', 0) + " C" : QStringLiteral("-");
        m["omur"] = yuzdeYazici(ad.omurYuzde);
        m["durum"] = eslesen ? QString::fromStdString(eslesen->saglikDurumu) : QStringLiteral("-");
        m["kullanilanYuzde"] = yuzdeYazici(ad.kullanilanYuzde);
        m["yedekYuzde"] = yuzdeYazici(ad.yedekYuzde);
        m["okunanGB"] = sayiYaziciM(ad.okunanGB, "GB");
        m["yazilanGB"] = sayiYaziciM(ad.yazilanGB, "GB");
        m["acilmaSayisi"] = ad.acilmaSayisi ? QString::number(*ad.acilmaSayisi) : QStringLiteral("-");
        m["calismaSaati"] = ad.calismaSaati ? QString::number(*ad.calismaSaati) + " saat" : QStringLiteral("-");
        m_diskler.append(m);
    }

    // Ajanda olmayip sadece DiskHealth'te bulunan diskler de eklenir.
    for (size_t i = 0; i < kayitlar.size(); ++i) {
        if (diskHealthKullanildi[int(i)]) continue;
        const auto &k = kayitlar[i];
        QVariantMap m;
        m["ad"] = QString::fromStdString(k.ad);
        {
            QString tur = QString::fromStdString(k.tur);
            if (tur.isEmpty() || tur.compare("bilinmiyor", Qt::CaseInsensitive) == 0) {
                tur = k.asinmaYuzde ? QStringLiteral("NVMe") : QStringLiteral("-");
            }
            m["tur"] = tur;
        }
        m["toplamGB"] = k.boyutGB;
        m["bosGB"] = 0.0;
        m["sicaklik"] = QStringLiteral("-");
        m["omur"] = QStringLiteral("-");
        m["durum"] = QString::fromStdString(k.saglikDurumu);
        m["kullanilanYuzde"] = k.asinmaYuzde ? yuzdeYazici(k.asinmaYuzde) : QStringLiteral("-");
        m["yedekYuzde"] = QStringLiteral("-");
        m["okunanGB"] = QStringLiteral("-");
        m["yazilanGB"] = QStringLiteral("-");
        m["acilmaSayisi"] = QStringLiteral("-");
        m["calismaSaati"] = QStringLiteral("-");
        m_diskler.append(m);
    }

    emit skorDegisti();
}
