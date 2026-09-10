// DiskTreeTab.h - AGAC TARA sekmesi. WizTree/FolderSize tarzi klasor boyutu
// gezgini (RamTemizleyici.ps1 satir 1470-1910, Expand-DiskNode/Fill-DiskNodes/
// Start-DiskScan/Init-DiskDrives/Fill-DetayGrid esdegeri).
#pragma once

#include <QWidget>
#include <QString>
#include <QSet>
#include <QList>

class QLabel;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QTableWidget;
class QThread;
class QHBoxLayout;
class QMenu;
class QPoint;

class DiskTreeScanWorker;

class DiskTreeTab : public QWidget
{
    Q_OBJECT

public:
    explicit DiskTreeTab(QWidget *parent = nullptr);
    ~DiskTreeTab() override;

private slots:
    void surucuTiklandi(const QString &kok);
    void dugumGenisletildi(QTreeWidgetItem *item);
    void secimDegisti();
    void sagTikMenusu(const QPoint &pos);

private:
    QWidget *surucuSatiriOlustur();
    // ustDugum == nullptr ise kok seviye (surucu) taramasi. Her cagri KENDI
    // thread/worker'ini acar - farkli node'lar AYNI ANDA taranabilir, tek bir
    // "su an tarayan" thread'e bagli DEGIL (once oyleydi, bir klasor
    // tarnirken digerlerine tiklamak hicbir sey yapmiyordu - kullanici
    // sikayeti buydu). Ayni node'un IKI KEZ ayni anda taranmasini
    // m_taraniyorHedefler engeller.
    void taramaBaslat(const QString &yol, QTreeWidgetItem *ustDugum);
    void detayPanelGuncelle(QTreeWidgetItem *item);
    void silDugum(QTreeWidgetItem *item);

    QHBoxLayout *m_surucuDuzeni = nullptr;
    QTreeWidget *m_agac = nullptr;
    QLabel *m_lblDurum = nullptr;

    // Sag panel
    QLabel *m_lblSeciliYol = nullptr;
    QLabel *m_lblSeciliBoyut = nullptr;
    QLabel *m_lblSeciliOzet = nullptr;
    QTableWidget *m_detayTablo = nullptr;

    QString m_kokSurucu;
    QSet<QTreeWidgetItem *> m_taraniyorHedefler; // nullptr = kok seviye taraniyor
    QList<QThread *> m_aktifThreadler;           // ~DiskTreeTab icin kapanista bekle
};
