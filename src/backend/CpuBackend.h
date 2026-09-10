// CpuBackend.h - ISLEMCI sayfasinin C++ tarafi. CPU kullanim/frekans/sicaklik
// (1 sn timer) + GPU listesi (kuruluşta bir kez okunur, degismez varsayilir).
// Sicaklik/saat hizi verileri oncelikle SensorAjani.exe'den (LibreHardwareMonitor)
// alinir, ajan yoksa/cokerse mevcut PDH/registry yontemlerine dusulur.
#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>

#include "core/SensorAjani.h"

#include <optional>

class CpuBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ad READ ad NOTIFY cpuDegisti)
    Q_PROPERTY(int cekirdek READ cekirdek NOTIFY cpuDegisti)
    Q_PROPERTY(int mantiksalCekirdek READ mantiksalCekirdek NOTIFY cpuDegisti)
    Q_PROPERTY(double kullanim READ kullanim NOTIFY cpuDegisti)
    Q_PROPERTY(int mhz READ mhz NOTIFY cpuDegisti)
    Q_PROPERTY(int maxMhz READ maxMhz NOTIFY cpuDegisti)
    Q_PROPERTY(QString sicaklik READ sicaklik NOTIFY cpuDegisti)
    Q_PROPERTY(QVariantList gecmis READ gecmis NOTIFY cpuDegisti)
    Q_PROPERTY(double gpuKullanim READ gpuKullanim NOTIFY cpuDegisti)
    Q_PROPERTY(QVariantList gpuGecmis READ gpuGecmis NOTIFY cpuDegisti)
    Q_PROPERTY(QVariantList gpular READ gpular NOTIFY gpuDegisti)
    Q_PROPERTY(QString gpuSicaklik READ gpuSicaklik NOTIFY gpuSensorDegisti)
    Q_PROPERTY(QString gpuCekirdekMhz READ gpuCekirdekMhz NOTIFY gpuSensorDegisti)
    Q_PROPERTY(QString gpuBellekMhz READ gpuBellekMhz NOTIFY gpuSensorDegisti)
    Q_PROPERTY(bool sensorAjaniVar READ sensorAjaniVar NOTIFY gpuSensorDegisti)

public:
    explicit CpuBackend(SensorAjaniIstemcisi *sensorAjani, QObject *parent = nullptr);

    QString ad() const { return m_ad; }
    int cekirdek() const { return m_cekirdek; }
    int mantiksalCekirdek() const { return m_mantiksalCekirdek; }
    double kullanim() const { return m_kullanim; }
    int mhz() const { return m_mhz; }
    int maxMhz() const { return m_maxMhz; }
    QString sicaklik() const { return m_sicaklik; }
    QVariantList gecmis() const { return m_gecmis; }
    double gpuKullanim() const { return m_gpuKullanim; }
    QVariantList gpuGecmis() const { return m_gpuGecmis; }
    QVariantList gpular() const { return m_gpular; }
    QString gpuSicaklik() const { return m_gpuSicaklik; }
    QString gpuCekirdekMhz() const { return m_gpuCekirdekMhz; }
    QString gpuBellekMhz() const { return m_gpuBellekMhz; }
    bool sensorAjaniVar() const { return m_sensorAjani->son().ajanCalisiyor; }

signals:
    void cpuDegisti();
    void gpuDegisti();
    void gpuSensorDegisti();

private:
    void cpuYenile();
    void ajanVerisiUygulandi();

    QString m_ad;
    int m_cekirdek = 0;
    int m_mantiksalCekirdek = 0;
    double m_kullanim = 0.0;
    int m_mhz = 0;
    int m_maxMhz = 0;
    QString m_sicaklik = "-";
    QVariantList m_gecmis;   // son 40 olcum (kullanim yuzdesi)
    double m_gpuKullanim = 0.0;
    QVariantList m_gpuGecmis; // son 40 olcum (GPU kullanim yuzdesi)
    QVariantList m_gpular;   // {ad, surucuVersiyon, surucuTarihi, bellekGB,
                             //  openglVersiyon, vulkanVersiyon, shaderModel}
    QString m_gpuSicaklik = "-";
    QString m_gpuCekirdekMhz = "-";
    QString m_gpuBellekMhz = "-";

    SensorAjaniIstemcisi *m_sensorAjani;

    QTimer m_timer;
};
