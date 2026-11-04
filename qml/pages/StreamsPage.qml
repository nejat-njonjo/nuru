import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Network streams: the user's saved links ("My streams") and the iptv-org channel browser.
Item {
    id: page

    readonly property bool browsing: AppState.streamsTab === "browse"

    Component.onCompleted: if (browsing) Catalog.ensureLoaded()
    onBrowsingChanged: if (browsing) Catalog.ensureLoaded()

    component FilterCombo: ComboBox {
        id: combo
        property string placeholder
        implicitHeight: 38
        implicitWidth: 200
        textRole: "label"
        valueRole: "id"
        font.pixelSize: Theme.fontBody
        Accessible.name: placeholder
        background: Rectangle {
            radius: height / 2
            color: combo.hovered ? Theme.hover : Theme.elevated
            border.width: combo.visualFocus ? 2 : 1
            border.color: combo.visualFocus ? Theme.accent : Theme.border
        }
        contentItem: Text {
            leftPadding: 16
            rightPadding: 30
            text: combo.displayText
            color: Theme.textPrimary
            font: combo.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        indicator: Icon {
            x: combo.width - width - 12
            y: (combo.height - height) / 2
            name: "chevron-down"
            size: 14
            color: Theme.textSecondary
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spaceL

        PageHeader {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spaceXxl
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            title: "Streams"
            subtitle: page.browsing ? Catalog.message
                                    : (Streams.count === 1 ? "1 saved stream" : Streams.count + " saved streams")
            PillButton {
                text: "Import playlist"
                iconName: "import"
                onClicked: AppState.openLinkRequested(true)
            }
            PillButton {
                primary: true
                text: "Open link"
                iconName: "link"
                onClicked: AppState.openLinkRequested(false)
            }
        }

        // Tabs
        RowLayout {
            Layout.leftMargin: Theme.pageMargin
            spacing: 6
            Repeater {
                model: [{ id: "saved", label: "My streams", icon: "bookmark" },
                        { id: "browse", label: "Browse free channels", icon: "globe" }]
                delegate: PillButton {
                    required property var modelData
                    text: modelData.label
                    iconName: modelData.icon
                    primary: AppState.streamsTab === modelData.id
                    onClicked: AppState.streamsTab = modelData.id
                }
            }
        }

        // ------------------------------------------------------------ My streams
        ListView {
            id: savedList
            visible: !page.browsing
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.pageMargin - 8
            Layout.rightMargin: Theme.pageMargin - 8
            clip: true
            spacing: 2
            model: Streams
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            delegate: StreamRow {
                required property var model
                width: ListView.view.width
                name: model.name
                url: model.url
                logo: model.logo
                category: model.category
                lastPlayed: model.lastPlayed
                onRenameRequested: AppState.renameStreamRequested(model.url, model.name)
            }

            EmptyState {
                anchors.centerIn: parent
                visible: savedList.count === 0
                iconName: "stream"
                title: "No saved streams yet"
                message: "Paste any playable link — HLS (.m3u8), HTTP, RTSP, RTMP, UDP or an internet radio URL — "
                         + "or browse thousands of free TV channels from iptv-org."
                actionText: "Open link"
                actionIcon: "link"
                onActionTriggered: AppState.openLinkRequested(false)
                secondaryActionText: "Browse channels"
                secondaryActionIcon: "globe"
                onSecondaryActionTriggered: AppState.streamsTab = "browse"
            }
        }

        // ------------------------------------------------------------ Browse iptv-org
        ColumnLayout {
            visible: page.browsing
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spaceM

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.pageMargin
                Layout.rightMargin: Theme.pageMargin
                spacing: Theme.spaceS
                enabled: Catalog.status === Catalog.Ready

                SearchField {
                    id: channelSearch
                    Layout.fillWidth: true
                    Layout.maximumWidth: 360
                    placeholderText: "Search channels"
                    onTextEdited: searchDebounce.restart()
                }
                Timer {
                    id: searchDebounce
                    interval: 200
                    onTriggered: channels.searchText = channelSearch.text
                }
                FilterCombo {
                    id: categoryCombo
                    placeholder: "Category"
                    model: [{ id: "", label: "All categories" }].concat(
                               Catalog.categories.map(c => ({ id: c.id, label: c.name + "  (" + c.count + ")" })))
                    onActivated: channels.category = currentValue
                }
                FilterCombo {
                    id: countryCombo
                    placeholder: "Country"
                    model: [{ id: "", label: "All countries" }].concat(
                               Catalog.countries.map(c => ({ id: c.id, label: (c.flag ? c.flag + "  " : "") + c.name + "  (" + c.count + ")" })))
                    onActivated: channels.country = currentValue
                }
                Item { Layout.fillWidth: true }
                ThemedCheckBox {
                    id: hideGeo
                    text: "Hide geo-blocked"
                    checked: true
                    onToggled: channels.hideGeoBlocked = checked
                }
                IconButton {
                    iconName: "refresh"
                    tip: "Download the latest channel list"
                    onClicked: Catalog.refresh()
                }
            }

            StreamListModel {
                id: channels
                catalog: Catalog
                saved: Streams
                hideGeoBlocked: true
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // Loading / error states
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: Catalog.status === Catalog.Loading
                    spacing: 12
                    BusyIndicator { Layout.alignment: Qt.AlignHCenter; running: parent.visible }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: Catalog.message
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontBody
                    }
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 240
                        height: 4
                        radius: 2
                        color: Theme.track
                        Rectangle {
                            width: parent.width * Catalog.progress
                            height: parent.height
                            radius: 2
                            color: Theme.accent
                        }
                    }
                }
                EmptyState {
                    anchors.centerIn: parent
                    visible: Catalog.status === Catalog.Error
                    iconName: "warning"
                    title: "Channels unavailable"
                    message: Catalog.message
                    actionText: "Try again"
                    actionIcon: "refresh"
                    onActionTriggered: Catalog.refresh()
                }
                EmptyState {
                    anchors.centerIn: parent
                    visible: Catalog.status === Catalog.Ready && grid.count === 0
                    iconName: "search"
                    title: "No channels match"
                    message: "Try another category, country or search term."
                }

                GridView {
                    id: grid
                    anchors.fill: parent
                    anchors.leftMargin: Theme.pageMargin - 12
                    anchors.rightMargin: Theme.pageMargin - 12
                    visible: Catalog.status === Catalog.Ready
                    clip: true
                    model: channels
                    cacheBuffer: 600
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {}

                    readonly property int columns: Math.max(2, Math.floor(width / 210))
                    cellWidth: width / columns
                    cellHeight: (cellWidth - 24) * 9 / 16 + 76

                    delegate: StreamCard {
                        required property int index
                        required property var model
                        width: grid.cellWidth
                        name: model.name
                        url: model.url
                        logo: model.logo
                        quality: model.quality
                        geoBlocked: model.geoBlocked
                        saved: model.saved
                        subtitle: [model.flag + " " + (model.countryName || model.country.toUpperCase()), model.category]
                                  .filter(s => s.trim().length > 0).join(" · ")
                        onPlayRequested: Playback.playPaths(channels.urls(), index)
                        onSaveToggled: saved ? Streams.remove(url) : Streams.save(url, name, logo, model.category)
                        onMenuRequested: saveToggled()
                    }

                    footer: Text {
                        width: grid.width
                        topPadding: Theme.spaceL
                        bottomPadding: Theme.spaceXl
                        leftPadding: 12
                        rightPadding: 12
                        wrapMode: Text.WordWrap
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontCaption + 1
                        text: "Showing " + grid.count + " of " + Catalog.channelCount + " channels. "
                              + "Channels are publicly available streams collected by the community project iptv-org "
                              + "(github.com/iptv-org/iptv). Nuru doesn't host or verify them — some may be offline or region-restricted."
                    }
                }
            }
        }
    }
}
