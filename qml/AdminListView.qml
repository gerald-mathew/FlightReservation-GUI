import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Item {
    id: view
    property string title: ""
    property var provider: function() { return []; }
    property var rows: []

    function reload() { rows = provider(); }
    Component.onCompleted: reload()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: view.title
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: 28
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            PrimaryButton {
                text: "Refresh"
                baseColor: Theme.surfaceAlt
                onClicked: view.reload()
            }
        }

        GlassCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ListView {
                anchors.fill: parent
                anchors.margins: 12
                clip: true
                model: view.rows
                spacing: 4
                ScrollBar.vertical: ScrollBar {}
                delegate: Rectangle {
                    required property string modelData
                    required property int index
                    width: ListView.view.width
                    height: 40
                    radius: 6
                    color: index % 2 === 0 ? "transparent" : Theme.bg1
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        text: modelData
                        color: Theme.textDim
                        font.family: Theme.fontFamily
                        font.pixelSize: 14
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: view.rows.length === 0
                    text: "Nothing to show yet."
                    color: Theme.textFaint
                    font.family: Theme.fontFamily
                    font.pixelSize: 15
                }
            }
        }
    }
}
