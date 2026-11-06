// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
Canvas {
    id: visual
    property var analyzer
    property string effect: "orbit"
    property real phase: 0
    property bool reducedMotion: false
    onEffectChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    Connections {
        target: visual.analyzer
        function onFrameReady() {
            if (visual.visible) {
                if (visual.analyzer.level > 0.0005 && !visual.reducedMotion) visual.phase += 0.012 + Math.min(0.035, visual.analyzer.level * 0.1)
                visual.requestPaint()
            }
        }
    }
    onPaint: {
        const c = getContext("2d")
        c.fillStyle = "#070e12"; c.fillRect(0, 0, width, height)
        if (!analyzer) return
        const bands = analyzer.bands
        const wave = analyzer.waveform
        const energy = Math.min(1, Math.sqrt(analyzer.level) * 1.8)
        c.shadowColor = "#72ec79"
        c.shadowBlur = analyzer.running ? 5 : 0
        if (effect === "spectrum") {
            const step = (width - 24) / Math.max(1, bands.length)
            for (let i = 0; i < bands.length; i++) {
                const h = bands[i] * (height - 32)
                const g = c.createLinearGradient(0, height, 0, 0)
                g.addColorStop(0, "#72d49f"); g.addColorStop(0.65, "#d9e982"); g.addColorStop(1, "#f0ad75")
                c.fillStyle = g; c.fillRect(12 + i * step, height - 16 - h, Math.max(1, step - 2), h)
            }
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
                c.stroke()
            }
            c.globalAlpha = 1
        } else {
            // Original audio-reactive ribbons: no copied WMP/MilkDrop assets.
            const cx = width / 2, cy = height / 2, radius = Math.min(width, height) * 0.29
            for (let layer = 0; layer < 5; layer++) {
                c.lineWidth = layer === 0 ? 2.4 : 1.2
                c.strokeStyle = ["#b5f083", "#75eadb", "#72aeea", "#b797e9", "#eb9dc6"][layer]
                c.globalAlpha = analyzer.running ? 0.35 + energy * 0.55 : 0.17
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
                c.closePath(); c.stroke()
            }
            c.globalAlpha = 1
        }
    }
}
