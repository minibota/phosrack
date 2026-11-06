// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
Slider {
    id: control
    implicitHeight: 23
    property real remoteValue: 0
    property real requestedValue: 0
    property bool pendingCommit: false
    signal valueCommitted(real nextValue)
    Binding { target: control; property: "value"; value: control.remoteValue; when: !control.pressed; restoreMode: Binding.RestoreNone }
    onMoved: { requestedValue = value; pendingCommit = true }
    Timer {
        interval: 100; repeat: true; running: control.pendingCommit
        onTriggered: if (!control.pressed) { control.pendingCommit = false; if (control.enabled) control.valueCommitted(control.requestedValue) }
    }
    background: Rectangle {
        x: control.leftPadding; y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth; height: 6; color: "#0c1018"; border.color: "#5c6073"
        Rectangle { x: 1; y: 2; width: control.visualPosition * (parent.width - 2); height: 2; color: control.enabled ? "#b9d964" : "#4c5363" }
    }
    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: 16; height: 11
        gradient: Gradient { GradientStop { position: 0; color: "#d5d6bb" } GradientStop { position: 1; color: "#9b8c59" } }
        border.color: control.activeFocus ? "#bcff6f" : "#111421"
        Rectangle { x: 2; y: 2; width: parent.width - 4; height: 2; color: "#f7e5a5" }
        Rectangle { x: 3; y: 6; width: parent.width - 6; height: 1; color: "#4c4c42" }
    }
}
