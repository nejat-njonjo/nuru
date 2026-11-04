import QtQuick
import QtQuick.Controls
import Nuru

// "Sort: <field>" button with a menu of sort keys and an ascending/descending toggle.
PillButton {
    id: root

    // [{ key: "title", label: "Title" }, ...]
    property var options: []
    property string currentKey
    property bool ascending: true
    signal sortChanged(string key, bool ascending)

    iconName: "sort"
    text: {
        for (const option of options)
            if (option.key === currentKey)
                return option.label
        return "Sort"
    }
    Accessible.name: "Sort by " + text
    onClicked: menu.popup(root, 0, root.height + 4)

    Menu {
        id: menu
        Repeater {
            model: root.options
            delegate: MenuItem {
                required property var modelData
                text: modelData.label
                checkable: true
                checked: modelData.key === root.currentKey
                onTriggered: root.sortChanged(modelData.key, modelData.key === root.currentKey ? root.ascending : (modelData.defaultAscending ?? true))
            }
        }
        MenuSeparator {}
        MenuItem {
            text: root.ascending ? "Ascending" : "Descending"
            icon.source: "image://icons/sort/" + Theme.textSecondary.toString().substring(1)
            onTriggered: root.sortChanged(root.currentKey, !root.ascending)
        }
    }
}
