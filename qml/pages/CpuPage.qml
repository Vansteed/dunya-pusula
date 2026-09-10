// CpuPage.qml - islemci kullanimi/frekans/sicaklik + ekran karti bilgisi.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RamTemizleyici
import "../components"

Item {
    // MemoryPage.qml'deki cizgi grafik ile ayni Canvas kodu - component olarak
    // paylasilir (ust grafik ve GPU kullanimi grafigi icin).
    component CizgiGrafik: Canvas {
        property var veri: []
        onVeriChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const v = veri
            ctx.strokeStyle = Tema.kenarSilik
            ctx.lineWidth = 1
            for (let g = 0; g <= 4; g++) {
                const y = height - (g / 4) * height
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

    property var gpu0: islemci.gpular.length > 0 ? islemci.gpular[0] : ({})

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Tema.bosluk
        spacing: Tema.bosluk

        RowLayout {
            Layout.fillWidth: true
            // Ic ice Layout'ta fillHeight varsayilani TRUE - acikca
            // kapatilmazsa bu sira tum boslugu yiyip alttaki karti eziyor.
            Layout.fillHeight: false
            Layout.preferredHeight: 190
            spacing: Tema.bosluk

            Card {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                ColumnLayout {
                    anchors.fill: parent
                    spacing: Tema.boslukKucuk

                    Text {
                        text: "İŞLEMCİ KULLANIMI"
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        font.letterSpacing: 0.6
                        color: Tema.metinSilik
                    }

                    Text {
                        text: islemci.kullanim.toFixed(0) + "%"
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoDev
                        font.weight: Font.DemiBold
                        color: islemci.kullanim >= 85 ? Tema.tehlike : islemci.kullanim >= 60 ? Tema.uyari : Tema.basari
                    }

                    Item { Layout.fillHeight: true }

                    Text {
                        Layout.fillWidth: true
                        text: islemci.ad
                        elide: Text.ElideRight
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        color: Tema.metinSilik
                    }
                    Text {
                        Layout.fillWidth: true
                        text: islemci.cekirdek + " çekirdek / " + islemci.mantiksalCekirdek + " mantıksal"
                        elide: Text.ElideRight
                        font.family: Tema.aile
                        font.pixelSize: Tema.puntoKucuk
                        color: Tema.metinSilik
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true
                CizgiGrafik {
                    anchors.fill: parent
                    veri: islemci.gecmis
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            // Ic ice Layout'ta fillHeight varsayilani TRUE - acikca
            // kapatilmazsa bu sira tum boslugu yiyip alttaki karti eziyor.
            Layout.fillHeight: false
            Layout.preferredHeight: 110
            spacing: Tema.bosluk

            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true
                RowLayout {
                    anchors.fill: parent
                    spacing: Tema.bosluk

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Tema.boslukKucuk

                        Text {
                            text: "FREKANS"
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            font.letterSpacing: 0.6
                            color: Tema.metinSilik
                        }
                        RowLayout {
                            spacing: 4
                            Text {
                                text: islemci.mhz
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoDev
                                font.weight: Font.DemiBold
                                color: Tema.metin
                            }
                            Text {
                                text: "MHz"
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinSilik
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: "Maks " + islemci.maxMhz + " MHz"
                            elide: Text.ElideRight
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            color: Tema.metinSilik
                        }
                    }

                    Kadran {
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        implicitWidth: 72
                        implicitHeight: 72
                        deger: islemci.mhz
                        enBuyuk: Math.max(islemci.maxMhz, islemci.mhz, 1)
                        metin: ""
                        baslik: ""
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                Layout.fillHeight: true
                RowLayout {
                    anchors.fill: parent
                    spacing: Tema.bosluk

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Tema.boslukKucuk

                        Text {
                            text: "SICAKLIK"
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            font.letterSpacing: 0.6
                            color: Tema.metinSilik
                        }
                        RowLayout {
                            spacing: 4
                            Text {
                                text: islemci.sicaklik === "-" ? "-" : parseFloat(islemci.sicaklik).toFixed(0)
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoDev
                                font.weight: Font.DemiBold
                                color: Tema.metin
                            }
                            Text {
                                visible: islemci.sicaklik !== "-"
                                text: "C"
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinSilik
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: islemci.sicaklik === "-"
                            text: islemci.sensorAjaniVar ? "Bu donanımda sensör bulunamadı" : "Sensör ajanı çalışmıyor"
                            elide: Text.ElideRight
                            font.family: Tema.aile
                            font.pixelSize: Tema.puntoKucuk
                            color: Tema.metinSilik
                        }
                    }

                    Kadran {
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        implicitWidth: 72
                        implicitHeight: 72
                        deger: islemci.sicaklik === "-" ? 0 : parseFloat(islemci.sicaklik)
                        enBuyuk: 100
                        metin: ""
                        baslik: ""
                        renk: {
                            const s = parseFloat(islemci.sicaklik)
                            if (islemci.sicaklik === "-") return Tema.metinSilik
                            return s >= 80 ? Tema.tehlike : s >= 65 ? Tema.uyari : Tema.basari
                        }
                    }
                }
            }
        }

        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                spacing: Tema.bosluk

                Text {
                    text: "GPU BİLGİLERİ"
                    font.family: Tema.aile
                    font.pixelSize: Tema.puntoKucuk
                    font.letterSpacing: 0.6
                    color: Tema.metinSilik
                }

                GridLayout {
                    id: gpuGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    columns: width < 780 ? 2 : 3
                    columnSpacing: Tema.boslukKucuk
                    rowSpacing: Tema.boslukKucuk

                    Card {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 120
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Tema.boslukKucuk
                            spacing: Tema.boslukKucuk

                            Text {
                                text: "GPU MODELİ"
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                font.letterSpacing: 0.6
                                color: Tema.metinSilik
                            }
                            Text {
                                Layout.fillWidth: true
                                text: gpu0.ad ? gpu0.ad : "-"
                                elide: Text.ElideRight
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoBuyuk
                                font.weight: Font.DemiBold
                                color: Tema.metin
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }

                    Card {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 120
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Tema.boslukKucuk
                            spacing: Tema.boslukKucuk

                            Text {
                                text: "GPU SAAT HIZLARI"
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                font.letterSpacing: 0.6
                                color: Tema.metinSilik
                            }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: Tema.boslukKucuk
                                rowSpacing: 2

                                Text { text: "Çekirdek:"; font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk; color: Tema.metinSilik }
                                Text { Layout.fillWidth: true; text: islemci.gpuCekirdekMhz; elide: Text.ElideRight; font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk; color: Tema.metin }

                                Text { text: "Bellek:"; font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk; color: Tema.metinSilik }
                                Text { Layout.fillWidth: true; text: islemci.gpuBellekMhz; elide: Text.ElideRight; font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk; color: Tema.metin }
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: islemci.gpuCekirdekMhz === "-" && islemci.gpuBellekMhz === "-"
                                text: islemci.sensorAjaniVar ? "Bu donanımda sensör bulunamadı" : "Sensör ajanı çalışmıyor"
                                wrapMode: Text.Wrap
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinSilik
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }

                    Card {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 120
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Tema.boslukKucuk
                            spacing: Tema.boslukKucuk

                            Text {
                                text: "DİĞER BİLGİLER"
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                font.letterSpacing: 0.6
                                color: Tema.metinSilik
                            }
                            Text {
                                Layout.fillWidth: true
                                text: "Sürücü: " + (gpu0.surucuVersiyon ? gpu0.surucuVersiyon : "-") + " - " + (gpu0.bellekGB ? gpu0.bellekGB.toFixed(1) : "0.0") + " GB VRAM"
                                elide: Text.ElideRight
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metin
                            }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: Tema.boslukKucuk
                                rowSpacing: 2

                                Text { text: "OpenGL Versiyon:"; font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk; color: Tema.metinSilik }
                                Text { Layout.fillWidth: true; text: gpu0.openglVersiyon ? gpu0.openglVersiyon : "-"; elide: Text.ElideRight; font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk; color: Tema.metin }

                                Text { text: "Vulkan Versiyon:"; font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk; color: Tema.metinSilik }
                                Text { Layout.fillWidth: true; text: gpu0.vulkanVersiyon ? gpu0.vulkanVersiyon : "-"; elide: Text.ElideRight; font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk; color: Tema.metin }

                                Text { text: "Shader Model:"; font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk; color: Tema.metinSilik }
                                Text { Layout.fillWidth: true; text: gpu0.shaderModel ? gpu0.shaderModel : "-"; elide: Text.ElideRight; font.family: Tema.aile; font.pixelSize: Tema.puntoKucuk; color: Tema.metin }
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }

                    Card {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 120
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Tema.boslukKucuk
                            spacing: Tema.boslukKucuk

                            Text {
                                Layout.fillWidth: true
                                text: "GPU KULLANIMI: " + islemci.gpuKullanim.toFixed(0) + "%"
                                elide: Text.ElideRight
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                font.letterSpacing: 0.6
                                color: Tema.metinSilik
                            }
                            CizgiGrafik {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                veri: islemci.gpuGecmis
                            }
                        }
                    }

                    Card {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredHeight: 120
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Tema.boslukKucuk
                            spacing: Tema.boslukKucuk

                            Text {
                                text: "GPU SICAKLIĞI"
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                font.letterSpacing: 0.6
                                color: Tema.metinSilik
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Tema.boslukKucuk
                                Text {
                                    Layout.fillWidth: true
                                    text: islemci.gpuSicaklik
                                    elide: Text.ElideRight
                                    font.family: Tema.aile
                                    font.pixelSize: Tema.puntoDev
                                    font.weight: Font.DemiBold
                                    color: Tema.metin
                                }
                                Kadran {
                                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                    implicitWidth: 64
                                    implicitHeight: 64
                                    deger: islemci.gpuSicaklik === "-" ? 0 : parseFloat(islemci.gpuSicaklik)
                                    enBuyuk: 100
                                    metin: ""
                                    baslik: ""
                                    renk: {
                                        const s = parseFloat(islemci.gpuSicaklik)
                                        if (islemci.gpuSicaklik === "-") return Tema.metinSilik
                                        return s >= 85 ? Tema.tehlike : s >= 70 ? Tema.uyari : Tema.basari
                                    }
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: islemci.gpuSicaklik === "-"
                                text: islemci.sensorAjaniVar ? "Bu donanımda sensör bulunamadı" : "Sensör ajanı çalışmıyor"
                                wrapMode: Text.Wrap
                                font.family: Tema.aile
                                font.pixelSize: Tema.puntoKucuk
                                color: Tema.metinSilik
                            }
                            Item { Layout.fillHeight: true }
                        }
                    }
                }
            }
        }
    }
}
