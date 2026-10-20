'use strict';

// Focused web regression test; no Qt project build or third-party install needed.
// Run: node tests/track3dweb_test.js
// Optional real Chromium/WebGL smoke: node tests/track3dweb_test.js --browser
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const webRoot = path.resolve(__dirname, '../resources/web/track3d');
const sources = Object.fromEntries(['three.min.js', 'OrbitControls.js', 'track3d.js', 'track3d.css']
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
                return { clearRect() {}, fillText() {}, measureText: text => ({ width: text.length * 20 }) };
            },
            focus() {}
        };
    }
    const nodes = {};
    ['scene-root', 'tooltip', 'status-overlay', 'axis-range'].forEach(id => { nodes[id] = element('div'); });
    const window = {
        devicePixelRatio: 1.5, location: { href: 'qrc:///track3d/index.html' },
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
    h.bridge.snapshotChanged.emit([track('2:201')], {}); h.flush();
    assert.equal(h.state.renders.length, before);
    h.bridge.activeChanged.emit(true); h.flush();
    assert.ok(h.state.renders.length > before);
    assert.deepEqual(h.visibleKeys(), ['2:201']);
    assert.equal(h.state.ready.length, 1, 'Resuming must not duplicate the page-ready handshake');
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
        await page.addStyleTag({ content: sources['track3d.css'] });
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
        // Read the composited screenshot rather than an already-cleared WebGL
        // backbuffer (preserveDrawingBuffer is deliberately false in production).
        const { PNG } = require(path.join(runtimeModules, 'pngjs'));
        const png = PNG.sync.read(await page.screenshot());
        let coloredScenePixels = 0;
        for (let y = 0; y < png.height - 80; ++y) {
            for (let x = 0; x < png.width; ++x) {
                const index = (y * png.width + x) * 4;
                if (png.data[index + 1] > 45 && (png.data[index + 1] > png.data[index] * 1.4
                    || png.data[index + 2] > png.data[index] * 1.4)) ++coloredScenePixels;
            }
        }
        assert.ok(coloredScenePixels > 100, 'Expected visible colored axes/grid outside the DOM legend');
        first.coloredScenePixels = coloredScenePixels;
        await page.setViewportSize({ width: 720, height: 480 });
        await page.waitForFunction(() => document.getElementById('scene-root').clientWidth === 720
            && document.getElementById('scene-root').clientHeight === 480);
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
