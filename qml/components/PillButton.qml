import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Text button with optional leading icon. `primary` uses the accent fill.
AbstractButton {
    id: control

    property string iconName
    property bool primary: false
    property bool destructive: false

    implicitHeight: 36
    implicitWidth: row.implicitWidth + 32
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus

    Accessible.role: Accessible.Button
    Accessible.name: text

    readonly property color foreground: primary ? Theme.accentText
                                                : destructive ? Theme.danger : Theme.textPrimary

    background: Rectangle {
        radius: height / 2
        color: control.primary ? (control.hovered ? Theme.accentHover : Theme.accent)
                               : (control.down ? Theme.pressed : control.hovered ? Theme.hover : Theme.elevated)
        border.width: control.visualFocus ? 2 : (control.primary ? 0 : 1)
        border.color: control.visualFocus ? Theme.accent : Theme.border
        opacity: control.enabled ? 1 : 0.45
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }

    contentItem: Item {
        RowLayout {
            id: row
            anchors.centerIn: parent
            spacing: 8
            Icon {
                visible: control.iconName.length > 0
                name: control.iconName
                size: 16
                color: control.foreground
            }
            Text {
                text: control.text
                color: control.foreground
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }
        }
    }

    scale: down ? 0.97 : 1
    Behavior on scale { NumberAnimation { duration: Theme.durationFast } }
}
