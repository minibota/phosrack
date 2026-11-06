// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: fullscreen
    objectName: "visualizerFullscreen"
    property var analyzer
    property var spotify
    property string effect: "orbit"
    property bool reducedMotion: false
    readonly property bool presentationActive: visible && visibility !== Window.Minimized
    readonly property var currentTrack: spotify ? spotify.track : ({})
    readonly property var nextTrack: spotify && spotify.queueItems.length > 0 ? spotify.queueItems[0] : ({})
    readonly property string currentSpotifyUrl: spotifyUrl(currentTrack)
    readonly property string nextSpotifyUrl: spotifyUrl(nextTrack)
    property string observedTrackIdentity: ""
    property bool wasAuthorized: false
    property bool queueRefreshPending: false
    property bool chromeVisible: true

    visible: false
    width: 1280; height: 720
    color: "#070c12"
    title: "Phosrack — " + qsTr("Visualizer")
    flags: Qt.Window | Qt.FramelessWindowHint

    function spotifyUrl(item) {
        if (/^https:\/\/open\.spotify\.com\//.test(item.url || "")) return item.url
        const match = /^spotify:(track|episode|album|playlist|show|artist|audiobook|chapter):([A-Za-z0-9]+)$/.exec(item.uri || "")
        return match ? "https://open.spotify.com/" + match[1] + "/" + match[2] : ""
    }
    function trackIdentity() {
        const track = currentTrack
        return [track.url || "", track.title || "", track.artist || "", track.duration || 0].join("\u001f")
    }
    function refreshQueue() {
        if (!presentationActive || !spotify || !spotify.authorized) return
        queueRefreshPending = true
        spotify.refreshQueue()
    }
    function showChrome() {
        chromeVisible = true
        chromeTimer.restart()
    }
    function open(targetScreen) {
        if (targetScreen) screen = targetScreen
        showFullScreen()
        visualFocus.forceActiveFocus()
        showChrome()
    }
    onPresentationActiveChanged: {
        if (presentationActive) {
            observedTrackIdentity = trackIdentity()
            wasAuthorized = !!spotify && spotify.authorized
            refreshQueue()
            showChrome()
        } else {
            queueDelay.stop()
            chromeTimer.stop()
            queueRefreshPending = false
        }
    }
    onCurrentTrackChanged: {
        const identity = trackIdentity()
        if (identity === observedTrackIdentity) return
        observedTrackIdentity = identity
        if (presentationActive) queueDelay.restart()
    }
    Connections {
        target: fullscreen.spotify
        function onQueueChanged() { fullscreen.queueRefreshPending = false }
        function onCollectionsChanged() {
            if (!fullscreen.spotify.collectionBusy) fullscreen.queueRefreshPending = false
        }
        function onSessionChanged() {
            const authorized = fullscreen.spotify.authorized
            if (authorized && !fullscreen.wasAuthorized) fullscreen.refreshQueue()
            fullscreen.wasAuthorized = authorized
        }
    }
    Timer { id: queueDelay; interval: 500; onTriggered: fullscreen.refreshQueue() }
    Timer { interval: 15000; repeat: true; running: fullscreen.presentationActive && !!fullscreen.spotify && fullscreen.spotify.authorized; onTriggered: fullscreen.refreshQueue() }
    Timer {
        id: chromeTimer; interval: 2800
        onTriggered: {
            if (exitButton.hovered || exitButton.activeFocus) restart()
            else fullscreen.chromeVisible = false
        }
    }
    Shortcut { sequence: "Escape"; context: Qt.WindowShortcut; enabled: fullscreen.presentationActive; onActivated: fullscreen.close() }
    Shortcut { sequence: "F11"; context: Qt.WindowShortcut; enabled: fullscreen.presentationActive; onActivated: fullscreen.close() }

    component ArtworkLink: Button {
        id: linkControl
        property string targetUrl
        property string songTitle
        enabled: targetUrl.length > 0
        hoverEnabled: true
        Accessible.role: Accessible.Link
        Accessible.name: qsTr("Open in Spotify")
        Accessible.description: songTitle
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Open in Spotify")
        contentItem: Item {}
        background: Rectangle {
            color: linkControl.hovered ? "#1629391e" : "transparent"
            border.color: "#baff74"; border.width: linkControl.activeFocus ? 1 : 0
        }
        HoverHandler { cursorShape: Qt.PointingHandCursor; enabled: linkControl.enabled }
        Rectangle {
            visible: linkControl.enabled
            anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 2
            width: 17; height: 17; radius: 1; color: "#d0070c12"
            Text { anchors.centerIn: parent; text: "↗"; color: "#baff74"; font.pixelSize: 14 }
        }
        onClicked: Qt.openUrlExternally(targetUrl)
        onHoveredChanged: if (hovered) fullscreen.showChrome()
        onActiveFocusChanged: if (activeFocus) fullscreen.showChrome()
    }

    FocusScope {
        id: visualFocus
        anchors.fill: parent; focus: true
        Keys.onEscapePressed: fullscreen.close()
        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_F11) { fullscreen.close(); event.accepted = true }
        }
        LiveCanvas {
            anchors.fill: parent
            visible: fullscreen.presentationActive
            analyzer: fullscreen.analyzer; effect: fullscreen.effect; reducedMotion: fullscreen.reducedMotion
        }
        MouseArea {
            anchors.fill: parent; acceptedButtons: Qt.NoButton; hoverEnabled: true
            onPositionChanged: fullscreen.showChrome()
        }
        RowLayout {
            anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
            anchors.margins: 22; spacing: 10
            opacity: fullscreen.chromeVisible ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: fullscreen.reducedMotion ? 0 : 250 } }
            Text { text: "PHOSRACK"; color: "#baff74"; style: Text.Outline; styleColor: "#27451b"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.pixelSize: 15; font.letterSpacing: 3 }
            Item { Layout.fillWidth: true }
            RackButton {
                id: exitButton
                objectName: "exitFullscreenButton"
                text: "Esc  ⤡"; description: qsTr("Exit full screen (Esc)")
                implicitWidth: 88; implicitHeight: 36
                onClicked: fullscreen.close()
                onHoveredChanged: if (hovered) fullscreen.showChrome()
                onActiveFocusChanged: if (activeFocus) fullscreen.showChrome()
            }
        }
        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            height: songOverlay.height + 100
            gradient: Gradient {
                GradientStop { position: 0; color: "#00070c12" }
                GradientStop { position: 0.55; color: "#b9070c12" }
                GradientStop { position: 1; color: "#ed070c12" }
            }
        }
        GridLayout {
            id: songOverlay
            objectName: "songInfoOverlay"
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.margins: Math.min(32, fullscreen.width * 0.035)
            columns: width < 700 ? 1 : 2
            columnSpacing: 42; rowSpacing: 20
            opacity: fullscreen.chromeVisible ? 1 : 0.80
            Behavior on opacity { NumberAnimation { duration: fullscreen.reducedMotion ? 0 : 350 } }
            RowLayout {
                Layout.fillWidth: true; spacing: 16
                Rectangle {
                    Layout.preferredWidth: 72; Layout.preferredHeight: 72
                    color: "#222b36"; border.color: "#66717e"; radius: 2
                    Text { anchors.centerIn: parent; text: "♫"; color: "#baff74"; font.pixelSize: 31; visible: currentArtwork.status !== Image.Ready }
                    Image { id: currentArtwork; anchors.fill: parent; source: fullscreen.currentTrack.art || ""; asynchronous: true; fillMode: Image.PreserveAspectCrop }
                    ArtworkLink {
                        objectName: "currentArtworkLink"; anchors.fill: parent
                        targetUrl: fullscreen.currentSpotifyUrl; songTitle: fullscreen.currentTrack.title || ""
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    Text { text: qsTr("NOW PLAYING"); color: "#d0b976"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.pixelSize: 11; font.letterSpacing: 2 }
                    Text {
                        objectName: "currentSongTitle"
                        Layout.fillWidth: true; text: fullscreen.currentTrack.title || qsTr("Nothing playing")
                        color: "#baff74"; style: Text.Outline; styleColor: "#27451b"; font.pixelSize: fullscreen.width < 700 ? 19 : 24
                        font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; maximumLineCount: 2; wrapMode: Text.Wrap; elide: Text.ElideRight
                    }
                    Text {
                        objectName: "currentSongArtist"
                        Layout.fillWidth: true; text: fullscreen.currentTrack.artist || ""
                        color: "#94ba7e"; font.pixelSize: 15; elide: Text.ElideRight
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 13
                Rectangle {
                    Layout.preferredWidth: 54; Layout.preferredHeight: 54
                    color: "#1b242e"; border.color: "#495563"; radius: 2
                    Text { anchors.centerIn: parent; text: "♫"; color: "#8ca580"; font.pixelSize: 25; visible: nextArtwork.status !== Image.Ready }
                    Image { id: nextArtwork; anchors.fill: parent; source: fullscreen.nextTrack.art || ""; asynchronous: true; fillMode: Image.PreserveAspectCrop }
                    ArtworkLink {
                        objectName: "nextArtworkLink"; anchors.fill: parent
                        targetUrl: fullscreen.nextSpotifyUrl; songTitle: fullscreen.nextTrack.title || ""
                    }
                }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 5
                    Text { text: qsTr("UP NEXT"); color: "#9faeba"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.pixelSize: 11; font.letterSpacing: 2 }
                    Text {
                        objectName: "nextSongTitle"
                        Layout.fillWidth: true
                        text: fullscreen.nextTrack.title || (fullscreen.queueRefreshPending ? qsTr("Loading queue…") : qsTr("Next track unavailable"))
                        color: "#b1d399"; style: Text.Outline; styleColor: "#203519"; font.pixelSize: fullscreen.width < 700 ? 16 : 19
                        maximumLineCount: 2; wrapMode: Text.Wrap; elide: Text.ElideRight
                    }
                    Text {
                        objectName: "nextSongArtist"
                        Layout.fillWidth: true; text: fullscreen.nextTrack.subtitle || ""
                        color: "#95a2af"; font.pixelSize: 14; elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
