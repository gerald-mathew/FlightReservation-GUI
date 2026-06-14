import QtQuick
import NexoraAirways

Rectangle {
    id: toast
    width: Math.min(parent.width - 48, Math.max(220, label.implicitWidth + 40))
    height: 46
    radius: Theme.radiusSmall
    color: Theme.surfaceAlt
    border.color: Theme.stroke
    border.width: 1
    opacity: 0
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 28
    z: 999

    property alias text: label.text

    Text {
        id: label
        anchors.centerIn: parent
        color: Theme.text
        font.family: Theme.fontFamily
        font.pixelSize: 14
    }

    function show(msg) {
        if (!msg || msg.length === 0) return;
        label.text = msg;
        anim.restart();
    }

    SequentialAnimation {
        id: anim
        NumberAnimation { target: toast; property: "opacity"; to: 1; duration: 180 }
        PauseAnimation { duration: 2600 }
        NumberAnimation { target: toast; property: "opacity"; to: 0; duration: 350 }
    }
}
