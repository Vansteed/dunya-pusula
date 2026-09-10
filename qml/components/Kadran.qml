// Kadran.qml - 240 derecelik dairesel gosterge (frekans/sicaklik icin).
import QtQuick
import RamTemizleyici

Item {
    id: kadran
    property string baslik: ""
    property real deger: 0
    property real enBuyuk: 1
    property string metin: ""
    property color renk: Tema.vurgu
    property string altNot: ""

    implicitWidth: 140
    implicitHeight: 140

    onDegerChanged: govde.requestPaint()
    onEnBuyukChanged: govde.requestPaint()
    onRenkChanged: govde.requestPaint()
    onWidthChanged: govde.requestPaint()
    onHeightChanged: govde.requestPaint()

    Canvas {
        id: govde
        anchors.fill: parent

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()

            const merkezX = width / 2
            const merkezY = height / 2
            const yaricap = Math.min(width, height) / 2 - 10
            const kalinlik = 8
            const baslangicAci = 135 * Math.PI / 180
            const bitisAci = baslangicAci + 240 * Math.PI / 180

            ctx.lineWidth = kalinlik
            ctx.lineCap = "round"

            ctx.strokeStyle = Tema.yuzeyVurgu
            ctx.beginPath()
            ctx.arc(merkezX, merkezY, yaricap, baslangicAci, bitisAci)
            ctx.stroke()

            const oran = kadran.enBuyuk > 0 ? Math.max(0, Math.min(1, kadran.deger / kadran.enBuyuk)) : 0
            if (oran > 0) {
                ctx.strokeStyle = kadran.renk
                ctx.beginPath()
                ctx.arc(merkezX, merkezY, yaricap, baslangicAci, baslangicAci + oran * (bitisAci - baslangicAci))
                ctx.stroke()
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 2

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: kadran.metin
            font.family: Tema.aile
            font.pixelSize: Tema.puntoBuyuk
            font.weight: Font.DemiBold
            color: Tema.metin
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: kadran.baslik
            font.family: Tema.aile
            font.pixelSize: Tema.puntoKucuk
            color: Tema.metinSilik
        }
    }

    Text {
        anchors.top: parent.bottom
        anchors.topMargin: 2
        anchors.horizontalCenter: parent.horizontalCenter
        visible: kadran.altNot.length > 0
        text: kadran.altNot
        font.family: Tema.aile
        font.pixelSize: Tema.puntoKucuk
        color: Tema.metinSilik
    }
}
