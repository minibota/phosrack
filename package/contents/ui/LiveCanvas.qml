// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Window
Item {
    id: visual
    property var analyzer
    property string effect: "orbit"
    property real phase: 0
    property bool reducedMotion: false
    property bool ambient: false
    property bool animationActive: true
    // Bound the CPU-rasterized texture in physical pixels, including HiDPI.
    // The scene graph scales it smoothly; text/controls remain native resolution.
    readonly property real backingDpr: Math.max(1, Screen.devicePixelRatio)
    readonly property real backingScale: Math.min(1, 1280 / Math.max(1, width * backingDpr), 720 / Math.max(1, height * backingDpr))
    readonly property int backingWidth: Math.max(1, Math.floor(width * backingScale))
    readonly property int backingHeight: Math.max(1, Math.floor(height * backingScale))
    property bool audioFramePending: false
    property double lastFrameTime: 0
    function requestPaint(audioFrame) {
        if (!visible || effect === "notes" || !paintTimer) return
        audioFramePending = audioFramePending || !!audioFrame
        if (!paintTimer.running) {
            paintTimer.interval = Math.max(1, 33 - (Date.now() - lastFrameTime))
            paintTimer.start()
        }
    }
    function glowStroke(c, lineWidth, opacity, running) {
        // Layered translucent strokes keep the phosphor bloom without Canvas'
        // expensive full-image software blur for every ribbon/waveform.
        const color = c.strokeStyle
        if (running) {
            c.strokeStyle = "#72ec79"
            c.globalAlpha = opacity * 0.10; c.lineWidth = lineWidth + 7; c.stroke()
            c.globalAlpha = opacity * 0.22; c.lineWidth = lineWidth + 3; c.stroke()
        }
        c.strokeStyle = color; c.globalAlpha = opacity; c.lineWidth = lineWidth; c.stroke()
    }
    function paintMeter(c, width, height, levels, peaks) {
        const count = 28
        const meterWidth = Math.min(width * 0.85, height * 5.2)
        const step = meterWidth / count
        const gap = Math.max(1, step * 0.20)
        const rowHeight = Math.max(1, Math.min(height * 0.19, step * 1.45))
        const left = (width - meterWidth) / 2
        const top = (height - rowHeight * 2 - gap * 2) / 2 + Math.min(height * 0.045, 8)
        c.font = Math.max(9, Math.min(22, rowHeight * 0.8)) + "px monospace"
        c.textAlign = "right"
        c.textBaseline = "middle"
        for (let channel = 0; channel < 2; ++channel) {
            const y = top + channel * (rowHeight + gap * 2)
            const lit = Math.ceil((levels[channel] || 0) * count)
            const peak = Math.ceil((peaks[channel] || 0) * count) - 1
            c.fillStyle = "#b3d491"
            c.fillText(channel === 0 ? "L" : "R", left - 7, y + rowHeight / 2)
            for (let segment = 0; segment < count; ++segment) {
                const x = left + segment * step
                const bright = segment < lit || segment === peak
                c.fillStyle = bright ? (segment === peak || segment >= count - 4 ? "#f7d27a" : "#b1ee86") : "#173124"
                if (bright) {
                    c.globalAlpha = 0.24
                    c.fillRect(x - gap * 0.28, y - 1, step - gap * 0.44, rowHeight + 2)
                }
                c.globalAlpha = 1
                c.fillRect(x, y, step - gap, rowHeight)
            }
        }
    }
    onAmbientChanged: requestPaint(false)
    onEffectChanged: requestPaint(false)
    onWidthChanged: requestPaint(false)
    onHeightChanged: requestPaint(false)
    onBackingDprChanged: requestPaint(false)
    onVisibleChanged: {
        if (visible) requestPaint(false)
        else { paintTimer.stop(); audioFramePending = false }
    }
    Timer {
        id: paintTimer
        onTriggered: {
            if (!visual.visible) return
            const elapsed = visual.lastFrameTime ? Math.min(3, (Date.now() - visual.lastFrameTime) / 33) : 1
            if (!visual.ambient && visual.audioFramePending && visual.analyzer && visual.analyzer.level > 0.0005 && !visual.reducedMotion)
                visual.phase += (0.012 + Math.min(0.035, visual.analyzer.level * 0.1)) * elapsed
            visual.audioFramePending = false
            visual.lastFrameTime = Date.now()
            surface.requestPaint()
        }
    }
    Timer {
        interval: 50; repeat: true
        running: visual.visible && visual.ambient && visual.animationActive && !visual.reducedMotion
        onTriggered: { visual.phase += 0.015; visual.requestPaint(false) }
    }
    Connections {
        target: visual.ambient || visual.effect === "notes" ? null : visual.analyzer
        function onFrameReady() { visual.requestPaint(true) }
    }
    Canvas {
        id: surface
        objectName: "visualizerSurface"
        visible: visual.effect !== "notes"
        width: visual.backingWidth; height: visual.backingHeight
        smooth: true
        transform: Scale { xScale: visual.width / surface.width; yScale: visual.height / surface.height }
        onAvailableChanged: if (available) visual.requestPaint(false)
        onPaint: {
            const c = getContext("2d")
            const analyzer = visual.analyzer, effect = visual.effect, phase = visual.phase
            c.globalAlpha = 1; c.shadowBlur = 0
            c.fillStyle = "#070e12"; c.fillRect(0, 0, width, height)
            if (!visual.ambient && !analyzer) return
            // Ambient mode is a clock-driven pattern; never inspect captured audio
            // or Spotify playback position/tempo to animate it.
            const bands = visual.ambient ? Array.from({length: 48}, (_, i) => 0.2 + 0.12 * Math.sin(i * 0.22 + phase)) : analyzer.bands
            const wave = visual.ambient ? Array.from({length: 128}, (_, i) => 0.3 * Math.sin(i * 0.12 + phase)) : analyzer.waveform
            const energy = visual.ambient ? 0.3 : Math.min(1, Math.sqrt(analyzer.level) * 1.8)
            const glowing = visual.ambient || analyzer.running
            if (effect === "meter") {
                visual.paintMeter(c, width, height, visual.ambient ? [] : analyzer.meterLevels,
                                  visual.ambient ? [] : analyzer.meterPeaks)
            } else if (effect === "spectrum") {
                const step = (width - 24) / Math.max(1, bands.length)
                const g = c.createLinearGradient(0, height, 0, 0)
                g.addColorStop(0, "#72d49f"); g.addColorStop(0.65, "#d9e982"); g.addColorStop(1, "#f0ad75")
                c.fillStyle = g
                c.beginPath()
                for (let i = 0; i < bands.length; i++) {
                    const h = bands[i] * (height - 32)
                    c.rect(12 + i * step, height - 16 - h, Math.max(1, step - 2), h)
                }
                if (glowing) {
                    c.strokeStyle = "#72ec79"
                    c.globalAlpha = 0.12; c.lineWidth = 7; c.stroke()
                    c.globalAlpha = 0.22; c.lineWidth = 3; c.stroke()
                }
                c.globalAlpha = 1
                c.fill()
            } else if (effect === "scope") {
                c.strokeStyle = "#20343a"; c.lineWidth = 1
                c.beginPath(); c.moveTo(0, height / 2); c.lineTo(width, height / 2); c.stroke()
                for (let layer = 0; layer < 3; layer++) {
                    c.strokeStyle = ["#87f0bb", "#82c7ed", "#c69aef"][layer]
                    c.globalAlpha = layer === 0 ? 1 : 0.3; c.lineWidth = layer === 0 ? 2 : 4
                    c.beginPath()
                    for (let i = 0; i < wave.length; i++) {
                        const x = i * width / Math.max(1, wave.length - 1)
                        const y = height / 2 + wave[i] * height * (0.4 - layer * 0.05)
                        if (i === 0) c.moveTo(x, y); else c.lineTo(x, y)
                    }
                    visual.glowStroke(c, layer === 0 ? 2 : 4, layer === 0 ? 1 : 0.3, glowing)
                }
                c.globalAlpha = 1
            } else {
                // Original audio-reactive ribbons: no copied WMP/MilkDrop assets.
                const cx = width / 2, cy = height / 2, radius = Math.min(width, height) * 0.29
                for (let layer = 0; layer < 5; layer++) {
                    c.lineWidth = layer === 0 ? 2.4 : 1.2
                    c.strokeStyle = ["#b5f083", "#75eadb", "#72aeea", "#b797e9", "#eb9dc6"][layer]
                    c.globalAlpha = glowing ? 0.35 + energy * 0.55 : 0.17
                    c.beginPath()
                    for (let i = 0; i <= 160; i++) {
                        const a = i / 160 * Math.PI * 2
                        const sample = wave.length ? wave[i % wave.length] : 0
                        const bin = bands.length ? bands[Math.floor(i / 161 * bands.length)] : 0
                        const r = radius * (1 + layer * 0.09) + energy * (sample * 26 + Math.sin(a * (3 + layer) + phase * 2) * 20 + bin * 13)
                        const rotation = phase * (layer % 2 ? -1 : 1) + layer * 0.25
                        const x = cx + Math.cos(a + rotation) * r * (1.5 - layer * 0.08)
                        const y = cy + Math.sin(a) * r * 0.77
                        if (i === 0) c.moveTo(x, y); else c.lineTo(x, y)
                    }
                    c.closePath(); visual.glowStroke(c, layer === 0 ? 2.4 : 1.2, glowing ? 0.35 + energy * 0.55 : 0.17, glowing)
                }
                c.globalAlpha = 1
            }
        }
    }
    Loader {
        anchors.fill: parent
        active: visual.visible && visual.effect === "notes"
        sourceComponent: HarmonicDisplay {
            analyzer: visual.ambient ? null : visual.analyzer
            reducedMotion: visual.reducedMotion
        }
    }
}
