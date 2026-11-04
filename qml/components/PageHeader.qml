import QtQuick
import QtQuick.Layouts
import Nuru

// Large page title with optional subtitle and right-aligned actions (children).
RowLayout {
    id: root

    property string title
    property string subtitle
    property string eyebrow
    default property alias actions: actionRow.data

    spacing: Theme.spaceL

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 4
        Text {
            visible: root.eyebrow.length > 0
            text: root.eyebrow.toUpperCase()
            color: Theme.accent
            font.pixelSize: Theme.fontCaption
            font.weight: Font.Bold
            font.letterSpacing: 1.4
        }
        Text {
            Layout.fillWidth: true
            text: root.title
            elide: Text.ElideRight
            color: Theme.textPrimary
            font.pixelSize: Theme.fontDisplay
            font.weight: Font.Bold
            font.letterSpacing: -0.5
        }
        Text {
            // Always occupies its line so list headers keep a stable height.
            Layout.fillWidth: true
            text: root.subtitle.length > 0 ? root.subtitle : " "
            elide: Text.ElideRight
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
        }
    }

    RowLayout {
        id: actionRow
        Layout.alignment: Qt.AlignBottom
        spacing: Theme.spaceS
    }
}
