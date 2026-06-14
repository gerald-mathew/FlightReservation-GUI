import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Item {
    id: home

    // currentIndex selects the active section
    property int currentIndex: 0
    readonly property var customerNav: ["Flights", "My Bookings", "Wallet", "Account"]
    readonly property var adminBaseNav: ["Flights", "Manage Flights", "All Bookings", "Customers", "Payments"]
    readonly property var adminNav: Backend.isMaxAdmin ? adminBaseNav.concat(["Super Admin"]) : adminBaseNav
    readonly property var navItems: Backend.isAdmin ? adminNav : customerNav

    // Map a nav label to the component that renders it.
    function componentFor(label) {
        switch (label) {
            case "Flights":       return Backend.isAdmin ? adminFlights : flightsView;
            case "My Bookings":   return bookingsView;
            case "Wallet":        return walletView;
            case "Account":       return accountView;
            case "Manage Flights":return manageFlights;
            case "All Bookings":  return allBookings;
            case "Customers":     return customersView;
            case "Payments":      return paymentsView;
            case "Super Admin":   return superAdminView;
        }
        return flightsView;
    }

    // Keep the selection valid when the nav set changes (e.g. role switch).
    onNavItemsChanged: if (currentIndex >= navItems.length) currentIndex = 0

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 240
            color: Theme.bg1
            border.color: Theme.stroke
            border.width: 0

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 8

                // Brand
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Rectangle {
                        Layout.preferredWidth: 40; Layout.preferredHeight: 40; radius: 12
                        color: Theme.accent
                        Text { anchors.centerIn: parent; text: "✈"; color: "white"; font.pixelSize: 22 }
                    }
                    Text {
                        text: "Nexora"
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: 22
                        font.bold: true
                    }
                }

                Item { Layout.preferredHeight: 12 }

                Repeater {
                    model: home.navItems
                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 46
                        radius: Theme.radiusSmall
                        color: home.currentIndex === index ? Theme.accent : "transparent"
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            text: modelData
                            color: home.currentIndex === index ? "white" : Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 16
                            font.bold: home.currentIndex === index
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: home.currentIndex = index
                        }
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                }

                Item { Layout.fillHeight: true }

                // User chip
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    radius: Theme.radiusSmall
                    color: Theme.surface
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10
                        Rectangle {
                            Layout.preferredWidth: 38; Layout.preferredHeight: 38; radius: 19
                            color: Theme.accentSoft
                            Text {
                                anchors.centerIn: parent
                                text: (Backend.displayName.length > 0 ? Backend.displayName : Backend.currentUser).charAt(0).toUpperCase()
                                color: "white"; font.pixelSize: 18; font.bold: true
                            }
                        }
                        ColumnLayout {
                            spacing: 0
                            Text {
                                text: Backend.currentUser
                                color: Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: 14
                                font.bold: true
                                elide: Text.ElideRight
                                Layout.maximumWidth: 110
                            }
                            Text {
                                text: Backend.isAdmin ? ("Admin · " + Backend.adminLevel) : "Customer"
                                color: Theme.textFaint
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                            }
                        }
                    }
                }

                PrimaryButton {
                    Layout.fillWidth: true
                    text: "Log out"
                    baseColor: Theme.surfaceAlt
                    onClicked: Backend.logout()
                }
            }
        }

        // Content
        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            StackLayout {
                anchors.fill: parent
                anchors.margins: 24
                currentIndex: home.currentIndex

                Repeater {
                    model: home.navItems
                    Loader {
                        required property string modelData
                        active: true
                        sourceComponent: home.componentFor(modelData)
                    }
                }
            }
        }
    }

    // Customer views
    Component { id: flightsView; FlightsView {} }
    Component { id: bookingsView; BookingsView {} }
    Component { id: walletView; WalletView {} }
    Component { id: accountView; AccountView {} }

    // Admin views
    Component { id: adminFlights; FlightsView { adminMode: true } }
    Component { id: manageFlights; ManageFlightsView {} }
    Component { id: allBookings; AdminListView { title: "All Bookings"; provider: function() { return Backend.adminAllBookings(); } } }
    Component { id: customersView; AdminListView { title: "Customers"; provider: function() { return Backend.adminCustomers(); } } }
    Component { id: paymentsView; PaymentsView {} }
    Component { id: superAdminView; SuperAdminView {} }
}
