#include "DiskTreeScanWorker.h"

void DiskTreeScanWorker::tara()
{
    int64_t kokDosyalari = 0;
    try {
        m_sonuc = FolderScanner::altKlasorleriTara(m_yol.toStdString(),
            [this](const std::string &yol) { emit ilerleme(QString::fromStdString(yol)); },
            [this](const FolderScanner::AltKlasorBilgisi &b) {
                emit ogeBulundu(QString::fromStdString(b.ad), QString::fromStdString(b.tamYol),
                                static_cast<qint64>(b.boyut), b.altKlasoruVar, b.baglanti);
            },
            [this](const FolderScanner::AltKlasorBilgisi &b) {
                emit ogeGuncellendi(QString::fromStdString(b.tamYol), static_cast<qint64>(b.boyut));
            },
            m_buyukDosyalar, &m_iptal, &kokDosyalari);

    } catch (const std::exception &) {
        // Genis catch - QThread worker slot'unda yakalanmayan istisna
        // butun uygulamayi cokertir, tarama bos sonucla devam eder.
        m_sonuc.clear();
    }
    m_dosyaBaytlari = static_cast<qint64>(kokDosyalari);
    emit tamamlandi();
}
