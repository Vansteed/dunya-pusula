#include "DiskTab.h"
#include "DiskScanWorker.h"

#include "core/SysInfo.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QFrame>
#include <QThread>
#include <QTimer>

DiskTab::DiskTab(QWidget *parent)
    : QWidget(parent)
{
    auto *anaDuzen = new QVBoxLayout(this);
    anaDuzen->setContentsMargins(12, 10, 12, 6);
    anaDuzen->setSpacing(8);

    anaDuzen->addWidget(diskKartiOlustur());
    anaDuzen->addWidget(taramaKartiOlustur(), 1);

    diskBilgisiYenile();

    auto *timerDisk = new QTimer(this);
    timerDisk->setInterval(2000);
    connect(timerDisk, &QTimer::timeout, this, &DiskTab::diskBilgisiYenile);
    timerDisk->start();
}

DiskTab::~DiskTab()
{
    if (m_taramaThread) {
        m_taramaThread->quit();
        m_taramaThread->wait();
    }
}

QWidget *DiskTab::diskKartiOlustur()
{
    auto *kart = new QFrame(this);
    kart->setObjectName("Kart");
    auto *duzen = new QVBoxLayout(kart);

    auto *baslik = new QLabel("DISK KULLANIMI", kart);
    baslik->setStyleSheet("color:#71717A; font-size:11px;");
    duzen->addWidget(baslik);

    auto *yuzdeSatir = new QHBoxLayout();
    m_lblPercent = new QLabel("--", kart);
    m_lblPercent->setStyleSheet("color:#F4F4F5; font-size:30px; font-weight:600;");
    yuzdeSatir->addWidget(m_lblPercent);
    auto *yuzdeIsareti = new QLabel("%", kart);
    yuzdeIsareti->setStyleSheet("color:#71717A; font-size:15px;");
    yuzdeSatir->addWidget(yuzdeIsareti);
    yuzdeSatir->addStretch(1);
    duzen->addLayout(yuzdeSatir);

    m_barDisk = new QProgressBar(kart);
    m_barDisk->setRange(0, 100);
    m_barDisk->setTextVisible(false);
    duzen->addWidget(m_barDisk);

    auto *altGrid = new QGridLayout();
    auto ekleSutun = [&](int sutun, const QString &baslikMetni, QLabel **hedef) {
        auto *b = new QLabel(baslikMetni, kart);
        b->setStyleSheet("color:#71717A; font-size:11px;");
        *hedef = new QLabel("- GB", kart);
        (*hedef)->setStyleSheet("color:#F4F4F5; font-size:13px; font-weight:500;");
        altGrid->addWidget(b, 0, sutun);
        altGrid->addWidget(*hedef, 1, sutun);
    };
    ekleSutun(0, "Toplam", &m_lblTotal);
    ekleSutun(1, "Kullanilan", &m_lblUsed);
    ekleSutun(2, "Bos", &m_lblFree);
    duzen->addLayout(altGrid);

    return kart;
}

QWidget *DiskTab::taramaKartiOlustur()
{
    auto *kart = new QFrame(this);
    kart->setObjectName("Kart");
    auto *duzen = new QVBoxLayout(kart);

    auto *ustSatir = new QHBoxLayout();
    auto *baslik = new QLabel("TEMIZLENEBILIR ALAN", kart);
    baslik->setStyleSheet("color:#71717A; font-size:11px;");
    ustSatir->addWidget(baslik);
    ustSatir->addStretch(1);
    m_btnTara = new QPushButton("TARA", kart);
    connect(m_btnTara, &QPushButton::clicked, this, &DiskTab::taramaBaslat);
    ustSatir->addWidget(m_btnTara);
    duzen->addLayout(ustSatir);

    m_barTarama = new QProgressBar(kart);
    m_barTarama->setRange(0, 100);
    m_barTarama->setValue(0);
    m_barTarama->setTextVisible(false);
    duzen->addWidget(m_barTarama);

    m_tablo = new QTableWidget(0, 4, kart);
    m_tablo->setHorizontalHeaderLabels({"Konum", "Boyut (MB)", "Dosya Sayisi", "Durum"});
    m_tablo->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tablo->verticalHeader()->setVisible(false);
    m_tablo->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablo->setSelectionBehavior(QAbstractItemView::SelectRows);
    duzen->addWidget(m_tablo, 1);

    return kart;
}

void DiskTab::diskBilgisiYenile()
{
    if (m_taraniyor) return;

    const DiskBilgisi bilgi = SysInfo::diskKullanim();
    m_lblPercent->setText(QString::number(bilgi.yuzde, 'f', 1));
    m_barDisk->setValue(static_cast<int>(bilgi.yuzde));
    m_lblUsed->setText(QString("%1 GB").arg(bilgi.kullanilanGB, 0, 'f', 2));
    m_lblTotal->setText(QString("%1 GB").arg(bilgi.toplamGB, 0, 'f', 2));
    m_lblFree->setText(QString("%1 GB").arg(bilgi.bosGB, 0, 'f', 2));
}

void DiskTab::taramaBaslat()
{
    if (m_taraniyor) return;
    m_taraniyor = true;
    m_btnTara->setEnabled(false);
    m_btnTara->setText("Taraniyor...");
    m_barTarama->setRange(0, 0); // belirsiz ilerleme

    m_taramaThread = new QThread(this);
    m_taramaWorker = new DiskScanWorker();
    m_taramaWorker->moveToThread(m_taramaThread);

    connect(m_taramaThread, &QThread::started, m_taramaWorker, &DiskScanWorker::tara);
    connect(m_taramaWorker, &DiskScanWorker::tamamlandi, this, &DiskTab::taramaBitti);
    connect(m_taramaWorker, &DiskScanWorker::tamamlandi, m_taramaThread, &QThread::quit);
    connect(m_taramaThread, &QThread::finished, m_taramaWorker, &QObject::deleteLater);

    m_taramaThread->start();
}

void DiskTab::taramaBitti()
{
    m_tablo->setRowCount(0);
    for (const auto &satir : m_taramaWorker->sonucAl()) {
        const int r = m_tablo->rowCount();
        m_tablo->insertRow(r);
        m_tablo->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(satir.konum)));
        m_tablo->setItem(r, 1, new QTableWidgetItem(QString::number(satir.boyut / (1024 * 1024))));
        m_tablo->setItem(r, 2, new QTableWidgetItem(QString::number(satir.sayi)));
        m_tablo->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(satir.durum)));
    }

    m_barTarama->setRange(0, 100);
    m_barTarama->setValue(100);
    m_taraniyor = false;
    m_btnTara->setEnabled(true);
    m_btnTara->setText("TARA");
    m_taramaThread = nullptr;
    m_taramaWorker = nullptr;
}
