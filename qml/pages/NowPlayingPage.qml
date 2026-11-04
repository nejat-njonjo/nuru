import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Window
import Nuru

// Video surface for video media; large artwork + up next for audio.
// In fullscreen the controls float over the video and hide after inactivity.
Item {
    id: page

    readonly property bool fullscreen: Window.visibility === Window.FullScreen
    property bool controlsVisible: true

    function poke() {
        controlsVisible = true
        hideTimer.restart()
    }

    Timer {
        id: hideTimer
        interval: 2600
        running: Playback.isVideo && Playback.playing
        onTriggered: if (!overlayHover.hovered && !fsHover.hovered) page.controlsVisible = false
    }
    Connections {
        target: Playback
        function onStateChanged() { if (!Playback.playing) page.controlsVisible = true }
    }

    // ---------------------------------------------------------------- Video
    Rectangle {
        anchors.fill: parent
        color: "black"
        visible: Playback.isVideo

        VideoOutput {
            id: video
            anchors.fill: parent
            sink: Playback.videoSink
        }

        BusyIndicator {
            anchors.centerIn: parent
            running: Playback.loading
            visible: running
        }

        Column {
            anchors.centerIn: parent
            spacing: 12
            visible: !Playback.videoSink.hasFrame && !Playback.loading
            Icon {
                anchors.horizontalCenter: parent.horizontalCenter
                name: Playback.state === Playback.Error ? "warning" : "video"
                size: 48
                color: "#A1A1AA"
            }
            Text {
                text: Playback.state === Playback.Error ? Playback.errorString
                      : Playback.state === Playback.Finished ? "Finished" : "Press play to start"
                color: "#A1A1AA"
                font.pixelSize: Theme.fontBodyLarge
            }
        }

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: page.controlsVisible || !Playback.playing ? Qt.ArrowCursor : Qt.BlankCursor
            onPositionChanged: page.poke()
            onClicked: { page.poke(); Playback.togglePlayPause() }
            onDoubleClicked: { Playback.togglePlayPause(); AppState.fullscreenToggleRequested() }
            onWheel: wheel => Playback.changeVolume(wheel.angleDelta.y > 0 ? 5 : -5)
        }

        // Top overlay: back + title
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 84
            opacity: page.controlsVisible ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: Theme.durationSlow } }
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.rgba(0, 0, 0, 0.7) }
                GradientStop { position: 1; color: "transparent" }
            }
            HoverHandler { id: overlayHover }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceL
                anchors.rightMargin: Theme.spaceL
                anchors.bottomMargin: 24
                spacing: Theme.spaceM
                IconButton {
                    iconName: "back"
                    iconColor: "white"
                    tip: page.fullscreen ? "Exit fullscreen (Esc)" : "Back to library"
                    onClicked: {
                        if (page.fullscreen)
                            AppState.fullscreenToggleRequested()
                        AppState.navigate(Playback.isStream ? "streams" : "videos")
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: Playback.title
                    color: "white"
                    elide: Text.ElideRight
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.DemiBold
                }
                IconButton {
                    iconName: page.fullscreen ? "fullscreen-exit" : "fullscreen"
                    iconColor: "white"
                    tip: page.fullscreen ? "Exit fullscreen (Esc)" : "Fullscreen (F)"
                    onClicked: AppState.fullscreenToggleRequested()
                }
            }
        }

        // Bottom overlay controls (fullscreen only; the playback bar is hidden there)
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 110
            visible: page.fullscreen && opacity > 0
            opacity: page.controlsVisible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.durationSlow } }
            gradient: Gradient {
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0.8) }
            }
            HoverHandler { id: fsHover }
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceXl
                anchors.rightMargin: Theme.spaceXl
                anchors.topMargin: 30
                anchors.bottomMargin: Theme.spaceM
                spacing: 4
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceM
                    Text { text: Playback.timeText; color: "white"; font.pixelSize: Theme.fontCaption; font.features: { "tnum": 1 } }
                    ProgressSlider {
                        Layout.fillWidth: true
                        playbackPosition: Playback.position
                        duration: Playback.duration
                        seekable: Playback.seekable && Playback.duration > 0
                        onSeekRequested: position => Playback.seek(position)
                    }
                    Text { text: Playback.durationText; color: "white"; font.pixelSize: Theme.fontCaption; font.features: { "tnum": 1 } }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceS
                    IconButton { iconName: "previous-fill"; iconColor: "white"; tip: "Previous"; onClicked: Playback.previous() }
                    IconButton {
                        accented: true
                        iconName: Playback.playing ? "pause-fill" : "play-fill"
                        tip: Playback.playing ? "Pause" : "Play"
                        onClicked: Playback.togglePlayPause()
                    }
                    IconButton { iconName: "next-fill"; iconColor: "white"; tip: "Next"; onClicked: Playback.next() }
                    IconButton { iconName: "rewind-10"; iconColor: "white"; tip: "Back 10 seconds (Left)"; onClicked: Playback.seekBy(-10000) }
                    IconButton { iconName: "forward-10"; iconColor: "white"; tip: "Forward 10 seconds (Right)"; onClicked: Playback.seekBy(10000) }
                    Item { Layout.fillWidth: true }
                    VolumeControl {}
                    IconButton {
                        iconName: "fullscreen-exit"
                        iconColor: "white"
                        tip: "Exit fullscreen (Esc)"
                        onClicked: AppState.fullscreenToggleRequested()
                    }
                }
            }
        }
    }

    // ---------------------------------------------------------------- Audio
    Item {
        anchors.fill: parent
        visible: !Playback.isVideo
        clip: true

        Image {
            id: backdropSource
            anchors.fill: parent
            source: Playback.artwork
            sourceSize: Qt.size(256, 256)
            fillMode: Image.PreserveAspectCrop
            visible: false
        }
        MultiEffect {
            anchors.fill: parent
            source: backdropSource
            visible: backdropSource.status === Image.Ready
            blurEnabled: true
            blurMax: 64
            blur: 1.0
            saturation: 0.2
            brightness: Theme.dark ? -0.45 : 0.35
            opacity: 0.85
        }
        Rectangle {
            anchors.fill: parent
            z: -1
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.22) }
                GradientStop { position: 1; color: Theme.background }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.pageMargin + 8
            spacing: Theme.spaceXxl * 1.5

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 3
                spacing: Theme.spaceL

                Item { Layout.fillHeight: true }
                Artwork {
                    Layout.alignment: Qt.AlignHCenter
                    readonly property real side: Math.min(parent.width, page.height - 300, 420)
                    Layout.preferredWidth: side
                    Layout.preferredHeight: side
                    source: Playback.artwork
                    seed: Playback.subtitle + Playback.title
                    fallbackIcon: !Playback.hasMedia ? "vinyl" : Playback.isStream ? "stream" : "music"
                    radius: Theme.radiusXl
                    decodeSize: 512
                }
                Text {
                    Layout.fillWidth: true
                    Layout.topMargin: Theme.spaceL
                    horizontalAlignment: Text.AlignHCenter
                    text: Playback.hasMedia ? Playback.title : "Nothing playing"
                    elide: Text.ElideRight
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontHeading + 4
                    font.weight: Font.Bold
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: Playback.hasMedia ? Playback.subtitle : "Choose something from your library"
                    elide: Text.ElideRight
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontTitle
                }
                Item { Layout.fillHeight: true }
            }

            // Up next
            ColumnLayout {
                Layout.fillHeight: true
                Layout.preferredWidth: 2
                Layout.maximumWidth: 420
                visible: page.width > 900
                spacing: Theme.spaceM
                QueuePanel {
                    embedded: true
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Qt.rgba(Theme.surface.r, Theme.surface.g, Theme.surface.b, 0.6)
                    radius: Theme.radiusL
                }
            }
        }
    }
}
