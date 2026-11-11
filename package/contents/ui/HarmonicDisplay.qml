// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: display
    color: "#070e12"
    property var analyzer
    property bool reducedMotion: false
    readonly property var levels: analyzer ? analyzer.pitchClasses : []
    readonly property var noteNames: qsTr("C, C♯, D, D♯, E, F, F♯, G, G♯, A, A♯, B").split(", ")
    readonly property string monoFont: Qt.platform.os === "windows" ? "Consolas" : "monospace"

    RowLayout {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: parent.height < 200 ? 6 : 0
        width: Math.max(0, parent.width - 20)
        height: Math.max(0, Math.min(parent.height - 44, 260))
        spacing: Math.max(2, Math.min(7, width * 0.012))
        Repeater {
            model: 12
            delegate: ColumnLayout {
                id: note
                required property int index
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                spacing: 4
                property real strength: display.levels[index] || 0
                Behavior on strength { enabled: !display.reducedMotion; NumberAnimation { duration: 105 } }

                Rectangle {
                    objectName: "noteBar" + note.index
                    Layout.fillWidth: true; Layout.fillHeight: true
                    radius: 2
                    color: [1, 3, 6, 8, 10].includes(note.index) ? "#0b1915" : "#12241b"
                    border.color: note.strength > 0.85 ? "#a8e97b" : "#284334"
                    Rectangle {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                        anchors.margins: 2
                        height: Math.max(0, parent.height - 4) * note.strength
                        radius: 2
                        gradient: Gradient {
                            GradientStop { position: 0; color: "#d8ff9f" }
                            GradientStop { position: 1; color: "#2b7956" }
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: display.noteNames[note.index]
                    color: note.strength > 0.50 ? "#d1ff99" : "#a1b8a0"
                    font.family: display.monoFont
                    font.pixelSize: Math.min(18, width * 0.52)
                    fontSizeMode: Text.HorizontalFit; minimumPixelSize: 7
                }
            }
        }
    }
}
