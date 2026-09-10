// DiskTreeScanWorker.h - FolderScanner::altKlasorleriTara() cagrisini
// QThread'e tasiyan worker. DiskScanWorker ile AYNI pattern - parametresiz
// tamamlandi() sinyali + sonucAl() getter, en az kod.
#pragma once

#include <QObject>
#include <QString>

#include <atomic>

#include "core/FolderScanner.h"

class DiskTreeScanWorker : public QObject
{
    Q_OBJECT

public:
    // buyukDosyalar: FolderTreeModel omru boyunca yasayan, tum es zamanli
    // taramalar arasinda PAYLASILAN toplayici (ham isaretci - sahiplik
    // FolderTreeModel'de, worker sadece dolduruyor).
    explicit DiskTreeScanWorker(const QString &yol, FolderScanner::BuyukDosyaToplayici *buyukDosyalar = nullptr,
                                QObject *parent = nullptr)
        : QObject(parent), m_yol(yol), m_buyukDosyalar(buyukDosyalar) {}

    const std::vector<FolderScanner::AltKlasorBilgisi> &sonucAl() const { return m_sonuc; }
    // Taranan dizinin dogrudan icindeki dosyalarin toplami.
    qint64 dosyaBaytlariAl() const { return m_dosyaBaytlari; }

public slots:
    void tara();
    // Devam eden taramayi erken kesmek icin - std::atomic bayrak sayesinde
    // baska thread'den (FolderTreeModel'in yasadigi ana thread) cagrilmasi
    // guvenli. tara() dongulerinde kontrol edilir, erken cikilir.
    void iptalEt() { m_iptal.store(true); }

signals:
    void tamamlandi();
    void ilerleme(const QString &yol);
    // Bir alt klasorun olcumu bitince canli satir eklemek icin - ad/tamYol/
    // boyut/altKlasoruVar tek tek, AltKlasorBilgisi Qt metatype'i olmadigi
    // icin (ekstra Q_DECLARE_METATYPE'a gerek yok, en az kod).
    void ogeBulundu(const QString &ad, const QString &tamYol, qint64 boyut, bool altKlasoruVar, bool baglanti);
    // Bir alt klasorun boyutu (ikinci gecis) olculunce - satiri yerinde
    // guncellemek icin.
    void ogeGuncellendi(const QString &tamYol, qint64 boyut);

private:
    QString m_yol;
    std::vector<FolderScanner::AltKlasorBilgisi> m_sonuc;
    FolderScanner::BuyukDosyaToplayici *m_buyukDosyalar;
    qint64 m_dosyaBaytlari = 0;
    std::atomic<bool> m_iptal{ false };
};
