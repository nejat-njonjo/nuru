import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Persistent transport bar: now playing, transport + timeline, queue/volume.
Rectangle {
    id: bar

    readonly property bool favorite: {
        Library.revision // re-evaluate whenever the library / saved streams change
        Streams.count
        if (!Playback.hasMedia)
            return false
        return Playback.isStream ? Streams.isSaved(Playback.currentPath) : Library.isFavorite(Playback.currentPath)
    }
    readonly property var queue: Playback.queue

    implicitHeight: Theme.playbackBarHeight
    color: Theme.surface

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceL
        anchors.rightMargin: Theme.spaceL
        spacing: Theme.spaceL

        // --- Now playing -----------------------------------------------------
        RowLayout {
            Layout.preferredWidth: 300
            Layout.minimumWidth: 200
            Layout.maximumWidth: 340
            spacing: Theme.spaceM

            AbstractButton {
                id: artButton
                implicitWidth: 56
                implicitHeight: 56
                enabled: Playback.hasMedia
                hoverEnabled: true
                focusPolicy: Qt.TabFocus
                Accessible.name: "Open now playing"
                ToolTip.visible: hovered
                ToolTip.delay: 600
                ToolTip.text: "Now playing"
                onClicked: AppState.navigate("nowPlaying")
                contentItem: Artwork {
                    source: Playback.artwork
                    seed: Playback.subtitle + Playback.title
                    kind: Playback.isVideo ? "video" : "audio"
                    fallbackIcon: !Playback.hasMedia ? "vinyl" : Playback.isStream ? "stream" : Playback.isVideo ? "video" : "music"
                    radius: Theme.radiusM
                    decodeSize: 128
                    scale: artButton.hovered ? 1.04 : 1
                    Behavior on scale { NumberAnimation { duration: Theme.durationNormal } }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    Layout.fillWidth: true
                    text: Playback.hasMedia ? Playback.title : "Nothing playing"
                    elide: Text.ElideRight
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontBodyLarge
                    font.weight: Font.DemiBold
                }
                Text {
                    Layout.fillWidth: true
                    text: Playback.state === Playback.Error ? Playback.errorString
                          : Playback.hasMedia ? Playback.subtitle : "Pick something from your library"
                    elide: Text.ElideRight
                    color: Playback.state === Playback.Error ? Theme.danger : Theme.textSecondary
                    font.pixelSize: Theme.fontCaption + 1
                }
            }

            IconButton {
                visible: Playback.hasMedia
                iconName: Playback.isStream ? (bar.favorite ? "bookmark-fill" : "bookmark")
                                            : (bar.favorite ? "heart-fill" : "heart")
                iconColor: bar.favorite ? Theme.accent : (hovered ? Theme.textPrimary : Theme.textSecondary)
                tip: Playback.isStream ? (bar.favorite ? "Remove from My streams" : "Save to My streams")
                                       : (bar.favorite ? "Remove from favorites" : "Add to favorites")
                onClicked: {
                    if (!Playback.isStream)
                        Library.toggleFavorite(Playback.currentPath)
                    else if (bar.favorite)
                        Streams.remove(Playback.currentPath)
                    else
                        Streams.save(Playback.currentPath, Playback.title, Playback.artwork, "")
                }
            }
        }

        // --- Transport + timeline -------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: 720
            Layout.alignment: Qt.AlignHCenter
            spacing: 4

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: Theme.spaceS

                IconButton {
                    iconName: "shuffle"
                    checked: bar.queue.shuffle
                    tip: bar.queue.shuffle ? "Shuffle on" : "Shuffle off"
                    onClicked: bar.queue.shuffle = !bar.queue.shuffle
                    Rectangle {
                        visible: parent.checked
                        width: 4; height: 4; radius: 2
                        color: Theme.accent
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                    }
                }
                IconButton {
                    iconName: "previous-fill"
                    tip: "Previous (Ctrl+Left)"
                    enabled: Playback.hasMedia
                    onClicked: Playback.previous()
                }
                Item {
                    implicitWidth: 44
                    implicitHeight: 44
                    IconButton {
                        anchors.fill: parent
                        accented: true
                        iconSize: 20
                        iconName: Playback.playing ? "pause-fill" : "play-fill"
                        tip: Playback.playing ? "Pause (Space)" : "Play (Space)"
                        enabled: Playback.hasMedia || Playback.queue.count > 0
                        onClicked: Playback.togglePlayPause()
                    }
                    BusyIndicator {
                        anchors.centerIn: parent
                        width: 52
                        height: 52
                        running: Playback.loading
                        visible: running
                    }
                }
                IconButton {
                    iconName: "next-fill"
                    tip: "Next (Ctrl+Right)"
                    enabled: Playback.hasMedia && bar.queue.count > 1
                    onClicked: Playback.next()
                }
                IconButton {
                    iconName: bar.queue.repeatMode === PlayQueue.RepeatOne ? "repeat-one" : "repeat"
                    checked: bar.queue.repeatMode !== PlayQueue.RepeatOff
                    tip: bar.queue.repeatMode === PlayQueue.RepeatOff ? "Repeat off"
                       : bar.queue.repeatMode === PlayQueue.RepeatAll ? "Repeat all" : "Repeat one"
                    onClicked: bar.queue.cycleRepeatMode()
                    Rectangle {
                        visible: parent.checked
                        width: 4; height: 4; radius: 2
                        color: Theme.accent
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                    }
                }
                IconButton {
                    iconName: "stop-fill"
                    iconSize: 14
                    tip: "Stop (S)"
                    enabled: Playback.hasMedia && Playback.state !== Playback.Stopped
                    onClicked: Playback.stop()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceS
                Text {
                    Layout.preferredWidth: 48
                    horizontalAlignment: Text.AlignRight
                    text: Playback.timeText
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontCaption
                    font.features: { "tnum": 1 }
                }
                ProgressSlider {
                    Layout.fillWidth: true
                    playbackPosition: Playback.position
                    duration: Playback.duration
                    seekable: Playback.hasMedia && Playback.seekable && Playback.duration > 0
                    onSeekRequested: position => Playback.seek(position)
                }
                Text {
                    Layout.preferredWidth: 48
                    text: Playback.isLive ? "● LIVE" : Playback.durationText
                    color: Playback.isLive && Playback.playing ? Theme.danger : Theme.textSecondary
                    font.pixelSize: Theme.fontCaption
                    font.weight: Playback.isLive ? Font.Bold : Font.Normal
                    font.features: { "tnum": 1 }
                }
            }
        }

        // --- Queue / view / volume ---------------------------------------------------
        RowLayout {
            Layout.preferredWidth: 300
            Layout.minimumWidth: 200
            Layout.maximumWidth: 340
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            spacing: 2

            Item { Layout.fillWidth: true }

            IconButton {
                iconName: "info"
                visible: Playback.hasMedia
                checked: AppState.page === "nowPlaying"
                tip: "Now playing"
                onClicked: AppState.navigate(checked ? "home" : "nowPlaying")
            }
            IconButton {
                iconName: "queue"
                checked: AppState.queueOpen
                tip: "Queue (Q)"
                onClicked: AppState.queueOpen = !AppState.queueOpen
            }
            IconButton {
                iconName: "fullscreen"
                visible: Playback.isVideo
                tip: "Fullscreen (F)"
                onClicked: AppState.fullscreenToggleRequested()
            }
            VolumeControl {}
        }
    }
}
