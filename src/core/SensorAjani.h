// SensorAjani.h - LibreHardwareMonitor kullanan harici C# sensor ajanini
// (SensorAjani.exe) QProcess ile calistirip stdout'undan JSON okur. Ajan
// bulunamaz/cokerse ajanCalisiyor false doner, uygulama normal calisir.
#pragma once

#include <QObject>
#include <QProcess>

#include <optional>
#include <string>

#include <QList>
#include <QString>

struct DiskSensorKaydi {
    QString ad;
    std::optional<double> sicaklikC, omurYuzde, yazilanGB, okunanGB, kullanilanYuzde, yedekYuzde, toplamGB, bosGB;
    std::optional<qint64> acilmaSayisi, calismaSaati;
};

struct SensorVerisi {
    std::optional<double> cpuSicaklik, cpuKullanim, gpuSicaklik, gpuKullanim, gpuFanRpm, gpuBellekKullanilanMB;
    std::optional<int> cpuMhz, gpuCekirdekMhz, gpuBellekMhz;
    std::string gpuAd;
    QList<DiskSensorKaydi> diskler;
    bool ajanCalisiyor = false;
};

class SensorAjaniIstemcisi : public QObject
{
    Q_OBJECT

public:
    explicit SensorAjaniIstemcisi(QObject *parent = nullptr);
    ~SensorAjaniIstemcisi() override;

    // SensorAjani.exe'yi bulup calistirir. Bulunamazsa sessizce cikar.
    void baslat();

    // Bosluklu yoldan calisan ajan surucuyu kuramaz - gerekiyorsa
    // bosluksuz bir dizine kopyalar, kullanilacak yolu doner.
    static QString calismaKopyasi(const QString &kaynak);

    const SensorVerisi &son() const { return m_son; }

signals:
    void veriGeldi();

private slots:
    void veriHazir();
    void surecCoktu();

private:
    QProcess m_surec;
    QByteArray m_arabellek;
    SensorVerisi m_son;
    int m_yenidenDenemeSayisi = 0;
    static constexpr int MaksimumYenidenDeneme = 3;
};
