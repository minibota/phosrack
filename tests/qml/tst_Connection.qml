// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import "../../package/contents/ui" as Rack

TestCase {
    id: suite
    name: "StartupConnection"
    when: windowShown
    visible: true; width: 520; height: 760
    QtObject {
        id: account
        property string clientId: "11111111111111111111111111111111"
        property bool authorizationRequired: false
        property bool authorized: false
        property bool busy: false
        property bool rateLimited: false
        signal sessionChanged()
        signal rateLimitChanged()
    }
    Component { id: connectionComponent; Rack.ConnectionDialog { spotify: account } }
    Component { id: consentComponent; Rack.SpotifyConsentDialog {} }
    SignalSpy { id: connectSpy; signalName: "connectRequested" }
    SignalSpy { id: settingsSpy; signalName: "settingsRequested" }
    SignalSpy { id: consentSpy; signalName: "consentAccepted" }
    function init() {
        account.clientId = "11111111111111111111111111111111"
        account.authorizationRequired = false; account.authorized = false; account.busy = false; account.rateLimited = false
        connectSpy.target = null; connectSpy.clear()
        settingsSpy.target = null; settingsSpy.clear()
        consentSpy.target = null; consentSpy.clear()
    }
    function test_existingSessionDoesNotInterruptStartup() {
        const view = createTemporaryObject(connectionComponent, suite)
        verify(view)
        connectSpy.target = view
        account.authorized = true; account.sessionChanged()
        compare(view.visible, false)
        compare(connectSpy.count, 0)
    }
    function test_permissionPromptOnlyLaunchesAfterUserContinues() {
        const view = createTemporaryObject(connectionComponent, suite)
        verify(view)
        connectSpy.target = view
        account.authorizationRequired = true
        tryCompare(view, "opened", true)
        compare(connectSpy.count, 0)
        compare(findChild(view, "connectionContinueButton").text, "Open browser")
        mouseClick(findChild(view, "connectionContinueButton"))
        compare(connectSpy.count, 1)
        tryCompare(view, "visible", false)
    }
    function test_missingIdOffersSetupAndDismissalDoesNotRepeat() {
        account.clientId = ""
        const view = createTemporaryObject(connectionComponent, suite)
        verify(view)
        settingsSpy.target = view
        account.authorizationRequired = true
        tryCompare(view, "opened", true)
        compare(findChild(view, "connectionContinueButton").text, "Open Settings")
        mouseClick(findChild(view, "connectionContinueButton"))
        compare(settingsSpy.count, 1)
        tryCompare(view, "visible", false)
        account.sessionChanged()
        compare(view.visible, false)
    }
    function test_notNowDoesNotConnect() {
        account.authorizationRequired = true
        const view = createTemporaryObject(connectionComponent, suite)
        verify(view)
        connectSpy.target = view
        tryCompare(view, "opened", true)
        mouseClick(findChild(view, "connectionLaterButton"))
        tryCompare(view, "visible", false)
        compare(connectSpy.count, 0)
    }
    function test_rateLimitWaitsBeforeOfferingBrowserSignIn() {
        account.rateLimited = true; account.authorizationRequired = true
        const view = createTemporaryObject(connectionComponent, suite)
        verify(view)
        connectSpy.target = view
        compare(view.visible, false)
        account.rateLimited = false; account.rateLimitChanged()
        tryCompare(view, "opened", true)
        compare(connectSpy.count, 0)
        account.rateLimited = true; account.rateLimitChanged()
        tryCompare(view, "visible", false)
    }
    function test_completedAuthorizationClosesPrompt() {
        account.authorizationRequired = true
        const view = createTemporaryObject(connectionComponent, suite)
        verify(view)
        tryCompare(view, "opened", true)
        account.authorized = true; account.sessionChanged()
        tryCompare(view, "visible", false)
    }
    function test_consentRequiresExplicitAcceptance() {
        const view = createTemporaryObject(consentComponent, suite)
        verify(view)
        consentSpy.target = view
        view.open(); tryCompare(view, "opened", true)
        compare(consentSpy.count, 0)
        view.close(); tryCompare(view, "visible", false)
        compare(consentSpy.count, 0)
        view.open(); tryCompare(view, "opened", true)
        mouseClick(findChild(view, "acceptSpotifyTermsButton"))
        compare(consentSpy.count, 1)
        tryCompare(view, "visible", false)
    }
}
