// CleanWorker.h - Start-CleanSequence (RamTemizleyici.ps1 ~1145) esdegeri,
// arayuz thread'ini bloklamamasi icin QThread'e tasinacak QObject.
#pragma once

#include <QObject>

class CleanWorker : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    // 3 adim: (1) hazirlik, (2) WorkingSet kirpma, (3) standby bosaltma.
    void calistir();

signals:
    void adimBasladi(int adim);
    void adimBitti(int adim, const QString &mesaj);
    void tamamlandi();
};
