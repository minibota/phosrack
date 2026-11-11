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
        property var pitchClasses: []
        property var meterLevels: [0, 0]
        property var meterPeaks: [0, 0]
        signal frameReady()
    }
    Component { id: component; Rack.LiveCanvas { analyzer: audio } }
    function init() {
        audio.bands = [0.2, 0.4, 0.6, 0.3]
        audio.waveform = [0.1, -0.2, 0.3, -0.1]
        audio.level = 0.1; audio.running = true
        audio.pitchClasses = []; audio.meterLevels = [0, 0]; audio.meterPeaks = [0, 0]
    }
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
    function test_meterReactsToIndependentChannels() {
        const view = createTemporaryObject(component, suite, {width: 450, height: 142, effect: "meter"})
        wait(100)
        const quiet = grabImage(view)
        audio.meterLevels = [0.75, 0]
        audio.meterPeaks = [0.9, 0]
        audio.frameReady()
        tryVerify(() => !grabImage(view).equals(quiet))
        const left = grabImage(view)
        audio.meterLevels = [0, 0.75]
        audio.meterPeaks = [0, 0.9]
        audio.frameReady()
        tryVerify(() => !grabImage(view).equals(left))
    }
    function test_notesUseFullWidthWithoutChordBadge() {
        const view = createTemporaryObject(component, suite, {width: 450, height: 142, effect: "notes"})
        tryVerify(() => !!findChild(view, "noteBar11"))
        verify(findChild(view, "noteBar0").mapToItem(view, 0, 0).x < 18)
        verify(!findChild(view, "estimatedChord"))
    }
    function test_ambientDoesNotReactToCapturedAudio() {
        const view = createTemporaryObject(component, suite, {width: 450, height: 108, ambient: true, animationActive: false})
        verify(view)
        wait(80)
        const before = grabImage(view)
        const phase = view.phase
        audio.bands = [1, 0, 1, 1]; audio.waveform = [1, -1, 1, -1]
        audio.level = 1; audio.running = false
        for (let i = 0; i < 100; ++i) audio.frameReady()
        view.requestPaint(false)
        wait(80)
        compare(view.phase, phase)
        const after = grabImage(view)
        verify(before.equals(after), "Ambient rendering must not depend on system audio samples")
    }
    function test_ambientRunsWithoutAnAudioSourceAndStopsWhenHidden() {
        const view = createTemporaryObject(component, suite, {width: 450, height: 108, ambient: true, analyzer: null})
        verify(view)
        tryVerify(() => view.phase > 0)
        view.visible = false
        const hiddenPhase = view.phase
        wait(150)
        compare(view.phase, hiddenPhase)
        view.visible = true
        tryVerify(() => view.phase > hiddenPhase)
        view.animationActive = false
        const inactivePhase = view.phase
        wait(150)
        compare(view.phase, inactivePhase)
    }
    function test_ambientHonorsReducedMotion() {
        const view = createTemporaryObject(component, suite, {width: 450, height: 108, ambient: true, reducedMotion: true})
        verify(view)
        wait(150)
        compare(view.phase, 0)
        for (let i = 0; i < 100; ++i) audio.frameReady()
        wait(80)
        compare(view.phase, 0)
    }
}
