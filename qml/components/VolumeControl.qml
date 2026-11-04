import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Mute toggle + draggable volume slider bound to the playback engine.
RowLayout {
    id: root
    spacing: 4

    IconButton {
        iconName: Playback.muted || Playback.volume === 0 ? "volume-mute"
                : Playback.volume < 50 ? "volume-low" : "volume-high"
        tip: Playback.muted ? "Unmute (M)" : "Mute (M)"
        checked: Playback.muted
        iconColor: hovered ? Theme.textPrimary : Theme.textSecondary
        onClicked: Playback.toggleMute()
    }

    AccentSlider {
        id: slider
        Layout.preferredWidth: 110
        from: 0
        to: 100
        stepSize: 1
        Accessible.name: "Volume"

        Binding on value {
            when: !slider.pressed
            value: Playback.muted ? 0 : Playback.volume
            restoreMode: Binding.RestoreNone
        }
        onMoved: Playback.volume = Math.round(value)

        WheelHandler {
            onWheel: event => Playback.changeVolume(event.angleDelta.y > 0 ? 5 : -5)
        }

        ToolTip.visible: pressed || hovered
        ToolTip.delay: pressed ? 0 : 600
        ToolTip.text: Math.round(value) + "%"
    }
}
