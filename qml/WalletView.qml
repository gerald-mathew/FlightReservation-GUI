import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import NexoraAirways
import "components"

Item {
    id: view

    property var history: []
    function reloadHistory() { history = Backend.transactionHistory(); }
    Component.onCompleted: reloadHistory()

    Connections {
        target: Backend
        function onProfileChanged() { view.reloadHistory(); }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            text: "Wallet"
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: 28
            font.bold: true
        }

        // Balance card
        GlassCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            color: Theme.accent
            border.width: 0
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 22
                spacing: 4
                Text { text: "Nexora Account Balance"; color: "#dce4ff"; font.family: Theme.fontFamily; font.pixelSize: 14 }
                Text {
                    text: "NGN " + Backend.balance.toFixed(2)
                    color: "white"
                    font.family: Theme.fontFamily
                    font.pixelSize: 40
                    font.bold: true
                }
                Text {
                    text: Backend.loyaltyPoints.toFixed(0) + " loyalty points · " + Backend.totalBookings + " bookings"
                    color: "#dce4ff"
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                }
            }
        }

        // Actions row
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 240
            spacing: 14

            // Deposit / Withdraw
            GlassCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 10
                    Text { text: "Add or remove funds"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 16; font.bold: true }
                    StyledField {
                        id: amountField
                        Layout.fillWidth: true
                        placeholderText: "Withdrawal amount (NGN)"
                        validator: DoubleValidator { bottom: 0 }
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        PrimaryButton {
                            Layout.fillWidth: true
                            text: "Deposit (Paystack)"
                            baseColor: Theme.success
                            onClicked: depositDialog.open()
                        }
                        PrimaryButton {
                            Layout.fillWidth: true
                            text: "Withdraw"
                            baseColor: Theme.surfaceAlt
                            onClicked: { if (Backend.withdraw(parseFloat(amountField.text))) amountField.text = ""; }
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "Deposits are processed securely through Paystack."
                        color: Theme.textFaint
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // Transfer
            GlassCard {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 10
                    Text { text: "Transfer to another account"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 16; font.bold: true }
                    StyledField { id: toUserField; Layout.fillWidth: true; placeholderText: "Recipient username" }
                    StyledField {
                        id: transferAmount
                        Layout.fillWidth: true
                        placeholderText: "Amount (NGN)"
                        validator: DoubleValidator { bottom: 0 }
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                    }
                    PrimaryButton {
                        Layout.fillWidth: true
                        text: "Send transfer"
                        onClicked: {
                            if (Backend.transfer(toUserField.text, parseFloat(transferAmount.text))) {
                                toUserField.text = ""; transferAmount.text = "";
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }

        // History
        GlassCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 8
                Text { text: "Recent transactions"; color: Theme.text; font.family: Theme.fontFamily; font.pixelSize: 16; font.bold: true }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: view.history
                    spacing: 4
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        required property string modelData
                        width: ListView.view.width
                        height: 34
                        radius: 6
                        color: "transparent"
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            text: modelData
                            color: Theme.textDim
                            font.family: Theme.fontFamily
                            font.pixelSize: 13
                        }
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: view.history.length === 0
                        text: "No transactions yet."
                        color: Theme.textFaint
                        font.family: Theme.fontFamily
                        font.pixelSize: 14
                    }
                }
            }
        }
    }

    // Paystack deposit flow: initialize -> browser -> verify
    Dialog {
        id: depositDialog
        modal: true
        anchors.centerIn: parent
        width: Math.min(480, parent ? parent.width - 80 : 480)
        padding: 0
        background: GlassCard {}

        // step 0 = enter amount/email, step 1 = awaiting verification
        property int step: 0

        onOpened: {
            step = 0;
            depAmount.text = "";
            depEmail.text = Backend.email;
        }

        Connections {
            target: Backend
            function onPaystackInitialized() {
                if (depositDialog.visible) depositDialog.step = 1;
            }
            function onPaystackVerifyResult(success, detail) {
                if (depositDialog.visible && success) depositDialog.close();
            }
        }

        contentItem: ColumnLayout {
            spacing: 14

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 20
                Layout.bottomMargin: 0
                Text {
                    text: "Deposit with Paystack"
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 20
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Rectangle {
                    Layout.preferredWidth: 32; Layout.preferredHeight: 32; radius: 16
                    color: Theme.surfaceAlt
                    Text { anchors.centerIn: parent; text: "✕"; color: Theme.textDim; font.pixelSize: 15 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: depositDialog.close() }
                }
            }

            // Step 0 — amount + email
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                spacing: 10
                visible: depositDialog.step === 0
                StyledField {
                    id: depAmount
                    Layout.fillWidth: true
                    placeholderText: "Amount (NGN)"
                    validator: DoubleValidator { bottom: 0 }
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                }
                StyledField {
                    id: depEmail
                    Layout.fillWidth: true
                    placeholderText: "Email for receipt"
                }
                Text {
                    text: "Your browser will open to complete payment. Return here and verify afterwards."
                    color: Theme.textFaint
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            // Step 1 — awaiting verification
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                spacing: 8
                visible: depositDialog.step === 1
                Text {
                    text: "Complete the payment in your browser."
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: 15
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    text: "Reference: " + Backend.paystackReference
                    color: Theme.textDim
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    wrapMode: Text.WrapAnywhere
                    Layout.fillWidth: true
                }
                Text {
                    visible: Backend.paystackBusy
                    text: "Verifying…"
                    color: Theme.gold
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                }
            }

            // Footer
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 20
                Layout.topMargin: 4
                Item { Layout.fillWidth: true }
                PrimaryButton {
                    visible: depositDialog.step === 0
                    text: "Pay with Paystack"
                    enabled: !Backend.paystackBusy
                    onClicked: Backend.paystackDeposit(parseFloat(depAmount.text), depEmail.text)
                }
                PrimaryButton {
                    visible: depositDialog.step === 1
                    text: "I've paid — Verify"
                    enabled: !Backend.paystackBusy
                    onClicked: Backend.paystackVerifyDeposit(Backend.paystackReference)
                }
            }
        }
    }
}
