// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import "../../package/contents/ui" as Rack

TestCase {
    id: suite
    name: "FullscreenVisualizer"
    when: windowShown
    visible: true; width: 480; height: 320
    QtObject {
        id: playback
        property bool authorized: true
        property bool collectionBusy: false
        property var track: ({title:"Current test track", artist:"Current artist", url:"test-current", duration:100000, position:0})
        property var queueItems: [{title:"Next test track", subtitle:"Next artist", uri:"test-next"}]
        property int queueRequests: 0
        signal queueChanged()
        signal sessionChanged()
        signal collectionsChanged()
        function refreshQueue() { queueRequests++ }
    }
    QtObject {
        id: audio
        property var bands: [0,0,0]
        property var waveform: [0,0,0]
        property real level: 0
        property bool running: false
        signal frameReady()
    }
    Component { id: component; Rack.VisualizerFullscreen { analyzer: audio; spotify: playback } }
    function init() {
        playback.authorized = true
        playback.track = {title:"Current test track", artist:"Current artist", url:"test-current", duration:100000, position:0}
        playback.queueItems = [{title:"Next test track", subtitle:"Next artist", uri:"test-next"}]
        playback.queueRequests = 0
    }
    function test_overlayShowsCurrentAndNextAndKeepsIdentityStable() {
        const view = createTemporaryObject(component, suite)
        verify(view)
        view.open()
        tryCompare(view, "presentationActive", true)
        compare(findChild(view,"currentSongTitle").text,"Current test track")
        compare(findChild(view,"nextSongTitle").text,"Next test track")
        compare(findChild(view,"nextSongArtist").text,"Next artist")
        const requests = playback.queueRequests
        playback.track = {title:"Current test track", artist:"Current artist", url:"test-current", duration:100000, position:500}
        wait(650)
        compare(playback.queueRequests, requests)
        playback.track = {title:"Another test track", artist:"Current artist", url:"changed-current", duration:120000, position:0}
        tryCompare(playback, "queueRequests", requests + 1)
        view.close()
        tryCompare(view,"presentationActive",false)
    }
    function test_missingNextDoesNotInventData() {
        playback.queueItems = []
        const view = createTemporaryObject(component, suite)
        verify(view)
        view.open()
        tryCompare(view, "presentationActive", true)
        verify(findChild(view,"songInfoOverlay").visible)
        verify(!findChild(view,"nextSongTitle").visible)
        playback.queueItems = [{title:"Next test track"}]
        tryCompare(findChild(view,"nextSongTitle"), "visible", true)
        view.close()
    }
    function test_escapeLeavesFullscreen() {
        const view = createTemporaryObject(component, suite)
        verify(view)
        view.open()
        tryCompare(view, "presentationActive", true)
        keyClick(Qt.Key_Escape)
        tryCompare(view, "presentationActive", false)
    }
    function test_exitButtonAndClosedWindowStopQueueRefresh() {
        const view = createTemporaryObject(component, suite)
        verify(view)
        view.open()
        tryCompare(view, "presentationActive", true)
        const button = findChild(view,"exitFullscreenButton")
        verify(button)
        mouseClick(button)
        tryCompare(view, "presentationActive", false)
        const requests = playback.queueRequests
        playback.track = {title:"Changed while closed", artist:"Artist", url:"closed-current", duration:120000, position:0}
        wait(650)
        compare(playback.queueRequests,requests)
    }
    function test_artworkLinksResolveSpotifyUrlsAndDisableWhenUnavailable() {
        playback.track = {title:"Linked current track", url:"https://open.spotify.com/track/current123"}
        playback.queueItems = [{title:"Linked next track", uri:"spotify:track:next123"}]
        const view = createTemporaryObject(component, suite)
        verify(view)
        compare(view.currentSpotifyUrl,"https://open.spotify.com/track/current123")
        compare(view.nextSpotifyUrl,"https://open.spotify.com/track/next123")
        compare(findChild(view,"currentArtworkLink").enabled,true)
        compare(findChild(view,"nextArtworkLink").enabled,true)
        playback.queueItems = []
        compare(findChild(view,"nextArtworkLink").enabled,false)
        compare(view.spotifyUrl({uri:"spotify:track:bad/path"}),"")
        compare(view.spotifyUrl({url:"https://example.org/unrelated"}),"")
    }
}
