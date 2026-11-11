// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

Item {
    id: title
    property string text
    property bool reducedMotion: false
    property real offset: 0
    readonly property real travel: measure.advanceWidth + 32
    readonly property bool scrolling: visible && width > 0 && measure.advanceWidth > width && !reducedMotion
    implicitHeight: first.implicitHeight
    TextMetrics { id: measure; font: first.font; text: title.text }
    clip: true
    Accessible.role: Accessible.StaticText
    Accessible.name: text

    function restart() {
        if (!motion) return
        motion.stop()
        offset = 0
        if (scrolling) motion.start()
    }
    onScrollingChanged: restart()
    onTextChanged: restart()
    onWidthChanged: restart()
    Component.onCompleted: restart()

    component TitleText: Text {
        text: title.text
        color: "#baff74"; style: Text.Outline; styleColor: "#27451b"
        font.pixelSize: 16; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"
        maximumLineCount: 1
        Accessible.ignored: true
    }
    TitleText {
        id: first
        x: -title.offset
        width: title.scrolling ? Math.ceil(measure.advanceWidth) : title.width
        elide: title.scrolling ? Text.ElideNone : Text.ElideRight
    }
    TitleText {
        visible: title.scrolling
        x: title.travel - title.offset
    }
    SequentialAnimation {
        id: motion
        loops: Animation.Infinite
        PauseAnimation { duration: 700 }
        NumberAnimation { target: title; property: "offset"; from: 0; to: title.travel; duration: Math.round(title.travel / 0.038) }
    }
}
