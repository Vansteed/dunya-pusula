// TikKutusu.qml - kendi cizdigimiz onay kutusu.
// Basic stilin CheckBox'i tik icin PNG kullaniyor; qtbase PNG destegi olmadan
// derlendigi icin o gorsel cozulemiyor. Tik burada Canvas ile ciziliyor.
import QtQuick
import RamTemizleyici

Item {
    id: kutu
    property bool checked: false
    signal toggled()

    implicitWidth: 18
    implicitHeight: 18

    Rectangle {
        anchors.fill: parent
        radius: Tema.koseKucuk - 2
        color: kutu.checked ? Tema.vurgu : "transparent"
        border.color: kutu.checked ? Tema.vurgu : Tema.kenar
        border.width: 1.5
        Behavior on color { ColorAnimation { duration: 110 } }

        Canvas {
            anchors.fill: parent
            visible: kutu.checked
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = Tema.zemin
                ctx.lineWidth = 2
                ctx.lineCap = "round"
                ctx.lineJoin = "round"
                ctx.beginPath()
                ctx.moveTo(width * 0.26, height * 0.52)
                ctx.lineTo(width * 0.44, height * 0.70)
                ctx.lineTo(width * 0.76, height * 0.32)
                ctx.stroke()
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: kutu.toggled()
    }
}
