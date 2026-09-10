#include "FolderTreeModel.h"
#include "ui/DiskTreeScanWorker.h"
#include "core/DosyaIslem.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QDir>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QThread>

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace {

QString boyutFormatla(qint64 bayt)
{
    const double b = static_cast<double>(bayt);
    if (b >= 1099511627776.0) return QString::number(b / 1099511627776.0, 'f', 2) + " TB";
    if (b >= 1073741824.0) return QString::number(b / 1073741824.0, 'f', 2) + " GB";
    if (b >= 1048576.0) return QString::number(b / 1048576.0, 'f', 2) + " MB";
    if (b >= 1024.0) return QString::number(b / 1024.0, 'f', 2) + " KB";
    return QString::number(bayt) + " B";
}

QString sureFormatla(qint64 saniye)
{
    if (saniye < 60) return QString("%1 sn").arg(saniye);
    if (saniye < 3600) return QString("%1 dk %2 sn").arg(saniye / 60).arg(saniye % 60);
    return QString("%1 sa %2 dk").arg(saniye / 3600).arg((saniye % 3600) / 60);
}

}

namespace {

QVariantMap surucuKarti(const QStorageInfo &s)
{
    QVariantMap m;
    const qint64 toplam = s.bytesTotal();
    const qint64 bos = s.bytesFree();
    const QString etiket = s.displayName().isEmpty() ? s.rootPath() : s.displayName();
    m["yol"] = s.rootPath();
    m["etiket"] = etiket;
    m["toplamGB"] = toplam / 1073741824.0;
    m["bosGB"] = bos / 1073741824.0;
    m["kullanilanYuzde"] = toplam > 0 ? (100.0 * (toplam - bos) / toplam) : 0.0;
    m["dosyaSistemi"] = QString::fromUtf8(s.fileSystemType());
    return m;
}

}

FolderTreeModel::FolderTreeModel(QObject *parent)
    : QAbstractItemModel(parent), m_kok(new Dugum())
{
    const auto surucular = QStorageInfo::mountedVolumes();
    for (const auto &s : surucular) {
        if (!s.isValid() || !s.isReady()) continue;
        m_suruculer.append(surucuKarti(s));
    }
    // Kullanicinin gercekten yer dolduran klasorleri - olmayanlar atlanir
    // (Windows dili/kurulumu farkliysa QStandardPaths dogru yolu verir).
    // Sik taranan iki klasor - digerleri surucu taramasinda zaten cikiyor.
    const QList<QPair<QString, QStandardPaths::StandardLocation>> kisayolTanim = {
        { "Masaüstü", QStandardPaths::DesktopLocation },
        { "İndirilenler", QStandardPaths::DownloadLocation },
    };
    for (const auto &tanim : kisayolTanim) {
        const QString yol = QStandardPaths::writableLocation(tanim.second);
        if (yol.isEmpty() || !QDir(yol).exists()) continue;
        QVariantMap m;
        m["ad"] = tanim.first;
        m["yol"] = yol;
        m_kisayollar.append(m);
    }

    m_buyukDosyaToplayici.esikAyarla(500LL * 1024 * 1024);

    m_saatTik.setInterval(1000);
    connect(&m_saatTik, &QTimer::timeout, this, [this] { ilerlemeYayinla(true); });

    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(this);
    m_proxy->setSortRole(BoyutRolu);
    m_proxy->sort(0, Qt::DescendingOrder);
}

FolderTreeModel::~FolderTreeModel()
{
    for (auto *t : std::as_const(m_aktifThreadler)) {
        t->quit();
        t->wait();
    }
    delete m_kok;
}

QHash<int, QByteArray> FolderTreeModel::roleNames() const
{
    return {
        { AdRolu, "ad" },
        { TamYolRolu, "tamYol" },
        { BoyutRolu, "boyut" },
        { BoyutMetniRolu, "boyutMetni" },
        { YuzdeRolu, "yuzde" },
        { AltKlasoruVarRolu, "altKlasoruVar" },
        { TaraniyorRolu, "taraniyor" },
        { OlculduRolu, "olculdu" }
    };
}

QModelIndex FolderTreeModel::index(int satir, int sutun, const QModelIndex &ust) const
{
    if (!hasIndex(satir, sutun, ust)) return {};
    Dugum *ustD = ust.isValid() ? static_cast<Dugum *>(ust.internalPointer()) : m_kok;
    if (satir < 0 || satir >= ustD->cocuklar.size()) return {};
    return createIndex(satir, sutun, ustD->cocuklar[satir]);
}

QModelIndex FolderTreeModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid()) return {};
    Dugum *d = static_cast<Dugum *>(idx.internalPointer());
    Dugum *ust = d->ust;
    if (!ust) return {};
    Dugum *ustust = ust->ust ? ust->ust : m_kok;
    const int satir = ustust->cocuklar.indexOf(ust);
    return createIndex(satir, 0, ust);
}

int FolderTreeModel::rowCount(const QModelIndex &ust) const
{
    if (ust.column() > 0) return 0;
    Dugum *d = ust.isValid() ? static_cast<Dugum *>(ust.internalPointer()) : m_kok;
    return d->cocuklar.size();
}

int FolderTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

bool FolderTreeModel::hasChildren(const QModelIndex &ust) const
{
    Dugum *d = ust.isValid() ? static_cast<Dugum *>(ust.internalPointer()) : m_kok;
    if (d == m_kok) return !d->cocuklar.isEmpty();
    return d->yuklendi ? !d->cocuklar.isEmpty() : d->altKlasoruVar;
}

QVariant FolderTreeModel::data(const QModelIndex &idx, int rol) const
{
    if (!idx.isValid()) return {};
    Dugum *d = static_cast<Dugum *>(idx.internalPointer());

    switch (rol) {
    case AdRolu: return d->ad;
    case TamYolRolu: return d->tamYol;
    case BoyutRolu: return static_cast<qlonglong>(d->boyut);
    case BoyutMetniRolu: return boyutFormatla(d->boyut);
    case YuzdeRolu: {
        if (!d->olculdu) return 0.0;
        Dugum *ust = d->ust ? d->ust : m_kok;
        qint64 maxB = 0;
        for (Dugum *c : std::as_const(ust->cocuklar)) {
            if (c->olculdu) maxB = std::max(maxB, c->boyut);
        }
        return maxB > 0 ? static_cast<double>(d->boyut) / static_cast<double>(maxB) : 0.0;
    }
    case AltKlasoruVarRolu: return d->altKlasoruVar;
    case TaraniyorRolu: return d->taraniyor;
    case OlculduRolu: return d->olculdu;
    default: return {};
    }
}

FolderTreeModel::Dugum *FolderTreeModel::dugumdenIsaretCikar(const QModelIndex &idx) const
{
    if (!idx.isValid()) return nullptr;
    return static_cast<Dugum *>(idx.internalPointer());
}

QModelIndex FolderTreeModel::dugumIcinIndex(Dugum *d) const
{
    if (!d || d == m_kok) return {};
    Dugum *ust = d->ust ? d->ust : m_kok;
    const int satir = ust->cocuklar.indexOf(d);
    if (satir < 0) return {};
    return createIndex(satir, 0, d);
}

void FolderTreeModel::ilerlemeYayinla(bool zorla)
{
    if (!zorla && m_sonYayinZamani.isValid() && m_sonYayinZamani.elapsed() < 200) return;
    m_sonYayinZamani.restart();

    // Bayt orani, klasor sayisi oranindan cok daha gercekci: kalan tek bir
    // klasor diskin yarisi olabilir.
    const double baytOrani = m_hedefBayt > 0
        ? qBound(0.0, static_cast<double>(m_olculenBayt) / static_cast<double>(m_hedefBayt), 1.0)
        : 0.0;
    const double sayiOrani = m_hedefToplam > 0
        ? static_cast<double>(m_hedefOlculen) / static_cast<double>(m_hedefToplam)
        : 0.0;
    // Son klasorler bitmeden bar dolmasin diye ikisinin kucugu alinir.
    m_ilerleme = m_hedefBayt > 0 ? qMin(baytOrani, sayiOrani) : sayiOrani;

    m_ilerlemeMetni = QString("%1 / %2 klasör").arg(m_hedefOlculen).arg(m_hedefToplam);
    const qint64 gecenMs = m_taramaZamani.isValid() ? m_taramaZamani.elapsed() : 0;
    // Ilk 2 saniyede orani guvenilir degil, tahmin gosterme.
    if (m_ilerleme > 0.01 && m_ilerleme < 1.0 && gecenMs > 2000) {
        const qint64 kalanSn = qint64((gecenMs / 1000.0) * (1.0 - m_ilerleme) / m_ilerleme);
        m_ilerlemeMetni += " - ~" + sureFormatla(kalanSn) + " kaldı";
    }
    emit ilerlemeDegisti();
}

void FolderTreeModel::agaciKur(Dugum *hedef, const FolderScanner::DizinDugumu &kaynak)
{
    hedef->dosyaBayt = kaynak.dosyaBayt;
    hedef->yuklendi = true;
    hedef->olculdu = true;
    hedef->altKlasoruVar = !kaynak.cocuklar.empty();
    for (const auto &c : kaynak.cocuklar) {
        auto *cocuk = new Dugum();
        cocuk->ad = c.baglanti ? QString::fromStdString(c.ad) + " [bağlantı]" : QString::fromStdString(c.ad);
        cocuk->tamYol = QString::fromStdString(c.tamYol);
        cocuk->boyut = c.boyut;
        cocuk->olculdu = true;   // tek taramada tam olculdu, "..." gostermeye gerek yok
        cocuk->ust = hedef;
        hedef->cocuklar.append(cocuk);
        if (!c.baglanti) agaciKur(cocuk, c);
    }
}

void FolderTreeModel::taramaBaslat(Dugum *ustDugum, const QString &yol)
{
    if (ustDugum->taraniyor) return;
    ustDugum->taraniyor = true;
    if (ustDugum != m_kok) {
        const QModelIndex idx = dugumIcinIndex(ustDugum);
        emit dataChanged(idx, idx, { TaraniyorRolu });
    }
    // Yeni bir tarama GRUBU basliyorsa (elde calisan thread yok) sayaclar
    // sifirlanir; yoksa kok taramasinin sayilari alt klasor taramasiyla
    // toplanip "168 / 170" gibi anlamsiz bir ilerleme cikiyor.
    if (m_aktifThreadler.isEmpty()) {
        m_hedefToplam = 0;
        m_hedefOlculen = 0;
        m_olculenBayt = 0;
        m_hedefBayt = (ustDugum == m_kok) ? m_hedefBayt : ustDugum->boyut;
        m_ilerleme = 0.0;
        m_taramaZamani.restart();
    }

    m_durum = "Taranıyor: " + yol;
    emit durumDegisti();
    if (!m_saatTik.isActive()) m_saatTik.start();

    auto *thread = new QThread(this);
    auto *worker = new DiskTreeScanWorker(yol, &m_buyukDosyaToplayici);
    worker->moveToThread(thread);
    m_aktifThreadler.append(thread);

    connect(thread, &QThread::started, worker, &DiskTreeScanWorker::tara);
    // Farkli thread'de yasayan worker'a - Qt otomatik kuyruklu baglanti kurar.
    connect(this, &FolderTreeModel::tumunuIptalEt, worker, &DiskTreeScanWorker::iptalEt);
    connect(worker, &DiskTreeScanWorker::ilerleme, this, [this](const QString &yol) {
        try {
            m_durum = "Taranıyor: " + yol;
            emit durumDegisti();
        } catch (const std::exception &) {}
    });
    connect(worker, &DiskTreeScanWorker::ogeBulundu, this,
            [this, ustDugum](const QString &ad, const QString &tamYol, qint64 boyut,
                              bool altKlasoruVar, bool baglanti) {
        try {
            auto *cocuk = new Dugum();
            cocuk->ad = baglanti ? ad + " [bağlantı]" : ad;
            cocuk->tamYol = tamYol;
            // boyut < 0: henuz olculmedi (ikinci gecis bekleniyor). Baglanti
            // olanlar hic olculmez, 0 olarak kalir ama "olculdu" sayilir.
            cocuk->olculdu = baglanti || boyut >= 0;
            cocuk->boyut = boyut >= 0 ? boyut : 0;
            cocuk->altKlasoruVar = altKlasoruVar;
            cocuk->ust = (ustDugum == m_kok) ? nullptr : ustDugum;

            const QModelIndex ustIdx = (ustDugum == m_kok) ? QModelIndex() : dugumIcinIndex(ustDugum);
            const int satir = ustDugum->cocuklar.size();
            beginInsertRows(ustIdx, satir, satir);
            ustDugum->cocuklar.append(cocuk);
            endInsertRows();

            // Kokun ilk cocugu geldi - QML'deki "sonucVar" bagi bos durumu
            // gizlesin diye bildirilir.
            if (ustDugum == m_kok && ustDugum->cocuklar.size() == 1) emit durumDegisti();

            // Yeni gelen boyut kardesler arasindaki en buyugu degistirmis
            // olabilir - hepsinin yuzde cubugunu guncelle.
            if (ustDugum->cocuklar.size() > 1) {
                const QModelIndex ilk = index(0, 0, ustIdx);
                const QModelIndex son = index(ustDugum->cocuklar.size() - 1, 0, ustIdx);
                emit dataChanged(ilk, son, { YuzdeRolu });
            }

            ++m_hedefToplam;
            ilerlemeYayinla(false);
        } catch (const std::exception &) {}
    });
    connect(worker, &DiskTreeScanWorker::ogeGuncellendi, this,
            [this, ustDugum](const QString &tamYol, qint64 boyut) {
        try {
            int satir = -1;
            for (int i = 0; i < ustDugum->cocuklar.size(); ++i) {
                if (ustDugum->cocuklar[i]->tamYol == tamYol) { satir = i; break; }
            }
            if (satir < 0) return;
            Dugum *cocuk = ustDugum->cocuklar[satir];
            cocuk->boyut = boyut;
            cocuk->olculdu = true;
            if (boyut > 0) m_olculenBayt += boyut;   // hangi dugum taraniyorsa onun hedefine sayilir

            const QModelIndex ustIdx = (ustDugum == m_kok) ? QModelIndex() : dugumIcinIndex(ustDugum);
            const QModelIndex idx = index(satir, 0, ustIdx);
            emit dataChanged(idx, idx, { BoyutRolu, BoyutMetniRolu, YuzdeRolu, OlculduRolu });

            // Yeni olculen boyut en buyuk olabilir - kardeslerin yuzdesini
            // de guncelle (olculmemis kardesler etkilenmez, yuzdeleri 0 kalir).
            if (ustDugum->cocuklar.size() > 1) {
                const QModelIndex ilk = index(0, 0, ustIdx);
                const QModelIndex son = index(ustDugum->cocuklar.size() - 1, 0, ustIdx);
                emit dataChanged(ilk, son, { YuzdeRolu });
            }

            ++m_hedefOlculen;
            ilerlemeYayinla(false);
            buyukDosyalariYenile(false);
        } catch (const std::exception &) {}
    });
    connect(worker, &DiskTreeScanWorker::tamamlandi, this, [this, ustDugum, worker]() {
        ustDugum->dosyaBayt = worker->dosyaBaytlariAl();
        try {
            // Tek taramada kurulan TAM ALT AGAC - her root cocugu icin
            // olculurken toplanan DizinDugumu agacini Dugum agacina cevir.
            // Boylece dugumuAc() yeniden tarama baslatmak zorunda kalmaz.
            // Reset - tek seferde toplu ekleme, satir satir beginInsertRows
            // yerine (QSortFilterProxyModel arkada, reset guvenli).
            const auto &sonuc = worker->sonucAl();
            if (!sonuc.empty()) {
                beginResetModel();
                for (const auto &bilgi : sonuc) {
                    if (bilgi.baglanti) continue;
                    Dugum *cocuk = nullptr;
                    for (Dugum *c : std::as_const(ustDugum->cocuklar)) {
                        if (c->tamYol == QString::fromStdString(bilgi.tamYol)) { cocuk = c; break; }
                    }
                    if (cocuk) agaciKur(cocuk, bilgi.agac);
                }
                endResetModel();
            }

            ustDugum->taraniyor = false;
            ustDugum->yuklendi = true;
            if (ustDugum->cocuklar.isEmpty()) ustDugum->altKlasoruVar = false;

            if (ustDugum != m_kok) {
                const QModelIndex idx = dugumIcinIndex(ustDugum);
                emit dataChanged(idx, idx, { TaraniyorRolu, AltKlasoruVarRolu });
                m_durum = QString("Hazır - %1 alt klasör.").arg(ustDugum->cocuklar.size());
            } else {
                m_durum = QString("Hazır - %1 klasör listelendi.").arg(ustDugum->cocuklar.size());
            }
            emit durumDegisti();
        } catch (const std::exception &) {}
    });
    connect(worker, &DiskTreeScanWorker::tamamlandi, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread]() {
        m_aktifThreadler.removeOne(thread);
        thread->deleteLater();
        emit durumDegisti();
        if (m_aktifThreadler.isEmpty()) {
            m_saatTik.stop();
            m_ilerleme = 1.0;
            m_ilerlemeMetni.clear();
            buyukDosyalariYenile(true);

            // Ozet seridi - kok seviyesindeki klasorler uzerinden (alt klasor
            // tek başına acilan tarama icin ozet gosterilmez, anlamsiz olurdu).
            if (!m_kok->cocuklar.isEmpty()) {
                qint64 toplamBoyut = m_kok->dosyaBayt;
                Dugum *enBuyuk = nullptr;
                for (Dugum *c : std::as_const(m_kok->cocuklar)) {
                    toplamBoyut += c->boyut;

                    if (!enBuyuk || c->boyut > enBuyuk->boyut) enBuyuk = c;
                }
                m_taramaOzeti = QString("%1 klasör tarandı · toplam %2 · en büyük: %3")
                                    .arg(m_hedefOlculen)
                                    .arg(boyutFormatla(toplamBoyut))
                                    .arg(enBuyuk ? enBuyuk->ad : "-");
            } else {
                m_taramaOzeti.clear();
            }
            depolamaKirilimiHesapla();
            emit ilerlemeDegisti();
        }
    });

    thread->start();
}

void FolderTreeModel::buyukDosyalariYenile(bool zorla)
{
    if (!zorla && m_buyukDosyaYayinZamani.isValid() && m_buyukDosyaYayinZamani.elapsed() < 200) return;
    m_buyukDosyaYayinZamani.restart();

    const bool turMod = !m_turFiltresi.isEmpty();
    const int64_t esikBayt = static_cast<int64_t>(m_esikMB) * 1024 * 1024;
    const auto kaynak = turMod
        ? m_buyukDosyaToplayici.turListesiAl(m_turFiltresi.toStdString())
        : m_buyukDosyaToplayici.listeAl();
    QVariantList liste;
    for (const auto &d : kaynak) {
        if (turMod && m_sistemGizle && d.sistem) continue;
        if (!turMod && d.boyut < esikBayt) {
            break;  // liste buyukten kucuge sirali
        }
        QVariantMap m;
        const QString tamYol = QString::fromStdString(d.tamYol);
        m["ad"] = tamYol.section('/', -1).section('\\', -1);
        m["tamYol"] = tamYol;
        m["boyutMetni"] = boyutFormatla(d.boyut);
        m["boyut"] = static_cast<qlonglong>(d.boyut);
        m["kategori"] = d.kategori ? QString::fromLatin1(d.kategori) : QString();
        liste.append(m);
    }
    m_buyukDosyalar = liste;
    emit buyukDosyalarDegisti();
}

void FolderTreeModel::depolamaKirilimiHesapla()
{
    m_depolamaKirilimi.clear();

    // Sadece surucu KOKU icin anlamli - alt klasor taramasinda kok
    // seviyesindeki isimler (Users, Windows, ...) yer almaz.
    if (m_kok->cocuklar.isEmpty() || !QDir(m_kokYol).isRoot()) {
        emit depolamaKirilimiDegisti();
        return;
    }

    static const QStringList windowsAdlari = {
        "windows", "$winreagent", "$windows.~ws", "$getcurrent", "$sysreset",
        "recovery", "system volume information", "$recycle.bin", "perflogs",
        "esd", "documents and settings", "msocache"
    };
    static const QStringList programAdlari = {
        "program files", "program files (x86)", "programdata"
    };

    qint64 kullaniciBayt = 0, windowsBayt = 0, programBayt = 0, uygulamaBayt = 0;
    const qint64 sistemBayt = m_kok->dosyaBayt;
    QList<Dugum *> kullaniciCocuklar, windowsCocuklar, programCocuklar, uygulamaCocuklar;
    for (Dugum *c : std::as_const(m_kok->cocuklar)) {
        const QString adKucuk = c->ad.toLower();
        if (adKucuk == "users") { kullaniciBayt += c->boyut; kullaniciCocuklar.append(c); }
        else if (windowsAdlari.contains(adKucuk)) { windowsBayt += c->boyut; windowsCocuklar.append(c); }
        else if (programAdlari.contains(adKucuk)) { programBayt += c->boyut; programCocuklar.append(c); }
        else { uygulamaBayt += c->boyut; uygulamaCocuklar.append(c); }
    }

    // Her kategorinin en buyuk 4 alt klasorunu "Ad - 12.3 GB" seklinde
    // alt alta yazan ayrinti metni uretir.
    auto ayrintiOlustur = [this](QList<Dugum *> cocuklar) -> QString {
        std::sort(cocuklar.begin(), cocuklar.end(), [](Dugum *a, Dugum *b) {
            return a->boyut > b->boyut;
        });
        QStringList satirlar;
        const int sinir = std::min(4, static_cast<int>(cocuklar.size()));
        for (int i = 0; i < sinir; ++i)
            satirlar << QString("%1 - %2").arg(cocuklar[i]->ad, boyutFormatla(cocuklar[i]->boyut));
        if (cocuklar.size() > 4)
            satirlar << QString("+%1 klasör daha").arg(cocuklar.size() - 4);
        return satirlar.join("\n");
    };

    auto ekle = [this](const QString &ad, qint64 bayt, const QString &ayrinti) {
        if (bayt <= 0) return;
        QVariantMap m;
        m["ad"] = ad;
        m["bayt"] = static_cast<qlonglong>(bayt);
        m["boyutMetni"] = boyutFormatla(bayt);
        m["ayrinti"] = ayrinti;
        m_depolamaKirilimi.append(m);
    };
    ekle("Kullanıcı dosyaları", kullaniciBayt, ayrintiOlustur(kullaniciCocuklar));
    ekle("Windows", windowsBayt, ayrintiOlustur(windowsCocuklar));
    ekle("Programlar", programBayt, ayrintiOlustur(programCocuklar));
    ekle("Uygulamalar ve oyunlar", uygulamaBayt, ayrintiOlustur(uygulamaCocuklar));
    ekle("Sistem dosyaları", sistemBayt, "Sayfa dosyası, hazırda bekletme vb.");

    emit depolamaKirilimiDegisti();
}

void FolderTreeModel::surucuSec(const QString &yol)
{
    // Devam eden tarama(lar) varken agaci silmek, o taramalarin lambda'larinin
    // tuttugu Dugum isaretcilerini dangling birakir - once bitmelerini bekle
    // (DiskTreeTab::surucuTiklandi ile ayni kural).
    if (taraniyor()) {
        m_durum = "Önce devam eden tarama(lar) bitsin.";
        emit durumDegisti();
        return;
    }

    beginResetModel();
    qDeleteAll(m_kok->cocuklar);
    m_kok->cocuklar.clear();
    m_kokYol = yol;
    emit kokYolDegisti();
    endResetModel();

    m_buyukDosyaToplayici.sifirla();
    m_buyukDosyalar.clear();
    emit buyukDosyalarDegisti();
    m_taramaOzeti.clear();
    m_depolamaKirilimi.clear();
    emit depolamaKirilimiDegisti();

    // Bayt tabanli ilerleme sadece surucu KOKU icin gecerli; alt klasor
    // taranirken surucunun dolu baytini hedef almak bari asla ilerletmezdi.
    const QStorageInfo bilgi(yol);
    const bool surucuKoku = QDir(yol).isRoot();
    m_hedefBayt = (surucuKoku && bilgi.isValid() && bilgi.isReady())
                      ? bilgi.bytesTotal() - bilgi.bytesFree()
                      : 0;
    m_olculenBayt = 0;
    m_hedefToplam = 0;
    m_hedefOlculen = 0;
    m_ilerleme = 0.0;
    m_ilerlemeMetni.clear();
    m_taramaZamani.start();
    m_sonYayinZamani.invalidate();
    emit ilerlemeDegisti();

    taramaBaslat(m_kok, yol);
}

QModelIndex FolderTreeModel::yoluBul(const QString &yol) const
{
    const QString hedef = QDir::cleanPath(QDir::fromNativeSeparators(yol));
    const QString kok = QDir::cleanPath(QDir::fromNativeSeparators(m_kokYol));
    if (kok.isEmpty() || !hedef.startsWith(kok, Qt::CaseInsensitive)) return {};
    if (hedef.compare(kok, Qt::CaseInsensitive) == 0) return {};

    QString kalan = hedef.mid(kok.length());
    if (kalan.startsWith('/')) kalan = kalan.mid(1);
    if (kalan.isEmpty()) return {};

    const QStringList parcalar = kalan.split('/', Qt::SkipEmptyParts);
    Dugum *d = m_kok;
    for (const QString &parca : parcalar) {
        Dugum *sonraki = nullptr;
        for (Dugum *c : std::as_const(d->cocuklar)) {
            const QString cAd = QDir::cleanPath(QDir::fromNativeSeparators(c->tamYol)).section('/', -1);
            if (cAd.compare(parca, Qt::CaseInsensitive) == 0) { sonraki = c; break; }
        }
        if (!sonraki) return {};
        d = sonraki;
    }

    const QModelIndex kaynakIdx = dugumIcinIndex(d);
    if (!kaynakIdx.isValid()) return {};
    return m_proxy->mapFromSource(kaynakIdx);
}

void FolderTreeModel::dugumuAc(const QModelIndex &idx)
{
    Dugum *d = dugumdenIsaretCikar(idx);
    if (!d || d->yuklendi || d->taraniyor || !d->altKlasoruVar) return;
    taramaBaslat(d, d->tamYol);
}

QVariantMap FolderTreeModel::dugumBilgisi(const QModelIndex &idx) const
{
    QVariantMap m;
    Dugum *d = dugumdenIsaretCikar(idx);
    if (!d) return m;
    m["ad"] = d->ad;
    m["tamYol"] = d->tamYol;
    m["boyut"] = static_cast<qlonglong>(d->boyut);
    m["boyutMetni"] = boyutFormatla(d->boyut);

    // Klasorun DOGRUDAN icerigi: dosyalar (tek seviye, senkron - hizli) +
    // zaten taranmis alt klasorler. Widgets surumundeki detay paneliyle ayni.
    int altKlasorSayisi = 0;
    int dosyaSayisi = 0;
    std::vector<std::pair<QString, qint64>> ogeler;
    std::error_code ec;
    for (auto it = fs::directory_iterator(d->tamYol.toStdString(),
                                          fs::directory_options::skip_permission_denied, ec);
         !ec && it != fs::directory_iterator(); it.increment(ec)) {
        try {
            std::error_code dirEc;
            if (it->is_directory(dirEc) && !dirEc) {
                ++altKlasorSayisi;
                continue;
            }
            std::error_code fileEc;
            if (it->is_regular_file(fileEc) && !fileEc) {
                ++dosyaSayisi;
                std::error_code szEc;
                const auto sz = it->file_size(szEc);
                if (!szEc)
                    ogeler.emplace_back(QString::fromStdString(it->path().filename().string()),
                                        static_cast<qint64>(sz));
            }
        } catch (const std::exception &) {
            continue;   // erisim hatasi - bu ogeyi atla
        }
    }

    for (Dugum *c : d->cocuklar) ogeler.emplace_back(c->ad, c->boyut);

    std::sort(ogeler.begin(), ogeler.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    qint64 enBuyuk = ogeler.empty() ? 0 : ogeler.front().second;
    QVariantList liste;
    const int gosterilecek = std::min<int>(30, static_cast<int>(ogeler.size()));
    for (int i = 0; i < gosterilecek; ++i) {
        QVariantMap o;
        o["ad"] = ogeler[i].first;
        o["boyutMetni"] = boyutFormatla(ogeler[i].second);
        o["yuzde"] = enBuyuk > 0 ? double(ogeler[i].second) / double(enBuyuk) : 0.0;
        liste.append(o);
    }

    m["ozet"] = QStringLiteral("%1 alt klasör, %2 dosya").arg(altKlasorSayisi).arg(dosyaSayisi);
    m["ogeler"] = liste;
    return m;
}

void FolderTreeModel::boyutaGoreSirala(bool azalan)
{
    m_proxy->sort(0, azalan ? Qt::DescendingOrder : Qt::AscendingOrder);
}

bool FolderTreeModel::sil(const QModelIndex &idx, bool kalici)
{
    if (taraniyor()) return false;
    Dugum *d = dugumdenIsaretCikar(idx);
    if (!d) return false;

    const bool basarili = kalici ? DosyaIslem::kaliciSil(d->tamYol)
                                  : DosyaIslem::geriDonusumeGonder(d->tamYol);
    if (!basarili) return false;

    const qint64 silinenBoyut = d->boyut;
    Dugum *ust = d->ust ? d->ust : m_kok;
    const QModelIndex ustIdx = (ust == m_kok) ? QModelIndex() : dugumIcinIndex(ust);
    const int satir = ust->cocuklar.indexOf(d);
    if (satir < 0) return false;

    beginRemoveRows(ustIdx, satir, satir);
    ust->cocuklar.removeAt(satir);
    delete d;
    endRemoveRows();

    // Ust dugumlerin boyutunu silinen boyut kadar azalt (kok haric - ekranda
    // boyutu gosterilmiyor).
    for (Dugum *p = (ust == m_kok) ? nullptr : ust; p; p = p->ust) {
        p->boyut = std::max<qint64>(0, p->boyut - silinenBoyut);
        const QModelIndex pIdx = dugumIcinIndex(p);
        emit dataChanged(pIdx, pIdx, { BoyutRolu, BoyutMetniRolu, YuzdeRolu });
    }

    // Silinen son cocuktu - genislet oku artik anlamsiz.
    if (ust != m_kok && ust->yuklendi && ust->cocuklar.isEmpty()) {
        ust->altKlasoruVar = false;
        emit dataChanged(ustIdx, ustIdx, { AltKlasoruVarRolu });
    }

    return true;
}

bool FolderTreeModel::dosyaSil(const QString &tamYol, bool kalici)
{
    if (taraniyor()) return false;
    return kalici ? DosyaIslem::kaliciSil(tamYol) : DosyaIslem::geriDonusumeGonder(tamYol);
}

QString FolderTreeModel::korumaSebebi(const QString &yol) const
{
    return DosyaIslem::korumaSebebi(yol);
}

QString FolderTreeModel::riskUyarisi(const QString &yol) const
{
    return DosyaIslem::riskUyarisi(yol);
}

void FolderTreeModel::explorerdaAc(const QString &tamYol)
{
    DosyaIslem::explorerdaGoster(tamYol);
}

void FolderTreeModel::konumunuGoster(const QString &tamYol)
{
    DosyaIslem::konumunuGoster(tamYol);
}

void FolderTreeModel::ozellikleriGoster(const QString &tamYol)
{
    DosyaIslem::ozellikleriGoster(tamYol);
}

void FolderTreeModel::komutIstemiAc(const QString &tamYol)
{
    DosyaIslem::komutIstemiAc(tamYol);
}

void FolderTreeModel::yenidenTara(const QModelIndex &idx)
{
    Dugum *d = dugumdenIsaretCikar(idx);
    if (!d || d->taraniyor) return;

    if (!d->cocuklar.isEmpty()) {
        const QModelIndex kendi = dugumIcinIndex(d);
        beginRemoveRows(kendi, 0, d->cocuklar.size() - 1);
        qDeleteAll(d->cocuklar);
        d->cocuklar.clear();
        endRemoveRows();
    }
    d->yuklendi = false;
    taramaBaslat(d, d->tamYol);
}

void FolderTreeModel::panoyaYaz(const QString &metin)
{
    QGuiApplication::clipboard()->setText(metin);
}

void FolderTreeModel::suruculeriYenile()
{
    QVariantList yeni;
    const auto surucular = QStorageInfo::mountedVolumes();
    for (const auto &s : surucular) {
        if (!s.isValid() || !s.isReady()) continue;
        yeni.append(surucuKarti(s));
    }
    m_suruculer = yeni;
    emit suruculerDegisti();
}

void FolderTreeModel::sistemGizleAyarla(bool gizle)
{
    if (gizle == m_sistemGizle) return;
    m_sistemGizle = gizle;
    emit sistemGizleDegisti();
    buyukDosyalariYenile(true);
}

void FolderTreeModel::turFiltresiAyarla(const QString &tur)
{
    if (tur == m_turFiltresi) return;
    m_turFiltresi = tur;
    emit turFiltresiDegisti();
    buyukDosyalariYenile(true);
}

void FolderTreeModel::esikMBAyarla(int mb)
{
    if (mb < 1) mb = 1;
    if (mb == m_esikMB) return;
    m_esikMB = mb;
    emit esikMBDegisti();
    buyukDosyalariYenile(true);
}

void FolderTreeModel::taramayiDurdur()
{
    if (!taraniyor()) return;
    emit tumunuIptalEt();
    m_durum = "Tarama durduruluyor...";
    emit durumDegisti();
}
