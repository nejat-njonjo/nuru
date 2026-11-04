import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Track list page used for Music, Recently played and Favorites.
Item {
    id: page

    property string filter: "audio"          // audio | recent | favorites
    property string title: "Music"
    property string emptyIcon: "music"
    property string emptyTitle: "No music yet"
    property string emptyMessage: "Add a folder with music files and it will appear here."
    property bool sortable: true

    readonly property var sortOptions: [
        { key: "title", label: "Title" },
        { key: "artist", label: "Artist" },
        { key: "album", label: "Album" },
        { key: "added", label: "Recently added", defaultAscending: false },
        { key: "duration", label: "Duration" },
        { key: "plays", label: "Most played", defaultAscending: false }
    ]

    MediaListModel {
        id: mediaModel
        library: Library
        filter: page.filter
    }

    TrackList {
        id: list
        anchors.fill: parent
        model: mediaModel

        header: ColumnLayout {
            width: list.width - list.leftMargin - list.rightMargin
            spacing: Theme.spaceL

            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spaceXxl
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                title: page.title
                subtitle: mediaModel.count === 0 ? ""
                          : (mediaModel.count === 1 ? "1 item" : mediaModel.count + " items")
                            + (mediaModel.totalDuration > 0 ? " · " + Theme.formatTotal(mediaModel.totalDuration) : "")
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.bottomMargin: Theme.spaceS
                opacity: mediaModel.count > 0 ? 1 : 0
                enabled: mediaModel.count > 0
                spacing: Theme.spaceS

                IconButton {
                    accented: true
                    diameter: 48
                    iconSize: 22
                    iconName: "play-fill"
                    tip: "Play all"
                    onClicked: {
                        Playback.queue.shuffle = false
                        Playback.playPaths(mediaModel.paths(), 0)
                    }
                }
                IconButton {
                    diameter: 40
                    iconSize: 22
                    iconName: "shuffle"
                    tip: "Shuffle play"
                    onClicked: {
                        Playback.queue.shuffle = true
                        const all = mediaModel.paths()
                        Playback.playPaths(all, Math.floor(Math.random() * all.length))
                    }
                }
                Item { Layout.fillWidth: true }
                SortMenuButton {
                    visible: page.sortable
                    options: page.sortOptions
                    currentKey: mediaModel.sortKey.length > 0 ? mediaModel.sortKey : "title"
                    ascending: mediaModel.sortAscending
                    onSortChanged: (key, ascending) => {
                        mediaModel.sortKey = key
                        mediaModel.sortAscending = ascending
                    }
                }
            }
        }

        emptyState: Component {
            EmptyState {
                iconName: page.emptyIcon
                title: Library.scanning ? "Scanning your library…" : page.emptyTitle
                message: page.emptyMessage
                actionText: page.filter === "audio" && !Library.scanning ? "Add folder" : ""
                actionIcon: "folder-add"
                onActionTriggered: AppState.addFolderRequested()
            }
        }
    }
}
