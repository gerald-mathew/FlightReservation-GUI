import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Item {
    id: view

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            text: "Manage Flights"
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: 28
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            // Add / edit flight
            GlassCard {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 3

                ScrollView {
                    id: editScroll
                    anchors.fill: parent
                    anchors.margins: 20
                    clip: true

                    ColumnLayout {
                        width: editScroll.availableWidth
                        spacing: 10

                        Text { text: "Add or edit a flight"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 18; font.bold: true }
                        GridLayout {
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 10
                            Layout.fillWidth: true

                            StyledField { id: fId; Layout.fillWidth: true; placeholderText: "Flight ID (e.g. NX101)" }
                            StyledField { id: fAirline; Layout.fillWidth: true; placeholderText: "Airline" }
                            StyledField { id: fOrigin; Layout.fillWidth: true; placeholderText: "Origin code (e.g. LOS)" }
                            StyledField { id: fDest; Layout.fillWidth: true; placeholderText: "Destination code (e.g. ABV)" }
                            StyledField { id: fDep; Layout.fillWidth: true; placeholderText: "Departure (YYYY-MM-DD HH:MM)" }
                            StyledField { id: fArr; Layout.fillWidth: true; placeholderText: "Arrival (YYYY-MM-DD HH:MM)" }
                            StyledField { id: fSeats; Layout.fillWidth: true; placeholderText: "Total seats (1-500)"; validator: IntValidator { bottom: 1; top: 500 } }
                            StyledField { id: fPrice; Layout.fillWidth: true; placeholderText: "Base price (NGN)"; validator: DoubleValidator { bottom: 0 } }
                        }
                        PrimaryButton {
                            text: "Save flight"
                            Layout.fillWidth: true
                            onClicked: {
                                if (Backend.adminSaveFlight(fId.text, fAirline.text, fOrigin.text, fDest.text,
                                                            fDep.text, fArr.text,
                                                            parseInt(fSeats.text), parseFloat(fPrice.text))) {
                                    fId.text = fAirline.text = fOrigin.text = fDest.text = "";
                                    fDep.text = fArr.text = fSeats.text = fPrice.text = "";
                                }
                            }
                        }
                        Text {
                            text: "Existing flight ID = edit (blank fields aren't allowed). Valid airport codes: " + Backend.airportCodes().join(", ")
                            color: Theme.textFaint
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // Right column: price + seat availability
            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                spacing: 14

                // Set price
                GlassCard {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 220
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 10
                        Text { text: "Update price"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 18; font.bold: true }
                        StyledField { id: pId; Layout.fillWidth: true; placeholderText: "Flight ID" }
                        StyledField { id: pPrice; Layout.fillWidth: true; placeholderText: "New price (NGN)"; validator: DoubleValidator { bottom: 0 } }
                        PrimaryButton {
                            text: "Set price"
                            Layout.fillWidth: true
                            baseColor: Theme.gold
                            textColor: "#1b1300"
                            onClicked: { if (Backend.adminSetPrice(pId.text, parseFloat(pPrice.text))) { pId.text = ""; pPrice.text = ""; } }
                        }
                        Item { Layout.fillHeight: true }
                    }
                }

                // Seat availability
                GlassCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 10
                        Text { text: "Seat availability"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 18; font.bold: true }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            StyledField { id: sId; Layout.fillWidth: true; placeholderText: "Flight ID" }
                            PrimaryButton {
                                text: "Check"
                                baseColor: Theme.surfaceAlt
                                onClicked: seatInfo.text = sId.text.length ? Backend.seatSummary(sId.text) : "Enter a flight ID."
                            }
                        }
                        Text {
                            id: seatInfo
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            text: "Enter a flight ID to view its cabin breakdown."
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignTop
                        }
                    }
                }
            }
        }
    }
}
