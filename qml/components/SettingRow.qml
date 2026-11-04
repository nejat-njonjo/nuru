import QtQuick
import QtQuick.Layouts
import Nuru

// Label + description on the left, control (children) on the right.
RowLayout {
    id: root

    property string title
    property string description
    default property alias control: holder.data

    Layout.fillWidth: true
    spacing: Theme.spaceL

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2
        Text {
            Layout.fillWidth: true
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: Theme.fontBodyLarge
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
        }
        Text {
            Layout.fillWidth: true
            visible: text.length > 0
            text: root.description
            color: Theme.textSecondary
            font.pixelSize: Theme.fontCaption + 1
            wrapMode: Text.WordWrap
        }
    }
    RowLayout {
        id: holder
        spacing: Theme.spaceS
    }
}
