// HealthBackend.h - SAGLIK sayfasi. Skor + kirilim + disk sagligi (SMART).
// Hesaplama hizli oldugu icin ayri thread YOK (Widgets surumuyle ayni karar).
#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>

#include "core/SensorAjani.h"

class HealthBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int puan READ puan NOTIFY skorDegisti)
    Q_PROPERTY(QString yorum READ yorum NOTIFY skorDegisti)
    Q_PROPERTY(int ramPuan READ ramPuan NOTIFY skorDegisti)
    Q_PROPERTY(int diskPuan READ diskPuan NOTIFY skorDegisti)
    Q_PROPERTY(int diskSaglikPuan READ diskSaglikPuan NOTIFY skorDegisti)
    Q_PROPERTY(bool yonetici READ yonetici NOTIFY skorDegisti)
    Q_PROPERTY(QVariantList diskler READ diskler NOTIFY skorDegisti)

public:
    explicit HealthBackend(SensorAjaniIstemcisi *sensorAjani, QObject *parent = nullptr);

    int puan() const { return m_puan; }
    QString yorum() const { return m_yorum; }
    int ramPuan() const { return m_ramPuan; }
    int diskPuan() const { return m_diskPuan; }
    int diskSaglikPuan() const { return m_diskSaglikPuan; }
    bool yonetici() const { return m_yonetici; }
    QVariantList diskler() const { return m_diskler; }

    Q_INVOKABLE void yenile();

signals:
    void skorDegisti();

private:
    int m_puan = 0;
    QString m_yorum;
    int m_ramPuan = 0;
    int m_diskPuan = 0;
    int m_diskSaglikPuan = 0;
    bool m_yonetici = false;
    QVariantList m_diskler;

    SensorAjaniIstemcisi *m_sensorAjani;
    QTimer m_timer;
};
