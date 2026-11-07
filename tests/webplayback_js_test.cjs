// SPDX-License-Identifier: GPL-3.0-or-later
// Executes the production SDK page with a fake browser/SDK; no network or credentials.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const html = fs.readFileSync(path.join(__dirname, '../webplayer/player.html'), 'utf8');
const source = [...html.matchAll(/<script>([\s\S]*?)<\/script>/g)].at(-1)[1];
const tick = () => new Promise(resolve => setImmediate(resolve));
function signal() {
    const listeners = [];
    return {connect: listener => listeners.push(listener), emit: (...args) => listeners.forEach(listener => listener(...args))};
}
async function fixture(drm = true) {
    const events = [], scripts = [], players = [], intervals = [];
    const bridge = {commandRequested: signal(), tokenUpdated: signal(),
        getToken: callback => callback('temporary-test-token'),
        report: (name, data) => events.push({name, data})};
    const sample = {paused: false, position: 1200, duration: 100000, repeat_mode: 2,
        shuffle: true, disallows: {skipping_next: true},
        track_window: {current_track: {uri: 'spotify:track:1234567890123456789012', name: 'Track',
            artists: [{name: 'Artist'}], album: {name: 'Album', images: [{url: 'https://i.scdn.co/example'}]}}}};
    class Player {
        constructor(options) {this.options = options; this.listeners = {}; this.volume = options.volume; this.calls = []; players.push(this);}
        addListener(event, callback) {this.listeners[event] = callback;}
        async connect() {this.calls.push('connect'); return true;}
        disconnect() {this.calls.push('disconnect');}
        async getVolume() {return this.volume;}
        async getCurrentState() {return sample;}
        async activateElement() {this.calls.push('activate');}
        async setVolume(value) {this.volume = value; this.calls.push('volume');}
        async pause() {this.calls.push('pause');}
        async resume() {this.calls.push('resume');}
        async seek() {this.calls.push('seek');}
    }
    const context = vm.createContext({
        window: {}, qt: {webChannelTransport: {}}, Spotify: {Player},
        QWebChannel: function(_, ready) {ready({objects: {playback: bridge}});},
        document: {createElement: kind => kind === 'audio' ? {canPlayType: () => 'probably'} : {}, head: {appendChild: script => scripts.push(script)}},
        isSecureContext: true,
        navigator: {requestMediaKeySystemAccess: async () => {
            if (!drm) throw new Error('unavailable');
            return {createMediaKeys: async () => ({})};
        }},
        setInterval: callback => intervals.push(callback)
    });
    vm.runInContext(source, context);
    await tick();
    return {events, scripts, players, intervals, bridge, context, sample};
}
(async () => {
    const missing = await fixture(false);
    assert.equal(missing.scripts.length, 0, 'No SDK startup when DRM is missing');
    assert.equal(missing.events.at(-1).name, 'drm_unavailable');
    const f = await fixture();
    assert.equal(f.scripts[0].src, 'https://sdk.scdn.co/spotify-player.js');
    f.context.window.onSpotifyWebPlaybackSDKReady(); await tick();
    const player = f.players[0];
    assert.equal(player.options.volume, 0.5);
    assert.deepEqual(player.calls, ['connect'], 'Registration must not start playback');
    player.listeners.ready({device_id: 'local-device'});
    assert.equal(f.events.at(-1).data.deviceId, 'local-device');
    player.listeners.player_state_changed(f.sample);
    const state = f.events.at(-1).data;
    assert.equal(state.title, 'Track'); assert.equal(state.active, true);
    assert.equal(state.canNext, false); assert.equal(state.canSeek, true);
    assert.equal(state.repeat, 'track'); assert.equal(state.volume, 50);
    f.bridge.commandRequested.emit('volume', 42); await tick();
    assert.equal(player.volume, 0.42);
    assert.equal(f.events.filter(e => e.name === 'state').at(-1).data.volume, 42);
    f.bridge.commandRequested.emit('resume'); await tick();
    assert.deepEqual(player.calls.slice(-2), ['activate', 'resume']);
    player.listeners.player_state_changed(null);
    assert.equal(f.events.at(-1).data.active, false);
    player.listeners.authentication_error({message: 'must not expose raw error'});
    assert.equal(f.events.at(-1).name, 'authentication_error');
    assert.deepEqual(Object.keys(f.events.at(-1).data), []);
    f.bridge.tokenUpdated.emit(); await tick();
    assert.equal(f.players.length, 2);
    assert(player.calls.includes('disconnect'));
    assert.deepEqual(f.players[1].calls, ['connect']);
    assert(!JSON.stringify(f.events).includes('temporary-test-token'));
    console.log('Web playback page: DRM gate, registration, native state, volume, commands and token recovery passed.');
})().catch(error => {console.error(error); process.exit(1);});
