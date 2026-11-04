import QtQuick
import Nuru

// Rounded artwork with a deterministic coloured placeholder when no image is available.
// Rounding/cropping happens in the "artwork" image provider (works with every renderer).
Item {
    id: root

    property string source            // file:// URL of cached artwork or http(s) logo URL (may be empty)
    property string seed
    property string kind: "audio"
    property real radius: Theme.radiusS
    property int decodeSize: 256      // pixels along the longer edge
    property string fallbackIcon: kind === "video" ? "video" : "music"
    readonly property bool loaded: image.status === Image.Ready

    implicitWidth: 48
    implicitHeight: 48

    readonly property size requestSize: width >= height
        ? Qt.size(decodeSize, Math.max(1, Math.round(decodeSize * height / Math.max(1, width))))
        : Qt.size(Math.max(1, Math.round(decodeSize * width / Math.max(1, height))), decodeSize)
    readonly property int requestRadius: Math.round(radius * requestSize.width / Math.max(1, width))

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        visible: !root.loaded
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.lighter(Theme.placeholderColor(root.seed), 1.25) }
            GradientStop { position: 1; color: Theme.placeholderColor(root.seed) }
        }
        Icon {
            anchors.centerIn: parent
            name: root.fallbackIcon
            size: Math.max(14, Math.min(root.width, root.height) * 0.36)
            color: Theme.dark ? Qt.rgba(1, 1, 1, 0.72) : Qt.rgba(0, 0, 0, 0.45)
        }
    }

    // Remote images (stream logos) are shown whole on a tile; local artwork is cropped + rounded
    // by the "artwork" image provider.
    readonly property bool remote: /^https?:\/\//.test(source)

    Rectangle {
        anchors.fill: parent
        radius: root.radius
        visible: root.remote && root.loaded
        color: Theme.dark ? "#E4E4E7" : "#FFFFFF"
        border.color: Theme.border
    }

    Image {
        id: image
        anchors.fill: parent
        anchors.margins: root.remote ? Math.round(Math.min(root.width, root.height) * 0.12) : 0
        source: root.source.length === 0 || root.width <= 0 || root.height <= 0 ? ""
              : root.remote ? root.source
              : "image://artwork/" + root.requestRadius + "/"
                + encodeURIComponent(decodeURIComponent(root.source.replace(/^file:\/\//, "")))
        // Remote logos use a fixed decode size: binding it to the (settling) item size would
        // restart the network request on every layout pass.
        sourceSize: root.remote ? Qt.size(root.decodeSize, root.decodeSize) : root.requestSize
        fillMode: root.remote ? Image.PreserveAspectFit : Image.Stretch
        asynchronous: true
        cache: true
        smooth: true
        opacity: root.loaded ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durationNormal } }
    }
}
