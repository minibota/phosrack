// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import "../../package/contents/ui" as Rack

TestCase {
    id: suite
    name: "SpotifySettings"
    when: windowShown
    visible: true; width: 520; height: 760
    QtObject {
        id: account
        readonly property string defaultId: "11111111111111111111111111111111"
        property string clientId: defaultId
        property bool hasDefaultClientId: true
        property bool integratedPlayback: true
        property bool usesDefaultClientId: hasDefaultClientId && clientId === defaultId
        property bool authorized: false
        property bool busy: false
        property string credentialStorage: "KWallet"
        property string error: ""
        property string status: "Not connected"
        property var devices: []
        property int connections: 0
        property int disconnections: 0
        property string connectedId: ""
        function connectSpotify() {
            connections++; connectedId = clientId
            if (!/^[a-fA-F0-9]{32}$/.test(clientId)) {
                error = "Invalid Client ID"; authorized = false; return
            }
            busy = true
        }
        function disconnectSpotify() { disconnections++; authorized = false; busy = false }
        function resetToDefaultClientId() { clientId = defaultId }
        function refreshDevices() {}
        function transfer(id) {}
    }
    QtObject {
        id: audioMock
        property var sources: []
        property string source: ""
        property string outputName: ""
        property string status: "Waiting for audio"
        function refreshSources() {}
    }
    QtObject { id: preferencesMock; function write(key,value) {} }
    Component { id: component; Rack.SettingsDialog { spotify: account; audio: audioMock; preferences: preferencesMock } }
    function init() {
        account.clientId = account.defaultId; account.hasDefaultClientId = true
        account.authorized = false; account.busy = false; account.error = ""
        account.connections = 0; account.disconnections = 0; account.connectedId = ""
    }
    function dialog() {
        const view = createTemporaryObject(component, suite)
        verify(view)
        view.open(); tryCompare(view, "opened", true)
        return view
    }
    function clickConnect(view) {
        const scroll = findChild(view, "settingsScroll")
        scroll.contentItem.contentY = 0
        wait(30)
        mouseClick(findChild(view, "connectSpotifyButton"))
    }
    function test_defaultFlowHidesTechnicalFieldsAndConnects() {
        const view = dialog()
        compare(view.advancedSpotifyExpanded, false)
        compare(findChild(view, "spotifyClientIdField").visible, false)
        compare(findChild(view, "spotifyRedirectUriField").visible, false)
        clickConnect(view)
        compare(account.connections, 1)
        compare(account.connectedId, account.defaultId)
        verify(!account.authorized)
        compare(findChild(view, "connectSpotifyButton").enabled, false)
        mouseClick(findChild(view, "cancelSpotifyButton"))
        compare(account.disconnections, 1)
        compare(account.busy, false)
        view.close()
    }
    function test_optionalBundledIdIsNeverDisplayedInAdvancedSettings() {
        const view = dialog()
        mouseClick(findChild(view, "advancedSpotifyButton"))
        const field = findChild(view, "spotifyClientIdField")
        tryCompare(field, "visible", true)
        compare(field.text, "")
        clickConnect(view)
        compare(account.connectedId, account.defaultId)
        view.close()
    }
    function test_connectUsesEffectiveIdRatherThanAnUneditedStaleField() {
        const view = dialog()
        const field = findChild(view, "spotifyClientIdField")
        // A programmatic display change is not an explicit custom-ID edit.
        field.text = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        clickConnect(view)
        compare(account.connectedId, account.defaultId)
        compare(account.clientId, account.defaultId)
        view.close()
    }
    function test_customDraftIsAppliedOnConnectAndCanReset() {
        const view = dialog()
        mouseClick(findChild(view, "advancedSpotifyButton"))
        const field = findChild(view, "spotifyClientIdField")
        tryCompare(field, "visible", true)
        field.forceActiveFocus(); field.selectAll()
        for (let i = 0; i < 32; ++i) keyClick(Qt.Key_B)
        compare(field.text, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")
        verify(view.clientDraftModified)
        clickConnect(view)
        compare(account.connections, 1)
        compare(account.connectedId, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")
        compare(view.clientDraftModified, false)
        account.busy = false
        findChild(view, "resetSpotifyClientIdButton").clicked()
        compare(account.clientId, account.defaultId)
        compare(field.text, "")
        view.close()
    }
    function test_missingDefaultExpandsAdvancedAndValidationStaysDisconnected() {
        account.hasDefaultClientId = false; account.clientId = ""
        const view = dialog()
        verify(view.advancedSpotifyExpanded)
        clickConnect(view)
        compare(account.connections, 1)
        verify(!account.authorized)
        verify(!account.busy)
        compare(account.error, "Invalid Client ID")
        compare(findChild(view, "connectSpotifyButton").text, "Connect Spotify")
        view.close()
    }
    function test_personalApplicationSetupConnectsWithEnteredId() {
        account.hasDefaultClientId = false; account.clientId = ""
        const view = dialog()
        verify(view.advancedSpotifyExpanded)
        verify(findChild(view, "advancedSpotifyButton").text.includes("Spotify application setup"))
        const field = findChild(view, "spotifyClientIdField")
        compare(field.text, "")
        compare(findChild(view, "resetSpotifyClientIdButton").visible, false)
        field.forceActiveFocus()
        for (let i = 0; i < 32; ++i) keyClick(Qt.Key_C)
        clickConnect(view)
        compare(account.connectedId, "cccccccccccccccccccccccccccccccc")
        compare(account.connections, 1)
        view.close()
    }
    function test_accountChangesAreDisabledWhileConnectedOrBusy() {
        account.authorized = true
        const view = dialog()
        view.advancedSpotifyExpanded = true
        compare(findChild(view, "spotifyClientIdField").enabled, false)
        compare(findChild(view, "resetSpotifyClientIdButton").enabled, false)
        account.busy = true
        compare(findChild(view, "connectSpotifyButton").enabled, false)
        account.busy = false
        clickConnect(view)
        compare(account.disconnections, 1)
        compare(account.connections, 0)
        view.close()
    }
}
