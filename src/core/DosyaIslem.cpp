#include "DosyaIslem.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include <windows.h>
#include <shellapi.h>

namespace {

// Yolu karsilastirmaya hazirlar - ayrac normalize, sondaki "/" atilir.
QString temizle(const QString &yol)
{
    QString t = QDir::cleanPath(QDir::fromNativeSeparators(yol));
    while (t.endsWith('/') && t.size() > 1) t.chop(1);
    return t;
}

// "yol", "kok" ile AYNI mi ya da onun ALTINDA mi (yol parcasi olarak,
// "C:/Windows" ile "C:/Windows2" karismasin diye "kok/" ile baslamali).
bool ayniVeyaAltinda(const QString &yol, const QString &kok)
{
    if (kok.isEmpty()) return false;
    if (yol.compare(kok, Qt::CaseInsensitive) == 0) return true;
    return yol.startsWith(kok + "/", Qt::CaseInsensitive);
}

bool tamEsit(const QString &yol, const QString &kok)
{
    return !kok.isEmpty() && yol.compare(kok, Qt::CaseInsensitive) == 0;
}

// FO_DELETE ortak govdesi. ekBayrak = FOF_ALLOWUNDO icin geri donusum, 0
// icin kalici silme. pFrom CIFT NULL ile bitmeli - SHFILEOPSTRUCTW'nin
// gerektirdigi format, tek NULL yanlis/eksik silme demek (KRITIK).
bool shSil(const QString &yol, DWORD ekBayrak)
{
    if (yol.isEmpty()) return false;

    std::wstring wyol = QDir::toNativeSeparators(yol).toStdWString();
    wyol.push_back(L'\0'); // c_str() zaten sonuna bir '\0' ekler - boylece iki NULL olur

    SHFILEOPSTRUCTW islem = {};
    islem.wFunc = FO_DELETE;
    islem.pFrom = wyol.c_str();
    islem.fFlags = static_cast<FILEOP_FLAGS>(FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT | ekBayrak);

    const int sonuc = SHFileOperationW(&islem);
    return sonuc == 0 && !islem.fAnyOperationsAborted;
}

}

namespace DosyaIslem {

bool geriDonusumeGonder(const QString &yol)
{
    try {
        // Asil guvenlik agi burada - korumaSebebi bos degilse hicbir cagiran
        // (agac dugumu, dosya listesi, ileride eklenecek her yeni yer) silemez.
        if (!korumaSebebi(yol).isEmpty()) return false;
        return shSil(yol, FOF_ALLOWUNDO);
    } catch (...) {
        return false;
    }
}

bool kaliciSil(const QString &yol)
{
    try {
        if (!korumaSebebi(yol).isEmpty()) return false;
        return shSil(yol, 0);
    } catch (...) {
        return false;
    }
}

QString korumaSebebi(const QString &yol)
{
    try {
        if (yol.isEmpty()) return QStringLiteral("Yol bos.");
        const QString y = temizle(yol);
        if (y.size() <= 3) return QStringLiteral("Bu bir surucu kokü, silinemez.");

        // Windows dizini ve altindaki her sey.
        wchar_t winDirBuf[MAX_PATH] = {};
        QString winDir;
        if (GetWindowsDirectoryW(winDirBuf, MAX_PATH) > 0)
            winDir = temizle(QString::fromWCharArray(winDirBuf));
        if (winDir.isEmpty())
            winDir = temizle(qEnvironmentVariable("SystemRoot"));
        if (!winDir.isEmpty() && ayniVeyaAltinda(y, winDir))
            return QStringLiteral("Windows sistem dizini.");

        // Uygulamanin kendi klasoru ve ustu (kendini silmesin).
        const QString appDir = temizle(QCoreApplication::applicationDirPath());
        if (!appDir.isEmpty() && ayniVeyaAltinda(appDir, y))
            return QStringLiteral("Bu uygulamanin calistigi klasor.");

        // Surucu kokleri altinda korunan sistem klasorleri (KENDILERI).
        static const QStringList surucuAltiKoklar = {
            "System Volume Information", "$Recycle.Bin", "Recovery",
            "Boot", "EFI", "$WinREAgent", "PerfLogs"
        };
        // Su köklerin KENDISI (altindakiler serbest).
        static const QStringList sabitKokler = {
            "Program Files", "Program Files (x86)", "ProgramData", "Users"
        };
        const QString ad = y.section('/', -1);
        for (const QString &k : surucuAltiKoklar) {
            if (ad.compare(k, Qt::CaseInsensitive) == 0 && y.count('/') <= 1)
                return QStringLiteral("Windows'un korudugu sistem klasoru.");
        }
        for (const QString &k : sabitKokler) {
            // "C:/Program Files" gibi tek seviye altinda olmali - alt
            // klasorleri (ic icerigi) serbest kalsin.
            if (ad.compare(k, Qt::CaseInsensitive) == 0)
                return QStringLiteral("Windows'un korudugu sistem klasoru.");
        }

        // Kullanici profili kokü.
        const QString ev = temizle(QDir::homePath());
        if (tamEsit(y, ev))
            return QStringLiteral("Kullanici profili klasoru.");

        // Kullanicinin standart klasorlerinin KENDISI (icerikleri serbest).
        static const QList<QStandardPaths::StandardLocation> standartKlasorler = {
            QStandardPaths::DesktopLocation, QStandardPaths::DocumentsLocation,
            QStandardPaths::DownloadLocation, QStandardPaths::PicturesLocation,
            QStandardPaths::MoviesLocation, QStandardPaths::MusicLocation
        };
        for (auto konum : standartKlasorler) {
            const QString k = temizle(QStandardPaths::writableLocation(konum));
            if (tamEsit(y, k))
                return QStringLiteral("Standart kullanici klasoru.");
        }

        // Sistem dosyalari - dosya adi karsilastirmasi.
        static const QStringList korumaliDosyalar = {
            "pagefile.sys", "hiberfil.sys", "swapfile.sys",
            "bootmgr", "ntldr", "boot.ini", "bootnxt"
        };
        for (const QString &d : korumaliDosyalar) {
            if (ad.compare(d, Qt::CaseInsensitive) == 0)
                return QStringLiteral("Windows sistem dosyasi.");
        }

        return QString();
    } catch (...) {
        // Beklenmeyen hata: guvenli tarafta kal, silmeyi engelle.
        return QStringLiteral("Yol dogrulanamadi.");
    }
}

QString riskUyarisi(const QString &yol)
{
    try {
        if (yol.isEmpty()) return QString();
        const QString y = temizle(yol);

        static const QStringList programKlasorleri = { "Program Files", "Program Files (x86)" };
        for (const QString &k : programKlasorleri) {
            // "sürücü:/Program Files/..." - kok kendisi degil ALTI aranir,
            // kendisi zaten korumaSebebi'nde engelleniyor.
            if (y.contains("/" + k + "/", Qt::CaseInsensitive))
                return QStringLiteral("Yüklü bir programın dosyaları. Silmek programı bozabilir.");
        }

        if (y.contains("/ProgramData/", Qt::CaseInsensitive) || y.contains("/AppData/", Qt::CaseInsensitive))
            return QStringLiteral("Uygulama verileri/ayarları. Silmek uygulamaları etkileyebilir.");

        const QString uzanti = QFileInfo(y).suffix().toLower();
        if (uzanti == "exe" || uzanti == "dll" || uzanti == "sys")
            return QStringLiteral("Program dosyası.");

        return QString();
    } catch (...) {
        return QString();
    }
}

bool explorerdaGoster(const QString &yol)
{
    try {
        const QString native = QDir::toNativeSeparators(yol);
        if (QFileInfo(yol).isFile())
            return QProcess::startDetached(QStringLiteral("explorer.exe"), { QStringLiteral("/select,"), native });
        return QProcess::startDetached(QStringLiteral("explorer.exe"), { native });
    } catch (...) {
        return false;
    }
}

bool konumunuGoster(const QString &yol)
{
    try {
        const QString native = QDir::toNativeSeparators(yol);
        // /select hem dosya hem klasor icin calisir - ust dizini acip
        // hedefi isaretler.
        return QProcess::startDetached(QStringLiteral("explorer.exe"),
                                       { QStringLiteral("/select,") + native });
    } catch (...) {
        return false;
    }
}

bool ozellikleriGoster(const QString &yol)
{
    try {
        const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const bool comBaslattik = hr == S_OK;
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return false;

        const std::wstring wyol = QDir::toNativeSeparators(yol).toStdWString();

        SHELLEXECUTEINFOW bilgi = {};
        bilgi.cbSize = sizeof(bilgi);
        bilgi.fMask = SEE_MASK_INVOKEIDLIST;
        bilgi.lpVerb = L"properties";
        bilgi.lpFile = wyol.c_str();
        bilgi.nShow = SW_SHOW;

        const bool basarili = ShellExecuteExW(&bilgi);
        if (comBaslattik) CoUninitialize();
        return basarili;
    } catch (...) {
        return false;
    }
}

bool komutIstemiAc(const QString &klasorYolu)
{
    try {
        const QString native = QDir::toNativeSeparators(klasorYolu);
        return QProcess::startDetached(QStringLiteral("cmd.exe"),
                                        { QStringLiteral("/K"), QStringLiteral("cd"), QStringLiteral("/d"), native },
                                        native);
    } catch (...) {
        return false;
    }
}

}
