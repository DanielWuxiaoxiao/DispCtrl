'use strict';

// Focused web regression test; no Qt project build or third-party install needed.
// Run: node tests/track3dweb_test.js
// Optional real Chromium/WebGL smoke: node tests/track3dweb_test.js --browser
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const webRoot = path.resolve(__dirname, '../resources/web/track3d');
const sources = Object.fromEntries(['three.min.js', 'OrbitControls.js', 'track3d.js', 'track3d.css', 'track3d_classic.css']
    .map(name => [name, fs.readFileSync(path.join(webRoot, name), 'utf8')]));

function signal() {
    const listeners = [];
    return {
        connect(callback) { listeners.push(callback); },
        emit(...args) { listeners.slice().forEach(callback => callback(...args)); }
    };
}

function harness(options = {}) {
    const state = {
        width: options.width === undefined ? 960 : options.width,
        height: options.height === undefined ? 640 : options.height,
        ready: [], errors: [], viewports: [], sizes: [], renders: [],
        uncaught: [], frames: [], jobs: [], observers: [], renderer: null,
        rendererConstructions: 0, channelConstructions: 0, now: 0
    };
    const documentListeners = new Map();
    const windowListeners = new Map();
    const bridge = {};
    ['snapshotChanged', 'trackDeltaChanged', 'sceneStateChanged', 'clearRequested',
        'resetViewRequested', 'activeChanged'].forEach(name => { bridge[name] = signal(); });
    bridge.pageReady = () => state.ready.push({ renderCount: state.renders.length });
    bridge.reportError = message => state.errors.push(String(message));
    bridge.reportViewport = (...args) => state.viewports.push(args);

    const document = {
        addEventListener(type, callback) { addListener(documentListeners, type, callback); },
        removeEventListener(type, callback) { removeListener(documentListeners, type, callback); },
        createElement: tag => element(tag),
        createElementNS: (_namespace, tag) => element(tag),
        getElementById: id => nodes[id]
    };
    function addListener(listeners, type, callback) {
        if (!listeners.has(type)) listeners.set(type, []);
        listeners.get(type).push(callback);
    }
    function removeListener(listeners, type, callback) {
        if (listeners.has(type)) listeners.set(type, listeners.get(type).filter(item => item !== callback));
    }
    function element(tag) {
        const listeners = new Map();
        const classes = new Set();
        return {
            tagName: tag.toUpperCase(), style: {}, children: [], ownerDocument: document,
            width: 300, height: 150, textContent: '', innerHTML: '',
            get clientWidth() { return state.width; },
            get clientHeight() { return state.height; },
            offsetWidth: 170, offsetHeight: 210,
            classList: {
                add: value => classes.add(value), remove: value => classes.delete(value),
                contains: value => classes.has(value)
            },
            appendChild(child) { this.children.push(child); child.parentNode = this; return child; },
            removeChild(child) { this.children = this.children.filter(item => item !== child); },
            addEventListener(type, callback) { addListener(listeners, type, callback); },
            removeEventListener(type, callback) { removeListener(listeners, type, callback); },
            dispatchEvent(event) { (listeners.get(event.type) || []).slice().forEach(fn => fn(event)); },
            setAttribute(name, value) { this[name] = value; },
            getBoundingClientRect() {
                return { left: 0, top: 0, right: state.width, bottom: state.height,
                    width: state.width, height: state.height };
            },
            getContext() {
                return { clearRect() {}, fillText() {}, beginPath() {}, moveTo() {},
                    lineTo() {}, closePath() {}, fill() {} };
            },
            focus() {}
        };
    }
    const nodes = {};
    document.documentElement = element('html');
    ['scene-root', 'tooltip', 'status-overlay', 'axis-range'].forEach(id => { nodes[id] = element('div'); });
    const window = {
        devicePixelRatio: 1.5, location: { href: 'qrc:///track3d/index.html?theme=' + (options.theme || 'graphite') },
        get innerWidth() { return state.width; }, get innerHeight() { return state.height; },
        addEventListener(type, callback) { addListener(windowListeners, type, callback); },
        removeEventListener(type, callback) { removeListener(windowListeners, type, callback); },
        requestAnimationFrame: callback => { state.frames.push(callback); return state.frames.length; },
        cancelAnimationFrame() {}, focus() {},
        setTimeout() { return 1; }, clearTimeout() {}
    };
    const context = vm.createContext({
        window, document, qt: { webChannelTransport: {} },
        navigator: { userAgent: 'Track3D regression mock' },
        console: { log() {}, warn() {}, error() {} },
        requestAnimationFrame: window.requestAnimationFrame,
        cancelAnimationFrame: window.cancelAnimationFrame,
        setTimeout: window.setTimeout, clearTimeout: window.clearTimeout,
        performance: { now: () => state.now },
        ResizeObserver: class {
            constructor(callback) { this.callback = callback; state.observers.push(this); }
            observe() {} unobserve() {} disconnect() {}
        },
        QWebChannel: function (_transport, callback) {
            ++state.channelConstructions;
            state.jobs.push(() => callback({ objects: { track3dBridge: bridge } }));
        }
    });
    window.ResizeObserver = context.ResizeObserver;
    vm.runInContext(sources['three.min.js'], context, { filename: 'three.min.js' });
    vm.runInContext(sources['OrbitControls.js'], context, { filename: 'OrbitControls.js' });
    context.THREE.WebGLRenderer = class {
        constructor() {
            ++state.rendererConstructions;
            if (options.rendererFailure) throw new Error('renderer-initialization-test-failure');
            this.domElement = element('canvas');
            this.pixelRatio = 1;
            this.logicalWidth = 300;
            this.logicalHeight = 150;
            this.info = { render: { calls: 0 }, memory: { geometries: 0, textures: 0 } };
            this.capabilities = { isWebGL2: false };
            state.renderer = this;
        }
        setClearColor() {}
        setPixelRatio(value) { this.pixelRatio = value; }
        getPixelRatio() { return this.pixelRatio; }
        setSize(width, height) {
            state.sizes.push([width, height]);
            this.logicalWidth = width;
            this.logicalHeight = height;
            this.domElement.width = Math.floor(width * this.pixelRatio);
            this.domElement.height = Math.floor(height * this.pixelRatio);
        }
        getSize(target) { return target.set(this.logicalWidth, this.logicalHeight); }
        getDrawingBufferSize(target) { return target.set(this.domElement.width, this.domElement.height); }
        getContext() {
            return { isContextLost: () => false,
                drawingBufferWidth: this.domElement.width, drawingBufferHeight: this.domElement.height };
        }
        render(scene, camera) {
            if (options.renderFailure) throw new Error('first-render-test-failure');
            state.renders.push({ scene, camera, width: this.logicalWidth, height: this.logicalHeight });
            ++this.info.render.calls;
        }
        dispose() {}
    };
    if (options.missingThree) context.THREE = undefined;
    else if (options.orbitFailure) context.THREE.OrbitControls = function () {
        throw new Error('orbit-initialization-test-failure');
    };
    else if (options.missingOrbit) context.THREE.OrbitControls = undefined;

    function guarded(callback) {
        try { callback(); } catch (error) {
            state.uncaught.push(error);
            const event = { message: error.message, filename: 'track3d.js', lineno: 1, error,
                preventDefault() {} };
            (windowListeners.get('error') || []).slice().forEach(listener => listener(event));
        }
    }
    return {
        state, bridge, nodes,
        boot() { guarded(() => vm.runInContext(sources['track3d.js'], context, { filename: 'track3d.js' })); },
        flush() {
            for (let iteration = 0; iteration < 100; ++iteration) {
                if (!state.jobs.length && !state.frames.length) return;
                state.jobs.splice(0).forEach(job => guarded(job));
                const frames = state.frames.splice(0);
                state.now += 16;
                frames.forEach(frame => guarded(() => frame(state.now)));
            }
            throw new Error('Render scheduling did not settle after 100 event-loop turns');
        },
        resize(width, height) {
            state.width = width; state.height = height;
            state.observers.forEach(observer => guarded(() => observer.callback([])));
            (windowListeners.get('resize') || []).slice().forEach(callback => guarded(callback));
        },
        visibleKeys() {
            const keys = [];
            if (state.renders.length) state.renders[state.renders.length - 1].scene.traverse(node => {
                if (node.isPoints) (node.userData.trackItems || []).forEach(track => keys.push(String(track.key)));
            });
            return keys.sort();
        },
        visibleSegments() {
            const segments = [];
            state.renders[state.renders.length - 1].scene.traverse(node => {
                if (!node.isLineSegments || !node.visible) return;
                const { geometry, material } = node;
                const vertices = geometry.getAttribute('position');
                const colors = geometry.getAttribute('color');
                const end = Math.min(vertices.count, geometry.drawRange.start + geometry.drawRange.count);
                for (let i = geometry.drawRange.start; i < end; i += 2) {
                    const rgb = colors ? (colors.array[i * 3] << 16) | (colors.array[i * 3 + 1] << 8)
                        | colors.array[i * 3 + 2] : material.color.getHex();
                    segments.push([rgb, Math.round(vertices.getX(i) * 5000),
                        Math.round(vertices.getX(i + 1) * 5000)]);
                }
            });
            return segments.sort((a, b) => a[1] - b[1] || a[2] - b[2] || a[0] - b[0]);
        }
    };
}

const tests = [];
function test(name, callback) { tests.push({ name, callback }); }
function boot(options) { const h = harness(options); h.boot(); h.flush(); return h; }
function track(key, overrides = {}) {
    return Object.assign({ key, type: 2, batch: Number(key.split(':')[1]), rangeM: 1000,
        eastM: 1000, northM: 0, heightM: 100, azimuthDeg: 90, elevationDeg: 0,
        speedMps: -15, snrDb: 20, amp: 10, targetRecResult: 1, color: '#ffff00' }, overrides);
}

const red = 0xea8c93;
const cyan = 0x75c5d1;
function sample(east, { height = 100, range = 1000, rgb = red } = {}) {
    return [east, 0, height, range, rgb];
}

test('CSS viewport bounds are compatible with Qt 5.14 Chromium 77', () => {
    const css = sources['track3d.css'].replace(/\/\*[\s\S]*?\*\//g, '');
    for (const selector of ['scene-root', 'status-overlay']) {
        const rule = css.match(new RegExp('#' + selector + '\\s*\\{([^}]+)\\}'));
        assert.ok(rule, 'Missing CSS rule #' + selector);
        for (const edge of ['top', 'right', 'bottom', 'left']) {
            assert.match(rule[1], new RegExp('(?:^|;)\\s*' + edge + '\\s*:\\s*0(?:px)?\\s*(?:;|$)'),
                '#' + selector + ' must define ' + edge + ': 0 explicitly');
        }
    }
});

test('pageReady follows a successful nonzero first frame', () => {
    const h = harness(); h.boot();
    assert.equal(h.state.ready.length, 0, 'Must not announce readiness synchronously before rendering');
    h.flush();
    assert.equal(h.state.uncaught.length, 0);
    assert.equal(h.state.ready.length, 1);
    assert.ok(h.state.ready[0].renderCount > 0);
    assert.ok(h.state.renders.every(frame => frame.width > 0 && frame.height > 0));
    assert.ok(h.state.viewports.length > 0, 'Viewport telemetry was not sent to C++');
});

test('zero initial viewport stays unready and recovers on a later resize', () => {
    const h = boot({ width: 0, height: 0 });
    assert.equal(h.state.ready.length, 0);
    assert.equal(h.state.renders.length, 0);
    assert.equal(h.state.sizes.length, 0, 'Do not turn a zero-sized hidden view into a fake 1px renderer');
    h.resize(800, 500); h.flush();
    assert.equal(h.state.ready.length, 1);
    assert.ok(h.state.renders.length > 0);
    assert.ok(h.state.sizes.some(size => size[0] === 800 && size[1] === 500));
});

test('the default narrow view marks north at 0 degrees, with east visibly to its left', () => {
    const h = boot({ width: 320, height: 520 });
    const { scene, camera } = h.state.renders[h.state.renders.length - 1];
    camera.updateMatrixWorld();
    const northLine = scene.getObjectByName('north-azimuth-zero');
    const vertices = northLine.geometry.getAttribute('position');
    assert.deepEqual(Array.from(vertices.array), [0, 0, 0, 0, 0, 1]);
    const north = scene.getObjectByName('0° 正北').position.clone().project(camera);
    const east = scene.getObjectByName('E').position.clone().project(camera);
    const west = scene.getObjectByName('W').position.clone().project(camera);
    assert.ok(Math.abs(north.x) < 1e-6 && north.y > -1 && north.y < 1);
    assert.ok(east.x > -1 && east.x < north.x);
    assert.ok(west.x < 1 && west.x > north.x);
});

test('latest markers retain a shared triangle texture and apply the 3D height window', () => {
    const h = boot();
    h.bridge.snapshotChanged.emit([track('2:101'), track('2:102', { heightM: 600, color: '#ff0000' })], {});
    h.flush();
    assert.deepEqual(h.visibleKeys(), ['2:101']);
    const scene = h.state.renders[h.state.renders.length - 1].scene;
    const materials = () => {
        const result = [];
        scene.traverse(node => { if (node.isPoints) result.push(node.material); });
        return result;
    };
    const [first] = materials();
    assert.ok(first.map.isCanvasTexture);
    assert.ok(first.size > 0.05);
    let disposals = 0;
    first.map.addEventListener('dispose', () => { ++disposals; });
    h.bridge.sceneStateChanged.emit({ minHeightM: -50, maxHeightM: 800 });
    h.flush();
    assert.deepEqual(h.visibleKeys(), ['2:101', '2:102']);
    assert.ok(materials().every(material => material.map === first.map));
    assert.equal(disposals, 0);
    assert.match(h.nodes['axis-range'].textContent, /-50–800 m/);

    h.bridge.sceneStateChanged.emit({ minHeightM: 99.75, maxHeightM: 100.25 });
    h.flush();
    assert.deepEqual(h.visibleKeys(), ['2:101']);
    assert.match(h.nodes['axis-range'].textContent, /99.75–100.25 m/);
    scene.traverse(node => {
        if (node.isPoints) assert.equal(node.geometry.getAttribute('position').getY(0), 0.5);
    });
});

test('snapshot, overwrite, removal and clear reach the real scene graph', () => {
    const h = boot();
    h.bridge.snapshotChanged.emit([track('2:101'), track('2:102')], {}); h.flush();
    assert.deepEqual(h.visibleKeys(), ['2:101', '2:102']);
    h.bridge.trackDeltaChanged.emit([track('2:101', { eastM: 500 }), track('2:103')], ['2:102']); h.flush();
    assert.deepEqual(h.visibleKeys(), ['2:101', '2:103']);
    h.bridge.clearRequested.emit(); h.flush();
    assert.deepEqual(h.visibleKeys(), []);
});

test('hidden updates do not render; reactivation renders the latest state', () => {
    const h = boot();
    h.bridge.activeChanged.emit(false); h.flush();
    const before = h.state.renders.length;
    h.bridge.snapshotChanged.emit([track('2:201', { path: sample(100).concat(sample(200)) })], {}); h.flush();
    assert.equal(h.state.renders.length, before);
    h.bridge.activeChanged.emit(true); h.flush();
    assert.ok(h.state.renders.length > before);
    assert.deepEqual(h.visibleKeys(), ['2:201']);
    assert.deepEqual(h.visibleSegments(), [[red, 100, 200]]);
    assert.equal(h.state.ready.length, 1, 'Resuming must not duplicate the page-ready handshake');
});

test('trail deltas connect only consecutive samples of the same typed batch and respect FIFO', () => {
    const h = boot();
    h.bridge.snapshotChanged.emit([
        track('2:101', { path: sample(100) }),
        track('3:101', { type: 3, path: sample(900, { rgb: cyan }) })
    ], { maxTrackPoints: 3 });
    h.flush();
    assert.deepEqual(h.visibleSegments(), []);
    h.bridge.trackDeltaChanged.emit([
        track('2:101', { path: sample(200).concat(sample(300)) }),
        track('3:101', { type: 3, path: sample(950, { rgb: cyan }) })
    ], []);
    h.flush();
    assert.deepEqual(h.visibleSegments(), [[red, 100, 200], [red, 200, 300], [cyan, 900, 950]]);
    h.bridge.trackDeltaChanged.emit([track('2:101', { path: sample(400, { rgb: cyan }) })], []);
    h.flush();
    assert.deepEqual(h.visibleSegments(), [[red, 200, 300], [cyan, 300, 400], [cyan, 900, 950]]);

    h.bridge.trackDeltaChanged.emit([track('2:101', { path: sample(700) })], ['2:101']);
    h.flush();
    assert.deepEqual(h.visibleSegments(), [[cyan, 900, 950]], 'A reused batch must not join its old trail');
    h.bridge.clearRequested.emit(); h.flush();
    assert.deepEqual(h.visibleSegments(), []);
});

test('trail visibility honors recognition, range and height without bridging filtered samples', () => {
    const h = boot();
    const path = sample(100).concat(sample(200, { height: -10 }), sample(300), sample(400),
        sample(500, { range: 6000 }));
    h.bridge.snapshotChanged.emit([track('2:101', { path, rangeM: 6000, targetRecResult: 0 })], {});
    h.flush();
    assert.deepEqual(h.visibleKeys(), []);
    assert.deepEqual(h.visibleSegments(), [[red, 300, 400]]);
    h.bridge.sceneStateChanged.emit({ onlyRecognizedDroneTracksVisible: true }); h.flush();
    assert.deepEqual(h.visibleSegments(), []);
    h.bridge.sceneStateChanged.emit({ onlyRecognizedDroneTracksVisible: false, maxHeightM: 50 }); h.flush();
    assert.deepEqual(h.visibleSegments(), []);
});

test('a full FIFO updates only one line slot and reuses the shared WebGL geometry', () => {
    const h = boot();
    h.bridge.snapshotChanged.emit([
        track('2:101', { path: sample(100).concat(sample(200), sample(300)) }),
        track('3:102', { type: 3, path: sample(800).concat(sample(900)) })
    ], { maxTrackPoints: 3 });
    h.flush();
    const scene = h.state.renders[h.state.renders.length - 1].scene;
    const lines = scene.children.find(node => node.isLineSegments);
    const { geometry } = lines;
    const positions = geometry.getAttribute('position');
    const version = positions.version;

    h.bridge.trackDeltaChanged.emit([track('2:101', { path: sample(400, { rgb: cyan }) })], []);
    h.flush();
    assert.equal(lines.geometry, geometry);
    assert.equal(positions.version, version + 1);
    assert.equal(positions.updateRange.count, 6);
    assert.deepEqual(h.visibleSegments(), [[red, 200, 300], [cyan, 300, 400], [red, 800, 900]]);

    h.bridge.sceneStateChanged.emit({ pointSizeRatio: 2 });
    h.flush();
    assert.equal(positions.version, version + 1, 'Changing marker size must leave line buffers alone');
    h.bridge.trackDeltaChanged.emit([], ['2:101']);
    h.flush();
    assert.deepEqual(h.visibleSegments(), [[red, 800, 900]], 'Deletion must keep the moved line live');
    h.bridge.trackDeltaChanged.emit([track('3:102', { type: 3, path: sample(950) })], []);
    h.flush();
    assert.deepEqual(h.visibleSegments(), [[red, 800, 900], [red, 900, 950]]);
});

test('a clipped eviction and a recognition change keep the surviving segments correct', () => {
    const h = boot();
    h.bridge.snapshotChanged.emit([
        track('2:101', { path: sample(100).concat(sample(200), sample(300)) })
    ], { maxTrackPoints: 3, onlyRecognizedDroneTracksVisible: true });
    h.flush();
    h.bridge.trackDeltaChanged.emit([track('2:101', {
        path: sample(400, { height: -1 }), heightM: -1
    })], []);
    h.flush();
    assert.deepEqual(h.visibleSegments(), [[red, 200, 300]]);
    h.bridge.trackDeltaChanged.emit([track('2:101', {
        path: sample(500).concat(sample(600)), targetRecResult: 0
    })], []);
    h.flush();
    assert.deepEqual(h.visibleSegments(), []);
    h.bridge.trackDeltaChanged.emit([track('2:101', { path: sample(700) })], []);
    h.flush();
    assert.deepEqual(h.visibleSegments(), [[red, 500, 600], [red, 600, 700]]);

    h.bridge.sceneStateChanged.emit({ maxTrackPoints: 4 });
    h.bridge.trackDeltaChanged.emit([track('2:101', { path: sample(750) })], []);
    h.flush();
    assert.deepEqual(h.visibleSegments(), [[red, 500, 600], [red, 600, 700], [red, 700, 750]]);

    h.bridge.sceneStateChanged.emit({ maxTrackPoints: 1 });
    h.bridge.trackDeltaChanged.emit([track('2:101', { path: sample(800) })], []);
    h.flush();
    assert.deepEqual(h.visibleSegments(), []);
    assert.deepEqual(h.visibleKeys(), ['2:101']);
});

test('scene palette switches between graphite and classic independently of track coordinates', () => {
    const h = boot();
    h.bridge.snapshotChanged.emit([track('2:101', { path: sample(100).concat(sample(200)) })], { theme: 'classic' });
    h.flush();
    assert.equal(h.state.renders[h.state.renders.length - 1].scene.background.getHex(), 0x0a1010);
    assert.deepEqual(h.visibleSegments(), [[red, 100, 200]]);
    h.bridge.sceneStateChanged.emit({ theme: 'graphite' }); h.flush();
    assert.equal(h.state.renders[h.state.renders.length - 1].scene.background.getHex(), 0x12171c);
    assert.deepEqual(h.visibleSegments(), [[red, 100, 200]]);
});

test('classic URL selects the palette before the first frame', () => {
    const h = boot({ theme: 'classic' });
    assert.equal(h.state.renders[0].scene.background.getHex(), 0x0a1010);
});

for (const [name, option, expected] of [
    ['missing Three.js', 'missingThree', /three|undefined|dependency|依赖/i],
    ['missing OrbitControls', 'missingOrbit', /orbit|constructor|dependency|依赖/i],
    ['WebGL constructor failure', 'rendererFailure', /renderer-initialization-test-failure/],
    ['OrbitControls constructor failure', 'orbitFailure', /orbit-initialization-test-failure/],
    ['first frame failure', 'renderFailure', /first-render-test-failure/]
]) test(name + ' is reported through the early WebChannel and never marks ready', () => {
    const h = boot({ [option]: true });
    assert.equal(h.state.ready.length, 0);
    assert.equal(h.state.channelConstructions, 1, 'Diagnostics channel must survive rendering initialization failure');
    assert.ok(h.state.errors.some(message => expected.test(message)),
        'Expected the original failure in C++ diagnostics; received: ' + JSON.stringify(h.state.errors));
});

async function browserSmoke() {
    const runtimeModules = 'C:/Users/win/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules';
    let playwright;
    try { playwright = require(path.join(runtimeModules, 'playwright')); }
    catch (_error) { throw new Error('Bundled Playwright is unavailable; no dependencies were downloaded'); }
    const candidates = [process.env.TRACK3D_TEST_CHROMIUM, playwright.chromium.executablePath(),
        'C:/Program Files/Google/Chrome/Application/chrome.exe',
        'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'].filter(Boolean);
    const executablePath = candidates.find(candidate => fs.existsSync(candidate));
    if (!executablePath) throw new Error('No existing Chromium executable found; no browser was downloaded');
    const browser = await playwright.chromium.launch({ executablePath, headless: true,
        args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--disable-background-timer-throttling'] });
    try {
        const page = await browser.newPage({ viewport: { width: 960, height: 640 } });
        const errors = [];
        page.on('pageerror', error => errors.push(error.message));
        await page.setContent('<!doctype html><html><head></head><body>'
            + '<div id="scene-root"></div><div id="axis-legend"><span id="axis-range"></span></div>'
            + '<div id="tooltip"></div><div id="status-overlay">loading</div></body></html>');
        const themeStyle = await page.addStyleTag({ content: sources['track3d.css'] });
        await page.evaluate(() => {
            window.testBridge = { ready: 0, errors: [], viewports: [] };
            const bridge = {};
            ['snapshotChanged', 'trackDeltaChanged', 'sceneStateChanged', 'clearRequested',
                'resetViewRequested', 'activeChanged'].forEach(name => {
                const callbacks = [];
                bridge[name] = { connect: callback => callbacks.push(callback),
                    emit: (...args) => callbacks.forEach(callback => callback(...args)) };
            });
            bridge.pageReady = () => { ++window.testBridge.ready; };
            bridge.reportError = message => window.testBridge.errors.push(String(message));
            bridge.reportViewport = (...args) => window.testBridge.viewports.push(args);
            window.testBridge.signals = bridge;
            window.qt = { webChannelTransport: {} };
            window.QWebChannel = function (_transport, callback) {
                setTimeout(() => callback({ objects: { track3dBridge: bridge } }), 0);
            };
        });
        await page.addScriptTag({ content: sources['three.min.js'] });
        await page.addScriptTag({ content: sources['OrbitControls.js'] });
        await page.evaluate(() => {
            const OriginalRenderer = THREE.WebGLRenderer;
            THREE.WebGLRenderer = function (options) {
                const renderer = new OriginalRenderer(options);
                const render = renderer.render.bind(renderer);
                renderer.render = (scene, camera) => {
                    render(scene, camera);
                    window.testBridge.camera = { position: camera.position.toArray(),
                        quaternion: camera.quaternion.toArray() };
                };
                return renderer;
            };
        });
        await page.addScriptTag({ content: sources['track3d.js'] });
        await page.waitForFunction(() => window.testBridge.ready === 1, null, { timeout: 10000 });
        const first = await page.evaluate(() => {
            const root = document.getElementById('scene-root');
            const canvas = root.querySelector('canvas');
            const context = canvas.getContext('webgl2') || canvas.getContext('webgl');
            return { width: root.clientWidth, height: root.clientHeight,
                bufferWidth: context.drawingBufferWidth, bufferHeight: context.drawingBufferHeight,
                errors: window.testBridge.errors };
        });
        assert.equal(first.width, 960); assert.equal(first.height, 640);
        assert.ok(first.bufferWidth > 0 && first.bufferHeight > 0);
        assert.deepEqual(first.errors, []); assert.deepEqual(errors, []);
        const cameraState = () => page.evaluate(() => window.testBridge.camera);
        const initialCamera = await cameraState();
        const distance = (a, b) => Math.hypot(...a.map((value, i) => value - b[i]));
        for (const button of ['right', 'left']) {
            await page.mouse.move(350, 250);
            if (button === 'left') await page.keyboard.down('Shift');
            await page.mouse.down({ button });
            await page.mouse.move(440, 285, { steps: 4 });
            await page.mouse.up({ button });
            if (button === 'left') await page.keyboard.up('Shift');
            await page.evaluate(() => new Promise(requestAnimationFrame));
            const moved = await cameraState();
            assert.ok(distance(moved.position, initialCamera.position) > 0.05, 'Pan must move the camera');
            assert.ok(distance(moved.quaternion, initialCamera.quaternion) < 1e-6, 'Pan must preserve orientation');
            await page.evaluate(() => new Promise(resolve => {
                window.testBridge.signals.resetViewRequested.emit();
                requestAnimationFrame(() => requestAnimationFrame(resolve));
            }));
            assert.ok(distance((await cameraState()).position, initialCamera.position) < 1e-6);
        }
        // Read the composited screenshot rather than an already-cleared WebGL
        // backbuffer (preserveDrawingBuffer is deliberately false in production).
        const { PNG } = require(path.join(runtimeModules, 'pngjs'));
        async function countScenePixels(matches) {
            const png = PNG.sync.read(await page.screenshot());
            let count = 0;
            for (let y = 0; y < png.height - 80; ++y) {
                for (let x = 0; x < png.width; ++x) {
                    const i = (y * png.width + x) * 4;
                    if (matches(png.data[i], png.data[i + 1], png.data[i + 2])) ++count;
                }
            }
            return count;
        }
        first.coloredScenePixels = await countScenePixels((r, g, b) => g > r + 15 && b > r + 20);
        assert.ok(first.coloredScenePixels > 100, 'Expected visible graphite/cyan axes outside the DOM legend');

        const magenta = (r, g, b) => r > 160 && b > 160 && g < 100;
        assert.equal(await countScenePixels(magenta), 0);
        await page.evaluate(data => new Promise(resolve => {
            window.testBridge.signals.snapshotChanged.emit([data], { pointSizeRatio: 3 });
            requestAnimationFrame(() => requestAnimationFrame(resolve));
        }), track('2:402', { eastM: 0, northM: 0, heightM: 175, rangeM: 0, color: '#ff20ff' }));
        const head = PNG.sync.read(await page.screenshot());
        const headRows = [];
        for (let y = 280; y < 360; ++y) {
            let pixels = 0;
            for (let x = 440; x < 520; ++x) {
                const i = (y * head.width + x) * 4;
                if (magenta(head.data[i], head.data[i + 1], head.data[i + 2])) ++pixels;
            }
            if (pixels > 0) headRows.push(pixels);
        }
        assert.ok(headRows.length >= 7 && Math.max(...headRows) >= 2 * headRows[0],
            'Expected a triangle that widens below its top vertex');
        await page.mouse.move(480, 320);
        await page.waitForFunction(() => document.getElementById('tooltip').innerText.includes('402'));
        await page.evaluate(data => new Promise(resolve => {
            window.testBridge.signals.snapshotChanged.emit([data], { maxTrackPoints: 4, pointSizeRatio: 1 });
            requestAnimationFrame(() => requestAnimationFrame(resolve));
        }), track('2:401', {
            // The current point is out of range, so only the two earlier segments can draw magenta pixels.
            color: '#ff20ff', rangeM: 6000,
            path: sample(-2900, { height: 180, rgb: 0xff20ff })
                .concat(sample(0, { height: 350, rgb: 0xff20ff }),
                    sample(2900, { height: 180, rgb: 0xff20ff }),
                    sample(6000, { range: 6000, rgb: 0xff20ff }))
        }));
        first.trailPixels = await countScenePixels(magenta);
        assert.ok(first.trailPixels > 30, 'Expected visible trail segments in the WebGL canvas');
        await page.evaluate(data => new Promise(resolve => {
            window.testBridge.signals.trackDeltaChanged.emit([data], []);
            requestAnimationFrame(() => requestAnimationFrame(resolve));
        }), track('2:401', {
            color: '#ff6a00', rangeM: 6000,
            path: sample(-2500, { height: 300, rgb: 0xff6a00 })
                .concat(sample(2500, { height: 370, rgb: 0xff6a00 }))
        }));
        const orange = (r, g, b) => r > 200 && g > 90 && g < 210 && b < 60;
        assert.equal(await countScenePixels(magenta), 0, 'Evicted trail pixels must disappear');
        first.updatedTrailPixels = await countScenePixels(orange);
        assert.ok(first.updatedTrailPixels > 30, 'Expected the changed GPU buffer slot to draw the new segment');
        assert.deepEqual(errors, []);
        await page.setViewportSize({ width: 720, height: 480 });
        await page.waitForFunction(() => document.getElementById('scene-root').clientWidth === 720
            && document.getElementById('scene-root').clientHeight === 480);
        await page.evaluate(() => new Promise(resolve => {
            window.testBridge.signals.sceneStateChanged.emit({ theme: 'classic' });
            requestAnimationFrame(() => requestAnimationFrame(resolve));
        }));
        // Production reloads the standalone page with the original x576 stylesheet.
        await themeStyle.evaluate(node => node.remove());
        await page.addStyleTag({ content: sources['track3d_classic.css'] });
        const classicBackground = await page.evaluate(() => getComputedStyle(document.body).backgroundColor);
        assert.equal(classicBackground, 'rgb(10, 16, 16)');
        first.classicGreenPixels = await countScenePixels((r, g, b) => g > 120 && g > r * 2 && g > b * 1.2);
        assert.ok(first.classicGreenPixels > 100, 'Expected actual classic green axes in WebGL');
        console.log('PASS real Chromium/WebGL smoke ' + JSON.stringify(first));
    } finally { await browser.close(); }
}

(async () => {
    let failed = 0;
    for (const item of tests) {
        try { item.callback(); console.log('PASS ' + item.name); }
        catch (error) { ++failed; console.error('FAIL ' + item.name + '\n' + error.stack); }
    }
    if (process.argv.includes('--browser')) {
        try { await browserSmoke(); }
        catch (error) { ++failed; console.error('FAIL real Chromium/WebGL smoke\n' + error.stack); }
    }
    console.log(tests.length + ' VM/static cases; ' + failed + ' failure(s)');
    process.exitCode = failed ? 1 : 0;
})();
