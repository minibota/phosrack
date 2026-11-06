// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
Button {
    id: control
    property bool lit: false
    property bool chrome: false
    property string description: text
    implicitHeight: 25
    implicitWidth: Math.max(28, contentItem.implicitWidth + 16)
    hoverEnabled: true
    Accessible.name: description
    ToolTip.visible: hovered && description !== text
    ToolTip.text: description
    contentItem: Text {
        text: control.text
        color: !control.enabled ? "#6b7080" : control.lit ? "#caff83" : control.chrome ? "#e1d8a3" : "#192032"
        font.pixelSize: control.chrome ? 11 : 12
        font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.bold: true
        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 0
        gradient: Gradient {
            GradientStop { position: 0; color: control.lit ? "#304b24" : control.chrome ? "#42445b" : control.down ? "#79849b" : "#c1ccd4" }
            GradientStop { position: 1; color: control.lit ? "#182412" : control.chrome ? "#252738" : control.down ? "#b2bdca" : "#8290a8" }
        }
        border.color: control.activeFocus ? "#bcff6f" : control.lit ? "#70994b" : "#10121d"
        Rectangle { anchors.fill: parent; anchors.margins: 1; color: "transparent"; border.color: control.lit ? "#3a572d" : control.chrome ? "#686980" : "#d4dfdf"; opacity: control.down ? 0.25 : 0.7 }
        Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 2; color: "#111421"; opacity: 0.6 }
    }
}
