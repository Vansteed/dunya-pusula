#include "DiskTreeTab.h"
#include "DiskTreeScanWorker.h"

#include "core/FolderScanner.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QHeaderView>
#include <QProgressBar>
#include <QStyle>
#include <QIcon>
#include <QPainter>
#include <QPolygonF>
#include <QThread>
#include <QStorageInfo>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QClipboard>
#include <QUrl>
#include <QDir>

#include <filesystem>
#include <algorithm>
#include <memory>
#include <utility>

namespace fs = std::filesystem;

namespace {

QString boyutFormatla(int64_t bayt)
{
    const double b = static_cast<double>(bayt);
    if (b >= 1099511627776.0) return QString::number(b / 1099511627776.0, 'f', 2) + " TB";
    if (b >= 1073741824.0) return QString::number(b / 1073741824.0, 'f', 2) + " GB";
    if (b >= 1048576.0) return QString::number(b / 1048576.0, 'f', 2) + " MB";
    if (b >= 1024.0) return QString::number(b / 1024.0, 'f', 2) + " KB";
    return QString::number(bayt) + " B";
}

constexpr int SutunAd = 0;
constexpr int SutunBoyut = 1;
constexpr int SutunYuzde = 2;
// Bu satirin acilir-kapanir oku var mi.
constexpr int RolOkVar = Qt::UserRole + 2;

// Acilir-kapanir ok ikonu. QSS branch resmi, QStyle::standardIcon ve
// QToolButton+setArrowType yollarinin UCU DE bu temada hic cizilmedi
// (uygulama genelinde stylesheet varken QStyleSheetStyle ok primitive'ini
// atliyor), o yuzden ucgeni QPainter ile kendimiz cizip ikon yapiyoruz.
// Ikon, ogenin dekorasyon yuvasinda durur - yani girinti hizasinda, ismin
// hemen solunda (Explorer'daki gibi), sabit bir sutunda degil.
QIcon okIkonu(bool acik, qreal dpr)
{
    const int kenar = 12;
    QPixmap pm(QSize(kenar, kenar) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);

    const QPointF merkez(kenar / 2.0, kenar / 2.0);
    constexpr qreal yari = 3.5;
    QPolygonF ucgen;
    if (acik) {
        ucgen << QPointF(merkez.x() - yari, merkez.y() - yari * 0.6)
              << QPointF(merkez.x() + yari, merkez.y() - yari * 0.6)
              << QPointF(merkez.x(), merkez.y() + yari * 0.9);
    } else {
        ucgen << QPointF(merkez.x() - yari * 0.6, merkez.y() - yari)
              << QPointF(merkez.x() - yari * 0.6, merkez.y() + yari)
              << QPointF(merkez.x() + yari * 0.9, merkez.y());
    }

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#A1A1AA"));
    p.drawPolygon(ucgen);
    p.end();

    return QIcon(pm);
}

// Boyut sutununda metne gore degil (Qt varsayilani) sayiya gore siralasin
// diye - "144,96 GB" ile "17,38 MB" alfabetik sirada yanlis sonuc verir.
// Boyut Qt::UserRole'de int64 olarak zaten saklaniyor (bkz. dugumOlustur).
class BoyutaGoreDugum : public QTreeWidgetItem
{
public:
    using QTreeWidgetItem::QTreeWidgetItem;

    bool operator<(const QTreeWidgetItem &other) const override
    {
        const int sutun = treeWidget() ? treeWidget()->sortColumn() : SutunBoyut;
        if (sutun == SutunBoyut || sutun == SutunYuzde) {
            return data(SutunBoyut, Qt::UserRole).toLongLong()
                 < other.data(SutunBoyut, Qt::UserRole).toLongLong();
        }
        return QTreeWidgetItem::operator<(other);
    }
};

// Bir alt klasor sonucu geldiginde (canli, tek tek) DOGRUDAN nihai agac
// ogesini kurar - ayri bir "gecici onizleme" asamasi YOK, boylece tarama
// suren bir kardes klasor yuzunden agaci sonradan yikip yeniden kurmaya
// gerek kalmaz (bu, farkli bir node'u AYNI ANDA genisletip kendi taramasini
// baslatmisken, ana tarama bitince o node'un dangling pointer'a donmesi
// riskini ortadan kaldirir - onceki tasarimda kokTaramaBitti/altTaramaBitti
// tum agaci silip yeniden kuruyordu). maxGorulen: o taramada su ana kadar
// gorulen en buyuk boyut - % cubugu buna gore olceklenir (kesin degil,
// sonradan gelen daha buyuk bir kardes eskilerin cubugunu GERIYE DONUK
// guncellemez - ponytail: kucuk bir yaklasik deger, WizTree kadar hassas
// degil ama tarama donmus gibi gorunmekten iyidir).
QTreeWidgetItem *canliDugumEkle(QTreeWidget *agac, QTreeWidgetItem *ustDugum,
                                 const QString &ad, const QString &tamYol,
                                 qint64 boyut, bool altKlasoruVar, bool baglanti,
                                 qint64 maxGorulen)
{
    auto *item = new BoyutaGoreDugum();
    item->setText(SutunAd, baglanti ? ad + " [baglanti]" : ad);
    item->setText(SutunBoyut, boyutFormatla(boyut));
    item->setData(0, Qt::UserRole, tamYol);
    item->setData(SutunBoyut, Qt::UserRole, static_cast<qlonglong>(boyut));

    if (altKlasoruVar) {
        item->setData(SutunAd, RolOkVar, true);
        item->setIcon(SutunAd, okIkonu(false, agac->devicePixelRatioF()));

        auto *dummy = new QTreeWidgetItem();
        dummy->setText(SutunAd, "Aciliyor...");
        dummy->setData(0, Qt::UserRole, QStringLiteral("DUMMY"));
        item->addChild(dummy);
    }

    if (ustDugum) ustDugum->addChild(item);
    else agac->addTopLevelItem(item);

    // Bos klasorde cubuk gostermek gorsel gurultu - sadece boyutu olanlarda
    // ince, yuvarlatilmis bir cubuk koy.
    if (boyut > 0) {
        auto *bar = new QProgressBar(agac);
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        bar->setFixedHeight(6);
        bar->setStyleSheet("QProgressBar{background:#1E1E22;border:none;border-radius:3px;}"
                            "QProgressBar::chunk{background:#4C5F7E;border-radius:3px;}");
        bar->setValue(maxGorulen > 0 ? static_cast<int>((boyut * 100) / maxGorulen) : 0);
        agac->setItemWidget(item, SutunYuzde, bar);
    }

    return item;
}

} // namespace

DiskTreeTab::DiskTreeTab(QWidget *parent)
    : QWidget(parent)
{
    auto *anaDuzen = new QVBoxLayout(this);
    anaDuzen->setContentsMargins(12, 10, 12, 6);
    anaDuzen->setSpacing(8);

    anaDuzen->addWidget(surucuSatiriOlustur());

    auto *govde = new QHBoxLayout();
    govde->setSpacing(8);

    auto *solDuzen = new QVBoxLayout();
    m_agac = new QTreeWidget(this);
    m_agac->setColumnCount(3);
    m_agac->setHeaderLabels({"Ad", "Boyut", "%"});
    // Ok'u ogenin kendi ikon yuvasinda cizdigimiz icin (girinti hizasinda,
    // ismin solunda) native branch dekorasyonuna gerek yok - zaten QSS
    // temasinda cizilmiyordu.
    m_agac->setRootIsDecorated(false);
    m_agac->setIndentation(18);
    m_agac->setUniformRowHeights(true);
    m_agac->setSortingEnabled(true);
    m_agac->sortByColumn(SutunBoyut, Qt::DescendingOrder);
    m_agac->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_agac->setContextMenuPolicy(Qt::CustomContextMenu);
    // Varsayilan edit trigger'lar (secili ogeye tekrar tiklamak gibi) satiri
    // yeniden adlandirma kutusuna ceviriyordu - klasor adi duzenlenemez,
    // tamamen kapat (aksi halde Ad sutununa tikla-genislet ile catisiyordu).
    m_agac->setEditTriggers(QAbstractItemView::NoEditTriggers);
    auto *baslik = m_agac->header();
    baslik->setSectionResizeMode(SutunAd, QHeaderView::Stretch);
    baslik->setSectionResizeMode(SutunBoyut, QHeaderView::Fixed);
    baslik->resizeSection(SutunBoyut, 110);
    baslik->setSectionResizeMode(SutunYuzde, QHeaderView::Fixed);
    baslik->resizeSection(SutunYuzde, 90);
    connect(m_agac, &QTreeWidget::itemExpanded, this, &DiskTreeTab::dugumGenisletildi);
    // Ada (ya da okuna) tiklamak acip kapatsin.
    connect(m_agac, &QTreeWidget::itemClicked, this, [](QTreeWidgetItem *item, int sutun) {
        if (sutun == SutunAd && item->childCount() > 0) item->setExpanded(!item->isExpanded());
    });
    connect(m_agac, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem *item) {
        if (item->data(SutunAd, RolOkVar).toBool())
            item->setIcon(SutunAd, okIkonu(true, m_agac->devicePixelRatioF()));
    });
    connect(m_agac, &QTreeWidget::itemCollapsed, this, [this](QTreeWidgetItem *item) {
        if (item->data(SutunAd, RolOkVar).toBool())
            item->setIcon(SutunAd, okIkonu(false, m_agac->devicePixelRatioF()));
    });
    connect(m_agac, &QTreeWidget::itemSelectionChanged, this, &DiskTreeTab::secimDegisti);
    connect(m_agac, &QTreeWidget::customContextMenuRequested, this, &DiskTreeTab::sagTikMenusu);
    solDuzen->addWidget(m_agac, 1);

    m_lblDurum = new QLabel("Bir surucu sec.", this);
    m_lblDurum->setStyleSheet("color:#A1A1AA; font-size:11px;");
    solDuzen->addWidget(m_lblDurum);

    auto *sagDuzen = new QVBoxLayout();
    m_lblSeciliYol = new QLabel("-", this);
    m_lblSeciliYol->setStyleSheet("color:#F4F4F5; font-size:12px; font-weight:600;");
    m_lblSeciliYol->setWordWrap(true);
    sagDuzen->addWidget(m_lblSeciliYol);

    m_lblSeciliBoyut = new QLabel("-", this);
    m_lblSeciliBoyut->setStyleSheet("color:#F4F4F5; font-size:20px; font-weight:600;");
    sagDuzen->addWidget(m_lblSeciliBoyut);

    m_lblSeciliOzet = new QLabel("-", this);
    m_lblSeciliOzet->setStyleSheet("color:#A1A1AA; font-size:11px;");
    sagDuzen->addWidget(m_lblSeciliOzet);

    m_detayTablo = new QTableWidget(0, 3, this);
    m_detayTablo->setHorizontalHeaderLabels({"Ad", "Boyut", "%"});
    m_detayTablo->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_detayTablo->verticalHeader()->setVisible(false);
    m_detayTablo->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_detayTablo->setSelectionBehavior(QAbstractItemView::SelectRows);
    sagDuzen->addWidget(m_detayTablo, 1);

    auto *solWidget = new QWidget(this);
    solWidget->setLayout(solDuzen);
    auto *sagWidget = new QWidget(this);
    sagWidget->setLayout(sagDuzen);
    sagWidget->setMinimumWidth(260);
    sagWidget->setMaximumWidth(360);

    govde->addWidget(solWidget, 2);
    govde->addWidget(sagWidget, 1);
    anaDuzen->addLayout(govde, 1);
}

DiskTreeTab::~DiskTreeTab()
{
    for (auto *t : std::as_const(m_aktifThreadler)) {
        t->quit();
        t->wait();
    }
}

QWidget *DiskTreeTab::surucuSatiriOlustur()
{
    auto *kart = new QWidget(this);
    m_surucuDuzeni = new QHBoxLayout(kart);
    m_surucuDuzeni->setContentsMargins(0, 0, 0, 0);
    m_surucuDuzeni->setSpacing(6);

    const auto surucular = QStorageInfo::mountedVolumes();
    for (const auto &s : surucular) {
        if (!s.isValid() || !s.isReady()) continue;
        const QString kok = s.rootPath();
        auto *btn = new QPushButton(kok, kart);
        btn->setFixedWidth(70);
        connect(btn, &QPushButton::clicked, this, [this, kok]() { surucuTiklandi(kok); });
        m_surucuDuzeni->addWidget(btn);
    }
    m_surucuDuzeni->addStretch(1);
    return kart;
}

void DiskTreeTab::surucuTiklandi(const QString &kok)
{
    // Baska bir tarama (kok ya da bir alt klasor) surerken agaci silmek,
    // o taramanin lambda'larinin tuttugu QTreeWidgetItem isaretcilerini
    // gecersiz (dangling) birakir - once mevcut taramalarin bitmesini bekle.
    if (!m_taraniyorHedefler.isEmpty()) {
        m_lblDurum->setText("Once devam eden tarama(lar) bitsin.");
        return;
    }
    m_kokSurucu = kok;
    m_agac->clear();
    taramaBaslat(kok, nullptr);
}

void DiskTreeTab::dugumGenisletildi(QTreeWidgetItem *item)
{
    if (!item || item->childCount() == 0) return;
    QTreeWidgetItem *ilk = item->child(0);
    if (ilk->data(0, Qt::UserRole).toString() != QStringLiteral("DUMMY")) return;
    taramaBaslat(item->data(0, Qt::UserRole).toString(), item);
}

void DiskTreeTab::taramaBaslat(const QString &yol, QTreeWidgetItem *ustDugum)
{
    // Ayni node iki kez ayni anda taranmasin. Farkli node'lar (ustDugum
    // farkli) SERBESTCE es zamanli taranabilir - kullanici bir klasor hala
    // olculurken baska (zaten listelenmis) bir klasoru de genisletebilsin
    // diye (once tek bir paylasimli thread vardi, biri tarrken hicbiri
    // tiklanamiyordu).
    if (m_taraniyorHedefler.contains(ustDugum)) return;
    m_taraniyorHedefler.insert(ustDugum);

    m_lblDurum->setText("Taraniyor: " + yol);

    auto *thread = new QThread(this);
    auto *worker = new DiskTreeScanWorker(yol);
    worker->moveToThread(thread);
    m_aktifThreadler.append(thread);

    // Bu taramaya ozel "su ana kadar gorulen en buyuk boyut" - paylasimli
    // uye degil, her tarama kendi olcegini tutar (es zamanli taramalar
    // birbirinin % cubugu olcegini bozmasin).
    auto maxGorulen = std::make_shared<qint64>(0);

    connect(thread, &QThread::started, worker, &DiskTreeScanWorker::tara);
    connect(worker, &DiskTreeScanWorker::ilerleme, this, [this](const QString &yol) {
        m_lblDurum->setText("Taraniyor: " + yol);
    });
    // Bir alt klasorun olcumu biter bitmez DOGRUDAN nihai satiri ekle -
    // tarama bitince agaci yikip yeniden kurmaya gerek yok (bkz.
    // canliDugumEkle yorumu).
    connect(worker, &DiskTreeScanWorker::ogeBulundu, this,
            [this, ustDugum, maxGorulen](const QString &ad, const QString &tamYol, qint64 boyut,
                                          bool altKlasoruVar, bool baglanti) {
        *maxGorulen = std::max(*maxGorulen, boyut);
        canliDugumEkle(m_agac, ustDugum, ad, tamYol, boyut, altKlasoruVar, baglanti, *maxGorulen);
    });
    connect(worker, &DiskTreeScanWorker::tamamlandi, this, [this, ustDugum]() {
        if (ustDugum) {
            // Tarama sonunda hic alt klasor cikmadiysa oku kaldir - acilacak
            // bir sey yokken ok gostermek yaniltici olur.
            if (ustDugum->childCount() == 0) {
                ustDugum->setData(SutunAd, RolOkVar, false);
                ustDugum->setIcon(SutunAd, QIcon());
            }
            m_lblDurum->setText(QString("Hazir - %1 alt klasor.").arg(ustDugum->childCount()));
            if (m_agac->currentItem() == ustDugum) detayPanelGuncelle(ustDugum);
        } else {
            m_lblDurum->setText(QString("Hazir - %1 klasor listelendi.").arg(m_agac->topLevelItemCount()));
        }
        m_taraniyorHedefler.remove(ustDugum);
    });
    connect(worker, &DiskTreeScanWorker::tamamlandi, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread]() {
        m_aktifThreadler.removeOne(thread);
        thread->deleteLater();
    });

    // Genisletilen node'un eski DUMMY cocugunu sil - yeni gercek cocuklar
    // yukaridaki ogeBulundu ile zaten teker teker eklenecek.
    if (ustDugum) {
        while (ustDugum->childCount() > 0) delete ustDugum->takeChild(0);
    }

    thread->start();
}

void DiskTreeTab::secimDegisti()
{
    auto *item = m_agac->currentItem();
    detayPanelGuncelle(item);
}

void DiskTreeTab::detayPanelGuncelle(QTreeWidgetItem *item)
{
    if (!item || item->data(0, Qt::UserRole).toString() == QStringLiteral("DUMMY")) {
        m_lblSeciliYol->setText("-");
        m_lblSeciliBoyut->setText("-");
        m_lblSeciliOzet->setText("-");
        m_detayTablo->setRowCount(0);
        return;
    }

    const QString yol = item->data(0, Qt::UserRole).toString();
    const int64_t boyut = item->data(SutunBoyut, Qt::UserRole).toLongLong();

    m_lblSeciliYol->setText(yol);
    m_lblSeciliBoyut->setText(boyutFormatla(boyut));

    // Dogrudan alt klasor/dosya sayisi - hizli senkron sayim, tek seviye.
    int altKlasorSayisi = 0;
    int dosyaSayisi = 0;
    std::vector<std::pair<QString, int64_t>> oge; // ad, boyut - genisletilmis alt klasorler + dosyalar
    std::error_code ec;
    const std::string yolStd = yol.toStdString();
    for (auto it = fs::directory_iterator(yolStd, fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::directory_iterator(); it.increment(ec)) {
        try {
            std::error_code dirEc;
            if (it->is_directory(dirEc) && !dirEc) {
                altKlasorSayisi++;
            } else {
                std::error_code fileEc;
                if (it->is_regular_file(fileEc) && !fileEc) {
                    dosyaSayisi++;
                    std::error_code szEc;
                    const auto sz = it->file_size(szEc);
                    if (!szEc) oge.emplace_back(QString::fromStdString(it->path().filename().string()), static_cast<int64_t>(sz));
                }
            }
        } catch (const std::exception &) {
            continue;
        }
    }

    // Zaten genisletilmis (yuklu) alt klasorlerin boyutlarini da ekle.
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem *cocuk = item->child(i);
        if (cocuk->data(0, Qt::UserRole).toString() == QStringLiteral("DUMMY")) continue;
        oge.emplace_back(cocuk->text(SutunAd), cocuk->data(SutunBoyut, Qt::UserRole).toLongLong());
    }

    std::sort(oge.begin(), oge.end(), [](const auto &a, const auto &b) { return a.second > b.second; });

    m_lblSeciliOzet->setText(QString("%1 alt klasor, %2 dosya").arg(altKlasorSayisi).arg(dosyaSayisi));

    int64_t maxOge = 0;
    for (const auto &o : oge) maxOge = std::max(maxOge, o.second);

    const int gosterilecek = std::min<int>(20, static_cast<int>(oge.size()));
    m_detayTablo->setRowCount(gosterilecek);
    for (int i = 0; i < gosterilecek; ++i) {
        m_detayTablo->setItem(i, 0, new QTableWidgetItem(oge[i].first));
        m_detayTablo->setItem(i, 1, new QTableWidgetItem(boyutFormatla(oge[i].second)));
        const int yuzde = (maxOge > 0) ? static_cast<int>((oge[i].second * 100) / maxOge) : 0;
        m_detayTablo->setItem(i, 2, new QTableWidgetItem(QString("%%1").arg(yuzde)));
    }
}

void DiskTreeTab::sagTikMenusu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_agac->itemAt(pos);
    if (!item || item->data(0, Qt::UserRole).toString() == QStringLiteral("DUMMY")) return;

    const QString yol = item->data(0, Qt::UserRole).toString();

    QMenu menu(this);
    QAction *actAc = menu.addAction("Klasorde Ac");
    QAction *actKopyala = menu.addAction("Yolu Kopyala");
    menu.addSeparator();
    QAction *actKaldir = menu.addAction("Listeden Kaldir");
    QAction *actSil = menu.addAction("Sil...");

    QAction *secilen = menu.exec(m_agac->viewport()->mapToGlobal(pos));
    if (!secilen) return;

    if (secilen == actAc) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(yol));
    } else if (secilen == actKopyala) {
        QGuiApplication::clipboard()->setText(yol);
        m_lblDurum->setText("Yol panoya kopyalandi.");
    } else if (secilen == actKaldir) {
        QTreeWidgetItem *ust = item->parent();
        if (ust) ust->removeChild(item);
        else m_agac->takeTopLevelItem(m_agac->indexOfTopLevelItem(item));
        delete item;
    } else if (secilen == actSil) {
        silDugum(item);
    }
}

void DiskTreeTab::silDugum(QTreeWidgetItem *item)
{
    const QString yol = item->data(0, Qt::UserRole).toString();
    const auto cevap = QMessageBox::warning(this, "Silme Onayi",
        "Klasor ve icindekiler silinecek:\n" + yol + "\n\nEmin misin?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (cevap != QMessageBox::Yes) return;

    try {
        std::error_code ec;
        fs::remove_all(yol.toStdString(), ec);
        if (ec) {
            QMessageBox::warning(this, "Silinemedi", QString::fromStdString(ec.message()));
            return;
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Silinemedi", QString::fromLocal8Bit(e.what()));
        return;
    }

    const int64_t silinenBoyut = item->data(SutunBoyut, Qt::UserRole).toLongLong();
    QTreeWidgetItem *ust = item->parent();
    if (ust) {
        ust->removeChild(item);
        // ust yuklenmisse (DUMMY degilse) boyutunu yaklasik azalt - ponytail:
        // tam dogruluk icin yeniden tarama gerekir, kullanici surucuye tekrar
        // tiklayarak tazeleyebilir.
        const int64_t ustBoyut = ust->data(SutunBoyut, Qt::UserRole).toLongLong();
        const int64_t yeniBoyut = std::max<int64_t>(0, ustBoyut - silinenBoyut);
        ust->setData(SutunBoyut, Qt::UserRole, static_cast<qlonglong>(yeniBoyut));
        ust->setText(SutunBoyut, boyutFormatla(yeniBoyut));
    } else {
        m_agac->takeTopLevelItem(m_agac->indexOfTopLevelItem(item));
    }
    delete item;
    m_lblDurum->setText("Silindi.");
}
