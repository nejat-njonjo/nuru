import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Landing page: greeting + hero, then shelves for recent, new, favourites and playlists.
Flickable {
    id: page

    readonly property bool libraryEmpty: Library.musicCount + Library.videoCount === 0

    contentHeight: column.implicitHeight + Theme.spaceXxl
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}

    function greeting() {
        const hour = new Date().getHours()
        if (hour < 5) return "Good night"
        if (hour < 12) return "Good morning"
        if (hour < 18) return "Good afternoon"
        return "Good evening"
    }

    MediaListModel { id: recentModel; library: Library; filter: "recent"; limit: 16 }
    MediaListModel { id: addedModel; library: Library; filter: "added"; limit: 16 }
    MediaListModel { id: favoritesModel; library: Library; filter: "favorites"; limit: 16 }

    component MediaShelfCard: MediaCard {
        required property int index
        required property var model
        required property var listModel
        width: 176
        title: model.title
        subtitle: model.subtitle
        artwork: model.artwork
        kind: model.kind
        seed: model.album.length > 0 ? model.album : model.title
        playing: model.path === Playback.currentPath
        onClicked: playRequested()
        onPlayRequested: Playback.playPaths(listModel.paths(), index)
        onMenuRequested: AppState.mediaMenuRequested(listModel.get(index), -1)
    }

    ColumnLayout {
        id: column
        width: page.width
        spacing: Theme.spaceXl

        // Hero
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 260
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.dark ? 0.28 : 0.22) }
                GradientStop { position: 1; color: Theme.background }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.pageMargin
                anchors.rightMargin: Theme.pageMargin
                anchors.topMargin: Theme.spaceXxl
                anchors.bottomMargin: Theme.spaceL
                spacing: Theme.spaceXl

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 8

                    Text {
                        text: page.greeting()
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontDisplay + 4
                        font.weight: Font.Black
                        font.letterSpacing: -1
                    }
                    Text {
                        Layout.fillWidth: true
                        text: page.libraryEmpty
                              ? "Your media. Your world. Add a folder of music or videos to get started."
                              : Library.musicCount + " songs · " + Library.videoCount + " videos in your library"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontBodyLarge
                        wrapMode: Text.WordWrap
                    }
                    RowLayout {
                        Layout.topMargin: 12
                        spacing: 10
                        PillButton {
                            primary: true
                            visible: !page.libraryEmpty
                            text: "Shuffle my music"
                            iconName: "shuffle"
                            enabled: Library.musicCount > 0
                            onClicked: shuffleModel.playShuffled()
                        }
                        PillButton {
                            primary: page.libraryEmpty
                            text: "Add folder"
                            iconName: "folder-add"
                            onClicked: AppState.addFolderRequested()
                        }
                        PillButton {
                            text: "Open files"
                            iconName: "file-add"
                            onClicked: AppState.openFilesRequested()
                        }
                    }
                }

                // Now playing summary
                AbstractButton {
                    id: nowCard
                    visible: Playback.hasMedia && page.width > 900
                    Layout.preferredWidth: 360
                    Layout.preferredHeight: 140
                    hoverEnabled: true
                    onClicked: AppState.navigate("nowPlaying")
                    Accessible.name: "Now playing: " + Playback.title
                    background: Rectangle {
                        radius: Theme.radiusL
                        color: nowCard.hovered ? Theme.hover : Theme.elevated
                        border.color: Theme.border
                    }
                    contentItem: RowLayout {
                        spacing: Theme.spaceL
                        Artwork {
                            Layout.leftMargin: 14
                            Layout.preferredWidth: 108
                            Layout.preferredHeight: 108
                            source: Playback.artwork
                            seed: Playback.subtitle + Playback.title
                            kind: Playback.isVideo ? "video" : "audio"
                            radius: Theme.radiusM
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.rightMargin: 14
                            spacing: 4
                            Text {
                                text: Playback.playing ? "NOW PLAYING" : "CONTINUE"
                                color: Theme.accent
                                font.pixelSize: Theme.fontCaption - 1
                                font.weight: Font.Bold
                                font.letterSpacing: 1.4
                            }
                            Text {
                                Layout.fillWidth: true
                                text: Playback.title
                                elide: Text.ElideRight
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontTitle
                                font.weight: Font.Bold
                            }
                            Text {
                                Layout.fillWidth: true
                                text: Playback.subtitle
                                elide: Text.ElideRight
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontBody
                            }
                        }
                    }
                }
            }
        }

        MediaListModel {
            id: shuffleModel
            library: Library
            filter: "audio"
            function playShuffled() {
                Playback.queue.shuffle = true
                const all = paths()
                if (all.length > 0)
                    Playback.playPaths(all, Math.floor(Math.random() * all.length))
            }
        }

        CardShelf {
            Layout.fillWidth: true
            title: "Recently played"
            seeAllPage: "recent"
            model: recentModel
            delegate: MediaShelfCard { listModel: recentModel }
        }
        CardShelf {
            Layout.fillWidth: true
            title: "Recently added"
            seeAllPage: "music"
            model: addedModel
            delegate: MediaShelfCard { listModel: addedModel }
        }
        CardShelf {
            Layout.fillWidth: true
            title: "Favorites"
            seeAllPage: "favorites"
            model: favoritesModel
            delegate: MediaShelfCard { listModel: favoritesModel }
        }
        CardShelf {
            Layout.fillWidth: true
            title: "Your streams"
            seeAllPage: "streams"
            model: Streams
            cardHeight: (cardWidth - 24) * 9 / 16 + 76
            cardWidth: 210
            delegate: StreamCard {
                required property int index
                required property var model
                width: 210
                name: model.name
                url: model.url
                logo: model.logo
                saved: true
                subtitle: model.category.length > 0 ? model.category : "Live stream"
                onPlayRequested: Playback.playPaths(Streams.urls(), index)
                onSaveToggled: Streams.remove(url)
            }
        }
        CardShelf {
            Layout.fillWidth: true
            title: "Your playlists"
            seeAllPage: "playlists"
            model: Library.playlists
            delegate: MediaCard {
                required property var modelData
                width: 176
                title: modelData.name
                subtitle: modelData.count === 1 ? "1 item" : modelData.count + " items"
                artwork: modelData.artwork
                fallbackIcon: "playlist"
                seed: modelData.name
                onClicked: AppState.navigate("playlist", modelData.id)
                onPlayRequested: Playback.playPaths(Library.playlistPaths(modelData.id), 0)
            }
        }

        EmptyState {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.spaceXl
            visible: page.libraryEmpty && !Library.scanning
            iconName: "library"
            title: "Your library is waiting"
            message: "Nuru indexes local folders and keeps everything on your computer. Add a folder of music or videos to fill this page."
            actionText: "Add folder"
            actionIcon: "folder-add"
            onActionTriggered: AppState.addFolderRequested()
        }
    }
}
