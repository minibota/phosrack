// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
Item {
    id: panel
    property string panelKey
    property string title
    property bool folded: false
    property bool canMoveUp: true
    property bool canMoveDown: true
    property real preferredBodyHeight: -1
    default property alias content: body.data
    signal moveRequested(int direction)
    signal foldRequested()
    implicitHeight: 23 + (folded ? 0 : (preferredBodyHeight >= 0 ? preferredBodyHeight : body.implicitHeight) + 12)
    Rectangle { anchors.fill: parent; color: "#272a3b"; border.color: "#11131d" }
    Rectangle { anchors.fill: parent; anchors.margins: 1; color: "transparent"; border.color: "#73778d" }
    Rectangle {
        id: header; x: 3; y: 3; width: parent.width - 6; height: 19; color: "#1d2030"
        Row {
            anchors.left: parent.left; anchors.leftMargin: 4; anchors.right: titleLabel.left; anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter; height: 6
            Rectangle { width: parent.width; height: 6; color: "transparent"; border.color: "#b59a52"; Rectangle { y: 2; width: parent.width; height: 1; color: "#ecd78f" } }
        }
        Text {
            id: titleLabel; anchors.horizontalCenter: parent.horizontalCenter; anchors.verticalCenter: parent.verticalCenter
            text: panel.title; color: "#e4e9df"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.bold: true; font.pixelSize: 10; font.letterSpacing: 1
        }
        Rectangle {
            anchors.left: titleLabel.right; anchors.leftMargin: 6; anchors.right: actions.left; anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter; height: 6; color: "transparent"; border.color: "#b59a52"
            Rectangle { y: 2; width: parent.width; height: 1; color: "#ecd78f" }
        }
        MouseArea {
            id: headerMouse; anchors.left: parent.left; anchors.right: actions.left; height: parent.height
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            property real startY: 0
            onPressed: function(mouse) { startY = mapToItem(null, mouse.x, mouse.y).y }
            onReleased: function(mouse) { const d = mapToItem(null, mouse.x, mouse.y).y - startY; if (Math.abs(d) > 30) panel.moveRequested(d > 0 ? 1 : -1) }
            onDoubleClicked: panel.foldRequested()
        }
        Row {
            id: actions; anchors.right: parent.right; anchors.rightMargin: 2; anchors.verticalCenter: parent.verticalCenter; spacing: 2
            RackButton { text: "↑"; chrome: true; description: qsTr("Move up"); implicitWidth: 17; implicitHeight: 15; enabled: panel.canMoveUp; onClicked: panel.moveRequested(-1) }
            RackButton { text: "↓"; chrome: true; description: qsTr("Move down"); implicitWidth: 17; implicitHeight: 15; enabled: panel.canMoveDown; onClicked: panel.moveRequested(1) }
            RackButton { text: panel.folded ? "+" : "−"; chrome: true; description: panel.folded ? qsTr("Expand") : qsTr("Collapse"); implicitWidth: 17; implicitHeight: 15; onClicked: panel.foldRequested() }
        }
    }
    Column { id: body; x: 8; y: 28; width: parent.width - 16; spacing: 5; visible: !panel.folded }
}
