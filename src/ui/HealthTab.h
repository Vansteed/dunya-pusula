// HealthTab.h - SAGLIK sekmesi. Skor karti + kirilim + disk sagligi tablosu
// (moduller\20-skor.ps1 esdegeri). Bu turda SIL dugmesi YOK.
#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;

class HealthTab : public QWidget
{
    Q_OBJECT

public:
    explicit HealthTab(QWidget *parent = nullptr);

private slots:
    void yenile();

private:
    QWidget *skorKartiOlustur();
    QWidget *saglikKartiOlustur();

    QLabel *m_lblPuan = nullptr;
    QLabel *m_lblYorum = nullptr;
    QLabel *m_lblRam = nullptr;
    QLabel *m_lblDisk = nullptr;
    QLabel *m_lblBaslangic = nullptr;
    QLabel *m_lblDiskSaglik = nullptr;
    QPushButton *m_btnYenile = nullptr;

    QLabel *m_lblUyari = nullptr;
    QTableWidget *m_tablo = nullptr;
};
