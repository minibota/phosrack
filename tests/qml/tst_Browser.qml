// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import "../../package/contents/ui" as Rack
TestCase {
    id: suite
    name: "BrowserNavigation"
    when: windowShown
    visible: true
    width: 420; height: 320
    QtObject {
        id: fake
        property bool authorized: true
        property bool busy: false
        property bool collectionBusy: false
        property bool hasMoreSearch: false
        property bool hasMoreLibrary: false
        property var searchResults: []
        property var libraryItems: []
        property var queueItems: []
        property int rootLoads: 0
        property int opens: 0
        property int queues: 0
        property string lastSearch: ""
        signal sessionChanged()
        function search(query,more) { lastSearch=query; if(!query)searchResults=[] }
        function loadLibrary(kind) { rootLoads++ }
        function openCollection(uri) { opens++ }
        function refreshQueue() { queues++ }
        function loadMoreLibrary() {}
        function playItem(uri,context) {}
        function addToQueue(uri) {}
    }
    Component { id: component; Rack.MusicBrowser { width: 400; height: 280; spotify: fake } }
    function test_collectionNavigation() {
        const state={page:"search",query:"album"}
        const view=createTemporaryObject(component,suite,{navigationState:state})
        verify(view)
        compare(fake.lastSearch,"album")
        const loads=fake.rootLoads
        const opens=fake.opens
        view.browseCollection({uri:"spotify:album:1111111111111111111111",title:"Album"})
        compare(view.page,"library")
        compare(fake.rootLoads,loads)
        compare(fake.opens,opens+1)
        view.refreshPage()
        compare(fake.opens,opens+2)
        view.goBack()
        compare(view.page,"search")
        compare(view.collectionUri,"")
        compare(state.query,"album")
    }
    function test_recreationPreservesQueryAndDoesNotRefetchOnCommands() {
        const state={page:"search",query:"kept"}
        const first=createTemporaryObject(component,suite,{navigationState:state})
        verify(first)
        first.page="queue"
        const queues=fake.queues
        fake.sessionChanged()
        fake.sessionChanged()
        compare(fake.queues,queues)
        first.page="search"
        const second=createTemporaryObject(component,suite,{navigationState:state})
        verify(second)
        compare(findChild(second,"searchField").text,"kept")
        compare(fake.lastSearch,"kept")
    }
}
