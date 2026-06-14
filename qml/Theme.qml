pragma Singleton
import QtQuick

QtObject {
    // Background palette (deep navy gradient)
    readonly property color bg0: "#0b1020"
    readonly property color bg1: "#121933"
    readonly property color surface: "#1b2447"
    readonly property color surfaceAlt: "#232e57"
    readonly property color stroke: "#2f3b6b"

    // Accents
    readonly property color accent: "#4f7cff"
    readonly property color accentSoft: "#6d93ff"
    readonly property color gold: "#f5b945"
    readonly property color success: "#36d399"
    readonly property color danger: "#f87272"

    // Text
    readonly property color text: "#eef2ff"
    readonly property color textDim: "#9aa6d4"
    readonly property color textFaint: "#6b769f"

    // Cabin colors
    readonly property color economy: "#3aa0a0"
    readonly property color business: "#7a5cff"
    readonly property color firstClass: "#f5b945"

    readonly property int radius: 14
    readonly property int radiusSmall: 9
    readonly property int pad: 18

    readonly property string fontFamily: "Segoe UI"
}
