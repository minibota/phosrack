// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Window

Button {
    id: attribution
    property string targetUrl: "https://open.spotify.com/"
    // The artwork itself stays at least 70 logical pixels wide. The surrounding
    // clear space is half the logo icon's height, per Spotify's brand guidelines.
    property real logoWidth: 70
    readonly property real actualLogoWidth: Math.max(70, logoWidth)
    readonly property real logoHeight: actualLogoWidth * 225.25 / 823.46
    padding: logoHeight / 2
    implicitWidth: actualLogoWidth + padding * 2
    implicitHeight: logoHeight + padding * 2
    hoverEnabled: true
    Accessible.role: Accessible.Link
    Accessible.name: qsTr("Open in Spotify")
    ToolTip.visible: hovered
    ToolTip.text: qsTr("Open in Spotify")
    onClicked: if (/^https:\/\/open\.spotify\.com\//.test(targetUrl)) Qt.openUrlExternally(targetUrl)
    contentItem: Image {
        source: "assets/spotify-logo-white.svg"
        fillMode: Image.PreserveAspectFit
        sourceSize.width: attribution.actualLogoWidth * Screen.devicePixelRatio
        sourceSize.height: attribution.logoHeight * Screen.devicePixelRatio
        smooth: true
    }
    background: Rectangle {
        color: "transparent"
        border.width: attribution.activeFocus ? 1 : 0
        border.color: "#baff74"
        radius: 2
    }
    HoverHandler { cursorShape: Qt.PointingHandCursor }
}
