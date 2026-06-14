import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Dialog {
    id: dialog
    property string flightId: ""
    property string routeLabel: ""
    property bool adminMode: false
    property string selectedSeat: ""
    property double selectedPrice: 0
    property string selectedCabin: ""

    modal: true
    anchors.centerIn: parent
    width: Math.min(720, parent ? parent.width - 80 : 720)
    height: Math.min(680, parent ? parent.height - 80 : 680)
    padding: 0

    background: GlassCard {}

    function cabinColor(c) {
        if (c === "Business") return Theme.business;
        if (c === "FirstClass") return Theme.firstClass;
        return Theme.economy;
    }

    contentItem: ColumnLayout {
        spacing: 14

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 20
            Layout.bottomMargin: 0
            ColumnLayout {
                spacing: 2
                Text {
                    text: "Choose your seat"
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 22
                    font.bold: true
                }
                Text {
                    text: dialog.routeLabel
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                }
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                Layout.preferredWidth: 34; Layout.preferredHeight: 34; radius: 17
                color: Theme.surfaceAlt
                Text { anchors.centerIn: parent; text: "✕"; color: Theme.textDim; font.pixelSize: 16 }
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: dialog.close() }
            }
        }

        // Legend
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            spacing: 16
            Repeater {
                model: [
                    { c: Theme.economy, t: "Economy" },
                    { c: Theme.business, t: "Business" },
                    { c: Theme.firstClass, t: "First" },
                    { c: Theme.surfaceAlt, t: "Booked" }
                ]
                delegate: Row {
                    required property var modelData
                    spacing: 6
                    Rectangle { width: 14; height: 14; radius: 4; color: modelData.c; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: modelData.t; color: Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                }
            }
        }

        // Seat grid
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            clip: true

            GridView {
                id: grid
                anchors.fill: parent
                cellWidth: 60
                cellHeight: 60
                model: Backend.seatsModel
                interactive: true

                delegate: Item {
                    required property string seatNumber
                    required property string cabin
                    required property bool booked
                    required property double price
                    width: grid.cellWidth
                    height: grid.cellHeight

                    Rectangle {
                        anchors.centerIn: parent
                        width: 48; height: 48
                        radius: 10
                        color: booked ? Theme.surfaceAlt
                               : dialog.selectedSeat === seatNumber ? Theme.accent
                               : dialog.cabinColor(cabin)
                        opacity: booked ? 0.55 : 1
                        border.width: dialog.selectedSeat === seatNumber ? 2 : 0
                        border.color: "white"

                        Text {
                            anchors.centerIn: parent
                            text: seatNumber
                            color: "white"
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !dialog.adminMode && !booked
                            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: {
                                dialog.selectedSeat = seatNumber;
                                dialog.selectedPrice = price;
                                dialog.selectedCabin = cabin;
                            }
                        }
                        Behavior on color { ColorAnimation { duration: 100 } }
                    }
                }
            }
        }

        // Footer action
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 20
            Layout.topMargin: 4
            visible: !dialog.adminMode

            ColumnLayout {
                spacing: 0
                Text {
                    text: dialog.selectedSeat === "" ? "No seat selected"
                          : "Seat " + dialog.selectedSeat + " · " + dialog.selectedCabin
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 15
                    font.bold: true
                }
                Text {
                    visible: dialog.selectedSeat !== ""
                    text: "NGN " + dialog.selectedPrice.toFixed(2)
                    color: Theme.gold
                    font.family: Theme.fontFamily
                    font.pixelSize: 14
                }
            }
            Item { Layout.fillWidth: true }
            PrimaryButton {
                text: "Confirm booking"
                enabled: dialog.selectedSeat !== ""
                onClicked: {
                    if (Backend.bookSeat(dialog.flightId, dialog.selectedSeat)) {
                        dialog.selectedSeat = "";
                        dialog.close();
                    }
                }
            }
        }
    }

    onClosed: {
        selectedSeat = "";
        selectedPrice = 0;
        selectedCabin = "";
    }
}
