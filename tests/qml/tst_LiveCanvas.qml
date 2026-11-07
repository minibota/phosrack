// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import "../../package/contents/ui" as Rack

TestCase {
    id: suite
    name: "BoundedVisualizer"
    when: windowShown
    visible: true; width: 480; height: 320
    QtObject {
        id: audio
        property var bands: [0.2, 0.4, 0.6, 0.3]
        property var waveform: [0.1, -0.2, 0.3, -0.1]
        property real level: 0.1
        property bool running: true
        signal frameReady()
    }
    Component { id: component; Rack.LiveCanvas { analyzer: audio } }
    function test_largeViewportHasBoundedPhysicalBacking() {
        const view = createTemporaryObject(component, suite, {width: 3840, height: 2160})
        verify(view)
        verify(view.backingWidth * view.backingDpr <= 1280)
        verify(view.backingHeight * view.backingDpr <= 720)
        view.width = 7680; view.height = 4320
        verify(view.backingWidth * view.backingDpr <= 1280)
        verify(view.backingHeight * view.backingDpr <= 720)
        const surface = findChild(view, "visualizerSurface")
        verify(surface)
        compare(surface.width, view.backingWidth)
        compare(surface.height, view.backingHeight)
    }
    function test_burstsCoalesceAndIdleDoesNotInventAnimation() {
        const view = createTemporaryObject(component, suite, {width: 450, height: 108})
        verify(view)
        wait(80)
        const initial = view.phase
        for (let i = 0; i < 100; ++i) audio.frameReady()
        tryVerify(() => view.phase > initial)
        verify(view.phase - initial < 0.2)
        const settled = view.phase
        wait(120)
        compare(view.phase, settled)
        view.visible = false
        for (let i = 0; i < 20; ++i) audio.frameReady()
        wait(80)
        compare(view.phase, settled)
    }
    function test_reduceMotionKeepsPhaseStable() {
        const view = createTemporaryObject(component, suite, {width: 450, height: 108, reducedMotion: true})
        verify(view)
        audio.frameReady();wait(80)
        compare(view.phase, 0)
    }
}
