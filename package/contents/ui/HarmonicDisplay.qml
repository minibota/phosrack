// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: display
    color: "#070e12"
    property var analyzer
    property bool reducedMotion: false
    readonly property var levels: analyzer ? analyzer.pitchClasses : []
    readonly property int rootNote: analyzer ? analyzer.chordRoot : -1
    readonly property var noteNames: qsTr("C, C♯, D, D♯, E, F, F♯, G, G♯, A, A♯, B").split(", ")
    readonly property string monoFont: Qt.platform.os === "windows" ? "Consolas" : "monospace"

    RowLayout {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: parent.height < 200 ? 6 : 0
        width: Math.max(0, Math.min(parent.width - 20, 950))
        height: Math.max(0, Math.min(parent.height - 44, 260))
        spacing: Math.min(22, width * 0.035)

        Rectangle {
            Layout.preferredWidth: Math.max(76, Math.min(210, parent.width * 0.25))
            Layout.fillHeight: true
            radius: 3; color: "#102219"; border.color: "#355a38"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 7
                spacing: 4
                Item { Layout.fillHeight: true }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Probable chord")
                    color: "#a6c18f"; font.family: display.monoFont
                    font.pixelSize: Math.min(15, parent.width * 0.11)
                    fontSizeMode: Text.HorizontalFit; minimumPixelSize: 8
                }
                Text {
                    objectName: "estimatedChord"
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: display.rootNote < 0 ? "—"
                        : (display.analyzer.minorChord ? qsTr("%1 minor") : qsTr("%1 major")).arg(display.noteNames[display.rootNote])
                    color: display.rootNote < 0 ? "#7c9873" : "#d1ff8f"
                    style: Text.Outline; styleColor: "#284e26"
                    font.family: display.monoFont; font.bold: true
                    font.pixelSize: Math.min(34, parent.width * 0.23)
                    fontSizeMode: Text.HorizontalFit; minimumPixelSize: 10
                }
                Item { Layout.fillHeight: true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
}
