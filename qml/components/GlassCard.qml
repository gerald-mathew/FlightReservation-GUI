import QtQuick
import QtQuick.Effects
import NexoraAirways

Rectangle {
    id: card
    radius: Theme.radius
    color: Theme.surface
    border.color: Theme.stroke
    border.width: 1

    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
        shadowColor: "#000000"
        shadowOpacity: 0.35
        shadowBlur: 0.6
        shadowVerticalOffset: 6
    }
}
