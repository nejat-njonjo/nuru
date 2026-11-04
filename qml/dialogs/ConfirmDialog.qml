import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Modal yes/no confirmation.
Dialog {
    id: dialog

    property string message
    property string confirmText: "OK"
    property bool destructive: false

    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(440, (parent ? parent.width : 440) - 48)
    padding: 24
    topPadding: 20

    background: Rectangle {
        radius: Theme.radiusL
        color: Theme.elevated
        border.color: Theme.border
    }
    Overlay.modal: Rectangle { color: Theme.scrim }

    header: Text {
        text: dialog.title
        padding: 24
        bottomPadding: 0
        color: Theme.textPrimary
        font.pixelSize: Theme.fontTitle + 1
        font.weight: Font.Bold
    }

    contentItem: Text {
        text: dialog.message
        wrapMode: Text.WordWrap
        color: Theme.textSecondary
        font.pixelSize: Theme.fontBody
        lineHeight: 1.2
    }

    footer: RowLayout {
        spacing: 10
        Item { Layout.fillWidth: true }
        PillButton {
            Layout.bottomMargin: 20
            text: "Cancel"
            onClicked: dialog.reject()
        }
        PillButton {
            Layout.rightMargin: 24
            Layout.bottomMargin: 20
            text: dialog.confirmText
            primary: !dialog.destructive
            destructive: dialog.destructive
            onClicked: dialog.accept()
        }
    }
}
