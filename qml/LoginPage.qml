import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Item {
    id: page

    property bool registerMode: false

    // Left brand panel
    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.46
            color: "transparent"

            Column {
                anchors.centerIn: parent
                width: parent.width - 120
                spacing: 18
                Row{
                    spacing: 10
                    Image {
                        width: 64; height: 64
                        source: "../icons/nexora.png"
                    }
                    Text {
                        text: "Nexora Airways"
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: 40
                        font.bold: true
                    }
                }

                Text {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: "Book your next journey across Nigeria. Real-time seat maps, instant confirmation, and a wallet built in."
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 17
                    lineHeight: 1.25
                }
            }
        }

        // Right auth card
        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            GlassCard {
                anchors.centerIn: parent
                width: Math.min(420, parent.width - 80)
                height: contentCol.implicitHeight + 56

                Column {
                    id: contentCol
                    anchors.centerIn: parent
                    width: parent.width - 56
                    spacing: 16

                    Text {
                        text: page.registerMode ? "Create your account" : "Welcome back"
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: 24
                        font.bold: true
                    }

                    StyledField {
                        id: usernameField
                        width: parent.width
                        placeholderText: "Username"
                    }

                    StyledField {
                        id: passwordField
                        width: parent.width
                        placeholderText: "Password"
                        echoMode: TextInput.Password
                        onAccepted: submit()
                    }

                    // Admin toggle (login only)
                    Row {
                        visible: !page.registerMode
                        spacing: 10
                        CheckBox {
                            id: adminCheck
                            indicator: Rectangle {
                                implicitWidth: 20; implicitHeight: 20
                                radius: 5
                                x: 0; y: parent.height/2 - height/2
                                color: adminCheck.checked ? Theme.accent : Theme.bg1
                                border.color: Theme.stroke
                                Text {
                                    anchors.centerIn: parent
                                    text: "✓"; color: "white"; font.pixelSize: 14
                                    visible: adminCheck.checked
                                }
                            }
                            contentItem: Text {
                                text: "Sign in as administrator"
                                color: Theme.textDim
                                font.family: Theme.fontFamily
                                font.pixelSize: 14
                                leftPadding: 28
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    PrimaryButton {
                        width: parent.width
                        text: page.registerMode ? "Create account" : "Sign in"
                        onClicked: page.submit()
                    }

                    Row {
                        spacing: 6
                        anchors.horizontalCenter: parent.horizontalCenter
                        Text {
                            text: page.registerMode ? "Already have an account?" : "New to Nexora?"
                            color: Theme.textFaint
                            font.family: Theme.fontFamily
                            font.pixelSize: 14
                        }
                        Text {
                            text: page.registerMode ? "Sign in" : "Create one"
                            color: Theme.accentSoft
                            font.family: Theme.fontFamily
                            font.pixelSize: 14
                            font.bold: true
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    page.registerMode = !page.registerMode;
                                    adminCheck.checked = false;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    function submit() {
        if (page.registerMode) {
            if (Backend.registerCustomer(usernameField.text, passwordField.text)) {
                page.registerMode = false;
                passwordField.text = "";
            }
        } else {
            Backend.login(usernameField.text, passwordField.text, adminCheck.checked);
        }
    }
}
