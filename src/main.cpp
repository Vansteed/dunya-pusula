// main.cpp - uygulama giris noktasi (QML / Qt Quick surumu).

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFont>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

#include "backend/CpuBackend.h"
#include "backend/FolderTreeModel.h"
#include "backend/HealthBackend.h"
#include "backend/MemoryBackend.h"
#include "backend/ProcessModel.h"
#include "core/SensorAjani.h"
#include "core/SingleInstance.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Dünya Pusula"));

    QFont f(QStringLiteral("Segoe UI"), 9);
    f.setHintingPreference(QFont::PreferFullHinting);
    f.setStyleStrategy(QFont::PreferAntialias);
    QGuiApplication::setFont(f);

    // Ikinci ornek sessizce cikar - QML yuklenmeden once kontrol edilir ki
    // gereksiz motor kurulumu olmasin.
    SingleInstance tekOrnek;
    if (!tekOrnek.edinildiMi()) return 0;

    // Tek SensorAjaniIstemcisi - CpuBackend ve HealthBackend ayni ajan
    // surecini paylasir, iki ayri SensorAjani.exe acilmasin.
    SensorAjaniIstemcisi sensorAjani;
    sensorAjani.baslat();

    MemoryBackend bellek;
    ProcessModel islemler;
    CpuBackend islemci(&sensorAjani);
    HealthBackend saglik(&sensorAjani);
    FolderTreeModel agac;

    QQmlApplicationEngine motor;
    // Qt'nin QML modul agaci exe yanina kopyalaniyor (windeployqt yok),
    // motorun varsayilan arama yolu oraya bakmiyor - elle eklenir.
    motor.addImportPath(QCoreApplication::applicationDirPath() + "/qml");
    motor.rootContext()->setContextProperty(QStringLiteral("bellek"), &bellek);
    motor.rootContext()->setContextProperty(QStringLiteral("islemler"), &islemler);
    motor.rootContext()->setContextProperty(QStringLiteral("islemci"), &islemci);
    motor.rootContext()->setContextProperty(QStringLiteral("saglik"), &saglik);
    motor.rootContext()->setContextProperty(QStringLiteral("agac"), &agac);

    // Yuklenemezse (QML sozdizim hatasi vs.) objectCreationFailed gelir -
    // sessizce takilip kalmak yerine hata koduyla cik.
    QObject::connect(&motor, &QQmlApplicationEngine::objectCreationFailed, &app,
                     []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    // RT_QML_DISK=1 ile QML gomulu kaynak yerine kaynak agacindaki qml/
    // klasorunden okunur ve dosya degisince arayuz kendini yeniler - QML
    // duzenlerken yeniden derleme gerekmez. C++ degisiklikleri yine derleme
    // ister. Uretim calismasinda bu degisken yoktur, gomulu surum kullanilir.
    const bool diskMod = qEnvironmentVariableIsSet("RT_QML_DISK");
    if (diskMod) {
        const QString kaynakDizin = QStringLiteral(RT_QML_KAYNAK);
        const QString anaDosya = kaynakDizin + "/Main.qml";
        motor.addImportPath(kaynakDizin + "/..");

        auto yukle = [&motor, anaDosya]() {
            motor.clearComponentCache();
            const auto kokler = motor.rootObjects();
            for (QObject *o : kokler) o->deleteLater();
            motor.load(QUrl::fromLocalFile(anaDosya));
        };

        auto *gozcu = new QFileSystemWatcher(&app);
        gozcu->addPath(kaynakDizin);
        QDirIterator yineleyici(kaynakDizin, QStringList{ "*.qml" }, QDir::Files,
                                QDirIterator::Subdirectories);
        while (yineleyici.hasNext()) {
            const QString dosya = yineleyici.next();
            gozcu->addPath(dosya);
            gozcu->addPath(QFileInfo(dosya).absolutePath());
        }

        // Editorler dosyayi silip yeniden yazar - izleme dusebilir, geri ekle.
        // Ustuste gelen olaylari tek yenilemede toplamak icin kisa gecikme.
        auto *gecikme = new QTimer(&app);
        gecikme->setSingleShot(true);
        gecikme->setInterval(150);
        QObject::connect(gecikme, &QTimer::timeout, &app, yukle);
        auto degisti = [gozcu, gecikme](const QString &yol) {
            if (QFile::exists(yol) && !gozcu->files().contains(yol) &&
                !gozcu->directories().contains(yol)) {
                gozcu->addPath(yol);
            }
            gecikme->start();
        };
        QObject::connect(gozcu, &QFileSystemWatcher::fileChanged, &app, degisti);
        QObject::connect(gozcu, &QFileSystemWatcher::directoryChanged, &app, degisti);

        yukle();
    } else {
        motor.loadFromModule("RamTemizleyici", "Main");
    }

    return app.exec();
}
