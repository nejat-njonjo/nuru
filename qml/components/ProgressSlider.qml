import QtQuick
import QtQuick.Controls
import Nuru

// Playback timeline.
//
// While the user presses/drags, the slider shows its own value; the engine position is only
// bound back once released. After a seek, the requested value is held briefly until the engine
// reports the new position, so the handle never snaps back.
Slider {
    id: control

    property real playbackPosition: 0     // 0..1 from the engine
    property real duration: 0             // ms, for the hover tooltip
    property bool seekable: true
    property real pendingSeek: -1
    signal seekRequested(real position)

    from: 0
    to: 1
    live: true
    enabled: seekable
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    stepSize: 0
    padding: 0
    implicitHeight: 18

    Accessible.name: "Playback position"

    Binding on value {
        when: !control.pressed
        value: control.pendingSeek >= 0 ? control.pendingSeek : control.playbackPosition
        restoreMode: Binding.RestoreNone
    }

    onPressedChanged: {
        if (!pressed) {
            pendingSeek = value
            seekRequested(value)
            settleTimer.restart()
        }
    }
    onPlaybackPositionChanged: {
        if (pendingSeek >= 0 && Math.abs(playbackPosition - pendingSeek) < 0.01)
            pendingSeek = -1
    }

    // Keyboard: arrow keys step 5 s (handled by Playback shortcuts elsewhere when unfocused)
    Keys.onLeftPressed: Playback.seekBy(-5000)
    Keys.onRightPressed: Playback.seekBy(5000)

    Timer {
        id: settleTimer
        interval: 900
        onTriggered: control.pendingSeek = -1
    }

    readonly property real hoverX: hoverHandler.point.position.x
    HoverHandler { id: hoverHandler }

    ToolTip {
        parent: control
        visible: hoverHandler.hovered && control.enabled && control.duration > 0
        x: Math.max(0, Math.min(control.width - width, control.hoverX - width / 2))
        y: -height - 6
        text: Theme.formatDuration(Math.max(0, Math.min(1, control.hoverX / control.width)) * control.duration)
    }

    background: Item {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: control.hovered || control.pressed ? 6 : 4
        Behavior on height { NumberAnimation { duration: Theme.durationFast } }

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: Theme.track
        }
        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: height / 2
            color: control.hovered || control.pressed ? Theme.accent : Theme.textPrimary
            opacity: control.enabled ? 1 : 0.4
            Behavior on color { ColorAnimation { duration: Theme.durationFast } }
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: 14
        height: 14
        radius: 7
        color: Theme.textPrimary
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.accent
        visible: control.enabled
        opacity: control.hovered || control.pressed || control.visualFocus ? 1 : 0
        scale: control.pressed ? 1.15 : 1
        Behavior on opacity { NumberAnimation { duration: Theme.durationFast } }
        Behavior on scale { NumberAnimation { duration: Theme.durationFast } }
    }
}
