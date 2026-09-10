// HealthPage.qml - saglik skoru, kirilim, disk saglik kartlari (SMART).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RamTemizleyici
import "../components"

Item {
    id: sayfa

    function skorRengi(p) {
        return p >= 80 ? Tema.basari : p >= 50 ? Tema.uyari : Tema.tehlike
    }

    function omurRengi(omurMetni) {
        var v = parseInt(omurMetni)
        if (isNaN(v)) return Tema.metinIkincil
        return v >= 80 ? Tema.basari : v >= 50 ? Tema.uyari : Tema.tehlike
    }

    function sicaklikRengi(sicaklikMetni) {
        var v = parseFloat(sicaklikMetni)
        if (isNaN(v)) return Tema.metin
        return v >= 70 ? Tema.tehlike : v >= 60 ? Tema.uyari : Tema.metin
    }

    // Etiket solda, deger sagda, altinda ince ayirac - hem kirilim kartinda
    // hem disk kartlarinda ayni satir bicimi kullanilir.
    component BilgiSatiri: ColumnLayout {
        property string etiket: ""
        property string deger: "-"
        property color degerRengi: Tema.metin
        property bool ayirac: true

        Layout.fillWidth: true
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: Tema.boslukKucuk
            Text {
                text: etiket
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.family: Tema.aile
                font.pixelSize: Tema.punto
                color: Tema.metinIkincil
            }
            Text {
                text: deger
                horizontalAlignment: Text.AlignRight
                font.family: Tema.aile
                font.pixelSize: Tema.punto
                color: degerRengi
            }
        }
        Rectangle {
            visible: ayirac
            Layout.fillWidth: true
            implicitHeight: 1
            color: Tema.kenarSilik
        }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: Tema.bosluk
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: Tema.bosluk

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: false
                spacing: Tema.bosluk

                Card {
                    Layout.preferredWidth: 280
                    Layout.preferredHeight: 190
                    Layout.fillHeight: false
                    icBosluk: Tema.bosluk + 4

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Tema.boslukKucuk

                        Text {
                            text: "SAĞLIK PUANI"
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            font.letterSpacing: 0.6
                            color: Tema.metinSilik
                        }
                        Text {
                            text: saglik.puan
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoDev
                            font.weight: Font.DemiBold
                            color: skorRengi(saglik.puan)
                        }
                        Item { Layout.fillHeight: true }
                        Text {
                            text: saglik.yorum
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                            font.family: Tema.aile
                            font.pixelSize: Tema.punto
                            color: Tema.metinIkincil
                        }
                    }
                }

                Card {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 190
                    Layout.fillHeight: false
                    icBosluk: Tema.bosluk + 4

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Tema.boslukKucuk

                        BilgiSatiri { etiket: "RAM";          deger: String(saglik.ramPuan) }
                        BilgiSatiri { etiket: "Disk";         deger: String(saglik.diskPuan) }
                        BilgiSatiri { etiket: "Disk sağlığı"; deger: String(saglik.diskSaglikPuan); ayirac: false }

                        Item { Layout.fillHeight: true }

                        // Buton kartin SAG ALT kosesinde.
                        PrimaryButton {
                            Layout.alignment: Qt.AlignRight
                            text: "YENİLE"
                            onClicked: saglik.yenile()
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: false
                visible: !saglik.yonetici
                radius: Tema.koseKucuk
                color: Qt.rgba(0.95, 0.75, 0.36, 0.14)
                implicitHeight: uyariMetni.implicitHeight + Tema.bosluk
                Text {
                    id: uyariMetni
                    anchors.fill: parent
                    anchors.margins: Tema.boslukKucuk
                    verticalAlignment: Text.AlignVCenter
                    text: "SMART için yönetici gerekli"
                    font.family: Tema.aile
                    font.pixelSize: Tema.punto
                    color: Tema.uyari
                }
            }

            Repeater {
                model: saglik.diskler

                delegate: Card {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 300
                    Layout.fillHeight: false
                    icBosluk: Tema.bosluk + 4

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: Tema.bosluk

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: modelData.ad
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoBuyuk
                                font.weight: Font.DemiBold
                                color: Tema.metin
                            }
                            Rectangle {
                                radius: Tema.koseKucuk
                                color: Qt.rgba(omurRengi(modelData.omur).r, omurRengi(modelData.omur).g,
                                               omurRengi(modelData.omur).b, 0.16)
                                implicitWidth: durumMetni.implicitWidth + Tema.bosluk
                                implicitHeight: durumMetni.implicitHeight + Tema.boslukKucuk
                                Text {
                                    id: durumMetni
                                    anchors.centerIn: parent
                                    text: modelData.durum
                                    font.family: Tema.aile
                                    font.pixelSize: Tema.puntoKucuk
                                    color: omurRengi(modelData.omur)
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Tema.bosluk * 2

                            ColumnLayout {
                                Layout.preferredWidth: 120
                                spacing: 4
                                Text {
                                    text: "ÖMÜR"; color: Tema.metinSilik
                                    font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk
                                    font.letterSpacing: 0.6
                                }
                                Text {
                                    text: modelData.omur; color: omurRengi(modelData.omur)
                                    font.family: Tema.aile; font.pixelSize: Tema.puntoBuyuk
                                    font.weight: Font.DemiBold
                                }
                            }
                            ColumnLayout {
                                Layout.preferredWidth: 120
                                spacing: 4
                                Text {
                                    text: "SICAKLIK"; color: Tema.metinSilik
                                    font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk
                                    font.letterSpacing: 0.6
                                }
                                Text {
                                    text: modelData.sicaklik; color: sicaklikRengi(modelData.sicaklik)
                                    font.family: Tema.aile; font.pixelSize: Tema.puntoBuyuk
                                    font.weight: Font.DemiBold
                                }
                            }
                            Item { Layout.fillWidth: true }
                        }

                        // Iki sutun; her sutun kendi icinde etiket sol / deger sag.
                        // Dar pencerede tek sutuna duser.
                        GridLayout {
                            Layout.fillWidth: true
                            columns: width < 620 ? 1 : 2
                            columnSpacing: Tema.bosluk * 2
                            rowSpacing: Tema.boslukKucuk

                            BilgiSatiri { etiket: "Tür";              deger: modelData.tur }
                            BilgiSatiri { etiket: "Toplam boyut";     deger: modelData.toplamGB.toFixed(0) + " GB" }
                            BilgiSatiri { etiket: "Boş alan";         deger: modelData.bosGB.toFixed(0) + " GB" }
                            BilgiSatiri { etiket: "Kullanılan yüzde"; deger: modelData.kullanilanYuzde }
                            BilgiSatiri { etiket: "Yedek alan";       deger: modelData.yedekYuzde }
                            BilgiSatiri { etiket: "Okunan toplam";    deger: modelData.okunanGB }
                            BilgiSatiri { etiket: "Yazılan toplam";   deger: modelData.yazilanGB }
                            BilgiSatiri { etiket: "Açılma sayısı";    deger: modelData.acilmaSayisi }
                            BilgiSatiri { etiket: "Çalışma süresi";   deger: modelData.calismaSaati; ayirac: false }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: saglik.diskler.length === 0
                text: "Disk bilgisi bekleniyor..."
                horizontalAlignment: Text.AlignHCenter
                font.family: Tema.aile
                font.pixelSize: Tema.punto
                color: Tema.metinSilik
            }

            Item { Layout.fillHeight: true }
        }
    }
}
