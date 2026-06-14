import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Item {
    id: view

    property string verifyResult: ""

    Connections {
        target: Backend
        function onPaystackVerifyResult(success, detail) { view.verifyResult = detail; }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            text: "Payments & Account"
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: 28
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            // Confirm payment
            GlassCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12
                    Text { text: "Confirm a payment"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 18; font.bold: true }
                    Text {
                        text: "Verify a Paystack transaction by reference. This checks status only — it does not credit any account."
                        color: Theme.textFaint
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    StyledField { id: refField; Layout.fillWidth: true; placeholderText: "Paystack reference" }
                    PrimaryButton {
                        text: "Verify"
                        Layout.fillWidth: true
                        enabled: !Backend.paystackBusy
                        onClicked: { view.verifyResult = ""; Backend.adminVerifyPayment(refField.text); }
                    }
                    Text {
                        visible: Backend.paystackBusy
                        text: "Verifying…"
                        color: Theme.gold
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                    }
                    Text {
                        visible: view.verifyResult.length > 0
                        text: view.verifyResult
                        color: Theme.textDim
                        font.family: Theme.fontFamily
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // My admin account
            GlassCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                ScrollView {
                    id: acctScroll
                    anchors.fill: parent
                    anchors.margins: 20
                    clip: true
                    ColumnLayout {
                        width: acctScroll.availableWidth
                        spacing: 12
                        Text { text: "My account"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 18; font.bold: true }

                        StyledField { id: aName; Layout.fillWidth: true; placeholderText: "Display name"; text: Backend.displayName }
                        StyledField { id: aEmail; Layout.fillWidth: true; placeholderText: "Email"; text: Backend.email }
                        StyledField { id: aPhone; Layout.fillWidth: true; placeholderText: "Phone (10-20 digits)"; text: Backend.phone }
                        StyledField { id: aGender; Layout.fillWidth: true; placeholderText: "Gender (M/F)"; text: Backend.gender; maximumLength: 1 }
                        StyledField { id: aAddress; Layout.fillWidth: true; placeholderText: "Address"; text: Backend.address }
                        StyledField { id: aDept; Layout.fillWidth: true; placeholderText: "Department"; text: Backend.department }
                        PrimaryButton {
                            text: "Save account"
                            Layout.fillWidth: true
                            onClicked: Backend.adminUpdateProfile(aName.text, aEmail.text, aPhone.text,
                                                                  aGender.text, aAddress.text, aDept.text)
                        }

                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.stroke }

                        Text { text: "Change password"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 16; font.bold: true }
                        StyledField { id: aCurPw; Layout.fillWidth: true; placeholderText: "Current password"; echoMode: TextInput.Password }
                        StyledField { id: aNewPw; Layout.fillWidth: true; placeholderText: "New password (min 8 chars)"; echoMode: TextInput.Password }
                        PrimaryButton {
                            text: "Update password"
                            Layout.fillWidth: true
                            onClicked: { if (Backend.adminChangePassword(aCurPw.text, aNewPw.text)) { aCurPw.text = ""; aNewPw.text = ""; } }
                        }
                    }
                }
            }
        }
    }
}
