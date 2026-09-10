// Card.qml - yuvarlak koseli, kenarli yuzey. Icerik default property ile
// dogrudan icine yazilir.
import QtQuick
import RamTemizleyici

Rectangle {
    id: kart
    default property alias icerik: govde.data
    property int icBosluk: Tema.bosluk

    color: Tema.yuzey
    border.color: Tema.kenarSilik
    border.width: 1
    radius: Tema.koseKart

    implicitHeight: govde.implicitHeight + icBosluk * 2
    implicitWidth: govde.implicitWidth + icBosluk * 2

    Item {
        id: govde
        anchors.fill: parent
        anchors.margins: kart.icBosluk
    }
}
