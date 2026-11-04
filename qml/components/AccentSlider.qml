import QtQuick
import QtQuick.Controls
import Nuru

// Themed horizontal slider (thin track, accent fill, handle on hover/focus).
Slider {
    id: control

    padding: 0
    implicitHeight: 18
    implicitWidth: 160
    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    background: Item {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: 4
        Rectangle { anchors.fill: parent; radius: 2; color: Theme.track }
        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 2
            color: control.hovered || control.pressed ? Theme.accent : Theme.textPrimary
            opacity: control.enabled ? 1 : 0.4
        }
    }
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: 12
        height: 12
        radius: 6
        color: Theme.textPrimary
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.accent
        opacity: control.hovered || control.pressed || control.visualFocus ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durationFast } }
    }
}
