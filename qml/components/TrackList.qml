import QtQuick
import QtQuick.Controls
import Nuru

// Scrollable list of media rows backed by a MediaListModel. Pages supply a `header`
// component so the page title scrolls together with the list.
ListView {
    id: list

    property int playlistId: -1
    property bool reorderable: false
    property Component emptyState

    clip: true
    reuseItems: true
    boundsBehavior: Flickable.StopAtBounds
    keyNavigationEnabled: true
    focus: true
    currentIndex: -1
    spacing: 2
    bottomMargin: Theme.spaceXl
    leftMargin: Theme.pageMargin - 8
    rightMargin: Theme.pageMargin - 8

    Accessible.role: Accessible.List

    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    // Keep the page header in view when its height settles after the first load.
    Connections {
        target: list.headerItem
        ignoreUnknownSignals: true
        function onHeightChanged() {
            if (!list.moving && list.contentY <= list.originY + list.headerItem.height)
                list.positionViewAtBeginning()
        }
    }

    function playFrom(row) {
        Playback.playPaths(model.paths(), row)
    }

    Keys.onReturnPressed: if (currentIndex >= 0) playFrom(currentIndex)
    Keys.onEnterPressed: if (currentIndex >= 0) playFrom(currentIndex)
    Keys.onMenuPressed: if (currentIndex >= 0) AppState.mediaMenuRequested(model.get(currentIndex), playlistId)

    delegate: TrackRow {
        required property int index
        required property var model

        number: index + 1
        title: model.title
        subtitle: model.subtitle
        album: model.album
        artwork: model.artwork
        kind: model.kind
        path: model.path
        duration: model.duration
        favorite: model.favorite
        width: ListView.view.width - list.leftMargin - list.rightMargin
        reorderable: list.reorderable
        canMoveUp: index > 0
        canMoveDown: index < list.count - 1

        onPlayRequested: list.playFrom(index)
        onClicked: {
            list.currentIndex = index
            list.forceActiveFocus()
        }
        onMenuRequested: AppState.mediaMenuRequested(list.model.get(index), list.playlistId)
        onMoveUpRequested: Library.movePlaylistItem(list.playlistId, index, index - 1)
        onMoveDownRequested: Library.movePlaylistItem(list.playlistId, index, index + 1)
        onRemoveRequested: Library.removeFromPlaylist(list.playlistId, model.path)
    }

    footer: Item {
        width: list.width - list.leftMargin - list.rightMargin
        height: list.count === 0 && list.emptyState ? Math.max(320, list.height - (list.headerItem ? list.headerItem.height : 0) - 40) : 0
        Loader {
            anchors.centerIn: parent
            active: list.count === 0 && list.emptyState !== null
            sourceComponent: list.emptyState
        }
    }
}
