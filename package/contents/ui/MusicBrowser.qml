// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: browser
    property var spotify
    property string page: "search"
    property string libraryKind: "tracks"
    property bool sessionWasAuthorized: false
    property bool ready: false
    property bool navigating: false
    property string collectionUri: ""
    property string collectionTitle: ""
    property string returnPage: "library"
    // The rack may supply a shared object so rebuilding panels preserves navigation.
    property var navigationState: ({})
    function remember() {
        navigationState.page = page; navigationState.query = query.text
        navigationState.libraryKind = libraryKind; navigationState.collectionUri = collectionUri
        navigationState.collectionTitle = collectionTitle; navigationState.returnPage = returnPage
    }
    function browseCollection(item) {
        navigating = true; returnPage = page; collectionUri = item.uri
        collectionTitle = item.title; page = "library"; navigating = false
        remember(); spotify.openCollection(collectionUri)
    }
    function goBack() {
        const target = returnPage
        collectionUri = ""; collectionTitle = ""; navigating = true
        page = target; navigating = false; remember()
        if (target === "library") spotify.loadLibrary(libraryKind)
    }
    Component.onCompleted: {
        page = navigationState.page || "search"
        libraryKind = navigationState.libraryKind || "tracks"
        collectionUri = navigationState.collectionUri || ""
        collectionTitle = navigationState.collectionTitle || ""
        returnPage = navigationState.returnPage || "library"
        query.text = navigationState.query || ""
        sessionWasAuthorized = spotify.authorized; ready = true; refreshPage()
    }
    onVisibleChanged: if (ready && visible) refreshPage()
    function refreshPage() {
        if (!ready || !visible || !spotify.authorized) return
        debounce.stop()
        if (page === "queue") spotify.refreshQueue()
        else if (page === "library") {
            if (collectionUri) spotify.openCollection(collectionUri)
            else spotify.loadLibrary(libraryKind)
        } else spotify.search(query.text.trim().length > 1 ? query.text : "")
    }
    onPageChanged: if (ready) { remember(); if (!navigating) refreshPage() }
    Connections {
        target: browser.spotify
        function onSessionChanged() {
            if (browser.spotify.authorized && !browser.sessionWasAuthorized) browser.refreshPage()
            browser.sessionWasAuthorized = browser.spotify.authorized
        }
    }
    Timer { id: debounce; interval: 400; onTriggered: if (browser.visible && browser.page === "search" && query.text.trim().length > 1) browser.spotify.search(query.text) }
    Timer { interval: 15000; repeat: true; running: browser.ready && browser.visible && browser.page === "queue" && browser.spotify.authorized; onTriggered: browser.spotify.refreshQueue() }
    ColumnLayout {
        anchors.fill: parent; spacing: 5
        RowLayout {
            Layout.fillWidth: true; spacing: 4
            Repeater {
                model: [{key:"search", label:qsTr("Search")}, {key:"queue", label:qsTr("Queue")}, {key:"library", label:qsTr("Library")}]
                delegate: RackButton {
                    required property var modelData
                    text: modelData.label; lit: browser.page === modelData.key
                    Layout.fillWidth: true; implicitHeight: 28
                    onClicked: browser.page = modelData.key
                }
            }
            RackButton { text: "↻"; description: qsTr("Refresh"); enabled: spotify.authorized && !spotify.collectionBusy; implicitWidth: 28; implicitHeight: 28; onClicked: browser.refreshPage() }
        }
        TextField {
            id: query; objectName: "searchField"; Layout.fillWidth: true; implicitHeight: 30
            visible: browser.page === "search"
            placeholderText: qsTr("Tracks, albums, playlists…"); selectByMouse: true
            placeholderTextColor: "#93ad7b"; color: "#baff74"; font.pixelSize: 12
            background: Rectangle { color: "#070c09"; radius: 3; border.color: query.activeFocus ? "#baff74" : "#62677b" }
            onTextChanged: if (browser.ready) {
                browser.remember()
                if (spotify.authorized) { if (text.trim().length < 2) { debounce.stop(); spotify.search("") } else debounce.restart() }
            }
            onAccepted: { debounce.stop(); if (text.trim().length > 1) spotify.search(text) }
        }
        RowLayout {
            visible: browser.page === "library"; Layout.fillWidth: true; spacing: 4
            Repeater {
                model: [{key:"tracks", label:qsTr("Liked songs")}, {key:"albums", label:qsTr("Albums")}, {key:"playlists", label:qsTr("Playlists")}]
                delegate: RackButton {
                    required property var modelData
                    text: modelData.label; Layout.fillWidth: true; implicitHeight: 26
                    lit: !browser.collectionUri && browser.libraryKind === modelData.key
                    onClicked: {
                        browser.collectionUri = ""; browser.collectionTitle = ""
                        browser.libraryKind = modelData.key; browser.remember()
                        if (spotify.authorized) spotify.loadLibrary(browser.libraryKind)
                    }
                }
            }
        }
        RowLayout {
            visible: browser.page === "library" && browser.collectionUri.length > 0
            Layout.fillWidth: true
            RackButton { text: "←"; description: qsTr("Back"); implicitWidth: 27; implicitHeight: 25; onClicked: browser.goBack() }
            Text { Layout.fillWidth: true; text: browser.collectionTitle; color: "#baff74"; font.pixelSize: 11; elide: Text.ElideRight }
        }
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumHeight: 50
            ListView {
                id: results; objectName: "musicResults"; anchors.fill: parent; clip: true; spacing: 3
                model: browser.page === "search" ? spotify.searchResults : browser.page === "queue" ? spotify.queueItems : spotify.libraryItems
                ScrollBar.vertical: RackScrollBar {}
                delegate: Rectangle {
                    required property var modelData
                    width: results.width - 6; height: 46; radius: 2; color: rowHover.hovered ? "#252d28" : "#0b100d"
                    HoverHandler { id: rowHover }
                    RowLayout {
                        anchors.fill: parent; anchors.margins: 4; spacing: 7
                        Rectangle {
                            Layout.preferredWidth: 36; Layout.preferredHeight: 36; color: "#172619"
                            Text { anchors.centerIn: parent; text: "♫"; color: "#baff74" }
                            Image { anchors.fill: parent; source: modelData.art || ""; fillMode: Image.PreserveAspectCrop; asynchronous: true }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; spacing: 2
                            Text { Layout.fillWidth: true; text: modelData.title || ""; color: "#8cff46"; font.pixelSize: 12; elide: Text.ElideRight }
                            Text { Layout.fillWidth: true; text: modelData.subtitle || ""; color: "#93ad7b"; font.pixelSize: 10; elide: Text.ElideRight }
                        }
                        RackButton {
                            text: modelData.contextUri ? "≡" : "▶"; description: modelData.contextUri ? qsTr("Browse collection") : qsTr("Play")
                            implicitWidth: 27; implicitHeight: 27; enabled: spotify.authorized && !!modelData.uri && (modelData.contextUri || modelData.canPlay !== false) && !spotify.busy
                            onClicked: { if (modelData.contextUri) browser.browseCollection(modelData); else spotify.playItem(modelData.uri) }
                        }
                        RackButton {
                            text: modelData.contextUri ? "▶" : "+"; description: modelData.contextUri ? qsTr("Play collection") : qsTr("Add to queue")
                            implicitWidth: 27; implicitHeight: 27; enabled: spotify.authorized && !!modelData.uri && modelData.canPlay !== false && !spotify.busy
                            onClicked: modelData.contextUri ? spotify.playItem(modelData.uri, true) : spotify.addToQueue(modelData.uri)
                        }
                    }
                }
            }
            Column {
                anchors.centerIn: parent; width: parent.width - 16; spacing: 7
                visible: results.count === 0
                Text {
                    width: parent.width; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.Wrap
                    text: !spotify.authorized ? qsTr("Connect Spotify in Settings") : spotify.collectionBusy ? qsTr("Loading…") : browser.page === "search" ? (query.text.trim().length < 2 ? qsTr("Find your next favourite") : qsTr("No results")) : qsTr("Nothing here yet")
                    color: "#93ad7b"; font.pixelSize: 12
                }
            }
        }
        RackButton {
            Layout.alignment: Qt.AlignHCenter; implicitHeight: 25
            text: spotify.collectionBusy ? qsTr("Loading…") : qsTr("Load more")
            visible: browser.page === "search" ? spotify.hasMoreSearch : browser.page === "library" && spotify.hasMoreLibrary
            enabled: spotify.authorized && !spotify.collectionBusy
            onClicked: browser.page === "search" ? spotify.search(query.text, true) : spotify.loadMoreLibrary()
        }
    }
}
