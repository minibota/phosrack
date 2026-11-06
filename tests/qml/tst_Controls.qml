// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import "../../package/contents/ui" as Rack

TestCase {
    name: "RackControls"
    when: windowShown
    visible: true
    width: 400; height: 150
    Rack.RackSlider { id: slider; x: 20; y: 20; width: 300; from: 0; to: 100; stepSize: 1; remoteValue: 50 }
    SignalSpy { id: committed; target: slider; signalName: "valueCommitted" }
    function init() {
        slider.pendingCommit = false
        slider.remoteValue = 49
        slider.remoteValue = 50
        compare(slider.value, 50)
        committed.clear()
    }
    function test_dragCommitsChosenPosition() {
        mousePress(slider, 150, 14)
        verify(slider.pressed)
        mouseMove(slider, 250, 14)
        wait(120)
        compare(committed.count, 0)
        mouseRelease(slider, 250, 14)
        tryCompare(committed, "count", 1)
        verify(committed.signalArguments[0][0] > 75)
        verify(committed.signalArguments[0][0] < 95)
    }
    function test_keyboardCommits() {
        slider.forceActiveFocus()
        keyClick(Qt.Key_Right)
        tryCompare(committed, "count", 1)
        compare(committed.signalArguments[0][0], 51)
        verify(!slider.pressed)
    }
    function test_serverUpdateDoesNotSendCommand() {
        slider.remoteValue = 25
        wait(150)
        compare(slider.value, 25)
        compare(committed.count, 0)
    }
}
