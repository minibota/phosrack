// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: connection
    objectName: "connectionDialog"
    property var spotify
    readonly property bool configured: !!spotify && /^[a-fA-F0-9]{32}$/.test(spotify.clientId)
    signal settingsRequested()
    signal connectRequested()
    function offerConnection() {
        if (spotify && spotify.authorizationRequired && !spotify.authorized && !spotify.busy && !spotify.rateLimited) open()
    }
    Connections {
        target: connection.spotify
        function onAuthorizationRequiredChanged() {
            if (connection.spotify.authorizationRequired) connection.offerConnection()
            else connection.close()
        }
        function onSessionChanged() {
            if (connection.spotify.authorized || connection.spotify.busy) connection.close()
        }
        function onRateLimitChanged() {
            if (connection.spotify.rateLimited) connection.close()
            else connection.offerConnection()
        }
    }
    Component.onCompleted: offerConnection()
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(410, parent.width - 24)
    modal: true; focus: true
    title: configured ? qsTr("Spotify needs your permission") : qsTr("Connect your music")
    background: Rectangle { color: "#272a3b"; radius: 4; border.color: "#73778d" }
    contentItem: ColumnLayout {
        spacing: 14
        Text {
            Layout.fillWidth: true; text: "PHOSRACK"; color: "#baff74"
            font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"
            font.letterSpacing: 2; font.pixelSize: 16
        }
        Text {
            Layout.fillWidth: true; color: "#dce3d4"; font.pixelSize: 13; wrapMode: Text.Wrap
            text: connection.configured
                ? qsTr("Approve access in your browser, then return here. Phosrack reconnects automatically when it can restore your saved session.")
                : qsTr("Add your Spotify Client ID once in Settings, then sign in. Phosrack will remember your connection when secure storage is available.")
        }
        Text {
            Layout.fillWidth: true; color: "#a6b49b"; font.pixelSize: 11; wrapMode: Text.Wrap
            text: qsTr("Spotify Premium is required.")
        }
    }
    footer: RowLayout {
        spacing: 8
        Item { Layout.fillWidth: true }
        RackButton { objectName: "connectionLaterButton"; text: qsTr("Not now"); onClicked: connection.close() }
        RackButton {
            objectName: "connectionContinueButton"
            text: connection.configured ? qsTr("Open browser") : qsTr("Open Settings")
            lit: true; enabled: !connection.spotify.busy && !connection.spotify.rateLimited
            onClicked: {
                connection.close()
                if (connection.configured) connection.connectRequested()
                else connection.settingsRequested()
            }
        }
        Item { implicitWidth: 4 }
    }
}
