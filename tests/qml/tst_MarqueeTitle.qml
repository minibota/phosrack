// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import "../../package/contents/ui" as Rack

TestCase {
    name: "MarqueeTitle"
    when: windowShown
    visible: true; width: 480; height: 320
    Component { id: component; Rack.MarqueeTitle {} }
    function test_scrollsOverflowAndRestartsForNewTrack() {
        const title = createTemporaryObject(component, this, {width: 100, text: "A title too long to fit in the music rack"})
        tryCompare(title, "scrolling", true)
        tryVerify(() => title.offset > 0, 1500)
        title.text = "A different song with another very long title"
        compare(title.offset, 0)
        title.reducedMotion = true
        verify(!title.scrolling)
        title.reducedMotion = false
        title.text = "Short"
        verify(!title.scrolling)
        compare(title.offset, 0)
    }
}
