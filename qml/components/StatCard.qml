// StatCard.qml - buyuk rakam + baslik + istege bagli alt satir.
import QtQuick
import QtQuick.Layouts
import RamTemizleyici

Card {
    id: kart
    property string baslik: ""
    property string deger: "-"
    property string birim: ""
    property string altBilgi: ""
    property color degerRengi: Tema.metin

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: kart.icBosluk
        spacing: Tema.boslukKucuk

        Text {
            text: kart.baslik
            font.family: Tema.aile
            font.pixelSize: Tema.puntoKucuk
            font.letterSpacing: 0.6
            color: Tema.metinSilik
        }

        RowLayout {
            spacing: 4
            Text {
                text: kart.deger
                font.family: Tema.aile
                font.pixelSize: Tema.puntoDev
                font.weight: Font.DemiBold
                color: kart.degerRengi
            }
            Text {
                text: kart.birim
                font.family: Tema.aile
                font.pixelSize: Tema.puntoOrta
                color: Tema.metinSilik
                Layout.alignment: Qt.AlignBottom
                Layout.bottomMargin: 6
                visible: kart.birim.length > 0
            }
            Item { Layout.fillWidth: true }
        }

        Text {
            text: kart.altBilgi
            font.family: Tema.aile
            font.pixelSize: Tema.puntoKucuk
            color: Tema.metinIkincil
            visible: kart.altBilgi.length > 0
            Layout.fillWidth: true
            elide: Text.ElideRight
        }
    }
}
