import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Grid of videos with generated thumbnails.
Item {
    id: page

    MediaListModel {
        id: videos
        library: Library
        filter: "video"
    }

    GridView {
        id: grid
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin - 12
        anchors.rightMargin: Theme.pageMargin - 12
        clip: true
        model: videos
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        readonly property int columns: Math.max(2, Math.floor(width / 260))
        cellWidth: width / columns
        cellHeight: cellWidth / (16 / 9) + 72

        header: ColumnLayout {
            width: grid.width
            spacing: Theme.spaceL
            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spaceXxl
                Layout.leftMargin: 12
                Layout.rightMargin: 12
                Layout.bottomMargin: Theme.spaceM
                title: "Videos"
                subtitle: videos.count === 0 ? "" : (videos.count === 1 ? "1 video" : videos.count + " videos")
                          + (videos.totalDuration > 0 ? " · " + Theme.formatTotal(videos.totalDuration) : "")

                SortMenuButton {
                    visible: videos.count > 0
                    options: [
                        { key: "title", label: "Title" },
                        { key: "added", label: "Recently added", defaultAscending: false },
                        { key: "duration", label: "Duration" },
                        { key: "recent", label: "Recently played", defaultAscending: false }
                    ]
                    currentKey: videos.sortKey.length > 0 ? videos.sortKey : "title"
                    ascending: videos.sortAscending
                    onSortChanged: (key, ascending) => {
                        videos.sortKey = key
                        videos.sortAscending = ascending
                    }
                }
            }
        }

        delegate: MediaCard {
            required property int index
            required property var model
            width: grid.cellWidth
            title: model.title
            subtitle: Theme.formatDuration(model.duration) + (model.subtitle !== "Video" ? " · " + model.subtitle : "")
            artwork: model.artwork
            kind: "video"
            playing: model.path === Playback.currentPath
            onClicked: playRequested()
            onPlayRequested: Playback.playPaths(videos.paths(), index)
            onMenuRequested: AppState.mediaMenuRequested(videos.get(index), -1)
        }

        footer: Item {
            width: grid.width
            height: videos.count === 0 ? 360 : Theme.spaceXl
            EmptyState {
                anchors.centerIn: parent
                visible: videos.count === 0
                iconName: "video"
                title: Library.scanning ? "Scanning your library…" : "No videos yet"
                message: "MP4, MKV, WebM, AVI, MOV and other formats supported by VLC appear here once their folder is in your library."
                actionText: Library.scanning ? "" : "Add folder"
                actionIcon: "folder-add"
                onActionTriggered: AppState.addFolderRequested()
            }
        }
    }
}
