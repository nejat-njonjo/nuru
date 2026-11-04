import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Channel tile: logo, name, country/category line, quality / geo-blocked badges.
AbstractButton {
    id: card

    property string name
    property string url
    property string logo
    property string subtitle
    property string quality
    property bool geoBlocked: false
    property bool saved: false
    readonly property bool playing: url.length > 0 && url === Playback.currentPath

    signal playRequested()
    signal saveToggled()
    signal menuRequested()

    implicitWidth: 200
    implicitHeight: content.implicitHeight + 24
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    Accessible.role: Accessible.Button
    Accessible.name: name

    onClicked: playRequested()

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: card.menuRequested()
    }

    background: Rectangle {
        radius: Theme.radiusL
        color: card.down ? Theme.pressed : card.hovered ? Theme.elevated : "transparent"
        border.width: card.visualFocus ? 2 : 0
        border.color: Theme.accent
        Behavior on color { ColorAnimation { duration: Theme.durationNormal } }
    }

    contentItem: Item {
        ColumnLayout {
            id: content
            x: 12
            y: 12
            width: card.width - 24
            spacing: 6

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: width * 9 / 16

                Artwork {
                    anchors.fill: parent
                    source: card.logo
                    seed: card.name
                    fallbackIcon: "stream"
                    radius: Theme.radiusM
                    decodeSize: 256
                }

                Row {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.margins: 6
                    spacing: 4
                    Rectangle {
                        visible: card.playing
                        width: liveText.implicitWidth + 12
                        height: 18
                        radius: 9
                        color: Theme.danger
                        Text {
                            id: liveText
                            anchors.centerIn: parent
                            text: "LIVE"
                            color: "white"
                            font.pixelSize: 9
                            font.weight: Font.Bold
                        }
                    }
                    Rectangle {
                        visible: card.quality.length > 0
                        width: qualityText.implicitWidth + 10
                        height: 18
                        radius: 9
                        color: Qt.rgba(0, 0, 0, 0.6)
                        Text {
                            id: qualityText
                            anchors.centerIn: parent
                            text: card.quality
                            color: "white"
                            font.pixelSize: 9
                            font.weight: Font.DemiBold
                        }
                    }
                }

                IconButton {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 4
                    diameter: 30
                    iconSize: 16
                    iconName: card.saved ? "bookmark-fill" : "bookmark"
                    iconColor: card.saved ? Theme.accent : "#18181B"
                    tip: card.saved ? "Remove from My streams" : "Save to My streams"
                    visible: card.hovered || card.saved
                    background: Rectangle { radius: 15; color: Qt.rgba(1, 1, 1, 0.85) }
                    onClicked: card.saveToggled()
                }

                IconButton {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 6
                    accented: true
                    diameter: 38
                    iconSize: 16
                    iconName: card.playing && Playback.playing ? "pause-fill" : "play-fill"
                    tip: card.playing && Playback.playing ? "Pause" : "Watch"
                    opacity: card.hovered || card.playing ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.durationNormal } }
                    onClicked: card.playing ? Playback.togglePlayPause() : card.playRequested()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Text {
                    Layout.fillWidth: true
                    text: card.name
                    elide: Text.ElideRight
                    color: card.playing ? Theme.accent : Theme.textPrimary
                    font.pixelSize: Theme.fontBody + 1
                    font.weight: Font.DemiBold
                }
                Icon {
                    visible: card.geoBlocked
                    name: "geo-blocked"
                    size: 14
                    color: Theme.textMuted
                    HoverHandler { id: geoHover }
                    ToolTip.visible: geoHover.hovered
                    ToolTip.text: "May be blocked outside its country"
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.topMargin: -2
                text: card.subtitle
                elide: Text.ElideRight
                color: Theme.textSecondary
                font.pixelSize: Theme.fontCaption + 1
            }
        }
    }
}
