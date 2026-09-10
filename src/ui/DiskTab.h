// DiskTab.h - DISK sekmesi. Disk kullanim karti + TARA dugmesi + temizlenebilir
// hedefler tablosu (RamTemizleyici.ps1 / moduller\10-disk.ps1 esdegeri).
// Bu turda SIL dugmesi YOK.
#pragma once

#include <QWidget>

class QLabel;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QThread;

class DiskScanWorker;

class DiskTab : public QWidget
{
    Q_OBJECT

public:
    explicit DiskTab(QWidget *parent = nullptr);
    ~DiskTab() override;

private slots:
    void diskBilgisiYenile();
    void taramaBaslat();
    void taramaBitti();

private:
    QWidget *diskKartiOlustur();
    QWidget *taramaKartiOlustur();

    QLabel *m_lblPercent = nullptr;
    QProgressBar *m_barDisk = nullptr;
    QLabel *m_lblUsed = nullptr;
    QLabel *m_lblTotal = nullptr;
    QLabel *m_lblFree = nullptr;

    QPushButton *m_btnTara = nullptr;
    QProgressBar *m_barTarama = nullptr;
    QTableWidget *m_tablo = nullptr;

    QThread *m_taramaThread = nullptr;
    DiskScanWorker *m_taramaWorker = nullptr;
    bool m_taraniyor = false;
};
