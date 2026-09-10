// DiskScanWorker.h - DiskCleaner::tara() cagrisini QThread'e tasiyan worker.
// Sonuc parametresiz sinyalle bildirilir, DiskTab worker'dan sonucAl() ile
// okur - Q_DECLARE_METATYPE gerektirmez, en az kod.
#pragma once

#include <QObject>

#include "core/DiskCleaner.h"

class DiskScanWorker : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    const std::vector<TemizlikSonucSatiri> &sonucAl() const { return m_sonuc; }

public slots:
    void tara();

signals:
    void tamamlandi();

private:
    std::vector<TemizlikSonucSatiri> m_sonuc;
};
