#include "CpuBackend.h"

#include "core/CpuInfo.h"

#include <QVariantMap>

namespace {
constexpr int GecmisUzunluk = 40;
}

namespace {
QString bosIseCizgi(const std::string &s)
{
    return s.empty() ? QStringLiteral("-") : QString::fromStdString(s);
}
}

CpuBackend::CpuBackend(SensorAjaniIstemcisi *sensorAjani, QObject *parent)
    : QObject(parent), m_sensorAjani(sensorAjani)
{
    const GrafikApiBilgisi api = CpuInfo::grafikApileri();
    for (const GpuBilgisi &g : CpuInfo::gpuBilgisi()) {
        QVariantMap m;
        m["ad"] = QString::fromStdString(g.ad);
        m["surucuVersiyon"] = QString::fromStdString(g.surucuVersiyon);
        m["surucuTarihi"] = QString::fromStdString(g.surucuTarihi);
        m["bellekGB"] = g.bellekGB;
        m["openglVersiyon"] = bosIseCizgi(api.opengl);
        m["vulkanVersiyon"] = bosIseCizgi(api.vulkan);
        m["shaderModel"] = bosIseCizgi(api.shaderModel);
        m_gpular.append(m);
    }
    emit gpuDegisti();

    connect(m_sensorAjani, &SensorAjaniIstemcisi::veriGeldi, this, &CpuBackend::ajanVerisiUygulandi);

    cpuYenile();
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, &CpuBackend::cpuYenile);
    m_timer.start();
}

void CpuBackend::cpuYenile()
{
    const CpuBilgisi bilgi = CpuInfo::cpuBilgisi();
    const SensorVerisi &ajan = m_sensorAjani->son();

    m_ad = QString::fromStdString(bilgi.ad);
    m_cekirdek = bilgi.cekirdek;
    m_mantiksalCekirdek = bilgi.mantiksalCekirdek;
    m_kullanim = bilgi.kullanimYuzde;
    m_mhz = bilgi.mhz;
    m_maxMhz = bilgi.maxMhz;

    // SADECE sensor ajanindan gelen deger kullanilir. Eski WMI/ACPI yedegi
    // (bilgi.sicaklikC) CPU degil, kasa/PCH termal bolgesini okuyordu ve
    // 70% yukte 28 C gibi YANLIS deger gosteriyordu - kaldirildi.
    const std::optional<double> sicaklikC = ajan.cpuSicaklik;
    m_sicaklik = sicaklikC ? QString::number(*sicaklikC, 'f', 1) + " C" : "-";

    m_gecmis.append(bilgi.kullanimYuzde);
    while (m_gecmis.size() > GecmisUzunluk) m_gecmis.removeFirst();

    m_gpuKullanim = ajan.gpuKullanim.value_or(CpuInfo::gpuKullanimYuzdesi());
    m_gpuGecmis.append(m_gpuKullanim);
    while (m_gpuGecmis.size() > GecmisUzunluk) m_gpuGecmis.removeFirst();

    m_gpuSicaklik = ajan.gpuSicaklik ? QString::number(*ajan.gpuSicaklik, 'f', 1) + " C" : "-";
    m_gpuCekirdekMhz = ajan.gpuCekirdekMhz ? QString::number(*ajan.gpuCekirdekMhz) + " MHz" : "-";
    m_gpuBellekMhz = ajan.gpuBellekMhz ? QString::number(*ajan.gpuBellekMhz) + " MHz" : "-";

    emit cpuDegisti();
}

void CpuBackend::ajanVerisiUygulandi()
{
    // Ajandan yeni veri geldiginde bir sonraki cpuYenile beklemeden GPU
    // alanlarini ve sensorAjaniVar bayragini guncelle.
    const SensorVerisi &ajan = m_sensorAjani->son();
    m_gpuSicaklik = ajan.gpuSicaklik ? QString::number(*ajan.gpuSicaklik, 'f', 1) + " C" : "-";
    m_gpuCekirdekMhz = ajan.gpuCekirdekMhz ? QString::number(*ajan.gpuCekirdekMhz) + " MHz" : "-";
    m_gpuBellekMhz = ajan.gpuBellekMhz ? QString::number(*ajan.gpuBellekMhz) + " MHz" : "-";
    emit gpuSensorDegisti();
}
