#include "GraphWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>

GraphWidget::GraphWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(104);
}

void GraphWidget::ekle(double yuzde)
{
    m_gecmis.append(yuzde);
    while (m_gecmis.size() > MaxOrnek) {
        m_gecmis.removeFirst();
    }
    update();
}

void GraphWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double w = width() > 10 ? width() : 320;
    const double h = height() > 10 ? height() : 104;
    const int n = m_gecmis.size();
    if (n < 2) return;

    QPolygonF cizgi;
    for (int i = 0; i < n; ++i) {
        // PowerShell'deki gibi 39'a bolunerek x konumlanir (MaxOrnek-1).
        const double x = (static_cast<double>(i) / (MaxOrnek - 1)) * w;
        const double v = m_gecmis[i];
        const double y = h - ((v / 100.0) * (h - 12.0)) - 6.0;
        cizgi << QPointF(x, y);
    }

    QPolygonF alan = cizgi;
    alan << QPointF(w, h) << QPointF(0, h);

    QLinearGradient grad(0, 0, 0, h);
    grad.setColorAt(0.0, QColor(76, 95, 126, 90));
    grad.setColorAt(1.0, QColor(76, 95, 126, 0));
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawPolygon(alan);

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor("#4C5F7E"), 2));
    p.drawPolyline(cizgi);
}
