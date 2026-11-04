import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Modal single-line text prompt (create / rename playlist).
Dialog {
    id: dialog

    property alias text: field.text
    property string placeholder
    property string confirmText: "Save"
    signal submitted(string value)

    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(420, (parent ? parent.width : 420) - 48)
    padding: 24
    topPadding: 16

    onOpened: {
        field.forceActiveFocus()
        field.selectAll()
    }
    onAccepted: if (field.text.trim().length > 0) submitted(field.text.trim())

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

    contentItem: TextField {
        id: field
        placeholderText: dialog.placeholder
        placeholderTextColor: Theme.textMuted
        color: Theme.textPrimary
        selectionColor: Theme.accent
        selectedTextColor: Theme.accentText
        font.pixelSize: Theme.fontBodyLarge
        implicitHeight: 42
        leftPadding: 14
        background: Rectangle {
            radius: Theme.radiusM
            color: Theme.surface
            border.width: field.activeFocus ? 2 : 1
            border.color: field.activeFocus ? Theme.accent : Theme.border
        }
        onAccepted: dialog.accept()
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
            primary: true
            enabled: field.text.trim().length > 0
            onClicked: dialog.accept()
        }
    }
}
