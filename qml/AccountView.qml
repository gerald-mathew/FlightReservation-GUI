import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Item {
    id: view

    property var nok: ["", "", "", ""]
    function reloadNok() { nok = Backend.nextOfKin(); }
    Component.onCompleted: reloadNok()

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            text: "Account"
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: 28
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            // Profile
            GlassCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12
                    Text { text: "Profile details"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 18; font.bold: true }

                    StyledField { id: nameField; Layout.fillWidth: true; placeholderText: "Display name"; text: Backend.displayName }
                    StyledField { id: emailField; Layout.fillWidth: true; placeholderText: "Email"; text: Backend.email }
                    StyledField { id: phoneField; Layout.fillWidth: true; placeholderText: "Phone (10-20 digits)"; text: Backend.phone }
                    StyledField { id: genderField; Layout.fillWidth: true; placeholderText: "Gender (M/F)"; text: Backend.gender; maximumLength: 1 }
                    StyledField { id: addressField; Layout.fillWidth: true; placeholderText: "Address"; text: Backend.address }

                    PrimaryButton {
                        text: "Save profile"
                        Layout.fillWidth: true
                        onClicked: Backend.updateProfile(nameField.text, emailField.text, phoneField.text,
                                                         genderField.text, addressField.text)
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // Next of kin
            GlassCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12
                    Text { text: "Next of kin"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 18; font.bold: true }

                    StyledField { id: nokName; Layout.fillWidth: true; placeholderText: "Name"; text: view.nok[0] }
                    StyledField { id: nokRel; Layout.fillWidth: true; placeholderText: "Relationship"; text: view.nok[1] }
                    StyledField { id: nokPhone; Layout.fillWidth: true; placeholderText: "Phone"; text: view.nok[2] }
                    StyledField { id: nokAddr; Layout.fillWidth: true; placeholderText: "Address"; text: view.nok[3] }

                    PrimaryButton {
                        text: "Save next of kin"
                        Layout.fillWidth: true
                        onClicked: { if (Backend.updateNextOfKin(nokName.text, nokRel.text, nokPhone.text, nokAddr.text)) view.reloadNok(); }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // Password
            GlassCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12
                    Text { text: "Change password"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 18; font.bold: true }

                    StyledField { id: currentPw; Layout.fillWidth: true; placeholderText: "Current password"; echoMode: TextInput.Password }
                    StyledField { id: newPw; Layout.fillWidth: true; placeholderText: "New password (min 8 chars)"; echoMode: TextInput.Password }

                    PrimaryButton {
                        text: "Update password"
                        Layout.fillWidth: true
                        onClicked: {
                            if (Backend.changePassword(currentPw.text, newPw.text)) {
                                currentPw.text = ""; newPw.text = "";
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
