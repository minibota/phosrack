// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: consent
    objectName: "spotifyConsentDialog"
    signal consentAccepted()
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(410, parent.width - 24)
    modal: true; focus: true; title: qsTr("Before connecting to Spotify")
    background: Rectangle { color: "#272a3b"; radius: 4; border.color: "#73778d" }
    contentItem: ColumnLayout {
        spacing: 14
        Text {
            Layout.fillWidth: true; color: "#dce3d4"; font.pixelSize: 13; wrapMode: Text.Wrap
            text: qsTr("Please read the Terms of Use and Privacy Notice. By choosing Accept and continue, you accept the Terms of Use and acknowledge the Privacy Notice.")
        }
        RackButton {
            text: qsTr("Terms of Use ↗"); Layout.fillWidth: true
            onClicked: Qt.openUrlExternally("https://github.com/minibota/phosrack/blob/main/TERMS.md")
        }
        RackButton {
            text: qsTr("Privacy Notice ↗"); Layout.fillWidth: true
            onClicked: Qt.openUrlExternally("https://github.com/minibota/phosrack/blob/main/PRIVACY.md")
        }
        Text {
            Layout.fillWidth: true; color: "#a6b49b"; font.pixelSize: 11; wrapMode: Text.Wrap
            text: qsTr("You can use Phosrack's local audio tools without connecting to Spotify.")
        }
    }
    footer: ColumnLayout {
        spacing: 6
        RackButton {
            objectName: "acceptSpotifyTermsButton"
            text: qsTr("Accept and continue"); lit: true; Layout.fillWidth: true
            onClicked: { consent.close(); consent.consentAccepted() }
        }
        RackButton { text: qsTr("Not now"); Layout.fillWidth: true; onClicked: consent.close() }
    }
}
