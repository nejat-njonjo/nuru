import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Play (and optionally save) any link, or import an .m3u playlist into My streams.
Dialog {
    id: dialog

    property bool importMode: false
    readonly property string location: Streams.normalizeLocation(urlField.text)
    readonly property bool valid: location.length > 0

    function openFor(importing) {
        importMode = importing
        urlField.text = ""
        nameField.text = ""
        open()
    }

    function submit() {
        if (!valid)
            return
        if (importMode) {
            Streams.importPlaylist(location)
        } else {
            if (saveBox.checked)
                Streams.save(location, nameField.text)
            Playback.openLocation(location)
        }
        close()
    }

    title: importMode ? "Import playlist" : "Open link"
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(560, (parent ? parent.width : 560) - 48)
    padding: 24
    topPadding: 16

    onOpened: urlField.forceActiveFocus()

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

    component Field: TextField {
        id: field
        implicitHeight: 42
        leftPadding: 14
        color: Theme.textPrimary
        placeholderTextColor: Theme.textMuted
        selectionColor: Theme.accent
        selectedTextColor: Theme.accentText
        font.pixelSize: Theme.fontBodyLarge
        selectByMouse: true
        background: Rectangle {
            radius: Theme.radiusM
            color: Theme.surface
            border.width: field.activeFocus ? 2 : 1
            border.color: field.activeFocus ? Theme.accent : Theme.border
        }
    }

    contentItem: ColumnLayout {
        spacing: 12

        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
            text: dialog.importMode
                  ? "Paste the address of an .m3u playlist (or a path to one). Every channel in it is added to My streams."
                  : "Paste a stream or media link: HLS (.m3u8), DASH, HTTP(S) files, RTSP, RTMP, UDP/RTP, SRT or internet radio."
        }
        Field {
            id: urlField
            Layout.fillWidth: true
            placeholderText: dialog.importMode ? "https://iptv-org.github.io/iptv/countries/ke.m3u"
                                               : "https://example.com/live/index.m3u8"
            Accessible.name: "Link"
            onAccepted: dialog.submit()
        }
        Text {
            visible: urlField.text.trim().length > 0 && !dialog.valid
            text: "This isn't a link Nuru can open (supported: http, https, rtsp, rtmp, udp, rtp, srt, mms, ftp, or a local file)."
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.danger
            font.pixelSize: Theme.fontCaption + 1
        }
        Field {
            id: nameField
            visible: !dialog.importMode && saveBox.checked
            Layout.fillWidth: true
            placeholderText: "Name (optional)"
            Accessible.name: "Name"
            onAccepted: dialog.submit()
        }
        ThemedCheckBox {
            id: saveBox
            visible: !dialog.importMode
            checked: true
            text: "Save to My streams"
        }
    }

    footer: RowLayout {
        spacing: 10
        Item { Layout.fillWidth: true }
        PillButton {
            Layout.bottomMargin: 20
            text: "Cancel"
            onClicked: dialog.close()
        }
        PillButton {
            Layout.rightMargin: 24
            Layout.bottomMargin: 20
            primary: true
            enabled: dialog.valid
            text: dialog.importMode ? "Import" : "Play"
            iconName: dialog.importMode ? "import" : "play-fill"
            onClicked: dialog.submit()
        }
    }
}
