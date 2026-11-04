import QtQuick
import QtQuick.Layouts
import Nuru

// Friendly placeholder for empty pages and searches.
ColumnLayout {
    id: root

    property string iconName: "music"
    property string title
    property string message
    property string actionText
    property string actionIcon
    property string secondaryActionText
    property string secondaryActionIcon
    signal actionTriggered()
    signal secondaryActionTriggered()

    spacing: 10

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        Layout.bottomMargin: 6
        width: 72
        height: 72
        radius: 36
        color: Theme.elevated
        Icon {
            anchors.centerIn: parent
            name: root.iconName
            size: 32
            color: Theme.textSecondary
        }
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: root.title
        color: Theme.textPrimary
        font.pixelSize: Theme.fontTitle
        font.weight: Font.DemiBold
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: 380
        visible: text.length > 0
        text: root.message
        color: Theme.textSecondary
        font.pixelSize: Theme.fontBody
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }
    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 8
        spacing: 10
        visible: root.actionText.length > 0
        PillButton {
            text: root.actionText
            iconName: root.actionIcon
            primary: true
            onClicked: root.actionTriggered()
        }
        PillButton {
            visible: root.secondaryActionText.length > 0
            text: root.secondaryActionText
            iconName: root.secondaryActionIcon
            onClicked: root.secondaryActionTriggered()
        }
    }
}
