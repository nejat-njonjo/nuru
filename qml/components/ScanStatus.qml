import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Compact library scan progress (sidebar footer and library page).
ColumnLayout {
    id: root
    spacing: 6
    visible: Library.scanning

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        BusyIndicator {
            implicitWidth: 18
            implicitHeight: 18
            running: Library.scanning
        }
        Text {
            Layout.fillWidth: true
            text: Library.scanMessage
            elide: Text.ElideRight
            color: Theme.textSecondary
            font.pixelSize: Theme.fontCaption
        }
        IconButton {
            diameter: 24
            iconSize: 14
            iconName: "close"
            tip: "Cancel scan"
            onClicked: Library.cancelScan()
        }
    }
    Rectangle {
        Layout.fillWidth: true
        height: 3
        radius: 2
        color: Theme.track
        Rectangle {
            width: parent.width * Library.scanProgress
            height: parent.height
            radius: 2
            color: Theme.accent
            Behavior on width { NumberAnimation { duration: Theme.durationNormal } }
        }
    }
}
