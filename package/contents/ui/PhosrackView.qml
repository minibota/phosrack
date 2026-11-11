// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Phosrack 1.0
Item {
    id: rack
    property bool pollingActive: visible
    property var panelOrder: ["player", "equalizer", "visualizer", "browser"]
    property string visualEffect: "orbit"
    readonly property var visualEffects: [
        {key: "orbit", icon: "◎", label: qsTr("Orbit")},
        {key: "spectrum", icon: "▥", label: qsTr("Spectrum")},
        {key: "scope", icon: "∿", label: qsTr("Oscilloscope")},
        {key: "grid", icon: "▦", label: qsTr("Phosphor grid")},
        {key: "notes", icon: "♫", label: qsTr("Notes and chords")}
    ]
    property bool reducedMotion: false
    property bool visualizerFolded: settingsStore.read("desktop/folded/visualizer", false)
    property bool browserFolded: settingsStore.read("desktop/folded/browser", false)
    property var browserNavigation: ({})
    property var musicBrowser: null
    property bool consentRestoreOnly: false
    property bool termsAcceptedThisSession: false
    SpotifyClient { id: session; objectName: "spotifySession"; active: rack.pollingActive || fullVisualizer.presentationActive }
    Equalizer { id: equalizerBackend; objectName: "equalizerBackend" }
    AudioAnalyzer {
        id: monitor
        objectName: "audioMonitor"
        active: (rack.pollingActive && rack.visible && !rack.visualizerFolded) || fullVisualizer.presentationActive
        pitchEnabled: rack.visualEffect === "notes"
    }
    Preferences { id: settingsStore }
    Component.onCompleted: {
        monitor.source = settingsStore.read("desktop/audioSource", "")
        try {
            const order = JSON.parse(settingsStore.read("desktop/panelOrder", "[]"))
            if (order.length === 3 && ["player", "visualizer", "browser"].every(k => order.indexOf(k) >= 0)) order.splice(order.indexOf("player") + 1, 0, "equalizer")
            if (order.length === 4 && ["player", "equalizer", "visualizer", "browser"].every(k => order.indexOf(k) >= 0)) panelOrder = order
        } catch (e) {}
        const effect = settingsStore.read("desktop/visualEffect", "orbit")
        if (visualEffects.some(entry => entry.key === effect)) visualEffect = effect
        reducedMotion = settingsStore.read("reducedMotion", false)
    }
    function time(ms) { const n = Math.max(0, Math.floor(ms / 1000)); return Math.floor(n / 60).toString().padStart(2, "0") + ":" + (n % 60).toString().padStart(2, "0") }
    function openSettings() { settings.open() }
    function openVisualizer() { fullVisualizer.open(rack.Window.window.screen) }
    function closeVisualizer() { fullVisualizer.close() }
    function requestSpotifyConnection(restoreOnly) {
        if (termsAcceptedThisSession || settingsStore.read("legal/acceptedVersion", "") === "2026-11-10") {
            if (restoreOnly) session.restoreSession()
            else session.connectSpotify()
        } else {
            consentRestoreOnly = restoreOnly
            spotifyConsent.open()
        }
    }
    function restoreSpotifySession() { requestSpotifyConnection(true) }
    function connectSpotify() { requestSpotifyConnection(false) }
    // Refresh on navigation and track changes, never poll an unchanged queue.
    property string lastQueueTrack: ""
    Connections {
        target: session
        function onTrackChanged() {
            const identity = session.track.url || session.track.title || ""
            if (identity === rack.lastQueueTrack) return
            rack.lastQueueTrack = identity
            if (rack.pollingActive && rack.musicBrowser && rack.musicBrowser.queueVisible)
                queueTrackDelay.restart()
        }
    }
    Timer { id: queueTrackDelay; interval: 500; onTriggered: if (session.authorized && !session.rateLimited) session.refreshQueue() }
    function movePanel(key, direction) {
        const order = panelOrder.slice(), from = order.indexOf(key), to = from + direction
        if (to < 0 || to >= order.length) return
        order.splice(from, 1); order.splice(to, 0, key); panelOrder = order
        settingsStore.write("desktop/panelOrder", JSON.stringify(order))
    }
    ColumnLayout {
        anchors.fill: parent; spacing: 4
        Repeater {
            model: rack.panelOrder
            delegate: RackPanel {
                id: panel
                required property string modelData
                required property int index
                Layout.fillWidth: true; Layout.fillHeight: panelKey === "browser" && !folded
                Layout.minimumHeight: panelKey === "browser" && !folded ? 168 : implicitHeight
                Layout.preferredHeight: implicitHeight
                panelKey: modelData; objectName: "panel_" + panelKey
                title: ({player:qsTr("PLAYER"), equalizer:qsTr("EQUALIZER"), visualizer:qsTr("VISUALIZER"), browser:qsTr("MUSIC")})[panelKey]
                preferredBodyHeight: panelKey === "browser" ? 192 : -1
                folded: panelKey === "visualizer" ? rack.visualizerFolded : panelKey === "browser" ? rack.browserFolded : settingsStore.read("desktop/folded/" + panelKey, false)
                canMoveUp: index > 0; canMoveDown: index < 3
                onMoveRequested: function(direction) { rack.movePanel(panelKey, direction) }
                onFoldRequested: {
                    if (panelKey === "visualizer") rack.visualizerFolded = !rack.visualizerFolded
                    else if (panelKey === "browser") rack.browserFolded = !rack.browserFolded
                    else folded = !folded
                    settingsStore.write("desktop/folded/" + panelKey, folded)
                }
                Loader {
                    width: parent.width
                    height: panel.panelKey === "browser" ? Math.max(116, panel.height - 35) : (item ? item.implicitHeight : 0)
                    sourceComponent: ({player:playerComponent, equalizer:equalizerComponent, visualizer:visualComponent, browser:browserComponent})[panel.modelData]
                }
            }
        }
        Item { Layout.fillHeight: true; visible: rack.browserFolded; Layout.preferredHeight: 0 }
    }
    Component {
        id: playerComponent
        ColumnLayout {
            spacing: 4
            Rectangle {
                id: playerDisplay
                objectName: "playerDisplay"
                Layout.fillWidth: true
                Layout.minimumHeight: implicitHeight
                implicitHeight: Math.max(93, displayContents.implicitHeight + 16)
                radius: 3; color: "#070d08"; border.color: "#65687b"
                Repeater {
                    model: Math.floor((playerDisplay.height - 2) / 3)
                    Rectangle { required property int index; x: 1; y: index * 3; width: parent.width - 2; height: 1; color: "#091208"; opacity: 0.4 }
                }
                RowLayout {
                    id: displayContents
                    anchors.fill: parent; anchors.margins: 8; spacing: 10
                    Rectangle {
                        Layout.preferredWidth: 62; Layout.preferredHeight: 62; color: "#172819"
                        Text { anchors.centerIn: parent; text: "♫"; color: "#c3e2a0"; font.pixelSize: 32; visible: artwork.status !== Image.Ready }
                        Image { id: artwork; anchors.fill: parent; source: session.track.art || ""; cache: false; fillMode: Image.PreserveAspectFit; asynchronous: true }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; Layout.minimumWidth: 0; spacing: 3
                        SpotifyAttribution { targetUrl: session.track.url || "https://open.spotify.com/" }
                        Text { objectName: "playerTrackTitle"; Layout.fillWidth: true; text: session.track.title || qsTr("Your music, your way"); color: "#baff74"; style: Text.Outline; styleColor: "#27451b"; font.pixelSize: 16; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; maximumLineCount: 1; elide: Text.ElideRight }
                        Text { objectName: "playerTrackArtist"; Layout.fillWidth: true; text: session.track.artist || (session.authorized ? (session.integratedPlayback ? qsTr("Choose a track in Search or Library") : qsTr("Start playing in Spotify")) : qsTr("Connect Spotify in Settings")); color: "#89b86d"; font.pixelSize: 11; maximumLineCount: 1; elide: Text.ElideRight }
                        Text { objectName: "playerTrackTime"; Layout.fillWidth: true; Layout.minimumWidth: 0; text: rack.time(session.track.position || 0) + " / " + rack.time(session.track.duration || 0); color: "#baff74"; style: Text.Outline; styleColor: "#27451b"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.pixelSize: 25; fontSizeMode: Text.HorizontalFit; minimumPixelSize: 16; maximumLineCount: 1 }
                    }
                }
            }
            RackSlider {
                Layout.fillWidth: true; implicitHeight: 20; from: 0; to: Math.max(1, session.track.duration || 0)
                enabled: session.track.canSeek && !session.busy; Accessible.name: qsTr("Playback position")
                remoteValue: session.track.position || 0
                onValueCommitted: function(nextValue) { session.seek(Math.round(nextValue)) }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 5
                RackButton { text: "⏮"; description: qsTr("Previous track"); enabled: session.track.canPrevious && !session.busy; onClicked: session.previous() }
                RackButton { text: session.track.playing ? "Ⅱ" : "▶"; description: session.track.playing ? qsTr("Pause") : qsTr("Play"); lit: true; implicitWidth: 43; enabled: session.track.canToggle && !session.busy; onClicked: session.playPause() }
                RackButton { text: "⏭"; description: qsTr("Next track"); enabled: session.track.canNext && !session.busy; onClicked: session.next() }
                Item { Layout.fillWidth: true }
                RackButton { text: "⤨"; description: qsTr("Shuffle"); lit: session.track.shuffle || false; enabled: session.track.canShuffle && !session.busy; onClicked: session.toggleShuffle() }
                RackButton { text: session.track.repeat === "track" ? "↻¹" : "↻"; description: qsTr("Repeat"); lit: session.track.repeat !== "off"; enabled: session.track.canRepeat && !session.busy; onClicked: session.cycleRepeat() }
                RackButton { text: "↗"; description: qsTr("Open in Spotify"); enabled: !!session.track.url; onClicked: Qt.openUrlExternally(session.track.url) }
            }
            RowLayout {
                Layout.fillWidth: true
                Text { text: "VOL"; color: "#b1bea3"; font.pixelSize: 10 }
                RackSlider {
                    id: volumeSlider; Layout.fillWidth: true; implicitHeight: 20; from: 0; to: 100; stepSize: 1
                    enabled: session.track.canVolume && !session.busy; Accessible.name: qsTr("Spotify volume")
                    remoteValue: session.track.volume || 0
                    onValueCommitted: function(nextValue) { session.setVolume(Math.round(nextValue)) }
                }
                Text { text: Math.round(volumeSlider.value) + "%"; color: "#c8d6b6"; font.pixelSize: 11 }
            }
            Text { Layout.fillWidth: true; text: session.rateLimited ? session.rateLimitMessage : session.error || (session.integratedPlayback && webPlayback.error ? webPlayback.error : session.track.device || (session.authorized && session.integratedPlayback ? webPlayback.status : session.status)); color: session.rateLimited || session.error || (session.integratedPlayback && webPlayback.error) ? "#f0b99a" : "#a6b49b"; font.pixelSize: 10; maximumLineCount: 2; wrapMode: Text.Wrap; elide: Text.ElideRight }
        }
    }
    Component {
        id: visualComponent
        Rectangle {
            implicitHeight: 142; color: "#080f13"; radius: 3; clip: true
            LiveCanvas { anchors.fill: parent; visible: !fullVisualizer.presentationActive; animationActive: rack.pollingActive; analyzer: monitor; effect: rack.visualEffect; reducedMotion: rack.reducedMotion }
            Text { anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 5; text: qsTr("LOCAL AUDIO"); color: "#93ad7b"; font.pixelSize: 8 }
            Row {
                anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 5; spacing: 3
                RackButton { text: "⛶"; chrome: true; description: qsTr("Full screen"); implicitWidth: 25; implicitHeight: 23; onClicked: rack.openVisualizer() }
                Repeater {
                    model: rack.visualEffects
                    delegate: RackButton {
                        required property var modelData
                        text: modelData.icon; chrome: true; description: modelData.label; implicitWidth: 25; implicitHeight: 23
                        lit: rack.visualEffect === modelData.key
                        onClicked: { rack.visualEffect = modelData.key; settingsStore.write("desktop/visualEffect", rack.visualEffect) }
                    }
                }
            }
        }
    }
    Component { id: equalizerComponent; EqualizerPanel { equalizer: equalizerBackend; preferences: settingsStore } }
    Component {
        id: browserComponent
        MusicBrowser {
            id: browserView
            spotify: session; navigationState: rack.browserNavigation
            Component.onCompleted: rack.musicBrowser = browserView
            Component.onDestruction: if (rack.musicBrowser === browserView) rack.musicBrowser = null
        }
    }
    VisualizerFullscreen {
        id: fullVisualizer; objectName: "visualizerFullscreen"
        analyzer: monitor; spotify: session; effect: rack.visualEffect; reducedMotion: rack.reducedMotion
    }
    SettingsDialog {
        id: settings; spotify: session; audio: monitor; preferences: settingsStore; equalizer: equalizerBackend; reducedMotion: rack.reducedMotion
        playbackController: webPlayback; playbackRuntime: playbackEnvironment
        connectionHandler: function() { rack.connectSpotify() }
        onMotionPreferenceChanged: function(reduced) { rack.reducedMotion = reduced; settingsStore.write("reducedMotion", reduced) }
    }
    ConnectionDialog {
        spotify: session
        onSettingsRequested: rack.openSettings()
        onConnectRequested: rack.connectSpotify()
    }
    SpotifyConsentDialog {
        id: spotifyConsent
        onConsentAccepted: {
            rack.termsAcceptedThisSession = true
            settingsStore.write("legal/acceptedVersion", "2026-11-10")
            rack.requestSpotifyConnection(rack.consentRestoreOnly)
        }
    }
}
