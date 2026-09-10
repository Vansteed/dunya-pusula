// PrimaryButton.qml - vurgu renkli ana eylem dugmesi.
import QtQuick
import QtQuick.Controls.Basic
import RamTemizleyici

Button {
    id: dugme
    implicitHeight: 38

    contentItem: Text {
        text: dugme.text
        font.family: Tema.aile
        font.pixelSize: Tema.puntoOrta
        font.weight: Font.DemiBold
        color: dugme.enabled ? Tema.metin : Tema.metinSilik
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Tema.koseDugme
        color: !dugme.enabled ? Tema.yuzeyYuksek
             : dugme.pressed ? Tema.vurguKoyu
             : dugme.hovered ? Qt.lighter(Tema.vurgu, 1.12)
             : Tema.vurgu

        Behavior on color {
            ColorAnimation { duration: 120; easing.type: Easing.OutQuad }
        }
    }
}
