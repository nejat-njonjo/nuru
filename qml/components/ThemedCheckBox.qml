import QtQuick
import QtQuick.Controls
import Nuru

// Checkbox in the Nuru style.
CheckBox {
    id: control

    hoverEnabled: true
    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: control.leftPadding
        y: (control.height - height) / 2
        radius: 5
        color: control.checked ? Theme.accent : "transparent"
        border.width: control.checked ? 0 : 1.5
        border.color: control.visualFocus ? Theme.accent : control.hovered ? Theme.textPrimary : Theme.textSecondary
        Icon {
            anchors.centerIn: parent
            visible: control.checked
            name: "check-small"
            size: 14
            color: Theme.accentText
        }
    }
    contentItem: Text {
        leftPadding: control.indicator.width + 8
        text: control.text
        color: Theme.textSecondary
        font.pixelSize: Theme.fontBody
        verticalAlignment: Text.AlignVCenter
    }
}
