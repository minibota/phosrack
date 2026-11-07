// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    id: window
    visible: true
    width: 470; height: 805; minimumWidth: 430; minimumHeight: 780
    flags: Qt.Window | Qt.FramelessWindowHint
    title: "Phosrack"; color: "#1b1d2b"
    palette.window: "#292c3d"; palette.windowText: "#dce3d4"; palette.text: "#baff7c"
    palette.placeholderText: "#829278"; palette.base: "#0b110b"; palette.button: "#6d7890"
    palette.buttonText: "#dde5d5"; palette.highlight: "#729947"; palette.highlightedText: "#070c06"
    onClosing: rack.closeVisualizer()
    Rectangle { anchors.fill: parent; color: "transparent"; border.color: "#0c0e17"; border.width: 2 }
    Rectangle { anchors.fill: parent; anchors.margins: 2; color: "transparent"; border.color: "#85889b" }
    Rectangle {
        id: caption; x: 5; y: 5; width: parent.width - 10; height: 22; color: "#222537"
        MouseArea {
            anchors.fill: parent
            onPressed: window.startSystemMove()
        }
        Text { x: 5; anchors.verticalCenter: parent.verticalCenter; text: "ϟ"; color: "#bfff79"; font.bold: true; font.pixelSize: 15 }
        Rectangle { x: 22; anchors.right: brand.left; anchors.rightMargin: 8; anchors.verticalCenter: parent.verticalCenter; height: 6; color: "transparent"; border.color: "#a58e53"; Rectangle { y: 2; width: parent.width; height: 1; color: "#e5cd88" } }
        Text { id: brand; anchors.centerIn: parent; text: "PHOSRACK"; color: "#bfff87"; style: Text.Outline; styleColor: "#355628"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.bold: true; font.pixelSize: 12; font.letterSpacing: 2 }
        Rectangle { anchors.left: brand.right; anchors.leftMargin: 8; anchors.right: windowActions.left; anchors.rightMargin: 7; anchors.verticalCenter: parent.verticalCenter; height: 6; color: "transparent"; border.color: "#a58e53"; Rectangle { y: 2; width: parent.width; height: 1; color: "#e5cd88" } }
        Row {
            id: windowActions; anchors.right: parent.right; anchors.rightMargin: 2; anchors.verticalCenter: parent.verticalCenter; spacing: 2
            RackButton { objectName: "settingsButton"; text: "⚙"; chrome: true; description: qsTr("Settings"); implicitWidth: 20; implicitHeight: 18; onClicked: rack.openSettings() }
            RackButton { text: "−"; chrome: true; description: qsTr("Minimize"); implicitWidth: 18; implicitHeight: 18; onClicked: window.showMinimized() }
            RackButton { text: "×"; chrome: true; description: qsTr("Close"); implicitWidth: 18; implicitHeight: 18; onClicked: window.close() }
        }
    }
    PhosrackView {
        id: rack; objectName: "rackView"; anchors.top: caption.bottom; anchors.bottom: parent.bottom
        anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 5; anchors.topMargin: 3
        pollingActive: window.visible && window.visibility !== Window.Minimized
    }
    Repeater {
        model: [
            {edges:Qt.LeftEdge, x:0, y:8, w:4, h:window.height-16, cursor:Qt.SizeHorCursor},
            {edges:Qt.RightEdge, x:window.width-4, y:8, w:4, h:window.height-16, cursor:Qt.SizeHorCursor},
            {edges:Qt.TopEdge, x:8, y:0, w:window.width-16, h:4, cursor:Qt.SizeVerCursor},
            {edges:Qt.BottomEdge, x:8, y:window.height-4, w:window.width-16, h:4, cursor:Qt.SizeVerCursor},
            {edges:Qt.TopEdge|Qt.LeftEdge, x:0, y:0, w:8, h:8, cursor:Qt.SizeFDiagCursor},
            {edges:Qt.TopEdge|Qt.RightEdge, x:window.width-8, y:0, w:8, h:8, cursor:Qt.SizeBDiagCursor},
            {edges:Qt.BottomEdge|Qt.LeftEdge, x:0, y:window.height-8, w:8, h:8, cursor:Qt.SizeBDiagCursor},
            {edges:Qt.BottomEdge|Qt.RightEdge, x:window.width-8, y:window.height-8, w:8, h:8, cursor:Qt.SizeFDiagCursor}
        ]
        delegate: MouseArea {
            required property var modelData
            x: modelData.x; y: modelData.y; width: modelData.w; height: modelData.h
            cursorShape: modelData.cursor
            enabled: window.visibility !== Window.Maximized
            onPressed: window.startSystemResize(modelData.edges)
        }
    }
}
