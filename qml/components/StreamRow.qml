import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Row in "My streams": logo, name, link, actions.
ItemDelegate {
    id: row

    property string name
    property string url
    property string logo
    property string category
    property real lastPlayed: 0
    readonly property bool playing: url === Playback.currentPath

    signal renameRequested()

    implicitHeight: 60
    hoverEnabled: true
    padding: 0
    focusPolicy: Qt.NoFocus
    Accessible.name: name

    onDoubleClicked: Playback.openLocation(url)

    background: Rectangle {
        radius: Theme.radiusM
        color: row.playing ? Theme.accentSoft : row.hovered ? Theme.hover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }

    contentItem: RowLayout {
        spacing: Theme.spaceM
        IconButton {
            Layout.leftMargin: 6
            diameter: 32
            iconSize: 14
            iconName: row.playing && Playback.playing ? "pause-fill" : "play-fill"
            tip: row.playing && Playback.playing ? "Pause" : "Play"
            onClicked: row.playing ? Playback.togglePlayPause() : Playback.openLocation(row.url)
        }
        Artwork {
            Layout.preferredWidth: 64
            Layout.preferredHeight: 40
            source: row.logo
            seed: row.name
            fallbackIcon: "stream"
            decodeSize: 128
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                Layout.fillWidth: true
                text: row.name
                elide: Text.ElideRight
                color: row.playing ? Theme.accent : Theme.textPrimary
                font.pixelSize: Theme.fontBodyLarge
                font.weight: Font.Medium
            }
            Text {
                Layout.fillWidth: true
                text: row.category.length > 0 ? row.category + " · " + row.url : row.url
                elide: Text.ElideMiddle
                color: Theme.textSecondary
                font.pixelSize: Theme.fontCaption + 1
            }
        }
        Text {
            visible: row.lastPlayed > 0 && row.width > 700
            text: "Played " + new Date(row.lastPlayed).toLocaleDateString(Qt.locale(), Locale.ShortFormat)
            color: Theme.textMuted
            font.pixelSize: Theme.fontCaption + 1
        }
        RowLayout {
            spacing: 0
            opacity: row.hovered ? 1 : 0
            IconButton {
                iconName: "copy"
                iconSize: 16
                tip: "Copy link"
                onClicked: {
                    clipboardHelper.text = row.url
                    clipboardHelper.selectAll()
                    clipboardHelper.copy()
                    AppState.toast("Link copied")
                }
            }
            IconButton {
                iconName: "edit"
                iconSize: 16
                tip: "Rename"
                onClicked: row.renameRequested()
            }
            IconButton {
                Layout.rightMargin: 6
                iconName: "trash"
                iconSize: 16
                tip: "Remove from My streams"
                onClicked: Streams.remove(row.url)
            }
        }
    }

    // TextEdit is the portable way to reach the clipboard from QML.
    TextEdit { id: clipboardHelper; visible: false }
}
