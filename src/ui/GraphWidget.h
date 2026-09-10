// GraphWidget.h - "Son 80 saniye" cizgi+alan grafik. WPF Polyline/Polygon
// (RamTemizleyici.ps1 Update-Graph, satir ~137) karsiligi, ozel QPainter cizimi.
// ponytail: QtCharts bagimliligi eklemek yerine kucuk bir custom paint yeterli.
#pragma once

#include <QWidget>
#include <QVector>

class GraphWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GraphWidget(QWidget *parent = nullptr);

    // PowerShell'deki $global:History ile ayni: en fazla 40 ornek tutulur.
    void ekle(double yuzde);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    static constexpr int MaxOrnek = 40;
    QVector<double> m_gecmis;
};
