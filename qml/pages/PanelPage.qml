// PanelPage.qml - OZET sayfasi (2x2 grid). Tum sayilar CANLI backend verisi:
// bellek / islemci / saglik / agac (depolama). Ornek gorunumdeki dagilim
// (sparkline + gosterge + liste + koyu butonlar) ayni, degerler gercek.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RamTemizleyici
import "../components"

Item {
    id: sayfa

    // Main.qml sekme indeksini degistirsin diye - StackLayout burada degil.
    signal detayIstendi(int index)

    // Tum kartlar ayni yukseklikte olsun - biri buyuyunce izgara kayiyor,
    // satirlar hizasini kaybediyordu. Icerik bu kutuya sigdiriliyor.
    readonly property int kartYuksekligi: 310

    // Kart basligi (BELLEK / ISLEMCI / ...) - sol ust kucuk gri metin.
    component KartBaslik: Text {
        font.family: Tema.aile
        font.pixelSize: Tema.puntoOrta
        font.letterSpacing: 0.6
        color: Tema.metinSilik
    }

    // Buyuk yuzde/sayi.
    // Kucuk etiket + degeri: CPU ve GPU sutunlarinda ayni bicim.
    component OlcuSatiri: ColumnLayout {
        property string etiket: ""
        property string deger: "-"
        spacing: 0
        Text {
            text: parent.etiket
            font.family: Tema.aile
            font.pixelSize: 12
            color: Tema.metinSilik
        }
        Text {
            text: parent.deger
            font.family: Tema.aile
            font.pixelSize: Tema.puntoOrta
            font.weight: Font.DemiBold
            color: Tema.metin
        }
    }

    component BuyukSayi: Text {
        font.family: Tema.aile
        font.pixelSize: 42
        font.weight: Font.DemiBold
        color: Tema.metin
    }

    // Cizgi grafik (MemoryPage/CpuPage ile ayni cizim, renk parametreli).
    component Sparkline: Canvas {
        property var veri: []
        property color cizgi: Tema.vurgu
        property color dolgu: Qt.rgba(0.36, 0.55, 0.94, 0.16)
        onVeriChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onCizgiChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const v = veri
            ctx.strokeStyle = Tema.kenarSilik
            ctx.lineWidth = 1
            for (let g = 0; g <= 3; g++) {
                const y = height - (g / 3) * height
                ctx.beginPath()
                ctx.moveTo(0, y)
                ctx.lineTo(width, y)
                ctx.stroke()
            }
            if (!v || v.length < 2) return
            const adim = width / (v.length - 1)
            ctx.beginPath()
            ctx.moveTo(0, height - (v[0] / 100) * height)
            for (let i = 1; i < v.length; i++)
                ctx.lineTo(i * adim, height - (v[i] / 100) * height)
            ctx.strokeStyle = cizgi
            ctx.lineWidth = 1.6
            ctx.stroke()
            ctx.lineTo(width, height)
            ctx.lineTo(0, height)
            ctx.closePath()
            ctx.fillStyle = dolgu
            ctx.fill()
        }
    }

    // Yarim daire gosterge (ibreli). oran 0..1.
    component YariGauge: Canvas {
        property real oran: 0
        property color renk: Tema.basari
        onOranChanged: requestPaint()
        onRenkChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const cx = width / 2
            const cy = height - 8
            const r = Math.min(width / 2 - 8, height - 16)
            if (r < 4) return
            ctx.lineCap = "round"
            ctx.lineWidth = 8
            ctx.strokeStyle = Tema.yuzeyVurgu
            ctx.beginPath()
            ctx.arc(cx, cy, r, Math.PI, 2 * Math.PI)
            ctx.stroke()
            const o = Math.max(0, Math.min(1, oran))
            if (o > 0) {
                ctx.strokeStyle = renk
                ctx.beginPath()
                ctx.arc(cx, cy, r, Math.PI, Math.PI + o * Math.PI)
                ctx.stroke()
            }
            const a = Math.PI + o * Math.PI
            ctx.strokeStyle = renk
            ctx.lineWidth = 2
            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.lineTo(cx + Math.cos(a) * (r - 5), cy + Math.sin(a) * (r - 5))
            ctx.stroke()
            ctx.fillStyle = renk
            ctx.beginPath()
            ctx.arc(cx, cy, 3, 0, 2 * Math.PI)
            ctx.fill()
        }
    }

    // Halka (donut) grafik - birden fazla dilim. dilimler: [{oran, renk}, ...]
    // Saat 12'den baslar, sirayla saat yonunde ciziyor (aralarinda bosluk yok,
    // basit tutuluyor).
    component DilimliHalka: Canvas {
        property var dilimler: []
        onDilimlerChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const cx = width / 2
            const cy = height / 2
            const rDis = Math.min(width, height) / 2
            const rIc = rDis * 0.62
            if (rDis < 4) return
            ctx.fillStyle = Tema.yuzeyVurgu
            ctx.beginPath()
            ctx.arc(cx, cy, rDis, 0, 2 * Math.PI)
            ctx.arc(cx, cy, rIc, 0, 2 * Math.PI, true)
            ctx.fill()

            let basla = -Math.PI / 2
            for (const d of dilimler) {
                const o = Math.max(0, Math.min(1, d.oran))
                if (o <= 0) continue
                const bitis = basla + o * 2 * Math.PI
                ctx.fillStyle = d.renk
                ctx.beginPath()
                ctx.arc(cx, cy, rDis, basla, bitis)
                ctx.arc(cx, cy, rIc, bitis, basla, true)
                ctx.closePath()
                ctx.fill()
                basla = bitis
            }
        }
    }

    function skorRengi(p) {
        return p >= 80 ? Tema.basari : p >= 50 ? Tema.uyari : Tema.tehlike
    }

    function gbYaz(mb) {
        return mb >= 1024 ? (mb / 1024).toFixed(1) + " GB" : mb.toFixed(0) + " MB"
    }

    // En cok RAM kullanan ilk 3 grup (CANLI - 5 sn'de bir tazelenir).
    property var enCok3: []
    Component.onCompleted: sayfa.enCok3 = islemler.enCokKullanan(3)
    Connections {
        target: islemler
        function onIsaretliDegisti() { sayfa.enCok3 = islemler.enCokKullanan(3) }
    }

    // Depolama - ilk surucu (CANLI).
    property var surucu: agac.suruculer.length > 0 ? agac.suruculer[0] : ({})
    property real depToplam: surucu.toplamGB ? surucu.toplamGB : 0
    property real depBos: surucu.bosGB ? surucu.bosGB : 0
    property real depDolu: Math.max(0, depToplam - depBos)
    property real depOran: depToplam > 0 ? depDolu / depToplam : 0

    // Kategorilere gore renk esleme - Tema'dan.
    function kategoriRengi(ad) {
        if (ad === "Kullanıcı dosyaları") return Tema.vurgu
        if (ad === "Windows") return Tema.uyari
        if (ad === "Programlar") return Tema.basari
        if (ad === "Uygulamalar ve oyunlar") return Tema.vurguKoyu
        return Tema.metinSilik // "Sistem dosyaları"
    }

    // Kategori kirilimi + "Bos" dilimi - toplam kapasiteye gore oranlanir.
    // Olculen toplam gercek dolu alandan (sabit baglar/sikistirma yuzunden)
    // biraz farkli cikabiliyor; kategori oranlarinin toplami 1'i asarsa
    // orantili kucultup tasmayi onluyoruz.
    property var depDilimler: {
        const kirilim = agac.depolamaKirilimi
        if (!kirilim || kirilim.length === 0 || sayfa.depToplam <= 0) return []
        let dilimler = []
        let kategoriToplamOran = 0
        for (const k of kirilim) {
            const oran = (k.bayt / 1073741824) / sayfa.depToplam
            dilimler.push({ oran: oran, renk: sayfa.kategoriRengi(k.ad) })
            kategoriToplamOran += oran
        }
        const bosOran = sayfa.depBos / sayfa.depToplam
        const kalan = 1 - bosOran
        if (kategoriToplamOran > kalan && kategoriToplamOran > 0) {
            const olcek = kalan / kategoriToplamOran
            for (const d of dilimler) d.oran *= olcek
        }
        dilimler.push({ oran: bosOran, renk: Tema.yuzeyVurgu })
        return dilimler
    }

    // Kritik / tavsiye listeleri - GERCEK puanlardan turetilir (sabit metin yok).
    // Olcek NOTU: kirilimlar 100'luk degil - RAM max 33, disk max 33, disk
    // sagligi max 34 (bkz. HealthScore.cpp). Esikler buna gore.
    // Kritik = simdi mudahale gerektiren durum. Puan tablosu:
    // RAM 33/24/13/5/0  -> 0 demek kullanim %95 ustu
    // Disk 33/24/13/5/0 -> 0 demek bos alan %5'in altinda
    // Disk sagligi 34 (iyi) / 24 (veri yok) / 20 (asinma basladi) / 7 (kotu)
    property var kritikler: {
        var l = []
        if (saglik.diskSaglikPuan <= 7) l.push("Disk sağlığı bozuluyor")
        if (saglik.ramPuan === 0) l.push("Bellek doldu")
        if (saglik.diskPuan === 0) l.push("Disk alanı bitmek üzere")
        return l
    }
    // Tavsiye = acil degil, gozden gecirilebilir. Normal kullanimda (RAM %85
    // alti, diskte %15 ustu bos) hicbir sey yazmaz.
    property var tavsiyeler: {
        var l = []
        if (saglik.ramPuan > 0 && saglik.ramPuan <= 13) l.push("Bellek kullanımı yüksek")
        if (saglik.diskPuan > 0 && saglik.diskPuan <= 13) l.push("Diskte yer azalıyor")
        if (saglik.diskSaglikPuan === 20) l.push("Disk aşınması izlenmeli")
        if (!saglik.yonetici) l.push("Yönetici olarak çalıştır")
        return l
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: Tema.bosluk
        clip: true
        contentWidth: availableWidth

        GridLayout {
            width: parent.width
            columns: width < 700 ? 1 : 2
            columnSpacing: Tema.bosluk
            rowSpacing: Tema.bosluk

            // ---------- 1. KART: BELLEK ----------
            Card {
                Layout.fillWidth: true
                Layout.preferredHeight: sayfa.kartYuksekligi

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        KartBaslik { text: "BELLEK" }
                        Item { Layout.fillWidth: true }
                        KartBaslik { text: "Kullanım 5 dakika" }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Tema.bosluk

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 170
                            spacing: 3
                            BuyukSayi {
                                text: bellek.yuzde.toFixed(0) + "%"
                                color: bellek.yuzde >= 85 ? Tema.tehlike : bellek.yuzde >= 65 ? Tema.uyari : Tema.basari
                            }
                            Text {
                                Layout.fillWidth: true
                                text: bellek.kullanilanGB.toFixed(1) + " / " + bellek.toplamGB.toFixed(1) + " GB kullanılıyor"
                                elide: Text.ElideRight
                                font.family: Tema.aile
                                font.pixelSize: 14
                                color: Tema.metinIkincil
                            }
                        }

                        Sparkline {
                            Layout.fillWidth: true
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 68
                            veri: bellek.gecmis
                            cizgi: Tema.vurgu
                        }
                        YariGauge {
                            Layout.preferredWidth: 92
                            Layout.preferredHeight: 64
                            oran: bellek.yuzde / 100
                            renk: Tema.basari
                        }
                    }

                    KartBaslik { text: "En Çok Kullanan Uygulamalar" }

                    Column {
                        Layout.fillWidth: true
                        spacing: 5
                        Repeater {
                            model: sayfa.enCok3
                            delegate: RowLayout {
                                required property var modelData
                                required property int index
                                width: parent.width
                                spacing: 8
                                Image {
                                    id: simge
                                    Layout.preferredWidth: 18
                                    Layout.preferredHeight: 18
                                    visible: modelData.simge !== undefined && modelData.simge !== ""
                                    source: visible ? modelData.simge : ""
                                    sourceSize.width: 18
                                    sourceSize.height: 18
                                    smooth: true
                                    fillMode: Image.PreserveAspectFit
                                }
                                Rectangle {
                                    Layout.preferredWidth: 8
                                    Layout.preferredHeight: 8
                                    radius: 4
                                    visible: !simge.visible
                                    color: ["#4ED4A0", "#5B8DEF", "#F2C05B"][index % 3]
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.ad
                                    elide: Text.ElideRight
                                    font.family: Tema.aile
                                    font.pixelSize: 14
                                    color: Tema.metin
                                }
                                Text {
                                    text: sayfa.gbYaz(modelData.mb)
                                    font.family: Tema.aile
                                    font.pixelSize: 14
                                    color: Tema.metinIkincil
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Item { Layout.fillWidth: true }
                        GhostButton {
                            text: "Detaylar"
                            implicitHeight: 28
                            onClicked: sayfa.detayIstendi(1)
                        }
                    }
                }
            }

            // ---------- 2. KART: ISLEMCI ----------
            Card {
                Layout.fillWidth: true
                Layout.preferredHeight: sayfa.kartYuksekligi

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    KartBaslik { text: "İŞLEMCİ VE EKRAN KARTI" }

                    // CPU | GPU - iki sutun, aralarinda dikey ayirici. Ayni
                    // kartta olduklari icin hangi rakamin kime ait oldugu
                    // renk seridi ve baslikla ayirt ediliyor.
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: Tema.bosluk

                        // ----- CPU -----
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 7
                                Rectangle {
                                    Layout.preferredWidth: 3
                                    Layout.preferredHeight: 15
                                    radius: 1.5
                                    color: Tema.basari
                                }
                                Text {
                                    text: "CPU"
                                    font.family: Tema.aile
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    font.letterSpacing: 1
                                    color: Tema.basari
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: islemci.mantiksalCekirdek + " çekirdek"
                                    horizontalAlignment: Text.AlignRight
                                    elide: Text.ElideRight
                                    font.family: Tema.aile
                                    font.pixelSize: 13
                                    color: Tema.metinSilik
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: islemci.ad
                                elide: Text.ElideRight
                                font.family: Tema.aile
                                font.pixelSize: 13
                                color: Tema.metinSilik
                            }

                            BuyukSayi {
                                text: islemci.kullanim.toFixed(0) + "%"
                                color: islemci.kullanim >= 85 ? Tema.tehlike
                                     : islemci.kullanim >= 60 ? Tema.uyari : Tema.basari
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 14
                                OlcuSatiri { etiket: "Saat"; deger: islemci.mhz + " MHz" }
                                OlcuSatiri {
                                    etiket: "Sıcaklık"
                                    deger: islemci.sicaklik === "-"
                                           ? "-"
                                           : parseFloat(islemci.sicaklik).toFixed(0) + " °C"
                                }
                                Item { Layout.fillWidth: true }
                            }

                            Sparkline {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 48
                                veri: islemci.gecmis
                                cizgi: Tema.basari
                                dolgu: Qt.rgba(0.31, 0.83, 0.63, 0.14)
                            }
                        }

                        // Dikey ayirici
                        Rectangle {
                            Layout.preferredWidth: 1
                            Layout.fillHeight: true
                            Layout.topMargin: 2
                            Layout.bottomMargin: 2
                            color: Tema.yuzeyVurgu
                        }

                        // ----- GPU -----
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 7
                                Rectangle {
                                    Layout.preferredWidth: 3
                                    Layout.preferredHeight: 15
                                    radius: 1.5
                                    color: Tema.uyari
                                }
                                Text {
                                    text: "GPU"
                                    font.family: Tema.aile
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    font.letterSpacing: 1
                                    color: Tema.uyari
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: islemci.sensorAjaniVar ? "" : "sensör yok"
                                    horizontalAlignment: Text.AlignRight
                                    elide: Text.ElideRight
                                    font.family: Tema.aile
                                    font.pixelSize: 13
                                    color: Tema.metinSilik
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: islemci.gpular.length > 0 ? islemci.gpular[0].ad : "-"
                                elide: Text.ElideRight
                                font.family: Tema.aile
                                font.pixelSize: 13
                                color: Tema.metinSilik
                            }

                            BuyukSayi {
                                text: islemci.gpuKullanim.toFixed(0) + "%"
                                color: islemci.gpuKullanim >= 85 ? Tema.tehlike
                                     : islemci.gpuKullanim >= 60 ? Tema.uyari : Tema.vurgu
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 14
                                OlcuSatiri { etiket: "Saat"; deger: islemci.gpuCekirdekMhz }
                                OlcuSatiri { etiket: "Sıcaklık"; deger: islemci.gpuSicaklik }
                                Item { Layout.fillWidth: true }
                            }

                            Sparkline {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: 48
                                veri: islemci.gpuGecmis
                                cizgi: Tema.uyari
                                dolgu: Qt.rgba(0.95, 0.75, 0.36, 0.14)
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Item { Layout.fillWidth: true }
                        GhostButton {
                            text: "Detay"
                            implicitHeight: 28
                            onClicked: sayfa.detayIstendi(2)
                        }
                    }
                }
            }

            // ---------- 3. KART: SAGLIK ----------
            Card {
                Layout.fillWidth: true
                Layout.preferredHeight: sayfa.kartYuksekligi

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    KartBaslik { text: "SAĞLIK" }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Tema.bosluk

                        Kadran {
                            Layout.preferredWidth: 120
                            Layout.preferredHeight: 120
                            deger: saglik.puan
                            enBuyuk: 100
                            metin: String(saglik.puan)
                            baslik: ""
                            renk: skorRengi(saglik.puan)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 3
                            Text {
                                text: saglik.puan
                                font.family: Tema.aile
                                font.pixelSize: 32
                                font.weight: Font.DemiBold
                                color: skorRengi(saglik.puan)
                            }
                            Text {
                                Layout.fillWidth: true
                                text: saglik.yorum
                                wrapMode: Text.Wrap
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoOrta
                                color: Tema.metinIkincil
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: Tema.bosluk

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Kritik Uyarılar (" + sayfa.kritikler.length + ")"
                                font.family: Tema.aile
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                color: Tema.metin
                            }
                            Repeater {
                                model: sayfa.kritikler
                                delegate: Text {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    text: modelData
                                    elide: Text.ElideRight
                                    font.family: Tema.aile
                                    font.pixelSize: 14
                                    color: Tema.tehlike
                                }
                            }
                            Text {
                                visible: sayfa.kritikler.length === 0
                                text: "Temiz"
                                font.family: Tema.aile
                                font.pixelSize: Tema.punto
                                color: Tema.metinSilik
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                text: "Tavsiyeler (" + sayfa.tavsiyeler.length + ")"
                                font.family: Tema.aile
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                                color: Tema.metin
                            }
                            Repeater {
                                model: sayfa.tavsiyeler
                                delegate: Text {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    text: modelData
                                    elide: Text.ElideRight
                                    font.family: Tema.aile
                                    font.pixelSize: 14
                                    color: Tema.metinIkincil
                                }
                            }
                            Text {
                                visible: sayfa.tavsiyeler.length === 0
                                text: "Yok"
                                font.family: Tema.aile
                                font.pixelSize: Tema.punto
                                color: Tema.metinSilik
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }
                        GhostButton {
                            text: "Detaylar"
                            implicitHeight: 28
                            onClicked: sayfa.detayIstendi(3)
                        }
                    }
                }
            }

            // ---------- 4. KART: DEPOLAMA ----------
            Card {
                Layout.fillWidth: true
                Layout.preferredHeight: sayfa.kartYuksekligi

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        KartBaslik { text: "DEPOLAMA" }
                        Item { Layout.fillWidth: true }
                        KartBaslik {
                            text: sayfa.surucu.yol
                                  ? (sayfa.surucu.yol
                                     + (sayfa.surucu.dosyaSistemi ? " · " + sayfa.surucu.dosyaSistemi : ""))
                                  : ""
                        }
                    }

                    // Halka zaten ayni orani gosteriyor; yatay cubuk ucuncu
                    // tekrar oluyordu, kaldirildi.
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        BuyukSayi {
                            text: sayfa.depToplam > 0 ? (sayfa.depBos.toFixed(0) + " GB boş") : "-"
                            color: sayfa.depOran >= 0.95 ? Tema.tehlike
                                 : sayfa.depOran >= 0.85 ? Tema.uyari : Tema.metin
                        }
                        Text {
                            Layout.fillWidth: true
                            text: sayfa.depToplam > 0
                                  ? (sayfa.depToplam.toFixed(0) + " GB toplam · %"
                                     + (sayfa.depOran * 100).toFixed(0) + " dolu")
                                  : ""
                            elide: Text.ElideRight
                            font.family: Tema.aile
                            font.pixelSize: 14
                            color: Tema.metinIkincil
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Tema.bosluk

                        // Tek halka iki durumu da karsiliyor: tarama varsa
                        // kategori dilimleri, yoksa sadece dolu/bos.
                        DilimliHalka {
                            Layout.preferredWidth: 88
                            Layout.preferredHeight: 88
                            dilimler: agac.depolamaKirilimi.length > 0
                                ? sayfa.depDilimler
                                : [{ oran: sayfa.depOran,
                                     renk: sayfa.depOran >= 0.95 ? Tema.tehlike
                                         : sayfa.depOran >= 0.85 ? Tema.uyari : Tema.vurgu },
                                   { oran: 1 - sayfa.depOran, renk: Tema.yuzeyVurgu }]
                        }

                        // Tarama yapilmadiysa eski Dolu/Bos gorunumu.
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            visible: agac.depolamaKirilimi.length === 0
                            RowLayout {
                                spacing: 6
                                Rectangle {
                                    Layout.preferredWidth: 8
                                    Layout.preferredHeight: 8
                                    radius: 4
                                    color: sayfa.depOran >= 0.95 ? Tema.tehlike
                                         : sayfa.depOran >= 0.85 ? Tema.uyari : Tema.vurgu
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: "Dolu: " + sayfa.depDolu.toFixed(0) + " GB"
                                    elide: Text.ElideRight
                                    font.family: Tema.aile
                                    font.pixelSize: 14
                                    color: Tema.metinIkincil
                                }
                            }
                            RowLayout {
                                spacing: 6
                                Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: Tema.basari }
                                Text {
                                    Layout.fillWidth: true
                                    text: "Boş: " + sayfa.depBos.toFixed(0) + " GB"
                                    elide: Text.ElideRight
                                    font.family: Tema.aile
                                    font.pixelSize: 14
                                    color: Tema.metinIkincil
                                }
                            }
                        }

                        // Tarama yapildiysa kategori efsanesi.
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            visible: agac.depolamaKirilimi.length > 0
                            Repeater {
                                model: agac.depolamaKirilimi
                                delegate: RowLayout {
                                    id: efsaneSatir
                                    required property var modelData
                                    Layout.fillWidth: true
                                    spacing: 6

                                    Rectangle {
                                        Layout.preferredWidth: 8
                                        Layout.preferredHeight: 8
                                        radius: 4
                                        color: sayfa.kategoriRengi(modelData.ad)
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.ad + ": " + modelData.boyutMetni
                                        elide: Text.ElideRight
                                        font.family: Tema.aile
                                        font.pixelSize: 13
                                        color: fareAlani.containsMouse ? Tema.metin : Tema.metinIkincil

                                        // Ipucu Popup'i Layout icine gomulunce acilmiyordu;
                                        // eklenti (attached) bicimi metnin uzerindeki
                                        // MouseArea'dan calisiyor.
                                        MouseArea {
                                            id: fareAlani
                                            anchors.fill: parent
                                            hoverEnabled: true
                                        }
                                        Ipucu {
                                            text: efsaneSatir.modelData.ayrinti
                                                  ? efsaneSatir.modelData.ayrinti : ""
                                            visible: fareAlani.containsMouse && text !== ""
                                        }
                                    }
                                }
                            }
                            RowLayout {
                                spacing: 6
                                Rectangle { Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4; color: Tema.yuzeyVurgu }
                                Text {
                                    Layout.fillWidth: true
                                    text: "Boş: " + sayfa.depBos.toFixed(0) + " GB"
                                    elide: Text.ElideRight
                                    font.family: Tema.aile
                                    font.pixelSize: 13
                                    color: Tema.metinIkincil
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        Text {
                            Layout.fillWidth: true
                            text: agac.sonucVar ? agac.taramaOzeti : "Henüz tarama yapılmadı"
                            elide: Text.ElideRight
                            font.family: Tema.aile
                            font.pixelSize: 14
                            color: Tema.metinSilik
                        }
                        GhostButton {
                            text: "Dosya Temizleme"
                            implicitHeight: 28
                            onClicked: sayfa.detayIstendi(4)
                        }
                    }
                }
            }
        }
    }
}
