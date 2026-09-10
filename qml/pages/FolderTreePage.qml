// FolderTreePage.qml - AGAC TARA sayfasi. Surucu sec, klasor agacini
// tembel yukleyerek gez, boyuta gore sirala, sag panelde detay goster.
// Mantik src/backend/FolderTreeModel.* icinde - burasi sadece goruntu.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RamTemizleyici
import "../components"

Item {
    id: sayfa

    property var secili: ({})
    property var seciliKaynakIndex: null
    property bool boyutaGoreAzalan: true
    property string aktifSurucu: ""
    property string silmeMesaji: ""
    property bool silmeHata: false
    property int sagPanelSekme: 0 // 0: klasor detayi, 1: en buyuk dosyalar

    // Taranan yollarin gecmisi - kisayola tiklayinca onceki taramaya
    // donebilmek icin. Diziye push/pop baglamayi tetiklemedigi icin
    // derinlik ayri bir sayaçta tutulur.
    property var gecmis: []
    property int gecmisDerinlik: 0

    function taramayaGit(yol) {
        if (yol === "" || agac.taraniyor) return
        if (agac.kokYol !== "" && agac.kokYol !== yol) {
            sayfa.gecmis.push(agac.kokYol)
            sayfa.gecmisDerinlik = sayfa.gecmis.length
        }
        sayfa.aktifSurucu = yol
        agac.surucuSec(yol)
    }

    // Kisayol tiklamasi - yol zaten taranmis C: agacinin icindeyse agaci
    // sifirlamadan sadece o dugume kadar acip gosterir; degilse eski
    // davranisa (yeni tarama) duser.
    function konumaGit(yol) {
        if (yol === "" || agac.taraniyor) return
        const idx = agac.yoluBul(yol)
        if (!idx.valid) {
            sayfa.taramayaGit(yol)   // agacta yok - o yolu kok alip tara
            return
        }

        agacGorunumu.expandToIndex(idx)
        // expandToIndex satirlari ayni karede olusturmuyor; rowAtIndex hemen
        // cagrilirsa -1 doner ve secim guncellenmez. Bir sonraki duzeni bekle.
        Qt.callLater(function () {
            const satir = agacGorunumu.rowAtIndex(idx)
            if (satir < 0) return
            agacGorunumu.expand(satir)   // hedef klasorun kendisi de acilsin
            agacGorunumu.positionViewAtRow(satir, TableView.Contain)
            const kaynak = agac.siraliModel.mapToSource(idx)
            sayfa.secili = agac.dugumBilgisi(kaynak)
            sayfa.seciliKaynakIndex = kaynak
        })
    }

    function geriGit() {
        if (sayfa.gecmis.length === 0 || agac.taraniyor) return
        const onceki = sayfa.gecmis.pop()
        sayfa.gecmisDerinlik = sayfa.gecmis.length
        sayfa.aktifSurucu = onceki
        agac.surucuSec(onceki)
    }

    // Yolun bir ust klasoru - kok ise bos.
    function ustKlasor(yol) {
        const temiz = yol.replace(/[\\/]+$/, "")
        const kesim = Math.max(temiz.lastIndexOf("/"), temiz.lastIndexOf("\\"))
        if (kesim < 0) return ""
        const ust = temiz.substring(0, kesim)
        if (ust.length <= 2) return ust + "/"   // "C:" -> "C:/"
        return ust
    }

    // Ust seritteki surucu karti - harf + etiket, doluluk cubugu, bos/toplam.
    component SurucuKarti: Rectangle {
        id: kart
        required property var modelData
        property bool secili: false
        signal tiklandi()

        implicitWidth: 190
        implicitHeight: 76
        radius: Tema.koseKucuk
        color: Tema.yuzeyYuksek
        border.width: kart.secili ? 2 : 1
        border.color: kart.secili ? Tema.vurgu : Tema.kenar

        readonly property real doluluk: kart.modelData.kullanilanYuzde / 100.0
        readonly property color cubukRengi: kart.modelData.kullanilanYuzde >= 85 ? Tema.tehlike
                                           : kart.modelData.kullanilanYuzde >= 70 ? Tema.uyari
                                           : Tema.vurgu

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Text {
                    text: kart.modelData.yol.charAt(0)
                    font.family: Tema.aile
                    font.pixelSize: Tema.puntoBuyuk
                    font.weight: Font.DemiBold
                    color: Tema.metin
                }
                Text {
                    Layout.fillWidth: true
                    text: kart.modelData.etiket
                    elide: Text.ElideRight
                    font.family: Tema.aile
                    font.pixelSize: Tema.puntoKucuk
                    color: Tema.metinIkincil
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 5
                radius: 2.5
                color: Tema.yuzeyVurgu
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    radius: 2.5
                    color: kart.cubukRengi
                    width: parent.width * Math.max(0, Math.min(1, kart.doluluk))
                }
            }

            Text {
                Layout.fillWidth: true
                text: kart.modelData.bosGB.toFixed(0) + " GB boş / " + kart.modelData.toplamGB.toFixed(0) + " GB"
                elide: Text.ElideRight
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
                color: Tema.metinSilik
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: kart.tiklandi()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Tema.bosluk
        spacing: Tema.bosluk

        // Surucu satiri - karti tiklamak sadece SECER, tarama TARA dugmesiyle baslar.
        RowLayout {
            Layout.fillWidth: true
            spacing: Tema.boslukKucuk

            Repeater {
                model: agac.suruculer
                delegate: SurucuKarti {
                    secili: sayfa.aktifSurucu === modelData.yol
                    onTiklandi: sayfa.aktifSurucu = modelData.yol
                }
            }

            Item { Layout.fillWidth: true }

            // Su an hangi yol tarandi - kisayol/ust klasor gezintisinde
            // kaybolmamak icin.
            Text {
                Layout.alignment: Qt.AlignVCenter
                Layout.maximumWidth: 320
                visible: agac.kokYol !== ""
                text: agac.kokYol
                elide: Text.ElideMiddle
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
                color: Tema.metinIkincil
            }

            ColumnLayout {
                Layout.preferredWidth: 330
                visible: agac.taraniyor
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    text: agac.ilerlemeMetni
                    font.family: Tema.aile
                    font.pixelSize: Tema.puntoKucuk
                    color: Tema.metinIkincil
                    elide: Text.ElideRight
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 6
                    radius: 3
                    color: Tema.yuzeyVurgu

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        radius: 3
                        color: Tema.vurgu
                        width: parent.width * Math.max(0, Math.min(1, agac.ilerleme))
                        Behavior on width { NumberAnimation { duration: 150 } }
                    }
                }
            }

            PrimaryButton {
                id: taraDugmesi
                Layout.preferredWidth: implicitWidth
                Layout.preferredHeight: 38
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignVCenter
                text: agac.taraniyor ? "TARAMAYI DURDUR" : "TARAMAYI BAŞLAT"
                enabled: agac.taraniyor || sayfa.aktifSurucu !== ""
                onClicked: {
                    if (agac.taraniyor) agac.taramayiDurdur()
                    else sayfa.taramayaGit(sayfa.aktifSurucu)
                }

                background: Rectangle {
                    radius: Tema.koseDugme
                    color: !taraDugmesi.enabled ? Tema.yuzeyYuksek
                         : agac.taraniyor
                           ? (taraDugmesi.pressed ? Qt.darker(Tema.tehlike, 1.2)
                                                   : taraDugmesi.hovered ? Qt.lighter(Tema.tehlike, 1.12) : Tema.tehlike)
                           : (taraDugmesi.pressed ? Tema.vurguKoyu
                                                   : taraDugmesi.hovered ? Qt.lighter(Tema.vurgu, 1.12) : Tema.vurgu)
                    Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutQuad } }
                }
            }
        }

        // Dosya turu - taranmis alandaki TUM videolar/resimler vs. Boyut
        // esiginden bagimsiz, kategori basina ayri havuzdan gelir.
        Flow {
            Layout.fillWidth: true
            spacing: 4

            GhostButton {
                implicitHeight: 26
                text: "◀ Geri"
                enabled: sayfa.gecmisDerinlik > 0 && !agac.taraniyor
                onClicked: sayfa.geriGit()
            }

            GhostButton {
                implicitHeight: 26
                text: "↑ Üst klasör"
                enabled: !agac.taraniyor && sayfa.ustKlasor(agac.kokYol) !== ""
                onClicked: sayfa.taramayaGit(sayfa.ustKlasor(agac.kokYol))
            }

            Item { width: 14; height: 26 }

            Text {
                text: "Klasör:"
                height: 26
                verticalAlignment: Text.AlignVCenter
                rightPadding: 4
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
                color: Tema.metinIkincil
            }

            Repeater {
                model: agac.kisayollar
                delegate: GhostButton {
                    required property var modelData
                    implicitHeight: 26
                    text: modelData.ad
                    // Tarama yapilmis olabilir - vurgu sadece bu yol GERCEKTEN
                    // kok oldugunda anlamli, yoksa yaniltici olur.
                    secili: agac.kokYol === modelData.yol
                    enabled: !agac.taraniyor
                    onClicked: sayfa.konumaGit(modelData.yol)
                }
            }

            // Iki grubu ayiran bosluk
            Item { width: 14; height: 26 }

            Text {
                text: "Dosya türü:"
                height: 26
                verticalAlignment: Text.AlignVCenter
                rightPadding: 4
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
                color: Tema.metinIkincil
            }

            Repeater {
                model: [
                    { ad: "En büyükler", kod: "" },
                    { ad: "Videolar", kod: "video" },
                    { ad: "Resimler", kod: "resim" },
                    { ad: "Ses", kod: "ses" },
                    { ad: "Arşivler", kod: "arsiv" },
                    { ad: "Kurulumlar", kod: "kurulum" }
                ]
                delegate: GhostButton {
                    required property var modelData
                    implicitHeight: 26
                    text: modelData.ad
                    secili: agac.turFiltresi === modelData.kod
                    onClicked: {
                        agac.turFiltresi = modelData.kod
                        sayfa.sagPanelSekme = 1  // sonuc listesi gorunur olsun
                    }
                }
            }

            GhostButton {
                implicitHeight: 26
                visible: agac.turFiltresi !== ""
                text: agac.sistemGizle ? "Sistem dosyaları gizli" : "Sistem dosyaları dahil"
                secili: agac.sistemGizle
                onClicked: agac.sistemGizle = !agac.sistemGizle
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Tema.bosluk

            // Sol/ana: agac
            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 2

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Tema.boslukKucuk

                    // Baslik satiri - Boyut'a tiklayinca sirala
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            Layout.fillWidth: true
                            text: "Ad"
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            color: Tema.metinSilik
                        }
                        Text {
                            text: (boyutaGoreAzalan ? "Boyut ▼" : "Boyut ▲")
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            color: Tema.metinSilik
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    boyutaGoreAzalan = !boyutaGoreAzalan
                                    agac.boyutaGoreSirala(boyutaGoreAzalan)
                                }
                            }
                        }
                        Item { Layout.preferredWidth: 70 } // yuzde cubugu genisligi kadar bosluk
                    }

                    // Agac + bos durum katmani ayni hucreyi paylasiyor - Item
                    // sarmalayici olmadan ColumnLayout ikisine de ayri satir
                    // ayirirdi.
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                    TreeView {
                        id: agacGorunumu
                        anchors.fill: parent
                        clip: true
                        model: agac.siraliModel

                        delegate: Item {
                            id: satir
                            implicitWidth: agacGorunumu.width
                            implicitHeight: 26

                            required property TreeView treeView
                            required property bool isTreeNode
                            required property bool expanded
                            required property bool hasChildren
                            required property int depth
                            required property int row
                            required property int column

                            required property string ad
                            required property string tamYol
                            required property string boyutMetni
                            required property double yuzde
                            required property bool taraniyor
                            required property bool olculdu
                            // QSortFilterProxyModel hasChildren'i kaynaktan
                            // almiyor (henuz taranmamis dugumde 0 satir gorur),
                            // bu yuzden modelin kendi rolu kullanilir.
                            required property bool altKlasoruVar

                            function kaynakIndex() {
                                return agac.siraliModel.mapToSource(treeView.index(row, column))
                            }

                            Rectangle {
                                anchors.fill: parent
                                color: mouseAlani.containsMouse ? Tema.yuzeyVurgu : "transparent"
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: satir.depth * 18 + 4
                                anchors.rightMargin: 6
                                spacing: 6

                                Text {
                                    text: satir.altKlasoruVar ? (satir.expanded ? "▾" : "▸") : ""
                                    color: Tema.metinSilik
                                    font.pixelSize: Tema.puntoKucuk
                                    width: 12
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: satir.ad + (satir.taraniyor ? " (taranıyor...)" : "")
                                    color: Tema.metin
                                    font.family: Tema.aile
                                    font.pixelSize: Tema.punto
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: satir.olculdu ? satir.boyutMetni : "..."
                                    color: Tema.metinIkincil
                                    font.family: Tema.aile
                                    font.pixelSize: Tema.puntoKucuk
                                    horizontalAlignment: Text.AlignRight
                                    Layout.preferredWidth: 90
                                }

                                Rectangle {
                                    Layout.preferredWidth: 60
                                    Layout.preferredHeight: 6
                                    radius: 3
                                    color: Tema.kenarSilik
                                    visible: satir.olculdu
                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        radius: 3
                                        color: Tema.vurgu
                                        width: parent.width * Math.max(0, Math.min(1, satir.yuzde))
                                    }
                                }
                            }

                            MouseArea {
                                id: mouseAlani
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                onClicked: (mouse) => {
                                    const kaynak = satir.kaynakIndex()
                                    sayfa.secili = agac.dugumBilgisi(kaynak)
                                    sayfa.seciliKaynakIndex = kaynak

                                    if (mouse.button === Qt.RightButton) {
                                        sagTikMenuAgac.ac(kaynak, satir.tamYol, satir.ad, satir.boyutMetni)
                                        return
                                    }
                                    if (satir.altKlasoruVar) {
                                        if (!satir.expanded) agac.dugumuAc(kaynak)
                                        treeView.toggleExpanded(satir.row)
                                    }
                                }
                            }
                        }
                    }

                        // Bos durum - hicbir tarama sonucu yokken (ilk acilis
                        // ya da surucu secilip tara BASLAMADAN once).
                        ColumnLayout {
                            anchors.centerIn: parent
                            width: parent.width * 0.7
                            visible: !agac.taraniyor && !agac.sonucVar
                            spacing: 8

                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: "📁"
                                font.pixelSize: 48
                            }
                            Text {
                                Layout.alignment: Qt.AlignHCenter
                                text: "Bir sürücü seç ve taramayı başlat"
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoOrta
                                font.weight: Font.DemiBold
                                color: Tema.metinIkincil
                            }
                            Text {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignHCenter
                                horizontalAlignment: Text.AlignHCenter
                                wrapMode: Text.WordWrap
                                text: "Klasörler boyuta göre sıralanır, en yer kaplayanı ilk sırada görürsün."
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinSilik
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: (sayfa.silmeMesaji.length > 0 ? sayfa.silmeMesaji : agac.durum)
                                  + (agac.taramaOzeti.length > 0 ? "  ·  " + agac.taramaOzeti : "")
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            color: sayfa.silmeMesaji.length > 0 && sayfa.silmeHata ? Tema.tehlike : Tema.metinSilik
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            visible: agac.taraniyor
                            text: "taranıyor..."
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            color: Tema.vurgu
                        }
                    }
                }
            }

            // Sag: sekmeli panel - klasor detayi / en buyuk dosyalar.
            Card {
                Layout.fillHeight: true
                Layout.preferredWidth: 280

                ColumnLayout {
                    anchors.fill: parent
                    spacing: Tema.boslukKucuk

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        GhostButton {
                            Layout.fillWidth: true
                            text: "DETAY"
                            secili: sayfa.sagPanelSekme === 0
                            onClicked: sayfa.sagPanelSekme = 0
                        }
                        GhostButton {
                            Layout.fillWidth: true
                            text: "BÜYÜK DOSYALAR"
                            secili: sayfa.sagPanelSekme === 1
                            onClicked: sayfa.sagPanelSekme = 1
                        }
                    }

                    // --- KLASÖR DETAYI ---
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: sayfa.sagPanelSekme === 0
                        spacing: Tema.boslukKucuk

                        Text {
                            Layout.fillWidth: true
                            text: sayfa.secili.tamYol ? sayfa.secili.tamYol : "-"
                            wrapMode: Text.WrapAnywhere
                            font.family: Tema.aile
                            font.pixelSize: Tema.punto
                            font.weight: Font.DemiBold
                            color: Tema.metin
                        }

                        Text {
                            text: sayfa.secili.boyutMetni ? sayfa.secili.boyutMetni : "-"
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoBuyuk
                            font.weight: Font.DemiBold
                            color: Tema.metin
                        }

                        Text {
                            text: sayfa.secili.ozet ? sayfa.secili.ozet : ""
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            color: Tema.metinIkincil
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: Tema.kenarSilik
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: sayfa.secili.ogeler ? sayfa.secili.ogeler : []
                            boundsBehavior: Flickable.StopAtBounds

                            delegate: Item {
                                id: dosyaSatiri
                                required property var modelData
                                width: ListView.view.width
                                height: 24

                                Rectangle {
                                    anchors.fill: parent
                                    color: dosyaMouseAlani.containsMouse ? Tema.yuzeyVurgu : "transparent"
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    spacing: 6

                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.ad
                                        elide: Text.ElideMiddle
                                        font.family: Tema.aile
                                        font.pixelSize: Tema.puntoKucuk
                                        color: Tema.metin
                                    }
                                    Text {
                                        text: modelData.boyutMetni
                                        font.family: Tema.aile
                                        font.pixelSize: Tema.puntoKucuk
                                        color: Tema.metinIkincil
                                    }
                                    Rectangle {
                                        Layout.preferredWidth: 44
                                        height: 5
                                        radius: 2.5
                                        color: Tema.yuzeyVurgu
                                        Rectangle {
                                            width: parent.width * modelData.yuzde
                                            height: parent.height
                                            radius: parent.radius
                                            color: Tema.vurgu
                                        }
                                    }
                                }

                                MouseArea {
                                    id: dosyaMouseAlani
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.RightButton
                                    onClicked: {
                                        const dosyaYolu = sayfa.secili.tamYol + "/" + dosyaSatiri.modelData.ad
                                        sagTikMenuDosya.ac(dosyaYolu, dosyaSatiri.modelData.ad, dosyaSatiri.modelData.boyutMetni)
                                    }
                                }
                            }
                        }
                    }

                    // --- EN BÜYÜK DOSYALAR ---
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: sayfa.sagPanelSekme === 1
                        spacing: Tema.boslukKucuk

                        // Cipler satira sigmazsa alt satira gecsin - sabit
                        // genislikli RowLayout panelden tasiyordu.
                        Flow {
                            Layout.fillWidth: true
                            spacing: 4
                            visible: agac.turFiltresi === ""

                            Text {
                                text: "Eşik:"
                                height: 24
                                verticalAlignment: Text.AlignVCenter
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinIkincil
                            }
                            Repeater {
                                model: [500, 1024, 2048, 5120, 10240]
                                delegate: GhostButton {
                                    required property int modelData
                                    implicitHeight: 24
                                    text: modelData >= 1024
                                          ? ((modelData / 1024) + " GB")
                                          : (modelData + " MB")
                                    secili: agac.esikMB === modelData
                                    onClicked: agac.esikMB = modelData
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: agac.buyukDosyalar.length === 0
                                  ? (agac.taraniyor ? "Taranıyor..." : "Henüz dosya bulunmadı.")
                                  : (agac.buyukDosyalar.length + " dosya"
                                     + (agac.turFiltresi === "" ? "" : " (en büyükten sıralı)"))
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            color: Tema.metinSilik
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: agac.buyukDosyalar
                            boundsBehavior: Flickable.StopAtBounds

                            delegate: Item {
                                id: buyukDosyaSatiri
                                required property var modelData
                                width: ListView.view.width
                                height: 38

                                Rectangle {
                                    anchors.fill: parent
                                    color: buyukDosyaMouseAlani.containsMouse ? Tema.yuzeyVurgu : "transparent"
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 4
                                    spacing: 1

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6
                                        Text {
                                            Layout.fillWidth: true
                                            text: buyukDosyaSatiri.modelData.ad
                                            elide: Text.ElideMiddle
                                            font.family: Tema.aile
                                            font.pixelSize: Tema.puntoKucuk
                                            color: Tema.metin
                                        }
                                        Text {
                                            text: buyukDosyaSatiri.modelData.boyutMetni
                                            font.family: Tema.aile
                                            font.pixelSize: Tema.puntoKucuk
                                            color: Tema.metinIkincil
                                        }
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: buyukDosyaSatiri.modelData.tamYol
                                        elide: Text.ElideMiddle
                                        font.family: Tema.aile
                                        font.pixelSize: Tema.puntoKucuk - 1
                                        color: Tema.metinSilik
                                    }
                                }

                                MouseArea {
                                    id: buyukDosyaMouseAlani
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.RightButton
                                    onClicked: sagTikMenuDosya.ac(buyukDosyaSatiri.modelData.tamYol,
                                                                   buyukDosyaSatiri.modelData.ad,
                                                                   buyukDosyaSatiri.modelData.boyutMetni)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Klavye kisayollari - menudeki ipuclariyla ayni islemler. Secili dugum
    // yoksa hicbiri calismaz; silme yine ONAY diyalogundan geciyor.
    Shortcut {
        sequence: "Delete"
        enabled: sayfa.seciliKaynakIndex !== null && !agac.taraniyor
        onActivated: onayDialog.ac(false, sayfa.secili.tamYol, sayfa.secili.boyutMetni,
                                   sayfa.seciliKaynakIndex, sayfa.secili.ad)
    }
    Shortcut {
        sequence: "Shift+Delete"
        enabled: sayfa.seciliKaynakIndex !== null && !agac.taraniyor
        onActivated: onayDialog.ac(true, sayfa.secili.tamYol, sayfa.secili.boyutMetni,
                                   sayfa.seciliKaynakIndex, sayfa.secili.ad)
    }
    Shortcut {
        sequences: [StandardKey.Copy]
        enabled: sayfa.secili.tamYol !== undefined
        onActivated: agac.panoyaYaz(sayfa.secili.tamYol)
    }
    Shortcut {
        sequences: [StandardKey.Refresh]
        enabled: sayfa.seciliKaynakIndex !== null && !agac.taraniyor
        onActivated: agac.yenidenTara(sayfa.seciliKaynakIndex)
    }
    Shortcut {
        sequence: "Alt+Return"
        enabled: sayfa.secili.tamYol !== undefined
        onActivated: agac.ozellikleriGoster(sayfa.secili.tamYol)
    }
    Shortcut {
        sequence: "Return"
        enabled: sayfa.secili.tamYol !== undefined
        onActivated: agac.explorerdaAc(sayfa.secili.tamYol)
    }

    // Ortak menu ogesi: solda simge yuvasi, ortada metin, sagda kisayol
    // ipucu. yikici=true olanlar tehlike renginde.
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

    // Sag tik menusu - agac satirlari.
    TemaMenu {
        id: sagTikMenuAgac
        property var kaynakIndex: null
        property string yol: ""
        property string ad: ""
        property string boyutMetni: ""

        baslik: ad

        function ac(index, hedefYol, hedefAd, hedefBoyutMetni) {
            sagTikMenuAgac.kaynakIndex = index
            sagTikMenuAgac.yol = hedefYol
            sagTikMenuAgac.ad = hedefAd
            sagTikMenuAgac.boyutMetni = hedefBoyutMetni
            sagTikMenuAgac.popup()
        }

        MenuBaslik { metin: sagTikMenuAgac.ad }

        MenuOgesi {
            text: "Klasörü aç"
            simge: "📁"
            kisayol: "Enter"
            onTriggered: agac.explorerdaAc(sagTikMenuAgac.yol)
        }
        MenuOgesi {
            text: "Explorer'da göster"
            simge: "🔍"
            onTriggered: agac.konumunuGoster(sagTikMenuAgac.yol)
        }
        MenuOgesi {
            text: "Komut istemi burada aç"
            simge: ">_"
            onTriggered: agac.komutIstemiAc(sagTikMenuAgac.yol)
        }
        MenuAyraci {}
        MenuOgesi {
            text: "Yolu kopyala"
            simge: "⎘"
            kisayol: "Ctrl+C"
            onTriggered: agac.panoyaYaz(sagTikMenuAgac.yol)
        }
        MenuOgesi {
            text: "Yeniden tara"
            simge: "↻"
            kisayol: "F5"
            enabled: !agac.taraniyor
            onTriggered: agac.yenidenTara(sagTikMenuAgac.kaynakIndex)
        }
        MenuAyraci {}
        MenuOgesi {
            text: "Geri Dönüşüm Kutusu'na gönder"
            simge: "♻"
            kisayol: "Del"
            enabled: !agac.taraniyor
            onTriggered: onayDialog.ac(false, sagTikMenuAgac.yol, sagTikMenuAgac.boyutMetni,
                                       sagTikMenuAgac.kaynakIndex, sagTikMenuAgac.ad)
        }
        MenuOgesi {
            text: "Kalıcı olarak sil"
            simge: "✕"
            kisayol: "Shift+Del"
            yikici: true
            enabled: !agac.taraniyor
            onTriggered: onayDialog.ac(true, sagTikMenuAgac.yol, sagTikMenuAgac.boyutMetni,
                                       sagTikMenuAgac.kaynakIndex, sagTikMenuAgac.ad)
        }
        MenuAyraci {}
        MenuOgesi {
            text: "Özellikler"
            simge: "ℹ"
            kisayol: "Alt+Enter"
            onTriggered: agac.ozellikleriGoster(sagTikMenuAgac.yol)
        }
    }

    // Sag tik menusu - sag paneldeki dosya listesi.
    TemaMenu {
        id: sagTikMenuDosya
        property string yol: ""
        property string ad: ""
        property string boyutMetni: ""

        baslik: ad

        function ac(hedefYol, hedefAd, hedefBoyutMetni) {
            sagTikMenuDosya.yol = hedefYol
            sagTikMenuDosya.ad = hedefAd
            sagTikMenuDosya.boyutMetni = hedefBoyutMetni
            sagTikMenuDosya.popup()
        }

        MenuBaslik { metin: sagTikMenuDosya.ad }

        MenuOgesi {
            text: "Explorer'da göster"
            simge: "🔍"
            onTriggered: agac.konumunuGoster(sagTikMenuDosya.yol)
        }
        MenuOgesi {
            text: "Yolu kopyala"
            simge: "⎘"
            kisayol: "Ctrl+C"
            onTriggered: agac.panoyaYaz(sagTikMenuDosya.yol)
        }
        MenuAyraci {}
        MenuOgesi {
            text: "Geri Dönüşüm Kutusu'na gönder"
            simge: "♻"
            kisayol: "Del"
            onTriggered: onayDialog.ac(false, sagTikMenuDosya.yol, sagTikMenuDosya.boyutMetni,
                                       null, sagTikMenuDosya.ad)
        }
        MenuOgesi {
            text: "Kalıcı olarak sil"
            simge: "✕"
            kisayol: "Shift+Del"
            yikici: true
            onTriggered: onayDialog.ac(true, sagTikMenuDosya.yol, sagTikMenuDosya.boyutMetni,
                                       null, sagTikMenuDosya.ad)
        }
        MenuAyraci {}
        MenuOgesi {
            text: "Özellikler"
            simge: "ℹ"
            kisayol: "Alt+Enter"
            onTriggered: agac.ozellikleriGoster(sagTikMenuDosya.yol)
        }
    }

    // Silme onay diyaloğu - hem agac dugumu hem sag paneldeki dosya icin
    // ortak. hedefIndex dolu ise agac.sil(), degilse (null) agac.dosyaSil()
    // cagrilir. Onay verilmeden HICBIR silme cagrisi yapilmaz.
    Dialog {
        id: onayDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnEscape

        property bool kalici: false
        property string hedefYol: ""
        property string hedefBoyutMetni: ""
        property var hedefIndex: null
        property string hedefAd: ""
        // Dolu ise silme kesinlikle engellenir (sistem klasoru vb.).
        property string engel: ""
        // Dolu ise silme yapilabilir ama ek uyari gosterilir.
        property string uyari: ""

        function ac(kaliciMi, hedefYolu, boyutMetni, index, ad) {
            onayDialog.kalici = kaliciMi
            onayDialog.hedefYol = hedefYolu
            onayDialog.hedefBoyutMetni = boyutMetni
            onayDialog.hedefIndex = index
            onayDialog.hedefAd = ad
            onayDialog.engel = agac.korumaSebebi(hedefYolu)
            onayDialog.uyari = onayDialog.engel === "" ? agac.riskUyarisi(hedefYolu) : ""
            onayDialog.open()
        }

        onOpened: vazgecDugmesi.forceActiveFocus()

        onAccepted: {
            if (onayDialog.engel !== "") return  // engel varken buraya duşulmemeli, yine de guvenlik icin
            let basarili
            if (onayDialog.hedefIndex !== null) {
                basarili = agac.sil(onayDialog.hedefIndex, onayDialog.kalici)
            } else {
                basarili = agac.dosyaSil(onayDialog.hedefYol, onayDialog.kalici)
                if (basarili && sayfa.seciliKaynakIndex !== null)
                    sayfa.secili = agac.dugumBilgisi(sayfa.seciliKaynakIndex)
            }
            sayfa.silmeHata = !basarili
            sayfa.silmeMesaji = basarili ? ("Silindi: " + onayDialog.hedefAd)
                                          : ("Silinemedi: " + onayDialog.hedefYol)
        }

        background: Rectangle {
            color: Tema.yuzeyYuksek
            border.color: Tema.kenar
            border.width: 1
            radius: Tema.koseKucuk
        }

        header: Text {
            text: onayDialog.engel !== "" ? "Silinemez"
                : (onayDialog.kalici ? "Kalıcı olarak sil" : "Geri Dönüşüm Kutusu'na gönder")
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
                text: onayDialog.hedefYol
                wrapMode: Text.WrapAnywhere
                color: Tema.metin
                font.family: Tema.aile
                font.pixelSize: Tema.punto
            }
            Text {
                visible: onayDialog.engel === ""
                text: onayDialog.hedefBoyutMetni
                color: Tema.metinIkincil
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
            }
            // KESIN ENGEL - sistem icin gerekli bir hedef, silme yolu tamamen kapatilir.
            Text {
                visible: onayDialog.engel !== ""
                Layout.fillWidth: true
                Layout.topMargin: Tema.boslukKucuk
                wrapMode: Text.WordWrap
                text: onayDialog.engel + " Bu öğe sistem için gerekli, silinemez."
                color: Tema.tehlike
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
            }
            // UYARI - silme yine mumkun ama dikkat cekilir.
            Text {
                visible: onayDialog.engel === "" && onayDialog.uyari !== ""
                Layout.fillWidth: true
                Layout.topMargin: Tema.boslukKucuk
                wrapMode: Text.WordWrap
                text: onayDialog.uyari
                color: Tema.tehlike
                font.family: Tema.aile
                font.pixelSize: Tema.puntoKucuk
            }
            Text {
                visible: onayDialog.engel === "" && onayDialog.kalici
                Layout.fillWidth: true
                Layout.topMargin: Tema.boslukKucuk
                wrapMode: Text.WordWrap
                text: "Bu işlem geri alınamaz. Dosyalar Geri Dönüşüm Kutusu'na gitmez."
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
                id: vazgecDugmesi
                text: onayDialog.engel !== "" ? "Kapat" : "Vazgeç"
                focus: true
                Keys.onReturnPressed: clicked()
                onClicked: onayDialog.reject()
                contentItem: Text {
                    text: vazgecDugmesi.text
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
                    color: vazgecDugmesi.pressed ? Tema.yuzeyVurgu : vazgecDugmesi.hovered ? Tema.yuzeyYuksek : "transparent"
                    border.width: 1
                    border.color: vazgecDugmesi.activeFocus ? Tema.vurgu : Tema.kenar
                }
            }
            Button {
                id: silDugmesi
                visible: onayDialog.engel === ""
                text: "Sil"
                Keys.onReturnPressed: clicked()
                onClicked: onayDialog.accept()
                contentItem: Text {
                    text: silDugmesi.text
                    color: onayDialog.kalici ? Tema.tehlike : Tema.metin
                    font.family: Tema.aile
                    font.pixelSize: Tema.punto
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    implicitWidth: 90
                    implicitHeight: 32
                    radius: Tema.koseDugme
                    color: silDugmesi.pressed ? Tema.yuzeyVurgu : silDugmesi.hovered ? Tema.yuzeyYuksek : "transparent"
                    border.width: 1
                    border.color: onayDialog.kalici ? Tema.tehlike : Tema.kenar
                }
            }
            Item { Layout.preferredWidth: Tema.bosluk }
        }
    }
}
