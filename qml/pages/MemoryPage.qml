// MemoryPage.qml - RAM karti, gecmis grafigi, temizleme, islem agaci, gunluk.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RamTemizleyici
import "../components"

Item {
    id: sayfa

    property int aktifSutun: -1      // -1 = siralanmadi, 0 = ad, 1 = bellek
    property bool azalanSirali: true
    property var enCokTriolar: []
    property var gunlukTersSirali: []

    function sutunaGoreSirala(sutun) {
        if (sayfa.aktifSutun === sutun) sayfa.azalanSirali = !sayfa.azalanSirali
        else { sayfa.aktifSutun = sutun; sayfa.azalanSirali = true }
        islemler.sirala(sutun, sayfa.azalanSirali)
    }

    function oncelikMetni(oncelik) {
        switch (oncelik) {
        case 0: return "Düşük"
        case 1: return "Normalin altı"
        case 2: return "Normal"
        case 3: return "Normalin üstü"
        case 4: return "Yüksek"
        default: return "-"
        }
    }

    Component.onCompleted: {
        sayfa.enCokTriolar = islemler.enCokKullanan(3)
        sayfa.gunlukTersSirali = bellek.gunlukler.slice().reverse()
    }

    Connections {
        target: islemler
        function onGunluk(satir) { bellek.gunlukEkle(satir) }
        function onIsaretliDegisti() { sayfa.enCokTriolar = islemler.enCokKullanan(3) }
    }

    Connections {
        target: bellek
        function onGunlukDegisti() {
            sayfa.gunlukTersSirali = bellek.gunlukler.slice().reverse()
            gunlukListesi.positionViewAtBeginning()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Tema.bosluk
        spacing: Tema.bosluk

        RowLayout {
            Layout.fillWidth: true
            // Ic ice Layout'ta fillHeight varsayilani TRUE - kapatilmazsa
            // ust sira asagiyi eziyor. Ust sira alcak, liste genis kalsin.
            Layout.fillHeight: false
            Layout.preferredHeight: 150
            spacing: Tema.bosluk

            StatCard {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                baslik: "RAM KULLANIMI"
                deger: bellek.yuzde.toFixed(0)
                birim: "%"
                altBilgi: bellek.kullanilanGB.toFixed(1) + " / " + bellek.toplamGB.toFixed(1) + " GB - boş " + bellek.bosGB.toFixed(1) + " GB"
                degerRengi: bellek.yuzde >= 85 ? Tema.tehlike : bellek.yuzde >= 65 ? Tema.uyari : Tema.basari

                // Son temizlikte kazanilan RAM - kart tabaninda kucuk yesil metin.
                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: Tema.bosluk
                    visible: bellek.sonKazanc.length > 0
                    text: bellek.sonKazanc
                    elide: Text.ElideRight
                    font.family: Tema.aile
                    font.pixelSize: Tema.puntoKucuk
                    color: Tema.basari
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Canvas {
                    id: grafik
                    anchors.fill: parent
                    property var veri: bellek.gecmis
                    onVeriChanged: requestPaint()
                    onWidthChanged: requestPaint()
                    onHeightChanged: requestPaint()

                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.reset()
                        const veri = grafik.veri
                        ctx.strokeStyle = Tema.kenarSilik
                        ctx.lineWidth = 1
                        for (let g = 0; g <= 4; g++) {
                            const y = height - (g / 4) * height
                            ctx.beginPath()
                            ctx.moveTo(0, y)
                            ctx.lineTo(width, y)
                            ctx.stroke()
                        }
                        if (!veri || veri.length < 2) return
                        const adim = width / (veri.length - 1)
                        ctx.beginPath()
                        ctx.moveTo(0, height - (veri[0] / 100) * height)
                        for (let i = 1; i < veri.length; i++)
                            ctx.lineTo(i * adim, height - (veri[i] / 100) * height)
                        ctx.strokeStyle = Tema.vurgu
                        ctx.lineWidth = 2
                        ctx.stroke()
                        ctx.lineTo(width, height)
                        ctx.lineTo(0, height)
                        ctx.closePath()
                        ctx.fillStyle = Qt.rgba(0.36, 0.55, 0.94, 0.16)
                        ctx.fill()
                    }
                }
            }
        }

        // EN COK TUKETEN UCLU - kucuk cipler, tiklaninca o ada gore filtrele.
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.preferredHeight: 26
            spacing: Tema.boslukKucuk
            visible: sayfa.enCokTriolar.length > 0

            Text {
                text: "EN ÇOK TÜKETEN:"
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
                color: Tema.metinSilik
            }

            Repeater {
                model: sayfa.enCokTriolar
                delegate: Rectangle {
                    id: cip
                    required property var modelData
                    radius: Tema.koseKucuk
                    color: cipFare.containsMouse ? Tema.yuzeyVurgu : Tema.yuzeyYuksek
                    border.color: Tema.kenar
                    border.width: 1
                    implicitWidth: cipIcerik.implicitWidth + 20
                    implicitHeight: 24

                    RowLayout {
                        id: cipIcerik
                        anchors.centerIn: parent
                        spacing: 6
                        Image {
                            visible: cip.modelData.simge !== undefined && cip.modelData.simge !== ""
                            source: visible ? cip.modelData.simge : ""
                            Layout.preferredWidth: 14
                            Layout.preferredHeight: 14
                            sourceSize.width: 14
                            sourceSize.height: 14
                            smooth: true
                            fillMode: Image.PreserveAspectFit
                        }
                        Text {
                            text: cip.modelData.ad
                            elide: Text.ElideRight
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            color: Tema.metin
                        }
                        Text {
                            text: cip.modelData.mb + " MB"
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            color: Tema.metinIkincil
                        }
                    }

                    MouseArea {
                        id: cipFare
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            filtreAlani.text = cip.modelData.ad
                            islemler.filtre = cip.modelData.ad
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                spacing: Tema.boslukKucuk

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tema.boslukKucuk

                    TextField {
                        id: filtreAlani
                        Layout.fillWidth: true
                        placeholderText: "İşlem adına göre filtrele..."
                        text: islemler.filtre
                        onTextEdited: islemler.filtre = text
                        color: Tema.metin
                        font.family: Tema.aile
                        font.pixelSize: Tema.punto
                        background: Rectangle {
                            radius: Tema.koseKucuk
                            color: Tema.yuzeyYuksek
                            border.color: Tema.kenar
                            border.width: 1
                        }
                    }

                    GhostButton {
                        text: "Yenile"
                        onClicked: islemler.yenile()
                    }
                }

                // SIMDI TEMIZLE: once isaretli uygulamalari kapatir, sonra
                // RAM bosaltma adimlarini calistirir.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Tema.boslukKucuk

                    PrimaryButton {
                        text: "SEÇİLENLERİ KAPAT (" + islemler.isaretliSayisi + ")"
                        enabled: islemler.isaretliSayisi > 0
                        onClicked: {
                            const sayi = islemler.isaretliSayisi
                            bellek.kapatmaBasla()
                            islemler.secilenleriKapat()
                            bellek.kapatmaBitti(sayi)
                            islemler.yenile()
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        text: islemler.isaretliSayisi > 0
                              ? (islemler.isaretliSayisi + " uygulama kapatılacak")
                              : "Kapatmak istediğin uygulamaları işaretle"
                        font.family: Tema.aile
                        font.pixelSize: Tema.punto
                        color: Tema.metinIkincil
                    }
                }

                // Sutun basliklari - Ad ve Bellek tiklanabilir, aktif sutunda ok.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    spacing: Tema.boslukKucuk

                    Item { Layout.preferredWidth: Tema.boslukKucuk }
                    Item { Layout.preferredWidth: 14 }  // agac oku hizasi
                    Item { Layout.preferredWidth: 18 }  // tik kutusu hizasi

                    Text {
                        text: "Ad" + (sayfa.aktifSutun === 0 ? (sayfa.azalanSirali ? " ▼" : " ▲") : "")
                        Layout.preferredWidth: 202
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        color: Tema.metinSilik
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sayfa.sutunaGoreSirala(0)
                        }
                    }
                    Text {
                        text: "İşlem/PID"
                        Layout.preferredWidth: 90
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        color: Tema.metinSilik
                    }
                    Text {
                        text: "Bellek" + (sayfa.aktifSutun === 1 ? (sayfa.azalanSirali ? " ▼" : " ▲") : "")
                        Layout.preferredWidth: 80
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        color: Tema.metinSilik
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sayfa.sutunaGoreSirala(1)
                        }
                    }
                    Text {
                        text: "Durum"
                        Layout.fillWidth: true
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        color: Tema.metinSilik
                    }
                    Text {
                        text: "Öncelik"
                        Layout.preferredWidth: 100
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        color: Tema.metinSilik
                    }
                }

                TreeView {
                    id: agac
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: islemler
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Item {
                        id: satir
                        implicitWidth: agac.width
                        implicitHeight: 30

                        required property int row
                        required property int depth
                        required property bool hasChildren
                        required property bool expanded
                        required property bool isTreeNode
                        required property bool grupMu
                        required property string ad
                        required property real mb
                        required property string durum
                        required property bool secili
                        required property int altSayisi
                        required property string pidMetni
                        required property string exeYolu
                        required property string simgeUrl
                        required property int oncelik
                        required property int ustSatirNo

                        Rectangle {
                            anchors.fill: parent
                            color: fareAlani.containsMouse ? Tema.yuzeyYuksek : "transparent"
                        }

                        MouseArea {
                            id: fareAlani
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.RightButton
                            onClicked: {
                                const mi = agac.index(satir.row, 0)
                                const sNo = satir.depth === 0 ? mi.row : satir.ustSatirNo
                                const aSatir = satir.depth === 0 ? -1 : mi.row
                                const bilgi = islemler.satirBilgisi(sNo, aSatir)
                                sagTikMenuIslem.ac(sNo, aSatir, bilgi.ad, bilgi.exeYolu,
                                                    bilgi.oncelik, bilgi.durum !== "Kapatılabilir")
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Tema.boslukKucuk
                            anchors.rightMargin: Tema.boslukKucuk
                            spacing: Tema.boslukKucuk

                            // Girinti SADECE ad sutununda; satirin tamamini
                            // kaydirmak sag sutunlari (islem/MB/durum) bozuyordu.
                            Item { Layout.preferredWidth: satir.depth * 18 }

                            Text {
                                text: satir.isTreeNode && satir.hasChildren ? (satir.expanded ? "▾" : "▸") : ""
                                Layout.preferredWidth: 14
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinIkincil
                                MouseArea {
                                    anchors.fill: parent
                                    visible: satir.hasChildren
                                    onClicked: agac.toggleExpanded(satir.row)
                                }
                            }

                            // Gizli oge Layout'tan tamamen dusuyor - sabit
                            // genislikte kap icine alinir ki alt satirlar kaymasin.
                            Item {
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                TikKutusu {
                                    anchors.centerIn: parent
                                    visible: satir.depth === 0
                                    checked: satir.secili
                                    onToggled: islemler.isaretiCevir(agac.index(satir.row, 0).row)
                                }
                            }

                            // Uygulama logosu - simgesiz satirda bos yuva durur,
                            // sutun hizasi bozulmaz.
                            Item {
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                Image {
                                    anchors.fill: parent
                                    visible: satir.simgeUrl !== ""
                                    source: satir.simgeUrl
                                    sourceSize.width: 18
                                    sourceSize.height: 18
                                    smooth: true
                                    fillMode: Image.PreserveAspectFit
                                }
                            }

                            Text {
                                text: satir.ad
                                Layout.preferredWidth: 202 - satir.depth * 18
                                elide: Text.ElideRight
                                font.family: Tema.aile
                                font.pixelSize: Tema.punto
                                color: Tema.metin
                            }
                            Text {
                                text: satir.grupMu ? (satir.altSayisi + " işlem") : ("PID " + satir.pidMetni)
                                Layout.preferredWidth: 90
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinIkincil
                            }
                            Text {
                                text: satir.mb.toFixed(0) + " MB"
                                Layout.preferredWidth: 80
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinIkincil
                            }
                            Text {
                                text: satir.durum
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinIkincil
                            }
                            Text {
                                text: sayfa.oncelikMetni(satir.oncelik)
                                Layout.preferredWidth: 100
                                elide: Text.ElideRight
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinIkincil
                            }
                        }
                    }
                }
            }
        }

        Card {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            ColumnLayout {
                anchors.fill: parent
                spacing: Tema.boslukKucuk

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    spacing: Tema.boslukKucuk

                    Text {
                        text: "GÜNLÜK"
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        font.letterSpacing: 0.6
                        color: Tema.metinSilik
                    }
                    Text {
                        text: "(" + bellek.gunlukler.length + ")"
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        color: Tema.metinSilik
                    }
                    Item { Layout.fillWidth: true }
                    GhostButton {
                        text: "Kopyala"
                        implicitHeight: 26
                        onClicked: islemler.panoyaYaz(bellek.gunlukler.join("\n"))
                    }
                    GhostButton {
                        text: "Temizle"
                        implicitHeight: 26
                        onClicked: bellek.gunlukTemizle()
                    }
                }

                ListView {
                    id: gunlukListesi
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: sayfa.gunlukTersSirali
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: Text {
                        required property string modelData
                        text: modelData
                        width: ListView.view.width
                        wrapMode: Text.Wrap
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        color: Tema.metinIkincil
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        onClicked: sagTikMenuGunluk.popup()
                    }
                }
            }
        }
    }

    // Ortak menu ogesi: solda simge yuvasi, ortada metin, sagda kisayol
    // ipucu. yikici=true olanlar tehlike renginde. (FolderTreePage.qml ile
    // AYNI - iki sayfa da ayni gorunumu kullansin diye kopyalandi.)
    component MenuOgesi: MenuItem {
        id: oge
        property string simge: ""
        property string kisayol: ""
        property bool yikici: false

        implicitHeight: 34
        padding: 0

        // Varsayilan indicator/arrow PNG kullaniyor; qtbase PNG'siz derlendigi
        // icin cozulemiyor. Kendi metin tabanli isaretlerimizi koyuyoruz.
        indicator: Text {
            visible: oge.checkable && oge.checked
            x: 12
            y: (oge.height - height) / 2
            text: "✓"
            font.family: Tema.aile
            font.pixelSize: Tema.punto
            color: Tema.vurgu
        }
        arrow: Text {
            visible: oge.subMenu !== null
            x: oge.width - width - 12
            y: (oge.height - height) / 2
            text: "›"
            font.family: Tema.aile
            font.pixelSize: Tema.punto
            color: Tema.metinIkincil
        }

        contentItem: RowLayout {
            spacing: 10
            Text {
                Layout.leftMargin: 12
                Layout.preferredWidth: 16
                text: oge.simge
                horizontalAlignment: Text.AlignHCenter
                font.family: Tema.aile
                font.pixelSize: Tema.punto
                color: oge.yikici ? Tema.tehlike
                                  : (oge.enabled ? Tema.metinIkincil : Tema.metinSilik)
            }
            Text {
                Layout.fillWidth: true
                text: oge.text
                elide: Text.ElideRight
                font.family: Tema.aile
                font.pixelSize: Tema.punto
                color: oge.yikici ? Tema.tehlike
                                  : (oge.enabled ? Tema.metin : Tema.metinSilik)
            }
            Text {
                Layout.rightMargin: 12
                visible: oge.kisayol !== ""
                text: oge.kisayol
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
                color: Tema.metinSilik
            }
        }

        background: Rectangle {
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            radius: Tema.koseKucuk
            color: oge.highlighted
                   ? (oge.yikici ? Qt.rgba(Tema.tehlike.r, Tema.tehlike.g, Tema.tehlike.b, 0.16)
                                 : Tema.yuzeyVurgu)
                   : "transparent"
        }
    }

    // Menu basligi - Menu'nun header ozelligi yok, bu yuzden menunun ILK
    // cocugu olarak eklenir. Tiklanabilir degil.
    component MenuBaslik: Item {
        property string metin: ""
        implicitHeight: 32
        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 6
            text: metin
            elide: Text.ElideMiddle
            font.family: Tema.aile
            font.pixelSize: Tema.puntoKucuk
            font.weight: Font.DemiBold
            color: Tema.metinIkincil
        }
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            height: 1
            color: Tema.kenarSilik
        }
    }

    // Varsayilan MenuSeparator cok yer kapliyor - sikistirilmis ayrac.
    component MenuAyraci: MenuSeparator {
        padding: 0
        topPadding: 5
        bottomPadding: 5
        contentItem: Rectangle {
            implicitHeight: 1
            color: Tema.kenarSilik
        }
    }

    // Menu govdesi - ustte hedefin adini gosteren baslik seridi.
    component TemaMenu: Menu {
        property string baslik: ""

        implicitWidth: 280
        padding: 5

        // Alt menu basligi gibi Qt'nin kendi urettigi ogeler varsayilan
        // PNG tik/ok gorseli kullaniyor - qtbase PNG'siz derlendigi icin
        // cozulemiyor. Delege ile onlari da metin isaretlere cevir.
        delegate: MenuItem {
            id: varsayilanOge
            implicitHeight: 34
            padding: 0

            indicator: Text {
                visible: varsayilanOge.checkable && varsayilanOge.checked
                x: 12
                y: (varsayilanOge.height - height) / 2
                text: "✓"
                font.family: Tema.aile
                font.pixelSize: Tema.punto
                color: Tema.vurgu
            }
            arrow: Text {
                visible: varsayilanOge.subMenu !== null
                x: varsayilanOge.width - width - 12
                y: (varsayilanOge.height - height) / 2
                text: "›"
                font.family: Tema.aile
                font.pixelSize: Tema.punto
                color: Tema.metinIkincil
            }
            contentItem: Text {
                leftPadding: 38
                rightPadding: 24
                text: varsayilanOge.text
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
                font.family: Tema.aile
                font.pixelSize: Tema.punto
                color: varsayilanOge.enabled ? Tema.metin : Tema.metinSilik
            }
            background: Rectangle {
                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                radius: Tema.koseKucuk
                color: varsayilanOge.highlighted ? Tema.yuzeyVurgu : "transparent"
            }
        }

        background: Rectangle {
            color: Tema.yuzeyYuksek
            border.color: Tema.kenar
            border.width: 1
            radius: Tema.koseKart
        }
    }

    // Sag tik menusu - islem satirlari.
    TemaMenu {
        id: sagTikMenuIslem
        onOpened: islemler.otomatikYenileAta(false)
        onClosed: islemler.otomatikYenileAta(true)
        property int satirNo: -1
        property int altSatir: -1
        property string ad: ""
        property string exeYoluDeger: ""
        property int oncelik: -1
        property bool korumali: false

        baslik: ad

        function ac(sNo, aSatir, hedefAd, yol, onc, korumaliMi) {
            sagTikMenuIslem.satirNo = sNo
            sagTikMenuIslem.altSatir = aSatir
            sagTikMenuIslem.ad = hedefAd
            sagTikMenuIslem.exeYoluDeger = yol
            sagTikMenuIslem.oncelik = onc
            sagTikMenuIslem.korumali = korumaliMi
            sagTikMenuIslem.popup()
        }

        MenuBaslik { metin: sagTikMenuIslem.ad }

        MenuOgesi {
            text: "İşlemi sonlandır"
            simge: "✕"
            kisayol: "Del"
            yikici: true
            enabled: !sagTikMenuIslem.korumali
            onTriggered: islemler.kapat(sagTikMenuIslem.satirNo, sagTikMenuIslem.altSatir)
        }
        MenuOgesi {
            text: "İşlem ağacını sonlandır"
            simge: "⛔"
            yikici: true
            enabled: !sagTikMenuIslem.korumali
            onTriggered: agacKapatOnay.ac(sagTikMenuIslem.satirNo, sagTikMenuIslem.altSatir, sagTikMenuIslem.ad)
        }
        MenuAyraci {}
        MenuOgesi {
            text: "Dosya konumunu aç"
            simge: "🔍"
            onTriggered: islemler.konumunuGoster(sagTikMenuIslem.satirNo, sagTikMenuIslem.altSatir)
        }
        MenuOgesi {
            text: "Özellikler"
            simge: "ℹ"
            kisayol: "Alt+Enter"
            onTriggered: islemler.ozellikleriGoster(sagTikMenuIslem.satirNo, sagTikMenuIslem.altSatir)
        }
        MenuAyraci {}
        TemaMenu {
            title: "Öncelik"
            baslik: "Öncelik"
            enabled: !sagTikMenuIslem.korumali

            MenuBaslik { metin: "Öncelik" }
            MenuOgesi {
                text: "Düşük"
                checkable: true
                checked: sagTikMenuIslem.oncelik === 0
                onTriggered: islemler.oncelikAta(sagTikMenuIslem.satirNo, sagTikMenuIslem.altSatir, 0)
            }
            MenuOgesi {
                text: "Normalin altı"
                checkable: true
                checked: sagTikMenuIslem.oncelik === 1
                onTriggered: islemler.oncelikAta(sagTikMenuIslem.satirNo, sagTikMenuIslem.altSatir, 1)
            }
            MenuOgesi {
                text: "Normal"
                checkable: true
                checked: sagTikMenuIslem.oncelik === 2
                onTriggered: islemler.oncelikAta(sagTikMenuIslem.satirNo, sagTikMenuIslem.altSatir, 2)
            }
            MenuOgesi {
                text: "Normalin üstü"
                checkable: true
                checked: sagTikMenuIslem.oncelik === 3
                onTriggered: islemler.oncelikAta(sagTikMenuIslem.satirNo, sagTikMenuIslem.altSatir, 3)
            }
            MenuOgesi {
                text: "Yüksek"
                checkable: true
                checked: sagTikMenuIslem.oncelik === 4
                onTriggered: islemler.oncelikAta(sagTikMenuIslem.satirNo, sagTikMenuIslem.altSatir, 4)
            }
        }
        MenuAyraci {}
        MenuOgesi {
            text: "Adı kopyala"
            simge: "⎘"
            onTriggered: islemler.panoyaYaz(sagTikMenuIslem.ad)
        }
        MenuOgesi {
            text: "PID kopyala"
            simge: "⎘"
            onTriggered: islemler.panoyaYaz(
                String(islemler.satirBilgisi(sagTikMenuIslem.satirNo, sagTikMenuIslem.altSatir).pid))
        }
        MenuOgesi {
            text: "Yolu kopyala"
            simge: "⎘"
            onTriggered: islemler.panoyaYaz(sagTikMenuIslem.exeYoluDeger)
        }
        MenuOgesi {
            text: "Bu ada göre filtrele"
            onTriggered: {
                filtreAlani.text = sagTikMenuIslem.ad
                islemler.filtre = sagTikMenuIslem.ad
            }
        }
    }

    // Sag tik menusu - gunluk paneli.
    TemaMenu {
        id: sagTikMenuGunluk
        onOpened: islemler.otomatikYenileAta(false)
        onClosed: islemler.otomatikYenileAta(true)
        baslik: "Günlük"

        MenuBaslik { metin: "GÜNLÜK" }
        MenuOgesi {
            text: "Tümünü kopyala"
            simge: "⎘"
            onTriggered: islemler.panoyaYaz(bellek.gunlukler.join("\n"))
        }
        MenuOgesi {
            text: "Günlüğü temizle"
            simge: "✕"
            yikici: true
            onTriggered: bellek.gunlukTemizle()
        }
    }

    // Islem agacini sonlandirma onayi - zincirleme etki var, ZORUNLU.
    Dialog {
        onClosed: islemler.otomatikYenileAta(true)
        id: agacKapatOnay
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnEscape

        property int satirNo: -1
        property int altSatir: -1
        property string ad: ""
        property int sayi: 0

        function ac(sNo, aSatir, hedefAd) {
            agacKapatOnay.satirNo = sNo
            agacKapatOnay.altSatir = aSatir
            agacKapatOnay.ad = hedefAd
            agacKapatOnay.sayi = islemler.agacBoyutu(sNo, aSatir)
            agacKapatOnay.open()
        }

        onOpened: {
            islemler.otomatikYenileAta(false)
            vazgecDugmesiAgac.forceActiveFocus()
        }
        onAccepted: islemler.agaciKapat(agacKapatOnay.satirNo, agacKapatOnay.altSatir)

        background: Rectangle {
            color: Tema.yuzeyYuksek
            border.color: Tema.kenar
            border.width: 1
            radius: Tema.koseKucuk
        }

        header: Text {
            text: "İşlem ağacını sonlandır"
            font.family: Tema.aile
            font.pixelSize: Tema.puntoOrta
            font.weight: Font.DemiBold
            color: Tema.metin
            padding: 16
        }

        contentItem: ColumnLayout {
            spacing: Tema.boslukKucuk
            implicitWidth: 380

            Text {
                Layout.fillWidth: true
                text: agacKapatOnay.ad + " ve tüm alt işlemleri kapatılacak."
                wrapMode: Text.WordWrap
                color: Tema.metin
                font.family: Tema.aile
                font.pixelSize: Tema.punto
            }
            Text {
                text: agacKapatOnay.sayi + " süreç kapatılacak."
                color: Tema.tehlike
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
            }
        }

        footer: RowLayout {
            Layout.fillWidth: true
            spacing: Tema.boslukKucuk

            Item { Layout.fillWidth: true }

            Button {
                id: vazgecDugmesiAgac
                text: "Vazgeç"
                focus: true
                Keys.onReturnPressed: clicked()
                onClicked: agacKapatOnay.reject()
                contentItem: Text {
                    text: vazgecDugmesiAgac.text
                    color: Tema.metin
                    font.family: Tema.aile
                    font.pixelSize: Tema.punto
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 90
                    implicitHeight: 32
                    radius: Tema.koseDugme
                    color: vazgecDugmesiAgac.pressed ? Tema.yuzeyVurgu : vazgecDugmesiAgac.hovered ? Tema.yuzeyYuksek : "transparent"
                    border.width: 1
                    border.color: vazgecDugmesiAgac.activeFocus ? Tema.vurgu : Tema.kenar
                }
            }
            Button {
                id: kapatDugmesiAgac
                text: "Kapat"
                Keys.onReturnPressed: clicked()
                onClicked: agacKapatOnay.accept()
                contentItem: Text {
                    text: kapatDugmesiAgac.text
                    color: Tema.tehlike
                    font.family: Tema.aile
                    font.pixelSize: Tema.punto
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 90
                    implicitHeight: 32
                    radius: Tema.koseDugme
                    color: kapatDugmesiAgac.pressed ? Tema.yuzeyVurgu : kapatDugmesiAgac.hovered ? Tema.yuzeyYuksek : "transparent"
                    border.width: 1
                    border.color: Tema.tehlike
                }
            }
            Item { Layout.preferredWidth: Tema.bosluk }
        }
    }
}
