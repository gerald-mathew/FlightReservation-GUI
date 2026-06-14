import QtQuick
import QtQuick.Controls.Basic
import NexoraAirways

Button {
    id: control
    property color baseColor: Theme.accent
    property color textColor: "white"
    implicitHeight: 46
    implicitWidth: Math.max(120, contentItem.implicitWidth + 40)
    font.family: Theme.fontFamily
    font.pixelSize: 15
    font.bold: true

    contentItem: Text {
        text: control.text
        color: control.enabled ? control.textColor : Theme.textFaint
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: !control.enabled ? Theme.surfaceAlt
               : control.down ? Qt.darker(control.baseColor, 1.2)
               : control.hovered ? Qt.lighter(control.baseColor, 1.12)
               : control.baseColor
        Behavior on color { ColorAnimation { duration: 120 } }
    }
}
