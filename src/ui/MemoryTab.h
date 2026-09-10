// MemoryTab.h - BELLEK sekmesi. RamTemizleyici.ps1'deki RAM karti + grafik +
// SIMDI TEMIZLE + islem tablosu + otomatik temizlik + log konsolu esdegeri.
#pragma once

#include <QWidget>
#include <QDateTime>
#include <QSet>
#include <QString>
#include <vector>

#include "core/ProcessList.h"

class QLabel;
class QProgressBar;
class QPushButton;
class QTreeWidget;
class QLineEdit;
class QCheckBox;
class QSlider;
class QPlainTextEdit;
class QThread;

class GraphWidget;
class CleanWorker;

class MemoryTab : public QWidget
{
    Q_OBJECT

public:
    explicit MemoryTab(QWidget *parent = nullptr);
    ~MemoryTab() override;

private slots:
    void ramBilgisiYenile();
    void islemListesiYenile();
    void temizlemeBaslat();
    void adimBasladi(int adim);
    void adimBitti(int adim, const QString &mesaj);
    void temizlemeBitti();
    void secilenleriKapat();
    void otomatikKontrolEt();
    void esikDegisti(int deger);
    void otomatikDegisti(bool acik);

private:
    QWidget *ramKartiOlustur();
    QWidget *grafikKartiOlustur();
    QWidget *temizleKartiOlustur();
    QWidget *islemTablosuKartiOlustur();
    QWidget *otomatikKartiOlustur();
    QWidget *logKartiOlustur();
    void ayarlariYukle();
    void ayarlariKaydet();
    void log(const QString &mesaj);
    void adimNoktasiGuncelle(QLabel *nokta, const QString &durum);

    // RAM karti
    QLabel *m_lblPercent = nullptr;
    QProgressBar *m_barRam = nullptr;
    QLabel *m_lblUsed = nullptr;
    QLabel *m_lblTotal = nullptr;
    QLabel *m_lblFree = nullptr;
    QLabel *m_lblWarn = nullptr;

    // Grafik
    GraphWidget *m_grafik = nullptr;

    // Temizle
    QPushButton *m_btnTemizle = nullptr;
    QLabel *m_step1 = nullptr;
    QLabel *m_step2 = nullptr;
    QLabel *m_step3 = nullptr;
    QThread *m_temizleThread = nullptr;
    CleanWorker *m_temizleWorker = nullptr;
    bool m_temizleniyor = false;

    // Islem tablosu
    QTreeWidget *m_tabloGorunum = nullptr;
    QLineEdit *m_txtArama = nullptr;
    QPushButton *m_btnYenile = nullptr;
    std::vector<ProcKaydi> m_sonKayitlar; // son yenileme sonucu (kapatma icin)
    QSet<QString> m_isaretliAd;           // kucuk harfli isim - checkbox durumu korunur
    void filtreUygula();
    std::vector<ProcKaydi> secilenleriTopla() const;
    int secilenleriSessizceKapat(const std::vector<ProcKaydi> &secilenler);

    // Otomatik temizlik
    QCheckBox *m_toggleOtomatik = nullptr;
    QSlider *m_sliderEsik = nullptr;
    QLabel *m_lblEsik = nullptr;
    QDateTime m_sonOtomatikTemizlik;

    // Log
    QPlainTextEdit *m_logKutusu = nullptr;
};
