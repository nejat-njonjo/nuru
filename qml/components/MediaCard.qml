import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Artwork card used for shelves and grids (albums, videos, playlists).
AbstractButton {
    id: card

    property string title
    property string subtitle
    property string artwork
    property string seed: title
    property string kind: "audio"
    property string fallbackIcon: kind === "video" ? "video" : "music"
    property real artAspect: kind === "video" ? 16 / 9 : 1
    property bool playing: false
    property bool showPlayButton: true

    signal playRequested()
    signal menuRequested()

    implicitWidth: 180
    implicitHeight: content.implicitHeight + 24
    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    Accessible.role: Accessible.Button
    Accessible.name: title

    background: Rectangle {
        radius: Theme.radiusL
        color: card.down ? Theme.pressed : card.hovered ? Theme.elevated : "transparent"
        border.width: card.visualFocus ? 2 : 0
        border.color: Theme.accent
        Behavior on color { ColorAnimation { duration: Theme.durationNormal } }
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: card.menuRequested()
    }

    contentItem: Item {
        ColumnLayout {
            id: content
            x: 12
            y: 12
            width: card.width - 24
            spacing: 8

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: width / card.artAspect

                Artwork {
                    id: art
                    anchors.fill: parent
                    source: card.artwork
                    seed: card.seed
                    kind: card.kind
                    fallbackIcon: card.fallbackIcon
                    radius: Theme.radiusM
                    decodeSize: 384
                    scale: card.hovered ? 1.02 : 1
                    Behavior on scale { NumberAnimation { duration: Theme.durationSlow; easing.type: Easing.OutCubic } }
                }

                IconButton {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 8
                    visible: card.showPlayButton
                    accented: true
                    diameter: 42
                    iconSize: 18
                    iconName: card.playing && Playback.playing ? "pause-fill" : "play-fill"
                    tip: card.playing && Playback.playing ? "Pause" : "Play"
                    opacity: card.hovered || card.playing ? 1 : 0
                    anchors.bottomMargin: card.hovered || card.playing ? 8 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.durationNormal } }
                    Behavior on anchors.bottomMargin { NumberAnimation { duration: Theme.durationNormal; easing.type: Easing.OutCubic } }
                    onClicked: card.playing ? Playback.togglePlayPause() : card.playRequested()
                }
            }

            Text {
                Layout.fillWidth: true
                text: card.title
                elide: Text.ElideRight
                color: card.playing ? Theme.accent : Theme.textPrimary
                font.pixelSize: Theme.fontBodyLarge
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                Layout.topMargin: -4
                visible: text.length > 0
                text: card.subtitle
                elide: Text.ElideRight
                color: Theme.textSecondary
                font.pixelSize: Theme.fontBody
            }
        }
    }
}
