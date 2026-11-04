import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// One media row: number/playing indicator, artwork, title + subtitle, album, duration, actions.
ItemDelegate {
    id: row

    property int number: 1
    property string title
    property string subtitle
    property string album
    property string artwork
    property string kind: "audio"
    property string path
    property real duration: 0
    property bool favorite: false
    property bool showAlbum: width > 720
    property bool reorderable: false
    property bool canMoveUp: false
    property bool canMoveDown: false
    property bool selected: ListView.isCurrentItem && ListView.view && ListView.view.activeFocus
    readonly property bool isCurrent: path.length > 0 && path === Playback.currentPath

    signal playRequested()
    signal menuRequested()
    signal moveUpRequested()
    signal moveDownRequested()
    signal removeRequested()

    implicitHeight: Theme.rowHeight
    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    padding: 0

    Accessible.role: Accessible.ListItem
    Accessible.name: title + (subtitle ? ", " + subtitle : "")

    background: Rectangle {
        radius: Theme.radiusM
        color: row.down ? Theme.pressed : row.hovered ? Theme.hover : row.selected ? Theme.elevated : "transparent"
        border.width: row.selected ? 1 : 0
        border.color: Theme.accent
        Behavior on color { ColorAnimation { duration: Theme.durationFast } }
    }

    onDoubleClicked: playRequested()

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: row.menuRequested()
    }

    contentItem: RowLayout {
        spacing: Theme.spaceM

        // Number / play button / now-playing indicator
        Item {
            Layout.preferredWidth: 32
            Layout.fillHeight: true
            Layout.leftMargin: 6

            Text {
                anchors.centerIn: parent
                visible: !row.hovered && !row.isCurrent
                text: row.number
                color: Theme.textMuted
                font.pixelSize: Theme.fontBody
                font.features: { "tnum": 1 }
            }
            Icon {
                anchors.centerIn: parent
                visible: !row.hovered && row.isCurrent
                name: Playback.playing ? "volume-high" : "pause-fill"
                size: 16
                color: Theme.accent
            }
            IconButton {
                anchors.centerIn: parent
                visible: row.hovered
                diameter: 30
                iconSize: 14
                iconName: row.isCurrent && Playback.playing ? "pause-fill" : "play-fill"
                tip: row.isCurrent && Playback.playing ? "Pause" : "Play"
                iconColor: Theme.textPrimary
                onClicked: row.isCurrent ? Playback.togglePlayPause() : row.playRequested()
            }
        }

        Artwork {
            Layout.preferredWidth: row.kind === "video" ? 64 : 40
            Layout.preferredHeight: 40
            source: row.artwork
            seed: row.album.length > 0 ? row.album : row.title
            kind: row.kind
            radius: Theme.radiusS
            decodeSize: 128
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: 3
            spacing: 2
            Text {
                Layout.fillWidth: true
                text: row.title
                elide: Text.ElideRight
                color: row.isCurrent ? Theme.accent : Theme.textPrimary
                font.pixelSize: Theme.fontBodyLarge
                font.weight: Font.Medium
            }
            Text {
                Layout.fillWidth: true
                text: row.subtitle
                elide: Text.ElideRight
                color: Theme.textSecondary
                font.pixelSize: Theme.fontCaption + 1
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.preferredWidth: 2
            visible: row.showAlbum
            text: row.album
            elide: Text.ElideRight
            color: Theme.textSecondary
            font.pixelSize: Theme.fontBody
        }

        IconButton {
            visible: row.hovered || row.favorite
            diameter: 30
            iconSize: 16
            iconName: row.favorite ? "heart-fill" : "heart"
            iconColor: row.favorite ? Theme.accent : (hovered ? Theme.textPrimary : Theme.textSecondary)
            tip: row.favorite ? "Remove from favorites" : "Add to favorites"
            onClicked: Library.toggleFavorite(row.path)
        }
        Item {
            visible: !(row.hovered || row.favorite)
            implicitWidth: 30
        }

        Text {
            Layout.preferredWidth: 52
            horizontalAlignment: Text.AlignRight
            text: Theme.formatDuration(row.duration)
            color: Theme.textMuted
            font.pixelSize: Theme.fontBody
            font.features: { "tnum": 1 }
        }

        RowLayout {
            visible: row.reorderable
            spacing: 0
            IconButton {
                diameter: 30; iconSize: 14
                iconName: "chevron-up"
                tip: "Move up"
                enabled: row.canMoveUp
                onClicked: row.moveUpRequested()
            }
            IconButton {
                diameter: 30; iconSize: 14
                iconName: "chevron-down"
                tip: "Move down"
                enabled: row.canMoveDown
                onClicked: row.moveDownRequested()
            }
            IconButton {
                diameter: 30; iconSize: 16
                iconName: "trash"
                tip: "Remove from playlist"
                onClicked: row.removeRequested()
            }
        }

        IconButton {
            Layout.rightMargin: 6
            diameter: 30
            iconSize: 16
            iconName: "more"
            tip: "More options"
            opacity: row.hovered || visualFocus ? 1 : 0
            onClicked: row.menuRequested()
        }
    }
}
