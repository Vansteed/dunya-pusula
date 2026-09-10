#include "MemoryBackend.h"

#include "core/SysInfo.h"
#include "core/Settings.h"
#include "ui/CleanWorker.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QThread>

namespace {
QString ayarDosyaYolu()
{
    // PowerShell ve Widgets surumleriyle AYNI konum/sema - eski ayarlar
    // dosyasi oldugu gibi okunur.
    return QCoreApplication::applicationDirPath() + "/ayarlar.json";
}
constexpr int GecmisUzunluk = 40;
constexpr qint64 OtomatikBekleme = 5 * 60 * 1000; // 5 dk
}

MemoryBackend::MemoryBackend(QObject *parent)
    : QObject(parent)
{
    const AyarVerisi ayar = Settings::yukle(ayarDosyaYolu().toStdString());
    m_otomatik = ayar.autoClean;
    m_esik = ayar.threshold;

    ramYenile();
    gunlukEkle("BELLEK sayfası hazır.");

    m_ramTimer.setInterval(2000);
    connect(&m_ramTimer, &QTimer::timeout, this, &MemoryBackend::ramYenile);
    m_ramTimer.start();

    // Otomatik temizlik OZELLIGI KALDIRILDI (kullanici istegi): temizlik
    // yalnizca SIMDI TEMIZLE ile calisir. Zamanlayici baslatilmaz.
}

MemoryBackend::~MemoryBackend()
{
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
}

void MemoryBackend::ramYenile()
{
    const RamBilgisi bilgi = SysInfo::ramBilgisi();
    m_yuzde = bilgi.percent;
    m_toplamGB = bilgi.totalGB;
    m_kullanilanGB = bilgi.usedGB;
    m_bosGB = bilgi.freeGB;

    m_gecmis.append(bilgi.percent);
    while (m_gecmis.size() > GecmisUzunluk) m_gecmis.removeFirst();

    emit ramDegisti();
}

void MemoryBackend::otomatikKontrol()
{
    if (!m_otomatik || m_temizleniyor) return;
    if (m_yuzde < m_esik) return;

    const qint64 simdi = QDateTime::currentMSecsSinceEpoch();
    if (m_sonOtomatik != 0 && simdi - m_sonOtomatik < OtomatikBekleme) return;

    m_sonOtomatik = simdi;
    gunlukEkle(QString("Otomatik temizlik tetiklendi (%1%).")
                   .arg(m_yuzde, 0, 'f', 1));
    temizle();
}

void MemoryBackend::kapatmaBasla()
{
    ramYenile();
    m_kullanilanGBOncesi = m_kullanilanGB;
}

void MemoryBackend::kapatmaBitti(int kapatilanSayisi)
{
    // Kapatilan surecin bellegi aninda dusmuyor - kisa gecikmeyle olc.
    QTimer::singleShot(900, this, [this, kapatilanSayisi]() {
        ramYenile();
        const double kazanc = m_kullanilanGBOncesi - m_kullanilanGB;
        m_sonKazanc = kazanc >= 0.05
                          ? QString("%1 GB boşaldı").arg(kazanc, 0, 'f', 1)
                          : QString();
        emit temizlemeDurumu();
        gunlukEkle(QString("%1 uygulama kapatıldı%2.")
                       .arg(kapatilanSayisi)
                       .arg(m_sonKazanc.isEmpty() ? QString()
                                                  : QString(" - ") + m_sonKazanc));
    });
}

void MemoryBackend::temizle()
{
    if (m_temizleniyor) return;
    m_temizleniyor = true;
    m_adim = 0;
    m_kullanilanGBOncesi = m_kullanilanGB;
    emit temizlemeDurumu();

    m_thread = new QThread(this);
    m_worker = new CleanWorker();
    m_worker->moveToThread(m_thread);

    connect(m_thread, &QThread::started, m_worker, &CleanWorker::calistir);
    connect(m_worker, &CleanWorker::adimBasladi, this, [this](int adim) {
        m_adim = adim;
        emit temizlemeDurumu();
    });
    connect(m_worker, &CleanWorker::adimBitti, this, [this](int, const QString &mesaj) {
        gunlukEkle(mesaj);
    });
    connect(m_worker, &CleanWorker::tamamlandi, this, [this]() {
        m_temizleniyor = false;
        m_adim = 3;
        ramYenile();
        const double kazanc = m_kullanilanGBOncesi - m_kullanilanGB;
        m_sonKazanc = kazanc >= 0.05 ? QString("%1 GB boşaltıldı").arg(kazanc, 0, 'f', 1)
                                     : QString();
        emit temizlemeDurumu();
        gunlukEkle("Temizlik tamamlandı.");
        m_thread = nullptr;
        m_worker = nullptr;
    });
    connect(m_worker, &CleanWorker::tamamlandi, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);

    m_thread->start();
}

void MemoryBackend::otomatikAta(bool acik)
{
    if (m_otomatik == acik) return;
    m_otomatik = acik;
    ayarlariKaydet();
    emit ayarDegisti();
}

void MemoryBackend::esikAta(int deger)
{
    if (m_esik == deger) return;
    m_esik = deger;
    ayarlariKaydet();
    emit ayarDegisti();
}

void MemoryBackend::ayarlariKaydet()
{
    AyarVerisi veri;
    veri.autoClean = m_otomatik;
    veri.threshold = m_esik;
    Settings::kaydet(ayarDosyaYolu().toStdString(), veri);
}

void MemoryBackend::gunlukEkle(const QString &satir)
{
    const QString zaman = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_gunlukler.append(QString("[%1] %2").arg(zaman, satir));
    while (m_gunlukler.size() > 200) m_gunlukler.removeFirst();
    emit gunlukDegisti();
}

void MemoryBackend::gunlukTemizle()
{
    m_gunlukler.clear();
    emit gunlukDegisti();
}
