// Main.qml - cercevesiz ana pencere, ozel baslik cubugu, 5 sekme.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RamTemizleyici
import "components"
import "pages"

Window {
    id: pencere
    visible: true
    width: 1060
    height: 780
    minimumWidth: 900
    minimumHeight: 620
    color: Tema.zemin
    title: "Dünya Pusula"
    flags: Qt.Window | Qt.FramelessWindowHint

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TitleBar {
            Layout.fillWidth: true
            baslik: "Dünya Pusula"
            yonetici: saglik.yonetici
            onKucultIstendi: pencere.showMinimized()
            onKapatIstendi: pencere.close()
        }

        // Sekme seridi
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.topMargin: 6
            spacing: 2

            Repeater {
                model: ["PANEL", "BELLEK", "İŞLEMCİ", "SAĞLIK", "BÜYÜK DOSYALAR"]
                delegate: Item {
                    required property int index
                    required property string modelData
                    implicitWidth: sekmeMetin.implicitWidth + 28
                    implicitHeight: 38

                    Text {
                        id: sekmeMetin
                        anchors.centerIn: parent
                        text: modelData
                        font.family: Tema.aile
                        font.pixelSize: Tema.punto
                        font.weight: Font.DemiBold
                        font.letterSpacing: 0.4
                        color: yigin.currentIndex === index ? Tema.metin : Tema.metinSilik
                        Behavior on color { ColorAnimation { duration: 130 } }
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: yigin.currentIndex === index ? parent.width - 16 : 0
                        height: 2
                        radius: 1
                        color: Tema.vurgu
                        Behavior on width {
                            NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: yigin.currentIndex = index
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Tema.kenarSilik
        }

        StackLayout {
            id: yigin
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: 0

            PanelPage {
                onDetayIstendi: (index) => yigin.currentIndex = index
            }
            MemoryPage {}
            CpuPage {}
            HealthPage {}
            FolderTreePage {}
        }
    }
}
