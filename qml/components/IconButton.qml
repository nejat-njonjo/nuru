import QtQuick
import QtQuick.Controls
import Nuru

// Round icon-only button with hover/press/focus states and a tooltip.
AbstractButton {
    id: control

    property string iconName
    property int iconSize: 18
    property int diameter: 34
    property string tip
    property bool accented: false        // filled accent background (primary action)
    property color iconColor: checked ? Theme.accent
                                      : (hovered || visualFocus ? Theme.textPrimary : Theme.textSecondary)

    implicitWidth: diameter
    implicitHeight: diameter
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    padding: 0

    Accessible.role: Accessible.Button
    Accessible.name: tip

    ToolTip.visible: hovered && tip.length > 0
    ToolTip.delay: 600
    ToolTip.text: tip

    background: Rectangle {
        radius: width / 2
        color: control.accented
               ? (control.hovered ? Theme.accentHover : Theme.accent)
               : (control.down ? Theme.pressed : control.hovered ? Theme.hover : "transparent")
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.accent
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }

    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: control.iconName
            size: control.iconSize
            color: control.accented ? Theme.accentText : control.iconColor
            opacity: control.enabled ? 1 : 0.35
        }
    }

    scale: down ? 0.92 : 1
    Behavior on scale { NumberAnimation { duration: Theme.durationFast; easing.type: Easing.OutQuad } }
}
