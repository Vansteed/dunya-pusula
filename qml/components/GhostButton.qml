// GhostButton.qml - ikincil, saydam zeminli dugme.
import QtQuick
import QtQuick.Controls.Basic
import RamTemizleyici

Button {
    id: dugme
    property bool secili: false
    implicitHeight: 32

    contentItem: Text {
        text: dugme.text
        font.family: Tema.aile
        font.pixelSize: Tema.punto
        color: dugme.secili ? Tema.metin : Tema.metinIkincil
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: Tema.koseDugme
        color: dugme.pressed ? Tema.yuzeyVurgu
             : dugme.hovered ? Tema.yuzeyYuksek
             : "transparent"
        border.width: 1
        border.color: dugme.secili ? Tema.vurgu : Tema.kenar

        Behavior on color {
            ColorAnimation { duration: 120; easing.type: Easing.OutQuad }
        }
    }
}
