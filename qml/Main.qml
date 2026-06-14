import QtQuick
import QtQuick.Controls.Basic
import NexoraAirways
import "components"

ApplicationWindow {
    id: window
    width: 1180
    height: 700
    minimumWidth: 940
    minimumHeight: 640
    visible: true
    title: "Nexora Airways"
    color: Theme.bg0

    // Animated background gradient
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.bg0 }
            GradientStop { position: 1.0; color: Theme.bg1 }
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: loginPage

        pushEnter: Transition {
            ParallelAnimation {
                NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 220 }
                NumberAnimation { property: "x"; from: 40; to: 0; duration: 240; easing.type: Easing.OutCubic }
            }
        }
        pushExit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 160 }
        }
        popEnter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200 }
        }
        popExit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 160 }
        }
    }

    Component { id: loginPage; LoginPage {} }
    Component { id: homePage; HomePage {} }

    Toast { id: toast }

    Connections {
        target: Backend
        function onMessageChanged() { toast.show(Backend.lastMessage); }
        function onSessionChanged() {
            if (Backend.loggedIn) {
                stack.push(homePage);
            } else {
                stack.pop(null);
            }
        }
    }

    function goHome() { stack.push(homePage); }
}
