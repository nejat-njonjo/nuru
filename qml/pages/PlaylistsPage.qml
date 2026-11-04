import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Grid of playlists.
Item {
    id: page

    GridView {
        id: grid
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin - 12
        anchors.rightMargin: Theme.pageMargin - 12
        clip: true
        model: Library.playlists
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        readonly property int columns: Math.max(2, Math.floor(width / 200))
        cellWidth: width / columns
        cellHeight: cellWidth + 56

        header: Item {
            width: grid.width
            height: header.implicitHeight + Theme.spaceXxl + Theme.spaceL
            PageHeader {
                id: header
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: Theme.spaceXxl
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                title: "Playlists"
                subtitle: grid.count === 1 ? "1 playlist" : grid.count + " playlists"
                PillButton {
                    primary: true
                    text: "New playlist"
                    iconName: "add"
                    onClicked: AppState.createPlaylistRequested([])
                }
            }
        }

        delegate: MediaCard {
            required property var modelData
            width: grid.cellWidth
            title: modelData.name
            subtitle: (modelData.count === 1 ? "1 item" : modelData.count + " items")
                      + (modelData.duration > 0 ? " · " + Theme.formatTotal(modelData.duration) : "")
            artwork: modelData.artwork
            fallbackIcon: "playlist"
            seed: modelData.name
            showPlayButton: modelData.count > 0
            onClicked: AppState.navigate("playlist", modelData.id)
            onPlayRequested: Playback.playPaths(Library.playlistPaths(modelData.id), 0)
        }

        footer: Item {
            width: grid.width
            height: grid.count === 0 ? 360 : Theme.spaceXl
            EmptyState {
                anchors.centerIn: parent
                visible: grid.count === 0
                iconName: "playlist"
                title: "Create your first playlist"
                message: "Group songs and videos together. Add items from any track’s ••• menu."
                actionText: "New playlist"
                actionIcon: "add"
                onActionTriggered: AppState.createPlaylistRequested([])
            }
        }
    }
}
