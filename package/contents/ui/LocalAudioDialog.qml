// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog
    objectName: "localAudioDialog"
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(430, parent.width - 24)
    modal: true; focus: true
    title: qsTr("Local audio tools")
    background: Rectangle { color: "#272a3b"; border.color: "#73778d"; radius: 4 }
    contentItem: ColumnLayout {
        spacing: 12
        Text {
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: "#c8d6b6"
            text: qsTr("The equalizer and audio-reactive visualizer are for non-Spotify audio only. Stop Spotify in other players before continuing: the selected output can contain sound from every app.")
        }
        Text {
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: "#93ad7b"
            text: qsTr("Phosrack will suspend its Spotify session. Your saved sign-in is kept for when you return to Spotify mode.")
        }
    }
    footer: RowLayout {
        spacing: 8
        Item { Layout.fillWidth: true }
        RackButton { text: qsTr("Cancel"); onClicked: dialog.reject() }
        RackButton { objectName: "acceptLocalAudioButton"; text: qsTr("Use non-Spotify audio"); onClicked: dialog.accept() }
    }
}
