// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    id: panel
    property var equalizer
    property var preferences
    spacing: 3
    function remember() {
        response.requestPaint()
    }
    RowLayout {
        Layout.fillWidth: true; spacing: 5
        RackButton { text: equalizer.enabled ? qsTr("ON") : qsTr("OFF"); lit: equalizer.enabled; implicitWidth: 43; implicitHeight: 22; description: qsTr("Enable local equalizer"); onClicked: equalizer.enabled = !equalizer.enabled }
        Text { text: qsTr("10 BAND · LOCAL DSP"); color: "#a9b5a2"; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace"; font.pixelSize: 9; Layout.fillWidth: true }
        ComboBox {
            id: presetPicker; Layout.preferredWidth: 126; implicitHeight: 23
            model: equalizer.presets; textRole: "name"; valueRole: "id"
            currentIndex: { for(let i=0;i<count;i++) if(model[i].id === equalizer.preset) return i; return -1 }
            displayText: currentIndex < 0 ? qsTr("Custom") : currentText
            Accessible.name: qsTr("Equalizer preset")
            onActivated: { equalizer.selectPreset(currentValue); panel.remember() }
        }
        RackButton { text: "↺"; description: qsTr("Reset equalizer"); implicitWidth: 25; implicitHeight: 22; onClicked: { equalizer.reset(); panel.remember() } }
    }
    Canvas {
        id: response; Layout.fillWidth: true; implicitHeight: 17
        Connections { target: panel.equalizer; function onGainsChanged() { response.requestPaint() } }
        onPaint: {
            const c=getContext("2d"); c.fillStyle="#090e0b";c.fillRect(0,0,width,height)
            c.strokeStyle="#30442a";c.lineWidth=1;c.beginPath();c.moveTo(0,height/2);c.lineTo(width,height/2);c.stroke()
            c.strokeStyle="#b9ff65";c.lineWidth=1.2;c.beginPath()
            const gains=panel.equalizer.gains
            for(let i=0;i<gains.length;i++){const x=8+i*(width-16)/9,y=height/2-gains[i]*(height-3)/24;if(i===0)c.moveTo(x,y);else c.lineTo(x,y)}
            c.stroke()
        }
    }
    RowLayout {
        Layout.fillWidth: true; spacing: 3
        ColumnLayout {
            spacing: 0; Layout.preferredWidth: 33
            RackFader {
                Layout.alignment: Qt.AlignHCenter; implicitHeight: 75; remoteValue: equalizer.preamp
                Accessible.name: qsTr("Preamp")
                onGainMoved: function(gain) { equalizer.preamp = gain; panel.remember() }
            }
            Text { Layout.alignment: Qt.AlignHCenter; text: qsTr("PRE"); color: "#c7d1c2"; font.pixelSize: 8; font.family: Qt.platform.os === "windows" ? "Consolas" : "monospace" }
        }
        Rectangle { Layout.preferredWidth: 1; Layout.preferredHeight: 80; color: "#6e7185" }
        Column {
            Layout.preferredWidth: 24; Layout.preferredHeight: 76
            Text { text:"+12";color:"#b3ae77";font.pixelSize:8;font.family:"monospace" }
            Item { width: 1; height: 22 }
            Text { text:"0 dB";color:"#dbd389";font.pixelSize:8;font.family:"monospace" }
            Item { width: 1; height: 22 }
            Text { text:"−12";color:"#b3ae77";font.pixelSize:8;font.family:"monospace" }
        }
        Repeater {
            model: ["31","63","125","250","500","1K","2K","4K","8K","16K"]
            delegate: ColumnLayout {
                required property int index
                required property string modelData
                Layout.fillWidth: true; Layout.preferredWidth: Math.max(20, (panel.width - 94) / 10); spacing: 0
                RackFader {
                    Layout.alignment: Qt.AlignHCenter; implicitHeight: 75; remoteValue: panel.equalizer.gains[index] || 0
                    Accessible.name: modelData + " Hz"
                    onGainMoved: function(gain) { panel.equalizer.setBand(index, gain); panel.remember() }
                }
                Text { Layout.alignment: Qt.AlignHCenter; text: modelData; color:"#c7d1c2";font.pixelSize:8;font.family:"monospace" }
            }
        }
    }
    Text {
        Layout.fillWidth: true; text: equalizer.error || (equalizer.enabled ? equalizer.status : qsTr("Local audio · bypassed"))
        color: equalizer.error ? "#edac7b" : "#9aa58f"; font.pixelSize: 9
        wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight
    }
}
