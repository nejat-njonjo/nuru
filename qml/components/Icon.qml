import QtQuick
import Nuru

// A Solar icon (assets/icons) tinted with any colour via the "icons" image provider.
Item {
    id: root

    property string name
    property color color: Theme.textPrimary
    property int size: 20

    implicitWidth: size
    implicitHeight: size
    width: size
    height: size

    Image {
        anchors.fill: parent
        sourceSize: Qt.size(root.size * 2, root.size * 2)
        source: root.name.length > 0 ? "image://icons/" + root.name + "/" + root.color.toString().substring(1) : ""
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        cache: true
    }
}
