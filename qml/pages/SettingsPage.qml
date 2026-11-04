import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Application preferences, persisted through AppSettings (QSettings).
Flickable {
    id: page

    contentHeight: column.implicitHeight + Theme.spaceXxl * 2
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}

    component Section: Rectangle {
        id: section
        default property alias content: inner.data
        property string heading
        Layout.fillWidth: true
        implicitHeight: inner.implicitHeight + 40
        radius: Theme.radiusL
        color: Theme.surface
        border.color: Theme.border
        ColumnLayout {
            id: inner
            anchors.fill: parent
            anchors.margins: 20
            spacing: 18
            Text {
                text: section.heading
                color: Theme.textPrimary
                font.pixelSize: Theme.fontTitle
                font.weight: Font.Bold
            }
        }
    }

    component ThemedSwitch: Switch {
        id: sw
        indicator: Rectangle {
            implicitWidth: 44
            implicitHeight: 24
            x: sw.leftPadding
            y: parent.height / 2 - height / 2
            radius: 12
            color: sw.checked ? Theme.accent : Theme.track
            border.width: sw.visualFocus ? 2 : 0
            border.color: Theme.textPrimary
            Behavior on color { ColorAnimation { duration: Theme.durationFast } }
            Rectangle {
                x: sw.checked ? parent.width - width - 3 : 3
                y: 3
                width: 18
                height: 18
                radius: 9
                color: "white"
                Behavior on x { NumberAnimation { duration: Theme.durationNormal; easing.type: Easing.OutCubic } }
            }
        }
    }

    component Segmented: RowLayout {
        id: seg
        property var options: []     // [{ value, label }]
        property var current
        signal picked(var value)
        spacing: 4
        Repeater {
            model: seg.options
            delegate: PillButton {
                required property var modelData
                text: modelData.label
                primary: modelData.value === seg.current
                implicitHeight: 32
                onClicked: seg.picked(modelData.value)
            }
        }
    }

    ColumnLayout {
        id: column
        x: Theme.pageMargin
        width: Math.min(page.width - Theme.pageMargin * 2, 860)
        spacing: Theme.spaceXl

        PageHeader {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spaceXxl
            title: "Settings"
            subtitle: "Changes are saved automatically"
        }

        Section {
            heading: "Appearance"
            SettingRow {
                title: "Theme"
                Segmented {
                    options: [{ value: "dark", label: "Dark" }, { value: "light", label: "Light" }]
                    current: AppSettings.theme
                    onPicked: value => AppSettings.theme = value
                }
            }
            SettingRow {
                title: "Accent colour"
                Repeater {
                    model: Theme.accentChoices
                    delegate: AbstractButton {
                        id: swatch
                        required property string modelData
                        implicitWidth: 28
                        implicitHeight: 28
                        hoverEnabled: true
                        focusPolicy: Qt.TabFocus
                        Accessible.name: "Accent " + modelData
                        onClicked: AppSettings.accentColor = modelData
                        background: Rectangle {
                            radius: 14
                            color: swatch.modelData
                            border.width: AppSettings.accentColor.toLowerCase() === swatch.modelData.toLowerCase() || swatch.visualFocus ? 3 : 0
                            border.color: Theme.textPrimary
                            scale: swatch.hovered ? 1.1 : 1
                            Behavior on scale { NumberAnimation { duration: Theme.durationFast } }
                        }
                    }
                }
            }
            SettingRow {
                title: "Interface scale"
                description: "Applies after restarting Nuru"
                Segmented {
                    options: [{ value: 0.9, label: "90%" }, { value: 1.0, label: "100%" },
                              { value: 1.15, label: "115%" }, { value: 1.25, label: "125%" }, { value: 1.5, label: "150%" }]
                    current: AppSettings.uiScale
                    onPicked: value => AppSettings.uiScale = value
                }
            }
        }

        Section {
            heading: "Playback"
            SettingRow {
                title: "Remember volume"
                description: "Start with the volume you used last time"
                ThemedSwitch {
                    checked: AppSettings.rememberVolume
                    onToggled: AppSettings.rememberVolume = checked
                }
            }
            SettingRow {
                title: "Default volume"
                description: AppSettings.rememberVolume ? "Used when “Remember volume” is off" : "Volume when Nuru starts"
                AccentSlider {
                    id: defaultVolume
                    Layout.preferredWidth: 180
                    from: 0; to: 100; stepSize: 5
                    value: AppSettings.defaultVolume
                    onMoved: AppSettings.defaultVolume = value
                    Accessible.name: "Default volume"
                }
                Text {
                    Layout.preferredWidth: 40
                    text: Math.round(defaultVolume.value) + "%"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontBody
                }
            }
            SettingRow {
                title: "Resume playback"
                description: "Restore the queue and position when Nuru starts"
                ThemedSwitch {
                    checked: AppSettings.resumePlayback
                    onToggled: AppSettings.resumePlayback = checked
                }
            }
            SettingRow {
                title: "Repeat"
                Segmented {
                    options: [{ value: 0, label: "Off" }, { value: 1, label: "All" }, { value: 2, label: "One" }]
                    current: Playback.queue.repeatMode
                    onPicked: value => Playback.queue.repeatMode = value
                }
            }
            SettingRow {
                title: "Shuffle"
                ThemedSwitch {
                    checked: Playback.queue.shuffle
                    onToggled: Playback.queue.shuffle = checked
                }
            }
            SettingRow {
                title: "Open videos in fullscreen"
                ThemedSwitch {
                    checked: AppSettings.openVideosFullscreen
                    onToggled: AppSettings.openVideosFullscreen = checked
                }
            }
        }

        Section {
            heading: "Library"
            SettingRow {
                title: "Scan for changes on startup"
                description: "Picks up new, changed and deleted files in your folders"
                ThemedSwitch {
                    checked: AppSettings.autoScan
                    onToggled: AppSettings.autoScan = checked
                }
            }
            SettingRow {
                title: "Folders"
                description: Library.folders.length === 1 ? "1 folder" : Library.folders.length + " folders"
                PillButton {
                    text: "Manage"
                    iconName: "folder"
                    onClicked: AppState.navigate("library")
                }
            }
            SettingRow {
                title: "Refresh metadata"
                description: "Re-read tags, artwork and video thumbnails"
                PillButton {
                    text: "Refresh"
                    iconName: "refresh"
                    enabled: !Library.scanning && Library.folders.length > 0
                    onClicked: Library.refreshMetadata()
                }
            }
            SettingRow {
                title: "Clear library"
                description: "Removes all folders, playlists, favorites and history from Nuru. Your files are not touched."
                PillButton {
                    text: "Clear…"
                    iconName: "trash"
                    destructive: true
                    enabled: !Library.scanning
                    onClicked: clearDialog.open()
                }
            }
        }

        Section {
            heading: "About"
            RowLayout {
                spacing: Theme.spaceL
                Image {
                    source: "qrc:/nuru/assets/icons/app/com.nuru.player.svg"
                    sourceSize: Qt.size(128, 128)
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: 64
                }
                ColumnLayout {
                    spacing: 2
                    Text {
                        text: "Nuru " + Qt.application.version
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontTitle
                        font.weight: Font.Bold
                    }
                    Text {
                        text: "Your media. Your world."
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                    }
                }
            }
            SettingRow {
                title: "Keyboard shortcuts"
                PillButton {
                    text: "Show"
                    iconName: "keyboard"
                    onClicked: AppState.shortcutsRequested()
                }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: Theme.textSecondary
                font.pixelSize: Theme.fontCaption + 1
                lineHeight: 1.3
                text: "Third-party software:\n"
                      + "• LibVLC " + Playback.engineVersion + " — LGPL 2.1 or later, © VideoLAN and VLC authors\n"
                      + "• Qt 6 — LGPL 3, © The Qt Company Ltd and other contributors\n"
                      + "• SQLite — public domain\n"
                      + "• Solar icon set by 480 Design — CC BY 4.0, via icones.js.org / Iconify"
            }
        }
    }

    ConfirmDialog {
        id: clearDialog
        title: "Clear your library?"
        message: "All folders, playlists, favorites and play history will be removed from Nuru. Media files on disk are not deleted."
        confirmText: "Clear library"
        destructive: true
        onAccepted: Library.clearLibrary()
    }
}
