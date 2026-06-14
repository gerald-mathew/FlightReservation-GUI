import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Item {
    id: view
    Component.onCompleted: Backend.refreshBookings()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            text: "My Bookings"
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: 28
            font.bold: true
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 14
            model: Backend.bookingsModel
            ScrollBar.vertical: ScrollBar {}

            delegate: GlassCard {
                required property string flightId
                required property string airline
                required property string origin
                required property string destination
                required property string departure
                required property string arrival

                width: ListView.view.width
                height: 110

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 18

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        RowLayout {
                            spacing: 10
                            Text {
                                text: origin + " → " + destination
                                color: Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: 24
                                font.bold: true
                            }
                            Rectangle {
                                radius: 6; color: Theme.surfaceAlt
                                implicitWidth: t.implicitWidth + 16; implicitHeight: 24
                                Text { id: t; anchors.centerIn: parent; text: flightId + " · " + airline
                                       color: Theme.textDim; font.family: Theme.fontFamily; font.pixelSize: 12 }
                            }
                        }
                        Text {
                            text: "Departs " + departure + "   ·   Arrives " + arrival
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 13
                        }
                    }

                    PrimaryButton {
                        text: "Cancel"
                        baseColor: Theme.danger
                        onClicked: {
                            cancelDialog.flightId = flightId;
                            cancelDialog.routeLabel = origin + " → " + destination + " (" + flightId + ")";
                            Backend.loadSeats(flightId);
                            cancelDialog.open();
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: Backend.bookingsModel.count === 0
                text: "You have no booked flights yet."
                color: Theme.textFaint
                font.family: Theme.fontFamily
                font.pixelSize: 16
            }
        }
    }

    // Cancel: pick the booked seat to release
    Dialog {
        id: cancelDialog
        property string flightId: ""
        property string routeLabel: ""
        modal: true
        anchors.centerIn: parent
        width: 420
        padding: 0
        background: GlassCard {}

        contentItem: ColumnLayout {
            spacing: 14
            Text {
                Layout.margins: 20
                Layout.bottomMargin: 0
                text: "Cancel a seat"
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: 20
                font.bold: true
            }
            Text {
                Layout.leftMargin: 20; Layout.rightMargin: 20
                text: cancelDialog.routeLabel
                color: Theme.textDim
                font.family: Theme.fontFamily
                font.pixelSize: 13
            }
            StyledField {
                id: seatInput
                Layout.fillWidth: true
                Layout.leftMargin: 20; Layout.rightMargin: 20
                placeholderText: "Seat number (e.g. 1A)"
            }
            RowLayout {
                Layout.margins: 20
                Layout.topMargin: 4
                Item { Layout.fillWidth: true }
                PrimaryButton {
                    text: "Keep"
                    baseColor: Theme.surfaceAlt
                    onClicked: cancelDialog.close()
                }
                PrimaryButton {
                    text: "Cancel seat"
                    baseColor: Theme.danger
                    onClicked: {
                        if (Backend.cancelSeat(cancelDialog.flightId, seatInput.text.toUpperCase())) {
                            seatInput.text = "";
                            cancelDialog.close();
                        }
                    }
                }
            }
        }
    }
}
