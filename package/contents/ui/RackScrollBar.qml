// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
ScrollBar {
    implicitWidth: 4; policy: ScrollBar.AsNeeded
    background: Item {}
    contentItem: Rectangle {
        radius: 2; color: "#8d9e78"; opacity: parent.active || parent.hovered ? 0.75 : 0
        Behavior on opacity { NumberAnimation { duration: 180 } }
    }
}
