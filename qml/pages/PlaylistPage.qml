import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// A single playlist: play/shuffle, rename/delete, reorder and remove items.
Item {
    id: page

    property int playlistId: AppState.playlistId
    readonly property string name: {
        Library.revision
        return Library.playlistName(playlistId)
    }

    MediaListModel {
        id: items
        library: Library
        filter: "playlist"
        playlistId: page.playlistId
    }

    TrackList {
        id: list
        anchors.fill: parent
        model: items
        playlistId: page.playlistId
        reorderable: true

        header: ColumnLayout {
            width: list.width - list.leftMargin - list.rightMargin
            spacing: Theme.spaceL

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spaceXxl
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                spacing: Theme.spaceXl

                Artwork {
                    Layout.preferredWidth: 168
                    Layout.preferredHeight: 168
                    source: items.count > 0 ? items.get(0).artwork : ""
                    seed: page.name
                    fallbackIcon: "playlist"
                    radius: Theme.radiusL
                    decodeSize: 384
                }
                PageHeader {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignBottom
                    eyebrow: "Playlist"
                    title: page.name
                    subtitle: (items.count === 1 ? "1 item" : items.count + " items")
                              + (items.totalDuration > 0 ? " · " + Theme.formatTotal(items.totalDuration) : "")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.bottomMargin: Theme.spaceS
                spacing: Theme.spaceS

                IconButton {
                    accented: true
                    diameter: 48
                    iconSize: 22
                    iconName: "play-fill"
                    tip: "Play playlist"
                    enabled: items.count > 0
                    onClicked: {
                        Playback.queue.shuffle = false
                        Playback.playPaths(items.paths(), 0)
                    }
                }
                IconButton {
                    diameter: 40
                    iconSize: 22
                    iconName: "shuffle"
                    tip: "Shuffle playlist"
                    enabled: items.count > 0
                    onClicked: {
                        Playback.queue.shuffle = true
                        const all = items.paths()
                        Playback.playPaths(all, Math.floor(Math.random() * all.length))
                    }
                }
                Item { Layout.fillWidth: true }
                PillButton {
                    text: "Rename"
                    iconName: "edit"
                    onClicked: AppState.renamePlaylistRequested(page.playlistId)
                }
                PillButton {
                    text: "Delete"
                    iconName: "trash"
                    destructive: true
                    onClicked: AppState.deletePlaylistRequested(page.playlistId)
                }
            }
        }

        emptyState: Component {
            EmptyState {
                iconName: "playlist"
                title: "This playlist is empty"
                message: "Use “Add to playlist” in any track’s ••• menu to add songs or videos."
                actionText: "Browse music"
                actionIcon: "music"
                onActionTriggered: AppState.navigate("music")
            }
        }
    }
}
