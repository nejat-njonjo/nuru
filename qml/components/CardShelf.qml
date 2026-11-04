import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Nuru

// Titled horizontal row of cards with an optional "See all" link.
ColumnLayout {
    id: shelf

    property string title
    property string seeAllPage
    property alias model: list.model
    property alias delegate: list.delegate
    property int cardWidth: 176
    property real cardHeight: cardWidth + 56
    readonly property int count: list.count

    spacing: Theme.spaceS
    visible: list.count > 0

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.pageMargin
        Layout.rightMargin: Theme.pageMargin
        Text {
            Layout.fillWidth: true
            text: shelf.title
            color: Theme.textPrimary
            font.pixelSize: Theme.fontTitle + 3
            font.weight: Font.Bold
        }
        AbstractButton {
            id: seeAll
            visible: shelf.seeAllPage.length > 0
            hoverEnabled: true
            focusPolicy: Qt.TabFocus
            Accessible.name: "See all " + shelf.title
            contentItem: Text {
                text: "See all"
                color: seeAll.hovered || seeAll.visualFocus ? Theme.textPrimary : Theme.textSecondary
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
                font.underline: seeAll.visualFocus
            }
            onClicked: AppState.navigate(shelf.seeAllPage)
        }
        IconButton {
            iconName: "chevron-right"
            iconSize: 16
            diameter: 30
            rotation: 180
            tip: "Scroll left"
            enabled: !list.atXBeginning
            onClicked: list.flick(1600, 0)
        }
        IconButton {
            iconName: "chevron-right"
            iconSize: 16
            diameter: 30
            tip: "Scroll right"
            enabled: !list.atXEnd
            onClicked: list.flick(-1600, 0)
        }
    }

    ListView {
        id: list
        Layout.fillWidth: true
        Layout.preferredHeight: shelf.cardHeight
        orientation: ListView.Horizontal
        spacing: 4
        clip: true
        leftMargin: Theme.pageMargin - 12
        rightMargin: Theme.pageMargin - 12
        boundsBehavior: Flickable.StopAtBounds
        flickDeceleration: 3000
        cacheBuffer: 400
    }
}
