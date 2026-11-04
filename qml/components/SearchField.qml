import QtQuick
import QtQuick.Controls
import Nuru

// Rounded search input with icon and clear button.
TextField {
    id: control

    implicitHeight: 42
    leftPadding: 42
    rightPadding: clearButton.visible ? 40 : 16
    placeholderText: "Search"
    placeholderTextColor: Theme.textMuted
    color: Theme.textPrimary
    selectionColor: Theme.accent
    selectedTextColor: Theme.accentText
    font.pixelSize: Theme.fontBodyLarge
    selectByMouse: true

    Accessible.role: Accessible.EditableText
    Accessible.name: placeholderText

    background: Rectangle {
        radius: height / 2
        color: Theme.elevated
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? Theme.accent : Theme.border
        Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }
    }

    Icon {
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        name: "search"
        size: 18
        color: control.activeFocus ? Theme.textPrimary : Theme.textMuted
    }

    IconButton {
        id: clearButton
        anchors.right: parent.right
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        visible: control.text.length > 0
        diameter: 30
        iconSize: 16
        iconName: "close"
        tip: "Clear search"
        onClicked: {
            control.clear()
            control.forceActiveFocus()
        }
    }

    Keys.onEscapePressed: event => {
        if (text.length > 0) {
            clear()
            event.accepted = true
        } else {
            event.accepted = false
        }
    }
}
