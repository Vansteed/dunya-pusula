#include "MemoryTab.h"
#include "GraphWidget.h"
#include "IconCache.h"
#include "CleanWorker.h"

#include "core/SysInfo.h"
#include "core/ProcessList.h"
#include "core/Settings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTreeWidget>
#include <QShortcut>
#include <QLineEdit>
#include <QCheckBox>
#include <QSlider>
#include <QPlainTextEdit>
#include <QThread>
#include <QMessageBox>
#include <QTimer>
#include <QCoreApplication>
#include <QHeaderView>
#include <QFrame>
#include <QScrollBar>

#include <windows.h>

namespace {
QString ayarDosyaYolu()
{
    // ayarlar.json exe yaninda - PowerShell surumundeki Join-Path
    // (Split-Path $PSCommandPath) 'ayarlar.json' ile AYNI konum mantigi.
    return QCoreApplication::applicationDirPath() + "/ayarlar.json";
}
}

MemoryTab::MemoryTab(QWidget *parent)
    : QWidget(parent)
{
    auto *anaDuzen = new QHBoxLayout(this);
    anaDuzen->setContentsMargins(12, 10, 12, 6);
    anaDuzen->setSpacing(8);

    // Sol sutun: RAM, grafik, temizle, otomatik ve log kartlari - sabit
    // yakin genislikte, alt alta (mevcut sirayla).
    auto *solSutun = new QWidget(this);
    solSutun->setMaximumWidth(440);
    auto *solDuzen = new QVBoxLayout(solSutun);
    solDuzen->setContentsMargins(0, 0, 0, 0);
    solDuzen->setSpacing(8);
    solDuzen->addWidget(ramKartiOlustur());
    solDuzen->addWidget(grafikKartiOlustur());
    solDuzen->addWidget(temizleKartiOlustur());
    solDuzen->addWidget(otomatikKartiOlustur());
    solDuzen->addWidget(logKartiOlustur());
    anaDuzen->addWidget(solSutun, 0);

    // Sag sutun: islem tablosu tek basina, bastan asagi tum yuksekligi kaplar.
    anaDuzen->addWidget(islemTablosuKartiOlustur(), 1);

    ayarlariYukle();

    // Ilk deger + islem listesi.
    ramBilgisiYenile();
    islemListesiYenile();
    log("BELLEK sekmesi hazir.");

    auto *timerRam = new QTimer(this);
    timerRam->setInterval(2000);
    connect(timerRam, &QTimer::timeout, this, &MemoryTab::ramBilgisiYenile);
    timerRam->start();

    auto *timerIslem = new QTimer(this);
    timerIslem->setInterval(3000);
    connect(timerIslem, &QTimer::timeout, this, &MemoryTab::islemListesiYenile);
    timerIslem->start();

    auto *timerOtomatik = new QTimer(this);
    timerOtomatik->setInterval(10000);
    connect(timerOtomatik, &QTimer::timeout, this, &MemoryTab::otomatikKontrolEt);
    timerOtomatik->start();
}

MemoryTab::~MemoryTab()
{
    if (m_temizleThread) {
        m_temizleThread->quit();
        m_temizleThread->wait();
    }
}

QWidget *MemoryTab::ramKartiOlustur()
{
    auto *kart = new QFrame(this);
    kart->setObjectName("Kart");
    auto *duzen = new QVBoxLayout(kart);

    auto *ustSatir = new QHBoxLayout();
    auto *baslik = new QLabel("RAM KULLANIMI", kart);
    baslik->setStyleSheet("color:#71717A; font-size:11px;");
    ustSatir->addWidget(baslik);
    ustSatir->addStretch(1);
    duzen->addLayout(ustSatir);

    auto *yuzdeSatir = new QHBoxLayout();
    m_lblPercent = new QLabel("--", kart);
    m_lblPercent->setStyleSheet("color:#F4F4F5; font-size:30px; font-weight:600;");
    yuzdeSatir->addWidget(m_lblPercent);
    auto *yuzdeIsareti = new QLabel("%", kart);
    yuzdeIsareti->setStyleSheet("color:#71717A; font-size:15px;");
    yuzdeSatir->addWidget(yuzdeIsareti);
    yuzdeSatir->addStretch(1);
    duzen->addLayout(yuzdeSatir);

    m_barRam = new QProgressBar(kart);
    m_barRam->setRange(0, 100);
    m_barRam->setTextVisible(false);
    duzen->addWidget(m_barRam);

    m_lblWarn = new QLabel("! RAM kullanimi kritik seviyede", kart);
    m_lblWarn->setStyleSheet("color:#E4E4E7; font-size:11px;");
    m_lblWarn->setVisible(false);
    duzen->addWidget(m_lblWarn);

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

QWidget *MemoryTab::grafikKartiOlustur()
{
    auto *kart = new QFrame(this);
    kart->setObjectName("Kart");
    auto *duzen = new QVBoxLayout(kart);
    auto *baslik = new QLabel("SON 80 SANIYE", kart);
    baslik->setStyleSheet("color:#71717A; font-size:11px;");
    duzen->addWidget(baslik);
    m_grafik = new GraphWidget(kart);
    duzen->addWidget(m_grafik);
    return kart;
}

QWidget *MemoryTab::temizleKartiOlustur()
{
    auto *kart = new QFrame(this);
    kart->setObjectName("Kart");
    auto *duzen = new QHBoxLayout(kart);

    m_btnTemizle = new QPushButton("SIMDI TEMIZLE", kart);
    connect(m_btnTemizle, &QPushButton::clicked, this, &MemoryTab::temizlemeBaslat);
    duzen->addWidget(m_btnTemizle, 1);

    auto *noktalar = new QHBoxLayout();
    m_step1 = new QLabel(kart);
    m_step2 = new QLabel(kart);
    m_step3 = new QLabel(kart);
    adimNoktasiGuncelle(m_step1, "idle");
    adimNoktasiGuncelle(m_step2, "idle");
    adimNoktasiGuncelle(m_step3, "idle");
    noktalar->addWidget(m_step1);
    noktalar->addWidget(m_step2);
    noktalar->addWidget(m_step3);
    duzen->addLayout(noktalar);

    return kart;
}

QWidget *MemoryTab::islemTablosuKartiOlustur()
{
    auto *kart = new QFrame(this);
    kart->setObjectName("Kart");
    auto *duzen = new QVBoxLayout(kart);

    auto *ustSatir = new QHBoxLayout();
    m_txtArama = new QLineEdit(kart);
    m_txtArama->setPlaceholderText("Islem ara...");
    ustSatir->addWidget(m_txtArama, 1);
    m_btnYenile = new QPushButton("Yenile", kart);
    connect(m_btnYenile, &QPushButton::clicked, this, [this] {
        islemListesiYenile();
        log("Liste yenilendi.");
    });
    ustSatir->addWidget(m_btnYenile);
    duzen->addLayout(ustSatir);

    connect(m_txtArama, &QLineEdit::textChanged, this, &MemoryTab::filtreUygula);

    m_tabloGorunum = new QTreeWidget(kart);
    m_tabloGorunum->setColumnCount(5);
    m_tabloGorunum->setHeaderLabels({"Sec", "Islem", "RAM", "PID", "Durum"});
    // Native expand/collapse oku QSS-temali agacta gorunmuyor (bkz. asagidaki
    // metin-tabanli ok cozumu) - bos yer kaplamasin diye kapatildi. Satira
    // cift tiklamak ya da metin okuna tiklamak genisletir/kapatir.
    m_tabloGorunum->setRootIsDecorated(false);
    m_tabloGorunum->setIndentation(16);
    m_tabloGorunum->setUniformRowHeights(true);
    m_tabloGorunum->setSelectionBehavior(QAbstractItemView::SelectRows);
    // Durum sutunu kisa metin icerdigi icin (Kapatilabilir/Sistem - Korumali)
    // setStretchLastSection ile boyle genis bosluk birakiyordu - stretch'i
    // Islem (isim) sutununa ver, digerlerini sabit dar genislikte tut.
    enum { SutunSec = 0, SutunAd, SutunRam, SutunPid, SutunDurum };
    auto *baslik = m_tabloGorunum->header();
    baslik->setSectionResizeMode(SutunSec, QHeaderView::Fixed);
    baslik->resizeSection(SutunSec, 40);
    baslik->setSectionResizeMode(SutunAd, QHeaderView::Stretch);
    baslik->setSectionResizeMode(SutunRam, QHeaderView::Fixed);
    baslik->resizeSection(SutunRam, 80);
    baslik->setSectionResizeMode(SutunPid, QHeaderView::Fixed);
    baslik->resizeSection(SutunPid, 70);
    baslik->setSectionResizeMode(SutunDurum, QHeaderView::Fixed);
    baslik->resizeSection(SutunDurum, 130);
    duzen->addWidget(m_tabloGorunum, 1);

    // Delete tusu ile de secilenleri kapat - WidgetShortcut context sadece
    // tablo odaktayken calissin diye (arama kutusunda yazarken karismasin).
    auto *silKisayolu = new QShortcut(QKeySequence(Qt::Key_Delete), m_tabloGorunum);
    silKisayolu->setContext(Qt::WidgetShortcut);
    connect(silKisayolu, &QShortcut::activated, this, &MemoryTab::secilenleriKapat);

    // Metin tabanli ok - genisletince/kapatinca yon degistir (bkz. yukaridaki
    // okKapali yorumu, islemListesiYenile icinde).
    connect(m_tabloGorunum, &QTreeWidget::itemExpanded, this, [](QTreeWidgetItem *it) {
        if (it->childCount() == 0) return;
        QString t = it->text(1);
        if (t.startsWith(QStringLiteral(u"▸ "))) t.replace(0, 2, QStringLiteral(u"▾ "));
        it->setText(1, t);
    });
    connect(m_tabloGorunum, &QTreeWidget::itemCollapsed, this, [](QTreeWidgetItem *it) {
        if (it->childCount() == 0) return;
        QString t = it->text(1);
        if (t.startsWith(QStringLiteral(u"▾ "))) t.replace(0, 2, QStringLiteral(u"▸ "));
        it->setText(1, t);
    });

    // Windows Gorev Yoneticisi gibi - ok isaretine degil, satirin Islem
    // sutununa tek tikla da genislet/kapat. Checkbox sutunu (Sec) kendi
    // davranisini korusun diye sadece SutunAd icin tetiklenir.
    connect(m_tabloGorunum, &QTreeWidget::itemClicked, this, [](QTreeWidgetItem *it, int sutun) {
        if (sutun != 1) return;
        if (it->childCount() == 0) return;
        it->setExpanded(!it->isExpanded());
    });

    return kart;
}

QWidget *MemoryTab::otomatikKartiOlustur()
{
    auto *kart = new QFrame(this);
    kart->setObjectName("Kart");
    auto *duzen = new QHBoxLayout(kart);

    m_toggleOtomatik = new QCheckBox("Otomatik temizlik", kart);
    connect(m_toggleOtomatik, &QCheckBox::toggled, this, &MemoryTab::otomatikDegisti);
    duzen->addWidget(m_toggleOtomatik);

    duzen->addStretch(1);

    m_sliderEsik = new QSlider(Qt::Horizontal, kart);
    m_sliderEsik->setRange(50, 97);
    m_sliderEsik->setValue(85);
    connect(m_sliderEsik, &QSlider::valueChanged, this, &MemoryTab::esikDegisti);
    duzen->addWidget(m_sliderEsik);

    m_lblEsik = new QLabel("%85", kart);
    duzen->addWidget(m_lblEsik);

    return kart;
}

QWidget *MemoryTab::logKartiOlustur()
{
    auto *kart = new QFrame(this);
    kart->setObjectName("Kart");
    auto *duzen = new QVBoxLayout(kart);
    auto *baslik = new QLabel("ISLEM GECMISI", kart);
    baslik->setStyleSheet("color:#71717A; font-size:11px;");
    duzen->addWidget(baslik);

    m_logKutusu = new QPlainTextEdit(kart);
    m_logKutusu->setReadOnly(true);
    m_logKutusu->setMaximumBlockCount(500);
    m_logKutusu->setStyleSheet("background-color:#0C0C0E; color:#C6C6CB;");
    m_logKutusu->setFixedHeight(120);
    duzen->addWidget(m_logKutusu);

    return kart;
}

void MemoryTab::ayarlariYukle()
{
    const AyarVerisi veri = Settings::yukle(ayarDosyaYolu().toStdString());
    m_sliderEsik->blockSignals(true);
    m_sliderEsik->setValue(veri.threshold);
    m_sliderEsik->blockSignals(false);
    m_lblEsik->setText(QString("%%1").arg(veri.threshold));
    m_toggleOtomatik->blockSignals(true);
    m_toggleOtomatik->setChecked(veri.autoClean);
    m_toggleOtomatik->blockSignals(false);
}

void MemoryTab::ayarlariKaydet()
{
    AyarVerisi veri;
    veri.autoClean = m_toggleOtomatik->isChecked();
    veri.threshold = m_sliderEsik->value();
    Settings::kaydet(ayarDosyaYolu().toStdString(), veri);
}

void MemoryTab::log(const QString &mesaj)
{
    const QString zamanDamgasi = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logKutusu->appendPlainText(QString("[%1] %2").arg(zamanDamgasi, mesaj));
    m_logKutusu->verticalScrollBar()->setValue(m_logKutusu->verticalScrollBar()->maximum());
}

void MemoryTab::adimNoktasiGuncelle(QLabel *nokta, const QString &durum)
{
    QString renk = "#2E2E33"; // idle
    if (durum == "work") renk = "#A1A1AA";
    else if (durum == "done") renk = "#4C5F7E";
    nokta->setFixedSize(10, 10);
    nokta->setStyleSheet(QString("background-color:%1; border-radius:5px;").arg(renk));
}

void MemoryTab::ramBilgisiYenile()
{
    if (m_temizleniyor) return;

    const RamBilgisi bilgi = SysInfo::ramBilgisi();
    m_lblPercent->setText(QString::number(bilgi.percent, 'f', 1));
    m_barRam->setValue(static_cast<int>(bilgi.percent));
    m_lblUsed->setText(QString("%1 GB").arg(bilgi.usedGB, 0, 'f', 2));
    m_lblTotal->setText(QString("%1 GB").arg(bilgi.totalGB, 0, 'f', 2));
    m_lblFree->setText(QString("%1 GB").arg(bilgi.freeGB, 0, 'f', 2));
    m_lblWarn->setVisible(bilgi.percent >= 90.0);
    m_grafik->ekle(bilgi.percent);
}

void MemoryTab::islemListesiYenile()
{
    std::vector<ProcKaydi> hamListe = ProcessList::yenile();
    const int hamBoyut = static_cast<int>(hamListe.size());

    QSet<QString> yeniIsaretli;
    for (const auto &k : hamListe) {
        const QString ad = QString::fromStdString(k.ad).toLower();
        const bool oncedenVardi = m_isaretliAd.contains(ad);
        const bool varsayilan = (!k.korumali) && (k.mb >= 150);
        const bool isaretli = k.korumali ? false : (m_isaretliAd.isEmpty() ? varsayilan : oncedenVardi);
        if (isaretli) yeniIsaretli.insert(ad);
    }
    m_isaretliAd = yeniIsaretli;

    // clear() tum agaci sifirlar - kullanicinin actigi gruplar kendiliginden
    // kapanmasin diye acik olan ust satirlarin isimlerini (ok prefix'i
    // temizlenmis haliyle) sakla, yeniden doldururken geri ac.
    QSet<QString> acikGruplar;
    for (int i = 0; i < m_tabloGorunum->topLevelItemCount(); ++i) {
        auto *ust = m_tabloGorunum->topLevelItem(i);
        if (!ust->isExpanded()) continue;
        QString ad = ust->text(1);
        if (ad.startsWith(QStringLiteral(u"▸ ")) || ad.startsWith(QStringLiteral(u"▾ ")))
            ad.remove(0, 2);
        acikGruplar.insert(ad.trimmed());
    }

    m_tabloGorunum->clear();
    for (const auto &k : hamListe) {
        // Sistem korumali islemler zaten kapatilamiyor - listede gostermenin
        // anlami yok, kullaniciyi kafasi karismasin diye tamamen gizle.
        if (k.korumali) continue;

        const QString ad = QString::fromStdString(k.ad).toLower();
        const bool isaretli = m_isaretliAd.contains(ad);

        auto *ust = new QTreeWidgetItem(m_tabloGorunum);
        ust->setFlags(ust->flags() | Qt::ItemIsUserCheckable);
        ust->setCheckState(0, isaretli ? Qt::Checked : Qt::Unchecked);

        const QString adGorunen = QString::fromStdString(k.ad);
        // QSS ile tema uygulanan QTreeWidget'ta Qt'nin varsayilan expand/collapse
        // oku QSS branch resmi olmadan cizilmiyor (bilinen Qt kisiti). Bunun
        // yerine metin tabanli ok karakteri kullaniyoruz - QSS'e bagimli degil,
        // her zaman gorunur. itemExpanded/itemCollapsed sinyalleri okunu gunceller.
        const QString okKapali = QStringLiteral(u"▸ "); // ▸
        ust->setText(1, k.altlar.size() > 1
                             ? okKapali + QString("%1 (%2)").arg(adGorunen).arg(k.altlar.size())
                             : QStringLiteral("   ") + adGorunen);
        ust->setIcon(1, IconCache::simgeGetir(k.tamYol));
        ust->setText(2, QString("%1 MB").arg(k.mb));
        if (k.altlar.size() == 1) ust->setText(3, QString::number(k.altlar[0].pid));
        else ust->setText(3, QString("%1 islem").arg(k.altlar.size()));
        ust->setText(4, QString::fromStdString(k.durum));

        if (k.altlar.size() > 1) {
            for (const auto &alt : k.altlar) {
                auto *altItem = new QTreeWidgetItem(ust);
                altItem->setText(1, adGorunen);
                altItem->setIcon(1, IconCache::simgeGetir(k.tamYol));
                altItem->setText(2, QString("%1 MB").arg(alt.mb));
                altItem->setText(3, QString::number(alt.pid));
                altItem->setText(4, QString::fromStdString(k.durum));
            }
        }
        m_tabloGorunum->addTopLevelItem(ust);

        if (k.altlar.size() > 1) {
            const QString grupAdi = QString("%1 (%2)").arg(adGorunen).arg(k.altlar.size());
            if (acikGruplar.contains(grupAdi)) ust->setExpanded(true);
        }
    }

    m_sonKayitlar = std::move(hamListe);
    filtreUygula();

    log(QString("Islem listesi guncellendi: ham liste %1 kayit, agac ust satir %2.")
        .arg(hamBoyut)
        .arg(m_tabloGorunum->topLevelItemCount()));
}

void MemoryTab::filtreUygula()
{
    const QString metin = m_txtArama->text().trimmed().toLower();
    for (int i = 0; i < m_tabloGorunum->topLevelItemCount(); ++i) {
        auto *ust = m_tabloGorunum->topLevelItem(i);
        if (metin.isEmpty()) {
            ust->setHidden(false);
            continue;
        }
        bool eslesti = ust->text(1).toLower().contains(metin);
        if (!eslesti) {
            for (int j = 0; j < ust->childCount(); ++j) {
                if (ust->child(j)->text(1).toLower().contains(metin)) { eslesti = true; break; }
            }
        }
        ust->setHidden(!eslesti);
    }
}

void MemoryTab::temizlemeBaslat()
{
    if (m_temizleniyor) return;
    m_temizleniyor = true;
    m_btnTemizle->setEnabled(false);
    m_btnTemizle->setText("Temizleniyor...");

    // Isaretli uygulamalari (Sec kutusu) once sessizce kapat - "Simdi Temizle"
    // hem RAM kirpma hem de secili uygulama kapatmayi tek adimda yapsin diye.
    const int kapatilan = secilenleriSessizceKapat(secilenleriTopla());
    if (kapatilan > 0) log(QString("%1 uygulama kapatildi.").arg(kapatilan));
    adimNoktasiGuncelle(m_step1, "idle");
    adimNoktasiGuncelle(m_step2, "idle");
    adimNoktasiGuncelle(m_step3, "idle");

    m_temizleThread = new QThread(this);
    m_temizleWorker = new CleanWorker();
    m_temizleWorker->moveToThread(m_temizleThread);

    connect(m_temizleThread, &QThread::started, m_temizleWorker, &CleanWorker::calistir);
    connect(m_temizleWorker, &CleanWorker::adimBasladi, this, &MemoryTab::adimBasladi);
    connect(m_temizleWorker, &CleanWorker::adimBitti, this, &MemoryTab::adimBitti);
    connect(m_temizleWorker, &CleanWorker::tamamlandi, this, &MemoryTab::temizlemeBitti);
    connect(m_temizleWorker, &CleanWorker::tamamlandi, m_temizleThread, &QThread::quit);
    connect(m_temizleThread, &QThread::finished, m_temizleWorker, &QObject::deleteLater);

    m_temizleThread->start();
}

void MemoryTab::adimBasladi(int adim)
{
    QLabel *nokta = (adim == 1) ? m_step1 : (adim == 2) ? m_step2 : m_step3;
    adimNoktasiGuncelle(nokta, "work");
    m_btnTemizle->setText(QString("Adim %1/3...").arg(adim));
}

void MemoryTab::adimBitti(int adim, const QString &mesaj)
{
    QLabel *nokta = (adim == 1) ? m_step1 : (adim == 2) ? m_step2 : m_step3;
    adimNoktasiGuncelle(nokta, "done");
    log(mesaj);
}

void MemoryTab::temizlemeBitti()
{
    m_temizleniyor = false;
    m_btnTemizle->setEnabled(true);
    m_btnTemizle->setText("SIMDI TEMIZLE");
    ramBilgisiYenile();
    islemListesiYenile();
    log("Temizlik tamamlandi.");
    m_temizleThread = nullptr;
    m_temizleWorker = nullptr;
}

std::vector<ProcKaydi> MemoryTab::secilenleriTopla() const
{
    std::vector<ProcKaydi> secilenler;
    for (int i = 0; i < m_tabloGorunum->topLevelItemCount(); ++i) {
        auto *ust = m_tabloGorunum->topLevelItem(i);
        if (ust->checkState(0) != Qt::Checked) continue;
        // Metin tabanli ok (bkz. islemListesiYenile) basa ▸/▾ ekliyor -
        // karsilastirmadan once temizle, yoksa isim eslesmesi hep basarisiz olur.
        QString ad = ust->text(1);
        if (ad.startsWith(QStringLiteral(u"▸ ")) || ad.startsWith(QStringLiteral(u"▾ ")))
            ad.remove(0, 2);
        ad = ad.trimmed();
        for (const auto &k : m_sonKayitlar) {
            const QString kAd = k.altlar.size() > 1
                                     ? QString("%1 (%2)").arg(QString::fromStdString(k.ad)).arg(k.altlar.size())
                                     : QString::fromStdString(k.ad);
            if (kAd == ad) { secilenler.push_back(k); break; }
        }
    }
    return secilenler;
}

void MemoryTab::secilenleriKapat()
{
    std::vector<ProcKaydi> secilenler = secilenleriTopla();
    if (secilenler.empty()) {
        QMessageBox::information(this, "Secim yok",
            "Once kapatmak istedigin uygulamalari isaretle (Sec kutusu).");
        return;
    }

    QStringList isimler;
    int toplamIslem = 0;
    for (const auto &s : secilenler) {
        isimler << QString::fromStdString(s.ad);
        toplamIslem += static_cast<int>(s.altlar.size());
    }
    const auto cevap = QMessageBox::question(this, "Onay",
        QString("%1 uygulama, toplam %2 islem kapatilacak: %3\n\nDevam edilsin mi?")
            .arg(secilenler.size()).arg(toplamIslem).arg(isimler.join(", ")));
    if (cevap != QMessageBox::Yes) return;

    secilenleriSessizceKapat(secilenler);
    islemListesiYenile();
}

int MemoryTab::secilenleriSessizceKapat(const std::vector<ProcKaydi> &secilenler)
{
    int kapatilan = 0;
    for (const auto &s : secilenler) {
        // korumali gruplar zaten isaretlenemedigi icin buraya gelmez.
        if (s.korumali) continue;
        for (const auto &alt : s.altlar) {
            HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, alt.pid);
            if (h && TerminateProcess(h, 0)) {
                log(QString("Kapatildi: %1 (%2)").arg(QString::fromStdString(s.ad)).arg(alt.pid));
                ++kapatilan;
            } else {
                log(QString("Kapatilamadi: %1 (%2)").arg(QString::fromStdString(s.ad)).arg(alt.pid));
            }
            if (h) CloseHandle(h);
        }
    }
    return kapatilan;
}

void MemoryTab::otomatikKontrolEt()
{
    if (m_temizleniyor) return;
    if (!m_toggleOtomatik->isChecked()) return;

    const RamBilgisi bilgi = SysInfo::ramBilgisi();
    if (bilgi.percent < m_sliderEsik->value()) return;

    // PowerShell'deki LastAutoClean 5 dakika cooldown mantigiyla AYNI - ayni
    // esigin ustundeyken her 10 saniyede bir temizlik tetiklenmesini engeller.
    if (m_sonOtomatikTemizlik.isValid() &&
        m_sonOtomatikTemizlik.secsTo(QDateTime::currentDateTime()) < 5 * 60) {
        return;
    }
    m_sonOtomatikTemizlik = QDateTime::currentDateTime();
    log(QString("Otomatik esik asildi (%%1), temizleniyor...").arg(bilgi.percent, 0, 'f', 1));
    temizlemeBaslat();
}

void MemoryTab::esikDegisti(int deger)
{
    m_lblEsik->setText(QString("%%1").arg(deger));
    ayarlariKaydet();
}

void MemoryTab::otomatikDegisti(bool acik)
{
    log(acik ? "Otomatik temizlik acildi." : "Otomatik temizlik kapatildi.");
    ayarlariKaydet();
}
