pragma Singleton

import QtQuick
import Nuru

// UI-level navigation state and app-wide requests. Pages emit requests here; Main.qml owns
// the dialogs, menus and toasts that fulfil them. This keeps pages decoupled from Main.
QtObject {
    // home | search | music | videos | recent | favorites | playlists | playlist | streams | library | nowPlaying | settings
    property string page: "home"
    property int playlistId: -1
    property string searchText: ""
    property bool queueOpen: false
    property string streamsTab: AppSettings.streamsTab // saved | browse (remembered)
    onStreamsTabChanged: AppSettings.streamsTab = streamsTab

    signal toastRequested(string text, bool isError)
    signal mediaMenuRequested(var record, int playlistId)
    signal openFilesRequested()
    signal addFolderRequested()
    signal addFilesRequested()
    signal createPlaylistRequested(var paths)
    signal renamePlaylistRequested(int id)
    signal deletePlaylistRequested(int id)
    signal shortcutsRequested()
    signal openLinkRequested(bool importPlaylist)
    signal renameStreamRequested(string url, string name)
    signal fullscreenToggleRequested()

    function navigate(target, id) {
        if (id !== undefined)
            playlistId = id
        page = target
    }

    function toast(text, isError) {
        toastRequested(text, isError === true)
    }
}
