import QtQuick
import QtQuick.Controls.Basic
import NexoraAirways

TextField {
    id: control
    implicitHeight: 46
    color: Theme.text
    font.family: Theme.fontFamily
    font.pixelSize: 15
    placeholderTextColor: Theme.textFaint
    selectionColor: Theme.accent
    leftPadding: 14
    rightPadding: 14

    background: Rectangle {
        radius: Theme.radiusSmall
        color: Theme.bg1
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? Theme.accent : Theme.stroke
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }
}
