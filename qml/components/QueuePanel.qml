import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Side panel listing the playback queue with reorder / remove / play-selected.
Rectangle {
    id: panel

    readonly property var queue: Playback.queue
    property bool embedded: false   // shown inside Now Playing rather than as a side panel

    color: Theme.surface
    implicitWidth: 360

    Rectangle {
        visible: !panel.embedded
        anchors.left: parent.left
        width: 1
        height: parent.height
        color: Theme.border
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceL
        spacing: Theme.spaceM

        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: "Queue"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontTitle + 2
                    font.weight: Font.Bold
                }
                Text {
                    text: panel.queue.count === 1 ? "1 item" : panel.queue.count + " items"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontCaption + 1
                }
            }
            IconButton {
                iconName: "trash"
                tip: "Clear queue"
                enabled: panel.queue.count > 0
                onClicked: {
                    Playback.stop()
                    panel.queue.clear()
                }
            }
            IconButton {
                visible: !panel.embedded
                iconName: "close"
                tip: "Close queue (Q)"
                onClicked: AppState.queueOpen = false
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: panel.queue
            spacing: 2
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            displaced: Transition { NumberAnimation { properties: "y"; duration: Theme.durationNormal; easing.type: Easing.OutCubic } }

            delegate: ItemDelegate {
                id: entry
                required property int index
                required property var model

                width: ListView.view.width
                implicitHeight: 52
                hoverEnabled: true
                padding: 0
                focusPolicy: Qt.NoFocus
                Accessible.name: model.title

                background: Rectangle {
                    radius: Theme.radiusM
                    color: entry.model.isCurrent ? Theme.accentSoft : entry.hovered ? Theme.hover : "transparent"
                }
                onDoubleClicked: Playback.playIndex(index)

                contentItem: RowLayout {
                    spacing: 10
                    Artwork {
                        Layout.leftMargin: 6
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 38
                        source: entry.model.artwork
                        seed: entry.model.subtitle + entry.model.title
                        kind: entry.model.kind
                        decodeSize: 96
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Text {
                            Layout.fillWidth: true
                            text: entry.model.title
                            elide: Text.ElideRight
                            color: entry.model.isCurrent ? Theme.accent : Theme.textPrimary
                            font.pixelSize: Theme.fontBody
                            font.weight: Font.Medium
                        }
                        Text {
                            Layout.fillWidth: true
                            text: entry.model.subtitle
                            elide: Text.ElideRight
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontCaption
                        }
                    }
                    RowLayout {
                        spacing: 0
                        visible: entry.hovered
                        IconButton {
                            diameter: 28; iconSize: 14
                            iconName: "play-fill"
                            tip: "Play"
                            onClicked: Playback.playIndex(entry.index)
                        }
                        IconButton {
                            diameter: 28; iconSize: 14
                            iconName: "chevron-up"
                            tip: "Move up"
                            enabled: entry.index > 0
                            onClicked: panel.queue.move(entry.index, entry.index - 1)
                        }
                        IconButton {
                            diameter: 28; iconSize: 14
                            iconName: "chevron-down"
                            tip: "Move down"
                            enabled: entry.index < list.count - 1
                            onClicked: panel.queue.move(entry.index, entry.index + 1)
                        }
                        IconButton {
                            diameter: 28; iconSize: 14
                            Layout.rightMargin: 4
                            iconName: "close"
                            tip: "Remove from queue"
                            onClicked: Playback.removeFromQueue(entry.index)
                        }
                    }
                    Text {
                        visible: !entry.hovered
                        Layout.rightMargin: 10
                        text: Theme.formatDuration(entry.model.duration)
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontCaption
                    }
                }
            }

            EmptyState {
                anchors.centerIn: parent
                visible: list.count === 0
                iconName: "queue"
                title: "Your queue is empty"
                message: "Play something or use “Add to queue” from any track’s menu."
            }
        }
    }
}
