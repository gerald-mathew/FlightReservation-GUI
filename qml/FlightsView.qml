import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Item {
    id: view
    property bool adminMode: false

    Component.onCompleted: Backend.refreshFlights()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            text: "Available Flights"
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: 28
            font.bold: true
        }

        // ---- Bookable (Scheduled) flights ----
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 14
            model: Backend.flightsModel
            delegate: flightCard

            ScrollBar.vertical: ScrollBar {}

            Text {
                anchors.centerIn: parent
                visible: Backend.flightsModel.count === 0
                text: "No flights open for booking right now."
                color: Theme.textFaint
                font.family: Theme.fontFamily
                font.pixelSize: 16
            }
        }

        // ---- Boarding & departed flights (not bookable) ----
        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 280
            spacing: 10
            visible: Backend.departedFlightsModel.count > 0

            Text {
                text: "Boarding & departed flights"
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: 20
                font.bold: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 14
                model: Backend.departedFlightsModel
                delegate: flightCard

                ScrollBar.vertical: ScrollBar {}
            }
        }
    }

    // ---- Shared flight card, used by both lists ----
    // Whether a card is bookable is driven entirely by its `status` role, so the
    // same delegate renders a "Select seat" action for Scheduled flights and a
    // read-only status badge for boarding/departed ones.
    Component {
        id: flightCard

        GlassCard {
            required property string flightId
            required property string airline
            required property string origin
            required property string destination
            required property string originName
            required property string destinationName
            required property string departure
            required property string arrival
            required property double price
            required property int availableSeats
            required property int totalSeats
            required property string status

            readonly property bool bookable: status === "Scheduled"

            width: ListView.view.width
            height: 132
            opacity: bookable ? 1.0 : 0.78

            RowLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 18

                // Route block
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    RowLayout {
                        spacing: 12
                        Text {
                            text: origin
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: 30
                            font.bold: true
                        }
                        Text {
                            text: "→"
                            color: Theme.accentSoft
                            font.pixelSize: 26
                        }
                        Text {
                            text: destination
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: 30
                            font.bold: true
                        }
                        Rectangle {
                            Layout.leftMargin: 8
                            radius: 6
                            color: Theme.surfaceAlt
                            implicitWidth: idLabel.implicitWidth + 16
                            implicitHeight: 24
                            Text {
                                id: idLabel
                                anchors.centerIn: parent
                                text: flightId + " · " + airline
                                color: Theme.textDim
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                            }
                        }
                        // Status badge (only for non-bookable flights)
                        Rectangle {
                            visible: !bookable
                            radius: 6
                            color: status === "Departed" ? Theme.danger : Theme.gold
                            implicitWidth: stLabel.implicitWidth + 16
                            implicitHeight: 24
                            Text {
                                id: stLabel
                                anchors.centerIn: parent
                                text: status
                                color: "#1b1300"
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                                font.bold: true
                            }
                        }
                    }
                    Text {
                        text: originName + "  —  " + destinationName
                        color: Theme.textFaint
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    Text {
                        text: "Departs " + departure + "   ·   Arrives " + arrival
                        color: Theme.textDim
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                    }
                }

                // Price + action
                ColumnLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 8
                    Text {
                        text: "from NGN " + price.toFixed(2)
                        color: Theme.gold
                        font.family: Theme.fontFamily
                        font.pixelSize: 20
                        font.bold: true
                        horizontalAlignment: Text.AlignRight
                        Layout.alignment: Qt.AlignRight
                    }
                    Text {
                        text: bookable ? (availableSeats + " / " + totalSeats + " seats free")
                                       : (status === "Departed" ? "Departed" : "Boarding — closed")
                        color: bookable ? (availableSeats > 0 ? Theme.success : Theme.danger)
                                        : Theme.textFaint
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        Layout.alignment: Qt.AlignRight
                    }
                    // Customers only act on bookable flights; admins may inspect
                    // the seat map of any flight.
                    PrimaryButton {
                        text: view.adminMode ? "View seats" : "Select seat"
                        visible: bookable || view.adminMode
                        Layout.alignment: Qt.AlignRight
                        onClicked: {
                            seatDialog.flightId = flightId;
                            seatDialog.routeLabel = origin + " → " + destination + "  (" + flightId + ")";
                            Backend.loadSeats(flightId);
                            seatDialog.open();
                        }
                    }
                }
            }
        }
    }

    SeatMapDialog {
        id: seatDialog
        adminMode: view.adminMode
    }
}
