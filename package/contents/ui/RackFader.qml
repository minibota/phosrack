// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
Slider {
    id: fader
    orientation: Qt.Vertical
    from: -12; to: 12; stepSize: 0.5
    implicitWidth: 24; implicitHeight: 84
    property real remoteValue: 0
    signal gainMoved(real gain)
    Binding { target: fader; property: "value"; value: fader.remoteValue; when: !fader.pressed; restoreMode: Binding.RestoreNone }
    onMoved: gainMoved(value)
    hoverEnabled: true
    ToolTip.visible: hovered || pressed
    ToolTip.text: value.toFixed(1) + " dB"
    background: Item {
        x: fader.leftPadding + fader.availableWidth / 2 - width / 2
        y: fader.topPadding; width: 5; height: fader.availableHeight
        Rectangle { anchors.fill: parent; color: "#080b12"; border.color: "#686d81" }
        Rectangle { x: 2; y: 1; width: 1; height: parent.height - 2; color: "#e1cb57" }
        Rectangle { x: -4; y: parent.height / 2; width: 13; height: 1; color: "#a1a4b1" }
    }
    handle: Rectangle {
        x: fader.leftPadding + fader.availableWidth / 2 - width / 2
        y: fader.topPadding + fader.visualPosition * (fader.availableHeight - height)
        width: 17; height: 10
        gradient: Gradient { GradientStop { position: 0; color: "#dae1df" } GradientStop { position: 1; color: "#7d91a3" } }
        border.color: fader.activeFocus ? "#baff74" : "#0e1420"
        Rectangle { x: 2; y: 2; width: parent.width - 4; height: 1; color: "#f4f3d4" }
        Rectangle { x: 4; y: 5; width: parent.width - 8; height: 1; color: "#30495a" }
    }
}
