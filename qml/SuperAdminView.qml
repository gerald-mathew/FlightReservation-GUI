import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Item {
    id: view

    property var admins: []
    property var txns: []
    function reloadAdmins() { admins = Backend.adminList(); }
    Component.onCompleted: reloadAdmins()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            text: "Super Admin"
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: 28
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            // Left: action cards (scrollable)
            GlassCard {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                ScrollView {
                    id: actionScroll
                    anchors.fill: parent
                    anchors.margins: 20
                    clip: true
                    ColumnLayout {
                        width: actionScroll.availableWidth
                        spacing: 16

                        // Create admin
                        Text { text: "Create admin account"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 16; font.bold: true }
                        StyledField { id: newAdminUser; Layout.fillWidth: true; placeholderText: "New admin username" }
                        StyledField { id: newAdminPw; Layout.fillWidth: true; placeholderText: "Password (min 8 chars)"; echoMode: TextInput.Password }
                        PrimaryButton {
                            text: "Create admin"
                            Layout.fillWidth: true
                            onClicked: {
                                if (Backend.createAdmin(newAdminUser.text, newAdminPw.text)) {
                                    newAdminUser.text = ""; newAdminPw.text = ""; view.reloadAdmins();
                                }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.stroke }

                        // Edit admin level
                        Text { text: "Edit admin level"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 16; font.bold: true }
                        StyledField { id: levelUser; Layout.fillWidth: true; placeholderText: "Admin username" }
                        StyledField { id: levelValue; Layout.fillWidth: true; placeholderText: "New level (e.g. Level 1, MAX)" }
                        PrimaryButton {
                            text: "Set level"
                            Layout.fillWidth: true
                            baseColor: Theme.gold
                            textColor: "#1b1300"
                            onClicked: {
                                if (Backend.setAdminLevelFor(levelUser.text, levelValue.text)) {
                                    levelUser.text = ""; levelValue.text = ""; view.reloadAdmins();
                                }
                            }
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.stroke }

                        // Delete account
                        Text { text: "Delete account"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 16; font.bold: true }
                        StyledField { id: delUser; Layout.fillWidth: true; placeholderText: "Username to delete" }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            PrimaryButton {
                                Layout.fillWidth: true
                                text: "Delete customer"
                                baseColor: Theme.danger
                                onClicked: { if (Backend.deleteCustomerAccount(delUser.text)) delUser.text = ""; }
                            }
                            PrimaryButton {
                                Layout.fillWidth: true
                                text: "Delete admin"
                                baseColor: Theme.danger
                                onClicked: { if (Backend.deleteAdminAccount(delUser.text)) { delUser.text = ""; view.reloadAdmins(); } }
                            }
                        }
                    }
                }
            }

            // Right: admins list + customer transactions
            ColumnLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                spacing: 14

                // Admins
                GlassCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 8
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Admin accounts"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 16; font.bold: true }
                            Item { Layout.fillWidth: true }
                            PrimaryButton { text: "Refresh"; baseColor: Theme.surfaceAlt; onClicked: view.reloadAdmins() }
                        }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: view.admins
                            spacing: 4
                            ScrollBar.vertical: ScrollBar {}
                            delegate: Rectangle {
                                required property string modelData
                                required property int index
                                width: ListView.view.width
                                height: 34
                                radius: 6
                                color: index % 2 === 0 ? "transparent" : Theme.bg1
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.leftMargin: 10
                                    text: modelData
                                    color: Theme.textDim
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
                }

                // Customer transactions
                GlassCard {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 8
                        Text { text: "Customer transactions"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 16; font.bold: true }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            StyledField { id: txUser; Layout.fillWidth: true; placeholderText: "Username (blank = all)" }
                            PrimaryButton { text: "Load"; baseColor: Theme.surfaceAlt; onClicked: view.txns = Backend.customerTransactions(txUser.text) }
                        }
                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: view.txns
                            spacing: 4
                            ScrollBar.vertical: ScrollBar {}
                            delegate: Rectangle {
                                required property string modelData
                                required property int index
                                width: ListView.view.width
                                height: 32
                                radius: 6
                                color: index % 2 === 0 ? "transparent" : Theme.bg1
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    anchors.left: parent.left
                                    anchors.leftMargin: 10
                                    text: modelData
                                    color: Theme.textDim
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 13
                                }
                            }
                            Text {
                                anchors.centerIn: parent
                                visible: view.txns.length === 0
                                text: "Load a customer's transactions."
                                color: Theme.textFaint
                                font.family: Theme.fontFamily
                                font.pixelSize: 14
                            }
                        }
                    }
                }
            }
        }
    }
}
