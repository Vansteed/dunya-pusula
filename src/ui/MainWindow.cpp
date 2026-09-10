// MainWindow.cpp - cercevesiz ana pencere. Baslik cubugu suruklemesi WPF
// TitleBar.Add_MouseLeftButtonDown + DragMove() karsiligidir
// (RamTemizleyici.ps1 satir 1912).
#include "MainWindow.h"
#include "MemoryTab.h"
#include "DiskTab.h"
#include "HealthTab.h"
#include "DiskTreeTab.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QMouseEvent>
#include <QShowEvent>
#include <QWidget>

#include <windows.h>
#include <shlobj.h>

namespace {

// Windows 11 yuvarlak pencere kosesi - DWMWA_WINDOW_CORNER_PREFERENCE (33),
// DWMWCP_ROUND (2). dwmapi.dll dinamik yuklenir (RestorePoint.cpp'deki
// srclient.dll yuklemesiyle ayni desen) - CMakeLists.txt'e dokunmaya gerek
// kalmaz. Windows 10'da fonksiyon/oznitelik yoksa sessizce atlanir.
void pencereKosesiniYuvarlaklastir(HWND pencere)
{
    using DwmSetWindowAttributeFn = HRESULT(WINAPI *)(HWND, DWORD, LPCVOID, DWORD);
    HMODULE dwmapi = LoadLibraryW(L"dwmapi.dll");
    if (!dwmapi) {
        return;
    }
    auto fn = reinterpret_cast<DwmSetWindowAttributeFn>(GetProcAddress(dwmapi, "DwmSetWindowAttribute"));
    if (fn) {
        const DWORD kosePolitikasi = 2; // DWMWCP_ROUND
        fn(pencere, 33, &kosePolitikasi, sizeof(kosePolitikasi));
    }
    FreeLibrary(dwmapi);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    resize(980, 760);

    auto *merkez = new QWidget(this);
    auto *anaDuzen = new QVBoxLayout(merkez);
    anaDuzen->setContentsMargins(1, 1, 1, 1);
    anaDuzen->setSpacing(0);

    anaDuzen->addWidget(baslikCubuguOlustur());

    m_sekmeler = new QTabWidget(merkez);
    m_sekmeler->addTab(new MemoryTab(m_sekmeler), "BELLEK");
    m_sekmeler->addTab(new DiskTab(m_sekmeler), "DISK");
    m_sekmeler->addTab(new HealthTab(m_sekmeler), "SAGLIK");
    m_sekmeler->addTab(new DiskTreeTab(m_sekmeler), "AGAC TARA");
    anaDuzen->addWidget(m_sekmeler, 1);

    setCentralWidget(merkez);
}

QWidget *MainWindow::baslikCubuguOlustur()
{
    auto *baslikCubugu = new QWidget(this);
    baslikCubugu->setObjectName("TitleBar");
    baslikCubugu->setFixedHeight(36);

    auto *duzen = new QHBoxLayout(baslikCubugu);
    duzen->setContentsMargins(12, 0, 6, 0);

    auto *baslikEtiket = new QLabel("Dünya Pusula", baslikCubugu);
    baslikEtiket->setStyleSheet("color: #F4F4F5; font-weight: 600; font-size: 12px;");
    duzen->addWidget(baslikEtiket);

    auto *adminEtiket = new QLabel(baslikCubugu);
    adminRozetiGuncelle(adminEtiket);
    duzen->addWidget(adminEtiket);

    duzen->addStretch(1);

    auto *btnKucult = new QPushButton("_", baslikCubugu);
    btnKucult->setObjectName("BtnBaslikDugmesi");
    btnKucult->setFixedSize(34, 24);
    connect(btnKucult, &QPushButton::clicked, this, &MainWindow::showMinimized);
    duzen->addWidget(btnKucult);

    auto *btnKapat = new QPushButton("X", baslikCubugu);
    btnKapat->setObjectName("BtnBaslikDugmesi");
    btnKapat->setFixedSize(34, 24);
    connect(btnKapat, &QPushButton::clicked, this, &MainWindow::close);
    duzen->addWidget(btnKapat);

    return baslikCubugu;
}

void MainWindow::adminRozetiGuncelle(QLabel *adminEtiket)
{
    // IsUserAnAdmin (Shell32) - RamTemizleyici.ps1'deki Test-IsAdmin karsiligi.
    const bool yonetici = IsUserAnAdmin();
    if (yonetici) {
        adminEtiket->setText("Yonetici");
        adminEtiket->setStyleSheet("color: #E4E4E7; font-size: 11px; margin-left: 8px;");
    } else {
        adminEtiket->setText("Normal");
        adminEtiket->setStyleSheet("color: #63636B; font-size: 11px; margin-left: 8px;");
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->position().y() <= 36) {
        m_surukleniyor = true;
        m_surukleBaslangic = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
        return;
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_surukleniyor && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_surukleBaslangic);
        event->accept();
        return;
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    m_surukleniyor = false;
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (!m_koseAyarlandi) {
        m_koseAyarlandi = true;
        pencereKosesiniYuvarlaklastir(reinterpret_cast<HWND>(winId()));
    }
}
