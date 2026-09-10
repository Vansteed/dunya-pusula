#include "HealthTab.h"

#include "core/SysInfo.h"
#include "core/DiskHealth.h"
#include "core/HealthScore.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QFrame>

HealthTab::HealthTab(QWidget *parent)
    : QWidget(parent)
{
    auto *anaDuzen = new QVBoxLayout(this);
    anaDuzen->setContentsMargins(12, 10, 12, 6);
    anaDuzen->setSpacing(8);

    anaDuzen->addWidget(skorKartiOlustur());
    anaDuzen->addWidget(saglikKartiOlustur(), 1);

    yenile();
}

QWidget *HealthTab::skorKartiOlustur()
{
    auto *kart = new QFrame(this);
    kart->setObjectName("Kart");
    auto *duzen = new QVBoxLayout(kart);

    auto *ustSatir = new QHBoxLayout();
    auto *baslik = new QLabel("SAGLIK SKORU", kart);
    baslik->setStyleSheet("color:#71717A; font-size:11px;");
    ustSatir->addWidget(baslik);
    ustSatir->addStretch(1);
    m_btnYenile = new QPushButton("YENILE", kart);
    connect(m_btnYenile, &QPushButton::clicked, this, &HealthTab::yenile);
    ustSatir->addWidget(m_btnYenile);
    duzen->addLayout(ustSatir);

    m_lblPuan = new QLabel("--", kart);
    m_lblPuan->setStyleSheet("font-size:30px; font-weight:600;");
    duzen->addWidget(m_lblPuan);

    m_lblYorum = new QLabel(kart);
    m_lblYorum->setStyleSheet("color:#A1A1AA; font-size:12px;");
    duzen->addWidget(m_lblYorum);

    auto *altGrid = new QGridLayout();
    auto ekleSutun = [&](int sutun, const QString &baslikMetni, QLabel **hedef) {
        auto *b = new QLabel(baslikMetni, kart);
        b->setStyleSheet("color:#71717A; font-size:11px;");
        *hedef = new QLabel("-", kart);
        (*hedef)->setStyleSheet("color:#F4F4F5; font-size:13px; font-weight:500;");
        altGrid->addWidget(b, 0, sutun);
        altGrid->addWidget(*hedef, 1, sutun);
    };
    ekleSutun(0, "RAM", &m_lblRam);
    ekleSutun(1, "Disk", &m_lblDisk);
    ekleSutun(2, "Baslangic", &m_lblBaslangic);
    ekleSutun(3, "Disk Sagligi", &m_lblDiskSaglik);
    duzen->addLayout(altGrid);

    return kart;
}

QWidget *HealthTab::saglikKartiOlustur()
{
    auto *kart = new QFrame(this);
    kart->setObjectName("Kart");
    auto *duzen = new QVBoxLayout(kart);

    auto *baslik = new QLabel("DISK SAGLIGI (SMART)", kart);
    baslik->setStyleSheet("color:#71717A; font-size:11px;");
    duzen->addWidget(baslik);

    m_lblUyari = new QLabel(
        "Yonetici yetkisi gerekli - disk sagligi bilgisi icin uygulamayi yonetici olarak calistirin.",
        kart);
    m_lblUyari->setStyleSheet("color:#D6B25E; font-size:11px;");
    m_lblUyari->setWordWrap(true);
    m_lblUyari->setVisible(false);
    duzen->addWidget(m_lblUyari);

    m_tablo = new QTableWidget(0, 5, kart);
    m_tablo->setHorizontalHeaderLabels({"Ad", "Tur", "Boyut (GB)", "Durum", "Asinma %"});
    m_tablo->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tablo->verticalHeader()->setVisible(false);
    m_tablo->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tablo->setSelectionBehavior(QAbstractItemView::SelectRows);
    duzen->addWidget(m_tablo, 1);

    return kart;
}

void HealthTab::yenile()
{
    const RamBilgisi ram = SysInfo::ramBilgisi();
    const DiskBilgisi disk = SysInfo::diskKullanim();
    const int baslangicSayisi = SysInfo::baslangicOgeSayisi();
    const std::vector<DiskSaglikKaydi> diskler = DiskHealth::diskSagligi();

    std::optional<double> asinma;
    bool diskSaglikli = false;
    if (!diskler.empty()) {
        asinma = diskler.front().asinmaYuzde;
        diskSaglikli = diskler.front().saglikDurumu == "Healthy";
    }

    const double diskBosYuzde = 100.0 - disk.yuzde;
    const SaglikSonuc sonuc = hesaplaSaglikSkoru(ram.percent, diskBosYuzde, baslangicSayisi,
                                                  asinma, diskSaglikli);

    m_lblPuan->setText(QString::number(sonuc.puan));
    QString renk = "#C46666"; // kirmizi
    if (sonuc.puan >= 80) renk = "#60A578"; // yesil
    else if (sonuc.puan >= 60) renk = "#D6B25E"; // sari
    m_lblPuan->setStyleSheet(QString("color:%1; font-size:30px; font-weight:600;").arg(renk));
    m_lblYorum->setText(QString::fromStdString(sonuc.yorum));

    m_lblRam->setText(QString::number(sonuc.kirilim.ram));
    m_lblDisk->setText(QString::number(sonuc.kirilim.disk));
    m_lblBaslangic->setText(QString::number(sonuc.kirilim.baslangic));
    m_lblDiskSaglik->setText(QString::number(sonuc.kirilim.diskSaglik));

    m_lblUyari->setVisible(!DiskHealth::yoneticiMi());

    m_tablo->setRowCount(0);
    for (const auto &d : diskler) {
        const int r = m_tablo->rowCount();
        m_tablo->insertRow(r);
        m_tablo->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(d.ad)));
        m_tablo->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(d.tur)));
        m_tablo->setItem(r, 2, new QTableWidgetItem(QString::number(d.boyutGB, 'f', 1)));
        m_tablo->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(d.saglikDurumu)));
        const QString asinmaMetni = d.asinmaYuzde ? QString::number(*d.asinmaYuzde, 'f', 1) : "-";
        m_tablo->setItem(r, 4, new QTableWidgetItem(asinmaMetni));
    }
}
