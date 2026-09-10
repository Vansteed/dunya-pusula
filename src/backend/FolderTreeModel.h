// FolderTreeModel.h - AGAC TARA sayfasi icin gercek agac modeli. Mantik
// DiskTreeTab.cpp (Widgets surumu) ile AYNI: tembel yukleme, dugum basina
// bagimsiz es zamanli tarama, canli satir ekleme. Siralama icin ayrica
// QSortFilterProxyModel sarmalanir (siraliModel) - kendi sort() kodu yazmaya
// gerek yok, Qt zaten yapiyor.
#pragma once

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QList>
#include <QElapsedTimer>
#include <QTimer>

#include "core/FolderScanner.h"

class QThread;

class FolderTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QVariantList suruculer READ suruculer NOTIFY suruculerDegisti)
    Q_PROPERTY(QVariantList kisayollar READ kisayollar CONSTANT)
    Q_PROPERTY(QString kokYol READ kokYol NOTIFY kokYolDegisti)
    Q_PROPERTY(QString durum READ durum NOTIFY durumDegisti)
    Q_PROPERTY(bool taraniyor READ taraniyor NOTIFY durumDegisti)
    Q_PROPERTY(QObject *siraliModel READ siraliModel CONSTANT)
    Q_PROPERTY(double ilerleme READ ilerleme NOTIFY ilerlemeDegisti)
    Q_PROPERTY(QString ilerlemeMetni READ ilerlemeMetni NOTIFY ilerlemeDegisti)
    Q_PROPERTY(QString taramaOzeti READ taramaOzeti NOTIFY ilerlemeDegisti)
    // QML'deki bos durum bloguyla agac arasinda gecis icin - kok en az bir
    // cocuk alinca (ilk oge) veya tarama bitip hic sonuc yoksa guncellenir.
    Q_PROPERTY(bool sonucVar READ sonucVar NOTIFY durumDegisti)
    Q_PROPERTY(QVariantList buyukDosyalar READ buyukDosyalar NOTIFY buyukDosyalarDegisti)
    Q_PROPERTY(int esikMB READ esikMB WRITE esikMBAyarla NOTIFY esikMBDegisti)
    Q_PROPERTY(QString turFiltresi READ turFiltresi WRITE turFiltresiAyarla NOTIFY turFiltresiDegisti)
    Q_PROPERTY(bool sistemGizle READ sistemGizle WRITE sistemGizleAyarla NOTIFY sistemGizleDegisti)
    // PANEL sayfasindaki DEPOLAMA karti icin - dolu alanin kategorilere
    // (Kullanici dosyalari/Windows/Programlar/Diger) kirilimi. Sadece surucu
    // KOKU tarandiginda dolar (bkz. surucuSec / tamamlandi lambda'si).
    Q_PROPERTY(QVariantList depolamaKirilimi READ depolamaKirilimi NOTIFY depolamaKirilimiDegisti)

public:
    enum Rol {
        AdRolu = Qt::UserRole + 1,
        TamYolRolu,
        BoyutRolu,
        BoyutMetniRolu,
        YuzdeRolu,
        AltKlasoruVarRolu,
        TaraniyorRolu,
        OlculduRolu
    };
    Q_ENUM(Rol)

    explicit FolderTreeModel(QObject *parent = nullptr);
    ~FolderTreeModel() override;

    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int satir, int sutun, const QModelIndex &ust) const override;
    QModelIndex parent(const QModelIndex &idx) const override;
    int rowCount(const QModelIndex &ust) const override;
    int columnCount(const QModelIndex &ust) const override;
    bool hasChildren(const QModelIndex &ust) const override;
    QVariant data(const QModelIndex &idx, int rol) const override;

    QVariantList suruculer() const { return m_suruculer; }
    QVariantList kisayollar() const { return m_kisayollar; }
    QString kokYol() const { return m_kokYol; }
    QString durum() const { return m_durum; }
    bool taraniyor() const { return !m_aktifThreadler.isEmpty(); }
    QObject *siraliModel() const { return m_proxy; }
    double ilerleme() const { return m_ilerleme; }
    QString ilerlemeMetni() const { return m_ilerlemeMetni; }
    QString taramaOzeti() const { return m_taramaOzeti; }
    bool sonucVar() const { return !m_kok->cocuklar.isEmpty(); }
    QVariantList buyukDosyalar() const { return m_buyukDosyalar; }
    QVariantList depolamaKirilimi() const { return m_depolamaKirilimi; }
    int esikMB() const { return m_esikMB; }
    void esikMBAyarla(int mb);
    QString turFiltresi() const { return m_turFiltresi; }
    // Bos ise boyut havuzu (esikMB gecerli), doluysa tur havuzu.
    void turFiltresiAyarla(const QString &tur);
    bool sistemGizle() const { return m_sistemGizle; }
    void sistemGizleAyarla(bool gizle);

    // Surucu kartlari kuruluşta bir kez dolduruluyor; disk takilip
    // cikarilabildigi icin (USB) manuel yenileme de sunuluyor.
    Q_INVOKABLE void suruculeriYenile();
    Q_INVOKABLE void surucuSec(const QString &yol);
    // Kisayol tiklamasinda agaci sifirlamamak icin - yol zaten taranmis
    // agacin icindeyse PROXY indeksini doner, degilse gecersiz doner (o
    // zaman cagiran surucuSec ile yeni tarama baslatir).
    Q_INVOKABLE QModelIndex yoluBul(const QString &yol) const;
    Q_INVOKABLE void dugumuAc(const QModelIndex &idx);
    // Devam eden tum taramalari iptal eder - worker'lar kendi dongulerinde
    // bayragi gorup erken cikar, thread'ler normal tamamlandi() akisiyla
    // (quit+wait) kapanir; dangling pointer riski yok.
    Q_INVOKABLE void taramayiDurdur();
    // Sag detay paneli icin - ad/tamYol/boyutMetni tek cagrida.
    Q_INVOKABLE QVariantMap dugumBilgisi(const QModelIndex &idx) const;
    Q_INVOKABLE void boyutaGoreSirala(bool azalan);

    // Sag tik menusu - agac dugumu silme (geri donusum/kalici). Tarama
    // surerken false doner (dangling pointer riski, KURALLAR'a bkz).
    Q_INVOKABLE bool sil(const QModelIndex &idx, bool kalici);
    // Sag paneldeki dosya listesi icin - modelde dugumu yok, sadece yol.
    Q_INVOKABLE bool dosyaSil(const QString &tamYol, bool kalici);
    // QML'in onay diyalogunda gosterebilmesi icin DosyaIslem'deki
    // karsiliklarina gecirgen erisim - engel doluysa silme yapilamaz.
    Q_INVOKABLE QString korumaSebebi(const QString &yol) const;
    Q_INVOKABLE QString riskUyarisi(const QString &yol) const;
    Q_INVOKABLE void explorerdaAc(const QString &tamYol);
    Q_INVOKABLE void konumunuGoster(const QString &tamYol);
    Q_INVOKABLE void ozellikleriGoster(const QString &tamYol);
    Q_INVOKABLE void komutIstemiAc(const QString &tamYol);
    Q_INVOKABLE void yenidenTara(const QModelIndex &idx);
    Q_INVOKABLE void panoyaYaz(const QString &metin);

signals:
    void durumDegisti();
    void ilerlemeDegisti();
    void suruculerDegisti();
    void kokYolDegisti();
    void buyukDosyalarDegisti();
    void depolamaKirilimiDegisti();
    void esikMBDegisti();
    void turFiltresiDegisti();
    void sistemGizleDegisti();
    // Devam eden tum worker'lara iptal bayragini kaldirmak icin - worker'lar
    // farkli thread'de yasadigindan Qt otomatik olarak kuyruklu baglanti
    // kurar (ayri thread'ler arasi sinyal/slot her zaman thread-safe'dir).
    void tumunuIptalEt();

private:
    struct Dugum {
        QString ad;
        QString tamYol;
        qint64 boyut = 0;
        bool altKlasoruVar = false;
        bool yuklendi = false;
        bool taraniyor = false;
        bool olculdu = false; // boyut ikinci gecisle olculdu mu (false = "...")
        // Bu dugumun DOGRUDAN icindeki dosyalarin toplami - alt
        // klasor satirlarina dahil degil, ozette ayri eklenir.
        qint64 dosyaBayt = 0;
        Dugum *ust = nullptr;
        QList<Dugum *> cocuklar;
        ~Dugum() { qDeleteAll(cocuklar); }
    };

    Dugum *dugumdenIsaretCikar(const QModelIndex &idx) const;
    QModelIndex dugumIcinIndex(Dugum *d) const;
    void taramaBaslat(Dugum *ustDugum, const QString &yol);
    // Tarama sirasinda kurulan FolderScanner::DizinDugumu alt agacini Dugum
    // agacina cevirir - taramanin bulup olctugu her alt klasor icin ayri bir
    // altKlasorleriTara cagrisi gerekmesin diye (tek taramada tam agac).
    void agaciKur(Dugum *hedef, const FolderScanner::DizinDugumu &kaynak);
    void ilerlemeYayinla(bool zorla);
    void buyukDosyalariYenile(bool zorla);
    void depolamaKirilimiHesapla();

    Dugum *m_kok; // gorunmez kok - cocuklari secili surucunun ust seviye klasorleri
    QString m_kokYol;
    QString m_durum = "Bir sürücü seç.";
    QVariantList m_suruculer;
    // Masaustu/Indirilenler gibi sik taranan klasorler - uygulama
    // acilirken bir kez bulunur, degismez (CONSTANT).
    QVariantList m_kisayollar;
    QList<QThread *> m_aktifThreadler; // ~FolderTreeModel icin kapanista bekle
    QSortFilterProxyModel *m_proxy;

    // Ilerleme cubugu icin - hedefToplam ogeBulundu ile, hedefOlculen
    // ogeGuncellendi ile artar. Yayin en fazla ~200ms'de bir (UI bogulmasin).
    int m_hedefToplam = 0;
    int m_hedefOlculen = 0;
    double m_ilerleme = 0.0;
    QString m_ilerlemeMetni;
    QString m_taramaOzeti;

    // Tum es zamanli taramalar arasinda PAYLASILAN toplayici - FolderTreeModel
    // omru boyunca yasar, worker'lara ham isaretci olarak verilir.
    FolderScanner::BuyukDosyaToplayici m_buyukDosyaToplayici;
    // Toplayici sabit bir tabanla (10 MB) top-N dosya biriktirir;
    // esik SADECE yayin filtresi - tarama bittikten sonra degistirilince
    // liste yeniden suzulur, yeniden taramak gerekmez.
    int m_esikMB = 500;
    QString m_turFiltresi;
    bool m_sistemGizle = true;
    QVariantList m_buyukDosyalar;
    QVariantList m_depolamaKirilimi;
    QElapsedTimer m_buyukDosyaYayinZamani;
    // Olcum olayi gelmeden uzun sure gecebilir (dev klasor); gecen sure
    // sayaci donmasin diye saniyede bir yayin tetiklenir.
    QTimer m_saatTik;
    // Kalan sure tahmini bayt uzerinden yapilir: klasor SAYISI yaniltici
    // (C:/Users tek satir ama diskin yarisi). Hedef = surucudeki dolu alan.
    qint64 m_hedefBayt = 0;
    qint64 m_olculenBayt = 0;
    QElapsedTimer m_taramaZamani;
    QElapsedTimer m_sonYayinZamani;
};
