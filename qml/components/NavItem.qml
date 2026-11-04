import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Sidebar navigation entry.
AbstractButton {
    id: control

    property string iconName
    property string activeIconName: iconName
    property bool active: false
    property string badge

    Layout.fillWidth: true
    implicitHeight: 40
    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    Accessible.role: Accessible.PageTab
    Accessible.name: text
    Accessible.selected: active

    background: Rectangle {
        radius: Theme.radiusM
        color: control.active ? Theme.accentSoft
                              : control.down ? Theme.pressed : control.hovered ? Theme.hover : "transparent"
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.accent
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }

        Rectangle {
            width: 3
            height: control.active ? 18 : 0
            radius: 2
            color: Theme.accent
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            Behavior on height { NumberAnimation { duration: Theme.durationNormal; easing.type: Easing.OutCubic } }
        }
    }

    contentItem: RowLayout {
        spacing: 12
        anchors.leftMargin: 12
        Item { implicitWidth: 2 }
        Icon {
            name: control.active ? control.activeIconName : control.iconName
            size: 20
            color: control.active ? Theme.accent : (control.hovered ? Theme.textPrimary : Theme.textSecondary)
        }
        Text {
            Layout.fillWidth: true
            text: control.text
            elide: Text.ElideRight
            color: control.active || control.hovered ? Theme.textPrimary : Theme.textSecondary
            font.pixelSize: Theme.fontBody
            font.weight: control.active ? Font.DemiBold : Font.Normal
        }
        Text {
            visible: control.badge.length > 0
            text: control.badge
            color: Theme.textMuted
            font.pixelSize: Theme.fontCaption
            rightPadding: 10
        }
    }
}
