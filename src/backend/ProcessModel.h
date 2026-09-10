// ProcessModel.h - ProcessList::yenile() sonucunu QML'e acan agac modeli.
// Ust satir = ayni isimdeki islem grubu, alt satir = tek PID (Widgets
// surumundeki QTreeWidget yapisinin model karsiligi).
#pragma once

#include <QAbstractItemModel>
#include <QHash>
#include <QIcon>
#include <QSet>
#include <QTimer>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <vector>

#include "core/ProcessList.h"

class ProcessModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString filtre READ filtre WRITE filtreAta NOTIFY filtreDegisti)
    Q_PROPERTY(int isaretliSayisi READ isaretliSayisi NOTIFY isaretliDegisti)

public:
    enum Rol {
        AdRolu = Qt::UserRole + 1,
        MbRolu,
        PidMetniRolu,
        DurumRolu,
        SeciliRolu,
        GrupMu,
        AltSayisi,
        SimgeYoluRolu,
        ExeYoluRolu,
        OncelikRolu,
        UstSatirRolu,  // -1 = ust satir, aksi halde ust satirin index'i (ic kullanim)
        SimgeUrlRolu   // exe simgesinin temp'teki PNG/XPM/BMP dosya URL'si (yoksa bos)
    };
    Q_ENUM(Rol)

    explicit ProcessModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int satir, int sutun, const QModelIndex &ust) const override;
    QModelIndex parent(const QModelIndex &idx) const override;
    int rowCount(const QModelIndex &ust) const override;
    int columnCount(const QModelIndex &ust) const override;
    QVariant data(const QModelIndex &idx, int rol) const override;

    QString filtre() const { return m_filtre; }
    void filtreAta(const QString &f);
    int isaretliSayisi() const { return m_isaretliAd.size(); }

    Q_INVOKABLE void yenile();

    // Acik menu/diyalog varken model reset baglari koparttigi icin
    // otomatik yenileme gecici olarak durdurulur.
    Q_INVOKABLE void otomatikYenileAta(bool acik);
    Q_INVOKABLE void isaretiCevir(int ustSatir);
    // Isaretli gruplarin tum PID'lerini kapatir, kapatilan islem sayisini doner.
    Q_INVOKABLE int secilenleriKapat();

    // 0=ad, 1=bellek. Listeyi yerinde siralar, sonraki yenile()'lerde korunur.
    Q_INVOKABLE void sirala(int sutun, bool azalan);

    // altSatir < 0 ise grubun TUMU, degilse tek PID. Korumali satirda false.
    Q_INVOKABLE bool kapat(int satir, int altSatir);
    Q_INVOKABLE bool agaciKapat(int satir, int altSatir);
    // Onay diyalogu icin: agaciKapat ONCESI kac surecin kapanacagini sayar.
    Q_INVOKABLE int agacBoyutu(int satir, int altSatir) const;
    Q_INVOKABLE bool oncelikAta(int satir, int altSatir, int sinif);
    Q_INVOKABLE QString exeYolu(int satir, int altSatir) const;
    Q_INVOKABLE QVariantMap satirBilgisi(int satir, int altSatir) const;
    Q_INVOKABLE QVariantList enCokKullanan(int adet) const;

    // Sag tik menusundeki "Dosya konumunu ac" / "Ozellikler" / kopyalama -
    // DosyaIslem cagrilarini QML'e acar (agac.* FolderTreeModel'de, burada yok).
    Q_INVOKABLE void konumunuGoster(int satir, int altSatir) const;
    Q_INVOKABLE void ozellikleriGoster(int satir, int altSatir) const;
    Q_INVOKABLE void panoyaYaz(const QString &metin) const;

signals:
    void filtreDegisti();
    void isaretliDegisti();
    void gunluk(const QString &satir);

private:
    bool filtreyeUyar(const ProcKaydi &k) const;
    void kayitlariSirala(std::vector<ProcKaydi> &kayitlar) const;
    bool kapatTekPid(uint32_t pid, const QString &ad);

    std::vector<ProcKaydi> m_kayitlar;   // filtreden GECEN kayitlar
    QTimer m_yenileTimer;
    QSet<QString> m_isaretliAd;          // kucuk harfli isim
    QString m_filtre;
    QHash<quint32, int> m_oncelikMap;    // pid -> oncelik (yenile()'de dolar)

    int m_sutun = -1;      // -1 = siralama uygulanmadi (dogal RAM sirasi)
    bool m_azalan = true;
};
