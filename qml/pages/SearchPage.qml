import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Debounced search across title, artist, album, album artist, genre and filename.
Item {
    id: page

    MediaListModel {
        id: results
        library: Library
        filter: "all"
        searchText: AppState.searchText
    }

    Timer {
        id: debounce
        interval: 220
        onTriggered: AppState.searchText = field.text.trim()
    }

    Component.onCompleted: field.forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceL

        PageHeader {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spaceXxl
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            title: "Search"
            subtitle: AppState.searchText.length > 0
                      ? (results.count === 1 ? "1 result" : results.count + " results")
                      : "Find songs, artists, albums, genres and videos"
        }

        SearchField {
            id: field
            Layout.fillWidth: true
            Layout.maximumWidth: 640
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            placeholderText: "What do you want to play?"
            text: AppState.searchText
            onTextEdited: debounce.restart()
            Keys.onDownPressed: list.forceActiveFocus()
            Keys.onReturnPressed: if (results.count > 0) Playback.playPaths(results.paths(), 0)
        }

        TrackList {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: AppState.searchText.length > 0 ? results : null
            emptyState: Component {
                EmptyState {
                    iconName: "search"
                    title: AppState.searchText.length > 0 ? "No results for “" + AppState.searchText + "”"
                                                          : "Search your library"
                    message: AppState.searchText.length > 0
                             ? "Check the spelling or try a different word."
                             : "Results appear as you type."
                }
            }
        }
    }
}
