import QtQuick
import QtQuick.Layouts
import Nuru

// Transient notification shown above the playback bar. Messages queue up.
Rectangle {
    id: toast

    property var pending: []
    property bool isError: false

    function show(text, error) {
        pending.push({ text: text, error: error })
        if (!timer.running && !visible)
            next()
    }
    function next() {
        if (pending.length === 0) {
            opacity = 0
            return
        }
        const item = pending.shift()
        label.text = item.text
        isError = item.error
        opacity = 1
        timer.interval = item.error ? 5000 : 2600
        timer.restart()
    }

    visible: opacity > 0
    opacity: 0
    width: Math.min(row.implicitWidth + 32, parent ? parent.width - 64 : 600)
    height: row.implicitHeight + 20
    radius: height / 2
    color: Theme.dark ? "#F4F4F5" : "#18181B"
    Behavior on opacity { NumberAnimation { duration: Theme.durationNormal } }

    Accessible.role: Accessible.AlertMessage
    Accessible.name: label.text

    RowLayout {
        id: row
        anchors.centerIn: parent
        width: parent.width - 32
        spacing: 10
        Icon {
            name: toast.isError ? "warning" : "check"
            size: 18
            color: toast.isError ? Theme.danger : Theme.accent
        }
        Text {
            id: label
            Layout.fillWidth: true
            color: Theme.dark ? "#18181B" : "#FAFAFA"
            font.pixelSize: Theme.fontBody
            font.weight: Font.Medium
            elide: Text.ElideRight
        }
    }

    Timer {
        id: timer
        onTriggered: toast.next()
    }

    TapHandler { onTapped: toast.next() }
}
