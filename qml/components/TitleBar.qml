// TitleBar.qml - cercevesiz pencerenin ozel baslik cubugu. Surukleme
// DragHandler ile (WPF DragMove / Widgets mousePressEvent karsiligi).
import QtQuick
import QtQuick.Layouts
import RamTemizleyici

Rectangle {
    id: cubuk
    property string baslik: ""
    property bool yonetici: false
    signal kucultIstendi()
    signal kapatIstendi()

    height: Tema.baslikYukseklik
    color: "transparent"

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Tema.kenarSilik
    }

    DragHandler {
        target: null
        grabPermissions: TapHandler.CanTakeOverFromAnything
        onActiveChanged: if (active) pencere.startSystemMove()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 6
        spacing: 10

        Logo {
            boyut: 20
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            text: cubuk.baslik
            font.family: Tema.aile
            font.pixelSize: Tema.puntoOrta
            font.weight: Font.DemiBold
            color: Tema.metin
        }

        Rectangle {
            visible: true
            radius: Tema.koseKucuk
            color: cubuk.yonetici ? Qt.rgba(0.31, 0.83, 0.63, 0.14)
                                   : Tema.yuzeyYuksek
            implicitWidth: rozetMetin.implicitWidth + 16
            implicitHeight: 22
            Text {
                id: rozetMetin
                anchors.centerIn: parent
                text: cubuk.yonetici ? "Yönetici" : "Normal"
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
                color: cubuk.yonetici ? Tema.basari : Tema.metinSilik
            }
        }

        Item { Layout.fillWidth: true }

        Repeater {
            model: [
                { simge: "–", eylem: "kucult" },
                { simge: "✕", eylem: "kapat" }
            ]
            delegate: Rectangle {
                required property var modelData
                width: 40
                height: 28
                radius: Tema.koseKucuk
                color: fare.containsMouse
                       ? (modelData.eylem === "kapat" ? Qt.rgba(0.95, 0.41, 0.36, 0.18)
                                                       : Tema.yuzeyYuksek)
                       : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: modelData.simge
                    font.family: Tema.aile
                    font.pixelSize: Tema.punto
                    color: fare.containsMouse && modelData.eylem === "kapat"
                           ? Tema.tehlike : Tema.metinIkincil
                }

                MouseArea {
                    id: fare
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: modelData.eylem === "kapat" ? cubuk.kapatIstendi()
                                                            : cubuk.kucultIstendi()
                }

                Behavior on color {
                    ColorAnimation { duration: 110 }
                }
            }
        }
    }
}
