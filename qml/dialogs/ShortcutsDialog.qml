import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Lists every keyboard shortcut so they are discoverable (Ctrl+/ or F1).
Dialog {
    id: dialog

    readonly property var shortcuts: [
        ["Space", "Play / pause"],
        ["Ctrl + Right", "Next"],
        ["Ctrl + Left", "Previous"],
        ["Right / Left", "Seek forward / back 5 seconds"],
        ["Ctrl + Up", "Volume up"],
        ["Ctrl + Down", "Volume down"],
        ["M", "Mute / unmute"],
        ["S", "Stop"],
        ["F", "Toggle fullscreen"],
        ["Esc", "Exit fullscreen"],
        ["Q", "Show / hide queue"],
        ["Ctrl + O", "Open media files"],
        ["Ctrl + Shift + O", "Add folder to library"],
        ["Ctrl + L", "Open a stream or link"],
        ["Ctrl + F", "Search"],
        ["Ctrl + ,", "Settings"],
        ["Ctrl + /", "Keyboard shortcuts"]
    ]

    title: "Keyboard shortcuts"
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(520, (parent ? parent.width : 520) - 48)
    height: Math.min(implicitHeight, (parent ? parent.height : 640) - 48)
    padding: 24

    background: Rectangle {
        radius: Theme.radiusL
        color: Theme.elevated
        border.color: Theme.border
    }
    Overlay.modal: Rectangle { color: Theme.scrim }

    header: RowLayout {
        Text {
            Layout.fillWidth: true
            Layout.margins: 24
            Layout.bottomMargin: 0
            text: dialog.title
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle + 1
            font.weight: Font.Bold
        }
        IconButton {
            Layout.rightMargin: 16
            Layout.topMargin: 16
            iconName: "close"
            tip: "Close"
            onClicked: dialog.close()
        }
    }

    contentItem: ListView {
        implicitHeight: contentHeight
        clip: true
        model: dialog.shortcuts
        spacing: 2
        boundsBehavior: Flickable.StopAtBounds
        delegate: RowLayout {
            required property var modelData
            width: ListView.view.width
            height: 34
            Text {
                Layout.fillWidth: true
                text: modelData[1]
                color: Theme.textPrimary
                font.pixelSize: Theme.fontBody
            }
            Rectangle {
                implicitWidth: keyText.implicitWidth + 16
                implicitHeight: 24
                radius: Theme.radiusS
                color: Theme.surface
                border.color: Theme.border
                Text {
                    id: keyText
                    anchors.centerIn: parent
                    text: modelData[0]
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontCaption
                    font.family: "monospace"
                }
            }
        }
    }
}
