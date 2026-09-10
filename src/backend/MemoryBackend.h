// MemoryBackend.h - BELLEK sayfasinin C++ tarafi. RAM bilgisi, 80 saniyelik
// gecmis, temizleme (arka plan thread), otomatik temizlik ayarlari.
#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

class QThread;
class CleanWorker;

class MemoryBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double yuzde READ yuzde NOTIFY ramDegisti)
    Q_PROPERTY(double toplamGB READ toplamGB NOTIFY ramDegisti)
    Q_PROPERTY(double kullanilanGB READ kullanilanGB NOTIFY ramDegisti)
    Q_PROPERTY(double bosGB READ bosGB NOTIFY ramDegisti)
    Q_PROPERTY(QVariantList gecmis READ gecmis NOTIFY ramDegisti)
    Q_PROPERTY(bool temizleniyor READ temizleniyor NOTIFY temizlemeDurumu)
    Q_PROPERTY(int adim READ adim NOTIFY temizlemeDurumu)
    Q_PROPERTY(bool otomatik READ otomatik WRITE otomatikAta NOTIFY ayarDegisti)
    Q_PROPERTY(int esik READ esik WRITE esikAta NOTIFY ayarDegisti)
    Q_PROPERTY(QStringList gunlukler READ gunlukler NOTIFY gunlukDegisti)
    Q_PROPERTY(QString sonKazanc READ sonKazanc NOTIFY temizlemeDurumu)

public:
    explicit MemoryBackend(QObject *parent = nullptr);
    ~MemoryBackend() override;

    double yuzde() const { return m_yuzde; }
    double toplamGB() const { return m_toplamGB; }
    double kullanilanGB() const { return m_kullanilanGB; }
    double bosGB() const { return m_bosGB; }
    QVariantList gecmis() const { return m_gecmis; }
    bool temizleniyor() const { return m_temizleniyor; }
    int adim() const { return m_adim; }
    bool otomatik() const { return m_otomatik; }
    int esik() const { return m_esik; }
    QStringList gunlukler() const { return m_gunlukler; }
    QString sonKazanc() const { return m_sonKazanc; }

    void otomatikAta(bool acik);
    void esikAta(int deger);

    // Uygulama kapatmanin RAM kazancini olcer: once kapatmaBasla(),
    // secilenler kapatildiktan sonra kapatmaBitti(). Isletim sistemi
    // bellegi hemen geri vermedigi icin olcum kisa bir gecikmeyle yapilir.
    Q_INVOKABLE void kapatmaBasla();
    Q_INVOKABLE void kapatmaBitti(int kapatilanSayisi);
    Q_INVOKABLE void temizle();
    Q_INVOKABLE void gunlukEkle(const QString &satir);
    Q_INVOKABLE void gunlukTemizle();

signals:
    void ramDegisti();
    void temizlemeDurumu();
    void ayarDegisti();
    void gunlukDegisti();

private:
    void ramYenile();
    void otomatikKontrol();
    void ayarlariKaydet();

    double m_yuzde = 0.0;
    double m_toplamGB = 0.0;
    double m_kullanilanGB = 0.0;
    double m_bosGB = 0.0;
    QVariantList m_gecmis;      // son 40 olcum (yuzde)
    QStringList m_gunlukler;
    QString m_sonKazanc;        // "1.2 GB boşaltıldı" - son temizlik kazanci

    bool m_temizleniyor = false;
    int m_adim = 0;
    double m_kullanilanGBOncesi = 0.0;  // temizlik BASLARKEN kullanilan GB
    QThread *m_thread = nullptr;
    CleanWorker *m_worker = nullptr;

    bool m_otomatik = false;
    int m_esik = 85;
    qint64 m_sonOtomatik = 0;   // epoch ms - 5 dakikalik bekleme icin

    QTimer m_ramTimer;
    QTimer m_otoTimer;
};
