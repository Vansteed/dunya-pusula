// MainWindow.h - cercevesiz ana pencere iskeleti. Ozel baslik cubugu, admin
// rozeti ve 4 sekmeli QTabWidget. Sekme icerikleri Phase 4-6'da doldurulacak.
#pragma once

#include <QMainWindow>
#include <QPoint>

class QLabel;
class QTabWidget;
class QMouseEvent;
class QShowEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QWidget *baslikCubuguOlustur();
    void adminRozetiGuncelle(QLabel *adminEtiket);

    QTabWidget *m_sekmeler = nullptr;
    bool m_surukleniyor = false;
    QPoint m_surukleBaslangic;
    bool m_koseAyarlandi = false;
};
