import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Primary navigation: discover pages, library pages, playlists, library actions.
Rectangle {
    id: sidebar

    color: Theme.surface
    implicitWidth: Theme.sidebarWidth

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.border
    }

    component SectionLabel: Text {
        Layout.topMargin: Theme.spaceL
        Layout.leftMargin: 12
        Layout.bottomMargin: 4
        color: Theme.textMuted
        font.pixelSize: Theme.fontCaption - 1
        font.weight: Font.Bold
        font.letterSpacing: 1.4
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceM
        anchors.rightMargin: Theme.spaceM + 1
        spacing: 2

        // Brand
        RowLayout {
            Layout.leftMargin: 10
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            spacing: 10
            Image {
                source: "qrc:/nuru/assets/icons/app/com.nuru.player.svg"
                sourceSize: Qt.size(68, 68)
                Layout.preferredWidth: 34
                Layout.preferredHeight: 34
            }
            Text {
                text: "nuru"
                color: Theme.textPrimary
                font.pixelSize: 24
                font.weight: Font.Black
                font.letterSpacing: -0.8
            }
        }

        SectionLabel { text: "DISCOVER" }
        NavItem {
            text: "Home"
            iconName: "home"; activeIconName: "home-fill"
            active: AppState.page === "home"
            onClicked: AppState.navigate("home")
        }
        NavItem {
            text: "Search"
            iconName: "search"
            active: AppState.page === "search"
            onClicked: AppState.navigate("search")
        }
        NavItem {
            text: "Music"
            iconName: "music"; activeIconName: "music-fill"
            badge: Library.musicCount > 0 ? Library.musicCount : ""
            active: AppState.page === "music"
            onClicked: AppState.navigate("music")
        }
        NavItem {
            text: "Videos"
            iconName: "video"; activeIconName: "video-fill"
            badge: Library.videoCount > 0 ? Library.videoCount : ""
            active: AppState.page === "videos"
            onClicked: AppState.navigate("videos")
        }
        NavItem {
            text: "Streams"
            iconName: "stream"; activeIconName: "stream-fill"
            active: AppState.page === "streams"
            onClicked: AppState.navigate("streams")
        }
        NavItem {
            text: "Recently played"
            iconName: "history"
            active: AppState.page === "recent"
            onClicked: AppState.navigate("recent")
        }

        SectionLabel { text: "YOUR LIBRARY" }
        NavItem {
            text: "Favorites"
            iconName: "heart"; activeIconName: "heart-fill"
            badge: Library.favoriteCount > 0 ? Library.favoriteCount : ""
            active: AppState.page === "favorites"
            onClicked: AppState.navigate("favorites")
        }
        NavItem {
            text: "Playlists"
            iconName: "playlist"; activeIconName: "playlist-fill"
            active: AppState.page === "playlists"
            onClicked: AppState.navigate("playlists")
        }

        // Playlist shortcuts
        ListView {
            id: playlistList
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 40
            clip: true
            model: Library.playlists
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            delegate: NavItem {
                required property var modelData
                width: ListView.view.width
                implicitHeight: 34
                text: modelData.name
                iconName: "playlist"
                badge: modelData.count
                active: AppState.page === "playlist" && AppState.playlistId === modelData.id
                onClicked: AppState.navigate("playlist", modelData.id)
            }
        }

        NavItem {
            text: "Open files…"
            iconName: "file-add"
            onClicked: AppState.openFilesRequested()
            ToolTip.visible: hovered
            ToolTip.delay: 800
            ToolTip.text: "Play files without adding them (Ctrl+O)"
        }
        NavItem {
            text: "Open link…"
            iconName: "link"
            onClicked: AppState.openLinkRequested(false)
            ToolTip.visible: hovered
            ToolTip.delay: 800
            ToolTip.text: "Play a stream or media URL (Ctrl+L)"
        }
        NavItem {
            text: "Add folder…"
            iconName: "folder-add"
            onClicked: AppState.addFolderRequested()
            ToolTip.visible: hovered
            ToolTip.delay: 800
            ToolTip.text: "Add a folder to your library (Ctrl+Shift+O)"
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 6
            Layout.bottomMargin: 6
            height: 1
            color: Theme.border
        }

        ScanStatus {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 4
            Layout.bottomMargin: 6
        }

        NavItem {
            text: "Manage library"
            iconName: "library"; activeIconName: "library-fill"
            active: AppState.page === "library"
            onClicked: AppState.navigate("library")
        }
        NavItem {
            text: "Settings"
            iconName: "settings"
            active: AppState.page === "settings"
            onClicked: AppState.navigate("settings")
        }
    }
}
