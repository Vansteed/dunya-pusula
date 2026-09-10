// Logo.qml - Canvas ile cizilen kucuk pusula/dunya isareti (PNG yok, qtbase
// PNG'siz derlenmis). res/DunyaPusula.ico ile uyumlu renk paleti.
import QtQuick

Canvas {
    id: pusula
    property real boyut: 22
    implicitWidth: boyut
    implicitHeight: boyut

    onBoyutChanged: requestPaint()
    onPaint: {
        var ctx = getContext("2d");
        var w = width, h = height;
        var cx = w / 2, cy = h / 2, r = Math.min(w, h) / 2;
        ctx.reset();

        // Koyu lacivert dolu daire
        ctx.beginPath();
        ctx.arc(cx, cy, r - 1, 0, Math.PI * 2);
        ctx.fillStyle = "#141E33";
        ctx.fill();

        // Ince camgobegi dis halka
        ctx.lineWidth = Math.max(1, r * 0.14);
        ctx.strokeStyle = "#38B6D8";
        ctx.beginPath();
        ctx.arc(cx, cy, r - ctx.lineWidth / 2, 0, Math.PI * 2);
        ctx.stroke();

        // Kuzey ibresi - mavi ucgen (yukari)
        var ibreUzunluk = r * 0.72;
        ctx.beginPath();
        ctx.moveTo(cx, cy - ibreUzunluk);
        ctx.lineTo(cx - r * 0.2, cy);
        ctx.lineTo(cx + r * 0.2, cy);
        ctx.closePath();
        ctx.fillStyle = "#4A9EFF";
        ctx.fill();

        // Guney ibresi - acik gri ucgen (asagi)
        ctx.beginPath();
        ctx.moveTo(cx, cy + ibreUzunluk);
        ctx.lineTo(cx - r * 0.2, cy);
        ctx.lineTo(cx + r * 0.2, cy);
        ctx.closePath();
        ctx.fillStyle = "#C6D2E8";
        ctx.fill();

        // Merkez - kucuk acik nokta
        ctx.beginPath();
        ctx.arc(cx, cy, r * 0.1, 0, Math.PI * 2);
        ctx.fillStyle = "#ECF4FF";
        ctx.fill();
    }
}
