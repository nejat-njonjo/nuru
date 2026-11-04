import QtQuick
import QtQuick.Controls
import Nuru

// Context menu for a media record (from MediaListModel.get()).
Menu {
    id: menu

    property var record: ({})
    property int playlistId: -1
    readonly property string path: record && record.path ? record.path : ""

    function show(rec, fromPlaylist) {
        record = rec
        playlistId = fromPlaylist
        popup()
    }

    MenuItem {
        text: "Play"
        icon.source: "image://icons/play-fill/" + Theme.textSecondary.toString().substring(1)
        onTriggered: Playback.playPaths([menu.path], 0)
    }
    MenuItem {
        text: "Play next"
        icon.source: "image://icons/add-to-queue/" + Theme.textSecondary.toString().substring(1)
        onTriggered: {
            Playback.playNext([menu.path])
            AppState.toast("Playing next: " + menu.record.title)
        }
    }
    MenuItem {
        text: "Add to queue"
        icon.source: "image://icons/queue/" + Theme.textSecondary.toString().substring(1)
        onTriggered: {
            Playback.enqueue([menu.path])
            AppState.toast("Added to queue")
        }
    }
    MenuSeparator {}
    Menu {
        id: playlistMenu
        title: "Add to playlist"
        icon.source: "image://icons/playlist/" + Theme.textSecondary.toString().substring(1)

        MenuItem {
            text: "New playlist…"
            icon.source: "image://icons/add/" + Theme.textSecondary.toString().substring(1)
            onTriggered: AppState.createPlaylistRequested([menu.path])
        }
        MenuSeparator {}
        Instantiator {
            model: Library.playlists
            delegate: MenuItem {
                required property var modelData
                text: modelData.name
                onTriggered: Library.addToPlaylist(modelData.id, [menu.path])
            }
            onObjectAdded: (index, object) => playlistMenu.insertItem(index + 2, object)
            onObjectRemoved: (index, object) => playlistMenu.removeItem(object)
        }
    }
    MenuItem {
        text: menu.record.favorite ? "Remove from favorites" : "Add to favorites"
        icon.source: "image://icons/" + (menu.record.favorite ? "heart-fill" : "heart") + "/" + Theme.textSecondary.toString().substring(1)
        onTriggered: Library.toggleFavorite(menu.path)
    }
    MenuItem {
        visible: menu.playlistId >= 0
        height: visible ? implicitHeight : 0
        text: "Remove from this playlist"
        icon.source: "image://icons/trash/" + Theme.textSecondary.toString().substring(1)
        onTriggered: Library.removeFromPlaylist(menu.playlistId, menu.path)
    }
    MenuSeparator {}
    MenuItem {
        text: "Open containing folder"
        icon.source: "image://icons/folder-open/" + Theme.textSecondary.toString().substring(1)
        onTriggered: Qt.openUrlExternally(Library.folderOf(menu.path))
    }
}
