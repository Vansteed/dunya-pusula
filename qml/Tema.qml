// Tema.qml - tum renk/olcu sabitlerinin TEK kaynagi (QML singleton).
// Modern koyu tema: katmanli yuzeyler, yumusak koseler, golge yerine
// kenar + katman farki (golge ekstra modul ister, eklemiyoruz).
pragma Singleton
import QtQuick

QtObject {
    // Zeminler
    readonly property color zemin: "#0B0B0F"
    readonly property color yuzey: "#15161B"
    readonly property color yuzeyYuksek: "#1C1D24"
    readonly property color yuzeyVurgu: "#22232B"

    // Kenar
    readonly property color kenar: "#26272F"
    readonly property color kenarSilik: "#1E1F26"

    // Metin
    readonly property color metin: "#F2F3F5"
    readonly property color metinIkincil: "#9BA1AC"
    readonly property color metinSilik: "#6B7280"

    // Vurgu ve durum
    readonly property color vurgu: "#5B8DEF"
    readonly property color vurguKoyu: "#3D6FD1"
    readonly property color basari: "#4ED4A0"
    readonly property color uyari: "#F2C05B"
    readonly property color tehlike: "#F2685B"

    // Olculer
    readonly property int koseKart: 14
    readonly property int koseDugme: 10
    readonly property int koseKucuk: 8
    readonly property int bosluk: 12
    readonly property int boslukKucuk: 8
    readonly property int baslikYukseklik: 44

    // Tipografi
    readonly property string aile: "Segoe UI"
    readonly property int puntoDev: 34
    readonly property int puntoBuyuk: 20
    readonly property int puntoOrta: 13
    readonly property int punto: 12
    readonly property int puntoKucuk: 11
}
