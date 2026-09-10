#include "ProcessModel.h"

#include "core/DosyaIslem.h"
#include "core/MemoryCleaner.h"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageWriter>
#include <QPixmap>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>
#include <windows.h>
#include <shellapi.h>

namespace {
// internalId kodlamasi: 0 = ust satir, aksi halde (ustSatir + 1).
constexpr quintptr UstIsaret = 0;

// Uygulama simgesini temp altina PNG olarak cikarir (yoksa olusturur),
// QML'in Image ile gosterebilecegi dosya URL'si doner. Simge yoksa bos.
QString simgeUrl(const QString &ad, const QString &exeYolu)
{
    if (exeYolu.isEmpty()) return {};
    const QString klasor = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                           + QStringLiteral("/DunyaPusula/simgeler");
    QDir().mkpath(klasor);
    QString guvenli;
    guvenli.reserve(ad.size());
    for (const QChar c : ad) {
        const bool ok = c.isLetterOrNumber() || c == u'.' || c == u'-' || c == u'_';
        guvenli.append(ok ? c : u'_');
    }
    // qtbase PNG'siz derlendigi icin PNG yazilamayabilir - calisma aninda
    // desteklenen saydamlikli formati sec (PNG > XPM > BMP). Hepsi QtGui
    // icinde gomulu, eklenti gerektirmez.
    QByteArray fmt = "BMP";
    const QList<QByteArray> desteklenen = QImageWriter::supportedImageFormats();
    if (desteklenen.contains("png")) fmt = "PNG";
    else if (desteklenen.contains("xpm")) fmt = "XPM";
    const QString yol = klasor + u'/' + guvenli + u'.' + QString::fromLatin1(fmt.toLower());
    if (!QFile::exists(yol)) {
        // QtWidgets'e baglanmadan (proje Widgets'siz) dogrudan WinAPI ile
        // exe simgesini cikar: SHGetFileInfo -> HICON -> QPixmap.
        SHFILEINFOW fi{};
        if (!SHGetFileInfoW(exeYolu.toStdWString().c_str(), 0, &fi, sizeof fi,
                            SHGFI_ICON | SHGFI_LARGEICON)) return {};
        const QPixmap pm = QPixmap::fromImage(QImage::fromHICON(fi.hIcon));
        DestroyIcon(fi.hIcon);
        if (pm.isNull()) return {};
        if (!pm.save(yol, fmt.constData())) return {};
    }
    return QUrl::fromLocalFile(yol).toString();
}
}

ProcessModel::ProcessModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    yenile();

    // Islem listesi kendiliginden tazelensin - kapatilan uygulama listede
    // asili kalmasin.
    m_yenileTimer.setInterval(5000);
    connect(&m_yenileTimer, &QTimer::timeout, this, &ProcessModel::yenile);
    m_yenileTimer.start();
}

void ProcessModel::otomatikYenileAta(bool acik)
{
    if (acik) {
        if (!m_yenileTimer.isActive()) m_yenileTimer.start();
    } else {
        m_yenileTimer.stop();
    }
}

QHash<int, QByteArray> ProcessModel::roleNames() const
{
    return {
        { AdRolu, "ad" },
        { MbRolu, "mb" },
        { PidMetniRolu, "pidMetni" },
        { DurumRolu, "durum" },
        { SeciliRolu, "secili" },
        { GrupMu, "grupMu" },
        { AltSayisi, "altSayisi" },
        { SimgeYoluRolu, "simgeYolu" },
        { ExeYoluRolu, "exeYolu" },
        { OncelikRolu, "oncelik" },
        { UstSatirRolu, "ustSatirNo" },
        { SimgeUrlRolu, "simgeUrl" }
    };
}

QModelIndex ProcessModel::index(int satir, int sutun, const QModelIndex &ust) const
{
    if (!hasIndex(satir, sutun, ust)) return {};
    if (!ust.isValid()) return createIndex(satir, sutun, UstIsaret);
    return createIndex(satir, sutun, static_cast<quintptr>(ust.row() + 1));
}

QModelIndex ProcessModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid() || idx.internalId() == UstIsaret) return {};
    return createIndex(static_cast<int>(idx.internalId()) - 1, 0, UstIsaret);
}

int ProcessModel::rowCount(const QModelIndex &ust) const
{
    if (!ust.isValid()) return static_cast<int>(m_kayitlar.size());
    if (ust.internalId() != UstIsaret) return 0; // alt satirin cocugu yok
    const int s = ust.row();
    if (s < 0 || s >= static_cast<int>(m_kayitlar.size())) return 0;
    const auto &k = m_kayitlar[s];
    return k.altlar.size() > 1 ? static_cast<int>(k.altlar.size()) : 0;
}

int ProcessModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant ProcessModel::data(const QModelIndex &idx, int rol) const
{
    if (!idx.isValid()) return {};

    if (idx.internalId() == UstIsaret) {
        if (idx.row() < 0 || idx.row() >= static_cast<int>(m_kayitlar.size())) return {};
        const auto &k = m_kayitlar[idx.row()];
        const QString ad = QString::fromStdString(k.ad);
        switch (rol) {
        case AdRolu: return ad;
        case MbRolu: return static_cast<qlonglong>(k.mb);
        case PidMetniRolu:
            return k.altlar.size() == 1 ? QString::number(k.altlar[0].pid)
                                        : QString("%1 islem").arg(k.altlar.size());
        case DurumRolu: return QString::fromStdString(k.durum);
        case SeciliRolu: return m_isaretliAd.contains(ad.toLower());
        case GrupMu: return k.altlar.size() > 1;
        case AltSayisi: return static_cast<int>(k.altlar.size());
        case SimgeYoluRolu: return QString::fromStdString(k.tamYol);
        case ExeYoluRolu: return QString::fromStdString(k.tamYol);
        case OncelikRolu:
            return k.altlar.size() == 1 ? m_oncelikMap.value(k.altlar[0].pid, -1) : -1;
        case UstSatirRolu: return -1;
        case SimgeUrlRolu:
            return simgeUrl(ad, QString::fromStdString(k.tamYol));
        default: return {};
        }
    }

    const int ustSatir = static_cast<int>(idx.internalId()) - 1;
    if (ustSatir < 0 || ustSatir >= static_cast<int>(m_kayitlar.size())) return {};
    const auto &k = m_kayitlar[ustSatir];
    if (idx.row() < 0 || idx.row() >= static_cast<int>(k.altlar.size())) return {};
    const auto &alt = k.altlar[idx.row()];

    switch (rol) {
    case AdRolu: return QString::fromStdString(k.ad);
    case MbRolu: return static_cast<qlonglong>(alt.mb);
    case PidMetniRolu: return QString::number(alt.pid);
    case DurumRolu: return QString::fromStdString(k.durum);
    case SeciliRolu: return false;
    case GrupMu: return false;
    case AltSayisi: return 0;
    case SimgeYoluRolu: return QString::fromStdString(k.tamYol);
    case ExeYoluRolu:
        return QString::fromStdString(alt.exeYolu.empty() ? k.tamYol : alt.exeYolu);
    case OncelikRolu: return m_oncelikMap.value(alt.pid, -1);
    case UstSatirRolu: return ustSatir;
    case SimgeUrlRolu:
        return simgeUrl(QString::fromStdString(k.ad),
                        QString::fromStdString(alt.exeYolu.empty() ? k.tamYol : alt.exeYolu));
    default: return {};
    }
}

void ProcessModel::filtreAta(const QString &f)
{
    if (m_filtre == f) return;
    m_filtre = f;
    emit filtreDegisti();
    yenile();
}

bool ProcessModel::filtreyeUyar(const ProcKaydi &k) const
{
    if (m_filtre.isEmpty()) return true;
    return QString::fromStdString(k.ad).contains(m_filtre, Qt::CaseInsensitive);
}

void ProcessModel::yenile()
{
    std::vector<ProcKaydi> ham = ProcessList::yenile();

    // Korumali islemler listede GOSTERILMEZ (Widgets surumundeki davranis).
    std::vector<ProcKaydi> suzulmus;
    suzulmus.reserve(ham.size());
    for (auto &k : ham) {
        if (k.korumali) continue;
        if (!filtreyeUyar(k)) continue;
        suzulmus.push_back(std::move(k));
    }

    // Ilk yuklemede 150 MB ustu her sey isaretli gelsin; sonrasinda
    // kullanicinin isaretleri korunur (yalnizca hala var olanlar).
    QSet<QString> yeniIsaretli;
    const bool ilkYukleme = m_isaretliAd.isEmpty() && m_kayitlar.empty();
    for (const auto &k : suzulmus) {
        const QString ad = QString::fromStdString(k.ad).toLower();
        if (ilkYukleme ? (k.mb >= 150) : m_isaretliAd.contains(ad))
            yeniIsaretli.insert(ad);
    }

    kayitlariSirala(suzulmus);

    QHash<quint32, int> yeniOncelik;
    for (const auto &k : suzulmus)
        for (const auto &alt : k.altlar)
            yeniOncelik.insert(alt.pid, ProcessList::oncelikOku(alt.pid));

    beginResetModel();
    m_kayitlar = std::move(suzulmus);
    m_isaretliAd = yeniIsaretli;
    m_oncelikMap = std::move(yeniOncelik);
    endResetModel();
    emit isaretliDegisti();
}

void ProcessModel::kayitlariSirala(std::vector<ProcKaydi> &kayitlar) const
{
    if (m_sutun < 0) return; // dogal RAM sirasi (ProcessList::yenile() zaten buyukten kucuge verir)

    const int sutun = m_sutun;
    const bool azalan = m_azalan;
    std::sort(kayitlar.begin(), kayitlar.end(),
              [sutun, azalan](const ProcKaydi &a, const ProcKaydi &b) {
                  if (sutun == 1) // bellek
                      return azalan ? a.mb > b.mb : a.mb < b.mb;
                  const int karsilastir = QString::localeAwareCompare(
                      QString::fromStdString(a.ad), QString::fromStdString(b.ad));
                  return azalan ? karsilastir > 0 : karsilastir < 0;
              });
}

void ProcessModel::sirala(int sutun, bool azalan)
{
    m_sutun = sutun;
    m_azalan = azalan;
    beginResetModel();
    kayitlariSirala(m_kayitlar);
    endResetModel();
}

void ProcessModel::isaretiCevir(int ustSatir)
{
    if (ustSatir < 0 || ustSatir >= static_cast<int>(m_kayitlar.size())) return;
    const QString ad = QString::fromStdString(m_kayitlar[ustSatir].ad).toLower();
    if (m_isaretliAd.contains(ad)) m_isaretliAd.remove(ad);
    else m_isaretliAd.insert(ad);

    const QModelIndex idx = index(ustSatir, 0, {});
    emit dataChanged(idx, idx, { SeciliRolu });
    emit isaretliDegisti();
}

bool ProcessModel::kapatTekPid(uint32_t pid, const QString &ad)
{
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    const bool basarili = h && TerminateProcess(h, 0);
    if (h) CloseHandle(h);

    emit gunluk(basarili ? QString("Kapatildi: %1 (%2)").arg(ad).arg(pid)
                          : QString("Kapatilamadi: %1 (%2)").arg(ad).arg(pid));
    return basarili;
}

int ProcessModel::secilenleriKapat()
{
    int kapatilan = 0;
    for (const auto &k : m_kayitlar) {
        const QString ad = QString::fromStdString(k.ad).toLower();
        if (!m_isaretliAd.contains(ad)) continue;
        if (k.korumali) continue;

        for (const auto &alt : k.altlar) {
            if (kapatTekPid(alt.pid, QString::fromStdString(k.ad))) ++kapatilan;
        }
    }
    yenile();
    return kapatilan;
}

bool ProcessModel::kapat(int satir, int altSatir)
{
    if (satir < 0 || satir >= static_cast<int>(m_kayitlar.size())) return false;
    const ProcKaydi &k = m_kayitlar[satir];
    if (k.korumali) return false;
    if (k.altlar.empty()) return false;

    // ad, yenile() referansi gecersiz kilmadan ONCE kopyalanir.
    const QString ad = QString::fromStdString(k.ad);
    bool basarili = true;
    if (altSatir < 0) {
        for (const auto &alt : k.altlar)
            basarili = kapatTekPid(alt.pid, ad) && basarili;
    } else {
        if (altSatir >= static_cast<int>(k.altlar.size())) return false;
        basarili = kapatTekPid(k.altlar[altSatir].pid, ad);
    }
    yenile();
    return basarili;
}

bool ProcessModel::agaciKapat(int satir, int altSatir)
{
    if (satir < 0 || satir >= static_cast<int>(m_kayitlar.size())) return false;
    const ProcKaydi &k = m_kayitlar[satir];
    if (k.korumali) return false;
    if (k.altlar.empty()) return false;

    bool basarili = true;
    if (altSatir < 0) {
        for (const auto &alt : k.altlar)
            basarili = ProcessList::agaciKapat(alt.pid) && basarili;
    } else {
        if (altSatir >= static_cast<int>(k.altlar.size())) return false;
        basarili = ProcessList::agaciKapat(k.altlar[altSatir].pid);
    }

    const QString ad = QString::fromStdString(k.ad);
    emit gunluk(basarili ? QString("Islem agaci kapatildi: %1").arg(ad)
                          : QString("Islem agaci tam kapatilamadi: %1").arg(ad));
    yenile();
    return basarili;
}

int ProcessModel::agacBoyutu(int satir, int altSatir) const
{
    if (satir < 0 || satir >= static_cast<int>(m_kayitlar.size())) return 0;
    const auto &k = m_kayitlar[satir];

    if (altSatir < 0) {
        int toplam = 0;
        for (const auto &alt : k.altlar) toplam += ProcessList::agacUyeSayisi(alt.pid);
        return toplam;
    }
    if (altSatir >= static_cast<int>(k.altlar.size())) return 0;
    return ProcessList::agacUyeSayisi(k.altlar[altSatir].pid);
}

bool ProcessModel::oncelikAta(int satir, int altSatir, int sinif)
{
    if (satir < 0 || satir >= static_cast<int>(m_kayitlar.size())) return false;
    const auto &k = m_kayitlar[satir];
    if (k.korumali) return false;
    if (k.altlar.empty()) return false;

    bool basarili = true;
    if (altSatir < 0) {
        for (const auto &alt : k.altlar)
            basarili = ProcessList::oncelikAta(alt.pid, sinif) && basarili;
    } else {
        if (altSatir >= static_cast<int>(k.altlar.size())) return false;
        basarili = ProcessList::oncelikAta(k.altlar[altSatir].pid, sinif);
    }
    if (basarili) yenile();
    return basarili;
}

QString ProcessModel::exeYolu(int satir, int altSatir) const
{
    if (satir < 0 || satir >= static_cast<int>(m_kayitlar.size())) return {};
    const auto &k = m_kayitlar[satir];
    if (altSatir < 0) return QString::fromStdString(k.tamYol);
    if (altSatir >= static_cast<int>(k.altlar.size())) return {};
    const auto &alt = k.altlar[altSatir];
    return QString::fromStdString(alt.exeYolu.empty() ? k.tamYol : alt.exeYolu);
}

QVariantMap ProcessModel::satirBilgisi(int satir, int altSatir) const
{
    QVariantMap bilgi;
    if (satir < 0 || satir >= static_cast<int>(m_kayitlar.size())) return bilgi;
    const auto &k = m_kayitlar[satir];

    bilgi["ad"] = QString::fromStdString(k.ad);
    bilgi["durum"] = QString::fromStdString(k.durum);
    bilgi["exeYolu"] = exeYolu(satir, altSatir);

    if (altSatir < 0) {
        bilgi["pid"] = k.altlar.size() == 1 ? static_cast<qlonglong>(k.altlar[0].pid) : 0;
        bilgi["mb"] = static_cast<qlonglong>(k.mb);
        bilgi["oncelik"] = k.altlar.size() == 1 ? ProcessList::oncelikOku(k.altlar[0].pid) : -1;
        bilgi["grupMu"] = k.altlar.size() > 1;
    } else {
        if (altSatir >= static_cast<int>(k.altlar.size())) return bilgi;
        const auto &alt = k.altlar[altSatir];
        bilgi["pid"] = static_cast<qlonglong>(alt.pid);
        bilgi["mb"] = static_cast<qlonglong>(alt.mb);
        bilgi["oncelik"] = ProcessList::oncelikOku(alt.pid);
        bilgi["grupMu"] = false;
    }
    return bilgi;
}

QVariantList ProcessModel::enCokKullanan(int adet) const
{
    std::vector<const ProcKaydi *> sirali;
    sirali.reserve(m_kayitlar.size());
    for (const auto &k : m_kayitlar) sirali.push_back(&k);
    std::sort(sirali.begin(), sirali.end(),
              [](const ProcKaydi *a, const ProcKaydi *b) { return a->mb > b->mb; });

    QVariantList sonuc;
    const int mevcut = static_cast<int>(sirali.size());
    const int sinir = adet < mevcut ? adet : mevcut; // std::min - windows.h min makrosuyla catisiyor
    for (int i = 0; i < sinir; ++i) {
        QVariantMap m;
        const QString ad = QString::fromStdString(sirali[i]->ad);
        m["ad"] = ad;
        m["mb"] = static_cast<qlonglong>(sirali[i]->mb);
        m["simge"] = simgeUrl(ad, QString::fromStdString(sirali[i]->tamYol));
        sonuc.append(m);
    }
    return sonuc;
}

void ProcessModel::konumunuGoster(int satir, int altSatir) const
{
    const QString yol = exeYolu(satir, altSatir);
    if (!yol.isEmpty()) DosyaIslem::konumunuGoster(yol);
}

void ProcessModel::ozellikleriGoster(int satir, int altSatir) const
{
    const QString yol = exeYolu(satir, altSatir);
    if (!yol.isEmpty()) DosyaIslem::ozellikleriGoster(yol);
}

void ProcessModel::panoyaYaz(const QString &metin) const
{
    QGuiApplication::clipboard()->setText(metin);
}
