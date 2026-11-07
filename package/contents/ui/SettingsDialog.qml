// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Dialog {
    id: settings
    objectName: "settingsDialog"
    property var spotify
    property var audio
    property var preferences
    property var equalizer
    property bool reducedMotion: false
    property bool advancedSpotifyExpanded: false
    property bool clientDraftModified: false
    signal motionPreferenceChanged(bool reduced)
    function syncClientDraft() {
        if (!clientDraftModified) clientField.text = spotify.usesDefaultClientId ? "" : spotify.clientId
    }
    function applyClientDraft() {
        if (!clientDraftModified) return true
        if (spotify.authorized || spotify.busy) return false
        spotify.clientId = clientField.text.trim()
        clientDraftModified = false
        syncClientDraft()
        return true
    }
    function connectAccount() {
        if (spotify.busy) return
        if (spotify.authorized) spotify.disconnectSpotify()
        else if (applyClientDraft()) {
            spotify.connectSpotify()
            if (!/^[a-fA-F0-9]{32}$/.test(spotify.clientId)) advancedSpotifyExpanded = true
        }
    }
    Connections {
        target: settings.spotify
        function onClientIdChanged() { settings.syncClientDraft() }
    }
    Component.onCompleted: {
        advancedSpotifyExpanded = !spotify.hasDefaultClientId
        syncClientDraft()
    }
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(450, parent.width - 24); height: Math.min(660, parent.height - 32)
    modal: true; focus: true; title: qsTr("Settings")
    footer: Item {
        implicitHeight: 42
        RackButton { anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: qsTr("Close"); onClicked: settings.close() }
    }
    background: Rectangle { color: "#272a3b"; radius: 4; border.color: "#73778d" }
    onOpened: {
        if (!spotify.hasDefaultClientId) advancedSpotifyExpanded = true
        syncClientDraft()
        audio.refreshSources()
        if (spotify.authorized) spotify.refreshDevices()
    }
    contentItem: ScrollView {
        id: settingsScroll; objectName: "settingsScroll"; clip: true; contentWidth: availableWidth
        ScrollBar.vertical: RackScrollBar {}
        ColumnLayout {
            width: settingsScroll.availableWidth; spacing: 10
            Text { text: "SPOTIFY"; color: "#baff74"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.pixelSize: 13 }
            Text { Layout.fillWidth: true; visible: !spotify.authorized; text: spotify.hasDefaultClientId ? qsTr("Sign in with your Spotify account in your browser, then return to Phosrack.") : qsTr("Create your own Spotify developer application, then enter its Client ID below. No Client Secret is needed."); color: "#b3bda9"; font.pixelSize: 12; wrapMode: Text.Wrap }
            RowLayout {
                Layout.fillWidth: true
                RackButton {
                    objectName: "connectSpotifyButton"
                    text: spotify.authorized ? qsTr("Disconnect") : spotify.busy ? qsTr("Connecting…") : qsTr("Connect Spotify")
                    lit: spotify.authorized; enabled: !spotify.busy
                    onClicked: settings.connectAccount()
                }
                RackButton { objectName: "cancelSpotifyButton"; text: qsTr("Cancel"); visible: spotify.busy && !spotify.authorized; onClicked: spotify.disconnectSpotify() }
            }
            Text { Layout.fillWidth: true; text: spotify.error || spotify.status; color: spotify.error ? "#f0b99a" : "#baff74"; font.pixelSize: 11; wrapMode: Text.Wrap }
            Text { Layout.fillWidth: true; text: qsTr("Premium required. Your session is stored in %1 when available.").arg(spotify.credentialStorage); color: "#93ad7b"; font.pixelSize: 11; wrapMode: Text.Wrap }
            Text { Layout.fillWidth: true; text: qsTr("Your Spotify account must be allowed to use the developer application you configure."); color: "#93ad7b"; font.pixelSize: 11; wrapMode: Text.Wrap }
            RackButton {
                objectName: "advancedSpotifyButton"
                text: (settings.advancedSpotifyExpanded ? "▾ " : "▸ ") + (spotify.hasDefaultClientId ? qsTr("Advanced Spotify settings") : qsTr("Spotify application setup"))
                Layout.fillWidth: true
                onClicked: settings.advancedSpotifyExpanded = !settings.advancedSpotifyExpanded
            }
            ColumnLayout {
                objectName: "advancedSpotifyContent"
                visible: settings.advancedSpotifyExpanded
                Layout.fillWidth: true; spacing: 8
                Text { Layout.fillWidth: true; text: qsTr("In the Spotify Developer Dashboard, create an app with Web API enabled and register the redirect URI below."); color: "#b3bda9"; font.pixelSize: 11; wrapMode: Text.Wrap }
                Text { Layout.fillWidth: true; visible: spotify.clientId.length > 0; text: spotify.usesDefaultClientId ? qsTr("Using the built-in Spotify application.") : qsTr("Using your Spotify application."); color: "#93ad7b"; font.pixelSize: 11; wrapMode: Text.Wrap }
                TextField {
                    id: clientField; objectName: "spotifyClientIdField"; Layout.fillWidth: true
                    placeholderText: spotify.hasDefaultClientId ? qsTr("Custom Client ID (empty uses the default)") : qsTr("Client ID (no Client Secret)")
                    placeholderTextColor: "#93ad7b"; enabled: !spotify.authorized && !spotify.busy
                    selectByMouse: true; maximumLength: 32
                    Accessible.name: qsTr("Spotify Client ID")
                    onTextEdited: settings.clientDraftModified = true
                    onEditingFinished: settings.applyClientDraft()
                }
                Text { Layout.fillWidth: true; text: qsTr("Register this redirect URI in your Spotify developer app:"); color: "#b3bda9"; font.pixelSize: 11; wrapMode: Text.Wrap }
                TextField {
                    objectName: "spotifyRedirectUriField"
                    Layout.fillWidth: true; text: "http://127.0.0.1:43821/callback"; readOnly: true; selectByMouse: true
                    Accessible.name: qsTr("Redirect URI"); font.pixelSize: 12
                }
                Flow {
                    Layout.fillWidth: true; spacing: 6
                    RackButton { text: qsTr("Dashboard ↗"); onClicked: Qt.openUrlExternally("https://developer.spotify.com/dashboard") }
                    RackButton {
                        objectName: "resetSpotifyClientIdButton"
                        text: qsTr("Use default application")
                        visible: spotify.hasDefaultClientId
                        enabled: !spotify.authorized && !spotify.busy && (!spotify.usesDefaultClientId || settings.clientDraftModified)
                        onClicked: {
                            settings.clientDraftModified = false
                            spotify.resetToDefaultClientId()
                            settings.syncClientDraft()
                        }
                    }
                }
            }
            ColumnLayout {
                visible: Qt.platform.os === "windows"
                Layout.fillWidth: true; spacing: 6
                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#4b4f66" }
                Text { text: qsTr("EQUALIZER APO"); color: "#baff74"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.pixelSize: 13 }
                Text { Layout.fillWidth: true; text: qsTr("Install Equalizer APO and enable it for your output device. Phosrack updates its own filters when EQ is on."); color: "#93ad7b"; font.pixelSize: 11; wrapMode: Text.Wrap }
                TextField {
                    Layout.fillWidth: true; text: equalizer ? equalizer.apoDirectory : ""
                    Accessible.name: qsTr("Equalizer APO config folder")
                    placeholderText: "C:/Program Files/EqualizerAPO/config"
                    selectByMouse: true
                    onEditingFinished: if (equalizer && Qt.platform.os === "windows") equalizer.apoDirectory = text
                }
                RackButton { text: qsTr("Equalizer APO setup ↗"); onClicked: Qt.openUrlExternally("https://sourceforge.net/p/equalizerapo/wiki/Documentation/") }
            }
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#4b4f66" }
            Text { text: qsTr("VISUALIZER"); color: "#baff74"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.pixelSize: 13 }
            ComboBox {
                id: outputs; Layout.fillWidth: true
                model: [{id:"", name:qsTr("System default output")}].concat(audio.sources)
                textRole: "name"; valueRole: "id"
                currentIndex: { for (let i = 0; i < model.length; ++i) if (model[i].id === audio.source) return i; return -1 }
                Accessible.name: qsTr("Audio output")
                onActivated: { audio.source = currentValue; preferences.write("desktop/audioSource", currentValue) }
            }
            Text { Layout.fillWidth: true; text: audio.outputName || audio.status; color: "#93ad7b"; font.pixelSize: 11; wrapMode: Text.Wrap }
            CheckBox { text: qsTr("Reduce motion"); checked: settings.reducedMotion; onToggled: settings.motionPreferenceChanged(checked) }
            Text { Layout.fillWidth: true; text: qsTr("Runs while the visualizer is visible. Analyses all local sound on this output; remote Spotify devices are not captured."); color: "#93ad7b"; font.pixelSize: 11; wrapMode: Text.Wrap }
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#4b4f66" }
            Text { text: qsTr("SPOTIFY DEVICES"); color: "#baff74"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.pixelSize: 13 }
            Repeater {
                model: spotify.devices
                delegate: RackButton {
                    required property var modelData
                    Layout.fillWidth: true; text: (modelData.active ? "●  " : "○  ") + modelData.name
                    lit: modelData.active; enabled: !modelData.restricted && !spotify.busy
                    onClicked: spotify.transfer(modelData.id)
                }
            }
            Text { Layout.fillWidth: true; visible: spotify.devices.length === 0; text: qsTr("Start Spotify on a device to see it here."); color: "#93ad7b"; font.pixelSize: 11; wrapMode: Text.Wrap }
            Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#4b4f66" }
            Text { Layout.fillWidth: true; text: qsTr("Language follows your system: English, Spanish or French. Other languages use English."); color: "#93ad7b"; font.pixelSize: 11; wrapMode: Text.Wrap }
        }
    }
}
