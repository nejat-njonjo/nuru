import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Library folders: add / remove / rescan / refresh metadata, with scan progress.
Item {
    id: page

    ListView {
        id: list
        anchors.fill: parent
        anchors.leftMargin: Theme.pageMargin
        anchors.rightMargin: Theme.pageMargin
        clip: true
        spacing: 6
        model: Library.folders
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}

        header: ColumnLayout {
            width: list.width
            spacing: Theme.spaceL

            PageHeader {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spaceXxl
                title: "Library"
                subtitle: Library.musicCount + " songs · " + Library.videoCount + " videos · "
                          + (list.count === 1 ? "1 folder" : list.count + " folders")
                PillButton {
                    text: "Add files"
                    iconName: "file-add"
                    onClicked: AppState.addFilesRequested()
                }
                PillButton {
                    primary: true
                    text: "Add folder"
                    iconName: "folder-add"
                    onClicked: AppState.addFolderRequested()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: statusRow.implicitHeight + 32
                radius: Theme.radiusL
                color: Theme.surface
                border.color: Theme.border

                ColumnLayout {
                    id: statusRow
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spaceM
                        Icon {
                            name: Library.scanning ? "refresh" : "check"
                            size: 20
                            color: Library.scanning ? Theme.textSecondary : Theme.accent
                        }
                        Text {
                            Layout.fillWidth: true
                            text: Library.scanMessage
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontBody
                            elide: Text.ElideRight
                        }
                        PillButton {
                            text: "Rescan"
                            iconName: "refresh"
                            enabled: !Library.scanning && list.count > 0
                            onClicked: Library.rescan()
                            ToolTip.visible: hovered
                            ToolTip.delay: 600
                            ToolTip.text: "Find new, changed and deleted files"
                        }
                        PillButton {
                            text: "Refresh metadata"
                            iconName: "tuning"
                            enabled: !Library.scanning && list.count > 0
                            onClicked: Library.refreshMetadata()
                            ToolTip.visible: hovered
                            ToolTip.delay: 600
                            ToolTip.text: "Re-read tags, artwork and thumbnails for every file"
                        }
                    }
                    ScanStatus { Layout.fillWidth: true }
                }
            }

            Text {
                Layout.topMargin: Theme.spaceS
                visible: list.count > 0
                text: "Folders"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontTitle
                font.weight: Font.Bold
            }
        }

        delegate: Rectangle {
            id: folderRow
            required property string modelData
            width: list.width
            height: 60
            radius: Theme.radiusM
            color: hover.hovered ? Theme.hover : Theme.surface
            border.color: Theme.border
            HoverHandler { id: hover }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 8
                spacing: Theme.spaceM
                Icon { name: "folder"; size: 22; color: Theme.accent }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1
                    Text {
                        Layout.fillWidth: true
                        text: folderRow.modelData.split("/").pop() || folderRow.modelData
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontBodyLarge
                        font.weight: Font.Medium
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        text: folderRow.modelData
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontCaption
                        elide: Text.ElideMiddle
                    }
                }
                IconButton {
                    iconName: "folder-open"
                    tip: "Open in file manager"
                    onClicked: Qt.openUrlExternally("file://" + folderRow.modelData)
                }
                IconButton {
                    iconName: "trash"
                    tip: "Remove from library"
                    enabled: !Library.scanning
                    onClicked: removeDialog.ask(folderRow.modelData)
                }
            }
        }

        footer: Item {
            width: list.width
            height: list.count === 0 ? 320 : Theme.spaceXl
            EmptyState {
                anchors.centerIn: parent
                visible: list.count === 0
                iconName: "folder-add"
                title: "No folders yet"
                message: "Nuru scans folders recursively and keeps your library up to date. Files never leave your computer."
                actionText: "Add folder"
                actionIcon: "folder-add"
                onActionTriggered: AppState.addFolderRequested()
            }
        }
    }

    ConfirmDialog {
        id: removeDialog
        property string folder
        function ask(path) {
            folder = path
            open()
        }
        title: "Remove folder?"
        message: "“" + folder + "” and its media will be removed from your Nuru library. Files on disk are not deleted."
        confirmText: "Remove"
        destructive: true
        onAccepted: Library.removeFolder(folder)
    }
}
