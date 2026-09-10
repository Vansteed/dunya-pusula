// Ipucu.qml - tema ile uyumlu ToolTip. Varsayilan Basic stili beyaz zemin /
// siyah cerceve ciziyor, koyu arayuzde yamalak duruyordu.
import QtQuick
import QtQuick.Controls.Basic
import RamTemizleyici

ToolTip {
    id: ipucu

    delay: 300
    timeout: 8000
    padding: 10

    contentItem: Text {
        text: ipucu.text
        wrapMode: Text.WordWrap
        // Cok uzun satirlarda ipucu ekrani kaplamasin.
        width: Math.min(implicitWidth, 260)
        lineHeight: 1.3
        font.family: Tema.aile
        font.pixelSize: Tema.puntoKucuk
        color: Tema.metinIkincil
    }

    background: Rectangle {
        color: Tema.yuzeyYuksek
        border.color: Tema.kenar
        border.width: 1
        radius: Tema.koseKucuk
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 110 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 90 }
    }
}
