// Theme.cpp - QSS uretimi. Renkler XAML kaynagindaki Card/BtnBase/TabItemStyle
// stillerinden (RamTemizleyici.ps1 satir ~179-301) tasindi.
#include "Theme.h"

namespace Theme {

QString qssKoyuTema()
{
    return QStringLiteral(R"(
QMainWindow {
    background-color: #101114;
}
QWidget#TitleBar {
    background-color: transparent;
    border-bottom: 1px solid #1E1E23;
}
QLabel {
    color: #F4F4F5;
}
QFrame#Kart {
    background-color: #141418;
    border: 1px solid #1E1E23;
    border-radius: 10px;
    padding: 10px;
}
QDialog, QMessageBox {
    background-color: #141418;
}
QTabWidget::pane {
    background-color: transparent;
    border: none;
    border-top: 1px solid #25252B;
}
QTabBar::tab {
    background-color: transparent;
    color: #A1A1AA;
    font-weight: 500;
    padding: 8px 16px;
    border-bottom: 2px solid transparent;
}
QTabBar::tab:selected {
    background-color: #141418;
    color: #F4F4F5;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    border-bottom: 2px solid #4C5F7E;
}
QPushButton {
    color: #F4F4F5;
    background-color: #4C5F7E;
    border: none;
    border-radius: 6px;
    padding: 8px 14px;
    font-weight: 500;
}
QPushButton:hover {
    background-color: #5A6E90;
}
QPushButton:pressed {
    background-color: #3E4E67;
}
QPushButton:disabled {
    background-color: #2E2E33;
    color: #71717A;
}
QPushButton#BtnBaslikDugmesi {
    background-color: transparent;
    color: #71717A;
    border-radius: 0px;
    padding: 0px;
    font-weight: 600;
}
QPushButton#BtnBaslikDugmesi:hover {
    background-color: #1E1E23;
}
QLineEdit {
    background-color: #18181C;
    color: #F4F4F5;
    border: 1px solid #25252B;
    border-radius: 6px;
    padding: 6px 8px;
    selection-background-color: #4C5F7E;
}
QLineEdit:focus {
    border: 1px solid #4C5F7E;
}
QProgressBar {
    background-color: #1E1E22;
    border: none;
    border-radius: 4px;
    height: 6px;
    text-align: center;
    color: transparent;
}
QProgressBar::chunk {
    background-color: #4C5F7E;
    border-radius: 4px;
}
QTreeWidget, QTableWidget {
    background-color: transparent;
    border: 1px solid #1E1E23;
    border-radius: 8px;
    color: #F4F4F5;
    alternate-background-color: #18181C;
    gridline-color: #1E1E23;
    /* Secili hucrenin etrafina cizilen acik renkli odak dikdortgeni (focus
       rect) temada yamalik duruyordu - kaldir. */
    outline: 0;
}
QTreeWidget::item, QTableWidget::item {
    padding: 6px 4px;
    border: none;
}
QTreeWidget::item:selected, QTableWidget::item:selected {
    background-color: #25252B;
    color: #F4F4F5;
    border: none;
    outline: none;
}
QTreeWidget::item:selected:active, QTableWidget::item:selected:active {
    background-color: #2C2C34;
}
QTreeWidget::item:hover, QTableWidget::item:hover {
    background-color: #1B1B20;
}
QHeaderView::section {
    background-color: #141418;
    color: #A1A1AA;
    border: none;
    border-bottom: 1px solid #25252B;
    padding: 6px 8px;
    font-weight: 600;
}
QMenu {
    background-color: #141418;
    color: #F4F4F5;
    border: 1px solid #25252B;
    border-radius: 8px;
    padding: 4px;
}
QMenu::item {
    padding: 6px 12px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: #25252B;
}
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 0px;
}
QScrollBar::handle:vertical {
    background: #2E2E35;
    border-radius: 5px;
    min-height: 24px;
}
QScrollBar::handle:vertical:hover {
    background: #3A3A44;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
    background: transparent;
}
QScrollBar:horizontal {
    background: transparent;
    height: 10px;
    margin: 0px;
}
QScrollBar::handle:horizontal {
    background: #2E2E35;
    border-radius: 5px;
    min-width: 24px;
}
QScrollBar::handle:horizontal:hover {
    background: #3A3A44;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0px;
}
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
    background: transparent;
}
QTreeWidget::indicator {
    width: 16px;
    height: 16px;
    border-radius: 4px;
    border: 1px solid #4C5F7E;
    background-color: transparent;
}
QTreeWidget::indicator:checked {
    background-color: #4C5F7E;
    border: 1px solid #4C5F7E;
}
QTreeWidget::indicator:disabled {
    border: 1px solid #3A3A40;
}
)");
}

} // namespace Theme
