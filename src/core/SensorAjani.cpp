#include "SensorAjani.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

namespace {

std::optional<double> ceviD(const QJsonObject &o, const char *ad)
{
    if (!o.contains(ad) || o.value(ad).isNull()) return std::nullopt;
    return o.value(ad).toDouble();
}

std::optional<int> ceviI(const QJsonObject &o, const char *ad)
{
    if (!o.contains(ad) || o.value(ad).isNull()) return std::nullopt;
    return o.value(ad).toInt();
}

std::optional<qint64> ceviI64(const QJsonObject &o, const char *ad)
{
    if (!o.contains(ad) || o.value(ad).isNull()) return std::nullopt;
    return static_cast<qint64>(o.value(ad).toDouble());
}

}

SensorAjaniIstemcisi::SensorAjaniIstemcisi(QObject *parent)
    : QObject(parent)
{
    connect(&m_surec, &QProcess::readyReadStandardOutput, this, &SensorAjaniIstemcisi::veriHazir);
    connect(&m_surec, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SensorAjaniIstemcisi::surecCoktu);
}

SensorAjaniIstemcisi::~SensorAjaniIstemcisi()
{
    if (m_surec.state() != QProcess::NotRunning) {
        m_surec.terminate();
        m_surec.waitForFinished(2000);
    }
}

// LibreHardwareMonitor CPU sicakligi icin cekirdek surucusunu ajan exe'sinin
// yaninda olusturur ve servis olarak kaydeder. Servis yolu BOSLUK iceriyorsa
// kayit basarisiz oluyor (klasik tirnaksiz ImagePath sorunu) ve tum MSR
// tabanli degerler - sicaklik, saat, guc - sessizce bos donuyor.
// Bu yuzden ajan bosluksuz sabit bir dizine kopyalanip oradan calistirilir.
QString SensorAjaniIstemcisi::calismaKopyasi(const QString &kaynak)
{
    if (!kaynak.contains(' ')) return kaynak;

    const QString hedefDizin = QStringLiteral("C:/ProgramData/DunyaPusula");
    QDir().mkpath(hedefDizin);
    const QString hedef = hedefDizin + "/SensorAjani.exe";

    const QFileInfo kaynakBilgi(kaynak);
    const QFileInfo hedefBilgi(hedef);
    const bool guncelle = !hedefBilgi.exists() ||
                          hedefBilgi.size() != kaynakBilgi.size() ||
                          hedefBilgi.lastModified() < kaynakBilgi.lastModified();
    if (guncelle) {
        QFile::remove(hedef);
        if (!QFile::copy(kaynak, hedef)) return kaynak;   // kopyalanamadi - yine de dene
    }
    return hedef;
}

void SensorAjaniIstemcisi::baslat()
{
    const QString kaynak = QCoreApplication::applicationDirPath() + "/SensorAjani.exe";
    if (!QFile::exists(kaynak)) {
        m_son.ajanCalisiyor = false;
        return;
    }
    const QString yol = calismaKopyasi(kaynak);
    m_arabellek.clear();
    m_surec.setWorkingDirectory(QFileInfo(yol).absolutePath());
    m_surec.start(yol);
}

void SensorAjaniIstemcisi::veriHazir()
{
    m_arabellek += m_surec.readAllStandardOutput();

    int satirSonu;
    while ((satirSonu = m_arabellek.indexOf('\n')) >= 0) {
        const QByteArray satir = m_arabellek.left(satirSonu).trimmed();
        m_arabellek.remove(0, satirSonu + 1);
        if (satir.isEmpty()) continue;

        QJsonParseError hata;
        const QJsonDocument belge = QJsonDocument::fromJson(satir, &hata);
        if (hata.error != QJsonParseError::NoError || !belge.isObject()) continue;

        const QJsonObject o = belge.object();
        SensorVerisi v;
        v.ajanCalisiyor = true;
        v.cpuSicaklik = ceviD(o, "cpuSicaklik");
        v.cpuKullanim = ceviD(o, "cpuKullanim");
        v.cpuMhz = ceviI(o, "cpuMhz");
        v.gpuAd = o.value("gpuAd").toString().toStdString();
        v.gpuSicaklik = ceviD(o, "gpuSicaklik");
        v.gpuKullanim = ceviD(o, "gpuKullanim");
        v.gpuCekirdekMhz = ceviI(o, "gpuCekirdekMhz");
        v.gpuBellekMhz = ceviI(o, "gpuBellekMhz");
        v.gpuFanRpm = ceviD(o, "gpuFanRpm");
        v.gpuBellekKullanilanMB = ceviD(o, "gpuBellekKullanilanMB");

        for (const QJsonValue &dv : o.value("diskler").toArray()) {
            const QJsonObject d = dv.toObject();
            DiskSensorKaydi k;
            k.ad = d.value("ad").toString();
            k.sicaklikC = ceviD(d, "sicaklikC");
            k.omurYuzde = ceviD(d, "omurYuzde");
            k.yazilanGB = ceviD(d, "yazilanGB");
            k.okunanGB = ceviD(d, "okunanGB");
            k.acilmaSayisi = ceviI64(d, "acilmaSayisi");
            k.calismaSaati = ceviI64(d, "calismaSaati");
            k.kullanilanYuzde = ceviD(d, "kullanilanYuzde");
            k.yedekYuzde = ceviD(d, "yedekYuzde");
            k.toplamGB = ceviD(d, "toplamGB");
            k.bosGB = ceviD(d, "bosGB");
            v.diskler.append(k);
        }

        m_son = v;
        emit veriGeldi();
        m_yenidenDenemeSayisi = 0; // basarili veri geldi, sayaci sifirla
    }
}

void SensorAjaniIstemcisi::surecCoktu()
{
    m_son.ajanCalisiyor = false;
    emit veriGeldi();

    if (m_yenidenDenemeSayisi >= MaksimumYenidenDeneme) return;
    ++m_yenidenDenemeSayisi;
    QTimer::singleShot(5000, this, [this]() { baslat(); });
}
