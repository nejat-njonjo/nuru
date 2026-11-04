import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import Nuru

// Application shell: sidebar, page stack, playback bar, queue panel, dialogs and shortcuts.
ApplicationWindow {
    id: window

    readonly property bool isFullscreen: visibility === Window.FullScreen
    // Immersive video: hide all chrome while a video plays fullscreen.
    readonly property bool immersive: isFullscreen && AppState.page === "nowPlaying" && Playback.isVideo
    property bool wasMaximized: false

    width: 1280
    height: 800
    minimumWidth: 950
    minimumHeight: 650
    visible: true
    title: Playback.hasMedia ? Playback.title + " — Nuru" : "Nuru"
    color: Theme.background

    palette {
        window: Theme.background
        windowText: Theme.textPrimary
        base: Theme.surface
        alternateBase: Theme.elevated
        text: Theme.textPrimary
        button: Theme.elevated
        buttonText: Theme.textPrimary
        highlight: Theme.accent
        highlightedText: Theme.accentText
        placeholderText: Theme.textMuted
        toolTipBase: Theme.dark ? "#F4F4F5" : "#18181B"
        toolTipText: Theme.dark ? "#18181B" : "#FAFAFA"
        light: Theme.hover
        midlight: Theme.elevated
        mid: Theme.border
        dark: Theme.track
        shadow: "#000000"
        brightText: Theme.textPrimary
        link: Theme.accent
    }

    function toggleFullscreen() {
        if (isFullscreen) {
            if (wasMaximized)
                showMaximized()
            else
                showNormal()
        } else {
            wasMaximized = visibility === Window.Maximized
            showFullScreen()
        }
    }

    // ------------------------------------------------------------------ Pages
    Component { id: homePage; HomePage {} }
    Component { id: searchPage; SearchPage {} }
    Component {
        id: musicPage
        CollectionPage { filter: "audio"; title: "Music" }
    }
    Component {
        id: recentPage
        CollectionPage {
            filter: "recent"
            title: "Recently played"
            sortable: false
            emptyIcon: "history"
            emptyTitle: "Nothing played yet"
            emptyMessage: "Songs and videos you play show up here."
        }
    }
    Component {
        id: favoritesPage
        CollectionPage {
            filter: "favorites"
            title: "Favorites"
            emptyIcon: "heart"
            emptyTitle: "No favorites yet"
            emptyMessage: "Tap the heart on any song or video to keep it here."
        }
    }
    Component { id: videosPage; VideosPage {} }
    Component { id: playlistsPage; PlaylistsPage {} }
    Component { id: playlistPage; PlaylistPage {} }
    Component { id: libraryPage; LibraryPage {} }
    Component { id: nowPlayingPage; NowPlayingPage {} }
    Component { id: settingsPage; SettingsPage {} }
    Component { id: streamsPage; StreamsPage {} }

    function pageComponent(name) {
        switch (name) {
        case "search": return searchPage
        case "music": return musicPage
        case "recent": return recentPage
        case "favorites": return favoritesPage
        case "videos": return videosPage
        case "playlists": return playlistsPage
        case "playlist": return playlistPage
        case "library": return libraryPage
        case "nowPlaying": return nowPlayingPage
        case "settings": return settingsPage
        case "streams": return streamsPage
        default: return homePage
        }
    }

    Connections {
        target: AppState
        function onPageChanged() {
            stack.replace(stack.currentItem, window.pageComponent(AppState.page))
        }
        function onPlaylistIdChanged() {
            if (AppState.page === "playlist")
                stack.replace(stack.currentItem, playlistPage)
        }
        function onToastRequested(text, isError) { toast.show(text, isError) }
        function onMediaMenuRequested(record, playlistId) { mediaMenu.show(record, playlistId) }
        function onOpenFilesRequested() { openDialog.open() }
        function onAddFolderRequested() { folderDialog.open() }
        function onAddFilesRequested() { addFilesDialog.open() }
        function onCreatePlaylistRequested(paths) {
            createDialog.pendingPaths = paths
            createDialog.text = ""
            createDialog.open()
        }
        function onRenamePlaylistRequested(id) {
            renameDialog.playlistId = id
            renameDialog.text = Library.playlistName(id)
            renameDialog.open()
        }
        function onDeletePlaylistRequested(id) {
            deleteDialog.playlistId = id
            deleteDialog.open()
        }
        function onShortcutsRequested() { shortcutsDialog.open() }
        function onOpenLinkRequested(importPlaylist) { openLinkDialog.openFor(importPlaylist) }
        function onRenameStreamRequested(url, name) {
            renameStreamDialog.url = url
            renameStreamDialog.text = name
            renameStreamDialog.open()
        }
        function onFullscreenToggleRequested() { window.toggleFullscreen() }
    }

    Connections {
        target: Library
        function onMessage(text, isError) { toast.show(text, isError) }
    }
    Connections {
        target: Streams
        function onMessage(text, isError) { toast.show(text, isError) }
    }

    Connections {
        target: Playback
        function onErrorOccurred(message) { toast.show(message, true) }
        // Open Now Playing once per item when it turns out to be video (streams only know this
        // after LibVLC creates a video output); metadata refreshes must not navigate again.
        property string autoOpenedPath
        function onCurrentMediaChanged() {
            if (Playback.isVideo && Playback.state !== Playback.Stopped && autoOpenedPath !== Playback.currentPath) {
                autoOpenedPath = Playback.currentPath
                AppState.navigate("nowPlaying")
                if (AppSettings.openVideosFullscreen && !window.isFullscreen)
                    window.toggleFullscreen()
            }
        }
    }

    // ------------------------------------------------------------------ Layout
    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            Layout.fillHeight: true
            visible: !window.immersive
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                StackView {
                    id: stack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    initialItem: homePage
                    onCurrentItemChanged: if (currentItem) currentItem.forceActiveFocus()

                    replaceEnter: Transition {
                        ParallelAnimation {
                            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durationNormal; easing.type: Easing.OutCubic }
                            NumberAnimation { property: "y"; from: 14; to: 0; duration: Theme.durationSlow; easing.type: Easing.OutCubic }
                        }
                    }
                    replaceExit: Transition {
                        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durationFast }
                    }
                }

                QueuePanel {
                    Layout.fillHeight: true
                    Layout.preferredWidth: AppState.queueOpen && !window.immersive && AppState.page !== "nowPlaying" ? 360 : 0
                    visible: Layout.preferredWidth > 0
                    clip: true
                    Behavior on Layout.preferredWidth { NumberAnimation { duration: Theme.durationNormal; easing.type: Easing.OutCubic } }
                }
            }

            PlaybackBar {
                Layout.fillWidth: true
                visible: !window.immersive
            }
        }
    }

    Toast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: window.immersive ? 32 : Theme.playbackBarHeight + 20
        z: 100
    }

    // ------------------------------------------------------------------ Dialogs & menus
    MediaMenu { id: mediaMenu }

    FileDialog {
        id: openDialog
        title: "Open media"
        fileMode: FileDialog.OpenFiles
        nameFilters: Library.nameFilters
        onAccepted: Playback.openUrls(selectedFiles)
    }
    FileDialog {
        id: addFilesDialog
        title: "Add files to your library"
        fileMode: FileDialog.OpenFiles
        nameFilters: Library.nameFilters
        onAccepted: Library.addFiles(selectedFiles)
    }
    FolderDialog {
        id: folderDialog
        title: "Add a folder to your library"
        onAccepted: {
            Library.addFolder(selectedFolder)
            AppState.navigate("library")
        }
    }

    TextInputDialog {
        id: createDialog
        property var pendingPaths: []
        title: "New playlist"
        placeholder: "My playlist"
        confirmText: "Create"
        onSubmitted: value => {
            const id = Library.createPlaylist(value)
            if (id < 0)
                return
            if (pendingPaths.length > 0)
                Library.addToPlaylist(id, pendingPaths)
            else
                AppState.navigate("playlist", id)
        }
    }
    TextInputDialog {
        id: renameDialog
        property int playlistId: -1
        title: "Rename playlist"
        confirmText: "Rename"
        onSubmitted: value => Library.renamePlaylist(playlistId, value)
    }
    ConfirmDialog {
        id: deleteDialog
        property int playlistId: -1
        title: "Delete playlist?"
        message: "“" + Library.playlistName(playlistId) + "” will be deleted. The media in it stays in your library."
        confirmText: "Delete"
        destructive: true
        onAccepted: {
            Library.deletePlaylist(playlistId)
            AppState.navigate("playlists")
        }
    }
    ShortcutsDialog { id: shortcutsDialog }
    OpenLinkDialog { id: openLinkDialog }
    TextInputDialog {
        id: renameStreamDialog
        property string url
        title: "Rename stream"
        confirmText: "Rename"
        onSubmitted: value => Streams.rename(url, value)
    }

    // ------------------------------------------------------------------ Shortcuts
    // Single-key shortcuts do not fire while a text field has focus: text inputs accept the
    // key through ShortcutOverride first.
    Shortcut { sequence: "Space"; onActivated: Playback.togglePlayPause() }
    Shortcut { sequences: ["Ctrl+Right", StandardKey.Forward]; onActivated: Playback.next() }
    Shortcut { sequences: ["Ctrl+Left", StandardKey.Back]; onActivated: Playback.previous() }
    Shortcut { sequence: "Right"; onActivated: Playback.seekBy(5000) }
    Shortcut { sequence: "Left"; onActivated: Playback.seekBy(-5000) }
    Shortcut { sequence: "Ctrl+Up"; onActivated: Playback.changeVolume(5) }
    Shortcut { sequence: "Ctrl+Down"; onActivated: Playback.changeVolume(-5) }
    Shortcut { sequence: "M"; onActivated: Playback.toggleMute() }
    Shortcut { sequence: "S"; onActivated: Playback.stop() }
    Shortcut { sequences: ["F", "F11"]; onActivated: window.toggleFullscreen() }
    Shortcut { sequence: "Esc"; enabled: window.isFullscreen; onActivated: window.toggleFullscreen() }
    Shortcut { sequence: "Q"; onActivated: AppState.queueOpen = !AppState.queueOpen }
    Shortcut { sequence: "Ctrl+O"; onActivated: openDialog.open() }
    Shortcut { sequence: "Ctrl+Shift+O"; onActivated: folderDialog.open() }
    Shortcut { sequences: ["Ctrl+L", "Ctrl+U"]; onActivated: openLinkDialog.openFor(false) }
    Shortcut { sequence: "Ctrl+F"; onActivated: AppState.navigate("search") }
    Shortcut { sequence: "Ctrl+,"; onActivated: AppState.navigate("settings") }
    Shortcut { sequences: ["Ctrl+/", "F1"]; onActivated: shortcutsDialog.open() }
    Shortcut { sequence: "Ctrl+Q"; onActivated: Qt.quit() }
}
