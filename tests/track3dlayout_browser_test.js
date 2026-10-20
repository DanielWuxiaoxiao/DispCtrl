'use strict';

// Real browser/WebGL regression, independent of Qt, DispCtrl, and UDP.
// Run: node tests/track3dlayout_browser_test.js
// This simulates unsupported CSS inset; it is NOT a Qt WebEngine 77 test.
const assert = require('assert').strict;
const fs = require('fs');
const os = require('os');
const path = require('path');

const repository = path.resolve(__dirname, '..');
const resources = path.join(repository, 'resources', 'web', 'track3d');
const artifactDirectory = path.join(repository, 'build', 'track3d-browser-test');
const runtimeModules = process.env.TRACK3D_NODE_MODULES || path.join(os.homedir(),
    '.cache', 'codex-runtimes', 'codex-primary-runtime', 'dependencies', 'node', 'node_modules');
let playwright;
try {
    playwright = require('playwright');
} catch (_) {
    playwright = require(path.join(runtimeModules, 'playwright'));
}

const browserCandidates = [
    process.env.TRACK3D_BROWSER_PATH,
    'C:/Program Files/Google/Chrome/Application/chrome.exe',
    'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe',
    'C:/Program Files/Microsoft/Edge/Application/msedge.exe'
].filter(Boolean);
const executablePath = browserCandidates.find(candidate => fs.existsSync(candidate));
assert(executablePath, 'Chrome/Edge not found; set TRACK3D_BROWSER_PATH. No browser is downloaded.');

function withoutInset(css) {
    return css.replace(/\binset\s*:[^;}]+;?/gi, '');
}

function legacyLayoutCss(css) {
    // A stable negative fixture, independent of Git history: simulate the old
    // inset-only layout while running exactly the current production script.
    const insetOnly = css.replace(/(#scene-root|#status-overlay)\s*\{([^}]*)\}/g,
        (_, selector, declarations) => selector + ' {'
            + declarations.replace(/\b(?:top|right|bottom|left)\s*:[^;}]+;?/gi, '')
            + '\n    inset: 0;\n}');
    return withoutInset(insetOnly);
}

const currentCss = fs.readFileSync(path.join(resources, 'track3d.css'), 'utf8');
const currentScript = fs.readFileSync(path.join(resources, 'track3d.js'), 'utf8');
const legacyCss = legacyLayoutCss(currentCss);
const bridgeMock = `
(function () {
    function signal() {
        var callbacks = [];
        return {
            connect: function (callback) { callbacks.push(callback); },
            emit: function () {
                var args = arguments;
                callbacks.forEach(function (callback) { callback.apply(null, args); });
            }
        };
    }
    var state = window.__track3dTest = {
        readyCount: 0, errors: [], renderCount: 0, viewports: []
    };
    state.bridge = {
        snapshotChanged: signal(), trackDeltaChanged: signal(), sceneStateChanged: signal(),
        clearRequested: signal(), resetViewRequested: signal(), activeChanged: signal(),
        pageReady: function () {
            state.readyCount += 1;
            state.readyRenderCount = state.renderCount;
        },
        reportViewport: function (width, height, canvasWidth, canvasHeight, details) {
            state.viewports.push({ width: width, height: height,
                canvasWidth: canvasWidth, canvasHeight: canvasHeight, details: details });
        },
        reportError: function (message) { state.errors.push(String(message)); }
    };
    window.qt = { webChannelTransport: {} };
    window.QWebChannel = function (transport, callback) {
        callback({ objects: { track3dBridge: state.bridge } });
    };
}());
`;

// Wrap the real renderer, not Three.js geometry or WebGL. Pixel reads occur in
// the actual render call so preserveDrawingBuffer is not required or changed.
const rendererInstrumentation = `
(function () {
    var OriginalRenderer = THREE.WebGLRenderer;
    function InstrumentedRenderer(options) {
        var renderer = new OriginalRenderer(options);
        var render = renderer.render;
        renderer.render = function (scene, camera) {
            render.call(renderer, scene, camera);
            var state = window.__track3dTest;
            state.renderCount += 1;
            var points = [], lineObjects = 0, sprites = 0;
            scene.traverse(function (object) {
                if (object.isLine) lineObjects += 1;
                if (object.isSprite) sprites += 1;
                if (object.isPoints) {
                    var position = object.geometry.getAttribute('position');
                    points.push({ count: position.count, positions: Array.from(position.array) });
                }
            });
            var gl = renderer.getContext();
            var width = gl.drawingBufferWidth, height = gl.drawingBufferHeight;
            var pixels = new Uint8Array(width * height * 4);
            gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
            var nonBackgroundPixels = 0, magentaPixels = 0;
            for (var i = 0; i < pixels.length; i += 4) {
                var r = pixels[i], g = pixels[i + 1], b = pixels[i + 2];
                if (r > 30 || g > 40 || b > 40) nonBackgroundPixels += 1;
                if (r > 160 && b > 160 && g < 80) magentaPixels += 1;
            }
            state.lastFrame = {
                width: width, height: height, lineObjects: lineObjects, sprites: sprites,
                points: points, nonBackgroundPixels: nonBackgroundPixels,
                magentaPixels: magentaPixels, drawCalls: renderer.info.render.calls
            };
        };
        return renderer;
    }
    InstrumentedRenderer.prototype = OriginalRenderer.prototype;
    THREE.WebGLRenderer = InstrumentedRenderer;
}());
`;

function pageHtml() {
    return fs.readFileSync(path.join(resources, 'index.html'), 'utf8')
        .replace('qrc:///qtwebchannel/qwebchannel.js', 'mock-qwebchannel.js')
        .replace('<script src="track3d.js"></script>',
            '<script src="instrument-renderer.js"></script>\n    <script src="track3d.js"></script>');
}

function localResponse(url) {
    const pathname = new URL(url).pathname;
    const legacy = pathname.startsWith('/legacy/');
    const name = path.posix.basename(pathname);
    let data;
    let type = 'text/javascript; charset=utf-8';
    if (name === 'index.html') {
        data = pageHtml();
        type = 'text/html; charset=utf-8';
    } else if (name === 'mock-qwebchannel.js') {
        data = bridgeMock;
    } else if (name === 'instrument-renderer.js') {
        data = rendererInstrumentation;
    } else if (name === 'track3d.css') {
        data = legacy ? legacyCss : withoutInset(currentCss);
        type = 'text/css; charset=utf-8';
    } else if (name === 'track3d.js') {
        data = currentScript;
    } else if (name === 'three.min.js' || name === 'OrbitControls.js') {
        data = fs.readFileSync(path.join(resources, name));
    } else if (name === 'favicon.ico') {
        return { status: 204, body: '' };
    } else {
        return { status: 404, body: '' };
    }
    return { status: 200, contentType: type, body: data,
        headers: { 'Cache-Control': 'no-store' } };
}

const snapshotTracks = [{
    key: '2:101', type: 2, batch: 101,
    eastM: 0, northM: 0, heightM: 250,
    rangeM: 1000, azimuthDeg: 0, elevationDeg: 0,
    snrDb: 20, speedMps: 10, amp: 30, targetRecResult: 1, color: '#ff00ff'
}, {
    key: '3:102', type: 3, batch: 102,
    eastM: 2000, northM: 1000, heightM: 100,
    rangeM: 2250, azimuthDeg: 63.4, elevationDeg: 0,
    snrDb: 15, speedMps: -5, amp: 25, targetRecResult: 0, color: '#ffff00'
}];
const snapshotState = {
    minRangeM: 0, maxRangeM: 5000, minHeightM: 0, maxHeightM: 500,
    trackVisible: true, tbdTrackVisible: true, cooperativeTrackVisible: true,
    onlyRecognizedDroneTracksVisible: false, pointSizeRatio: 3
};

async function metrics(page) {
    return page.evaluate(() => {
        const root = document.getElementById('scene-root');
        const canvas = root.querySelector('canvas');
        const rect = root.getBoundingClientRect();
        const canvasRect = canvas ? canvas.getBoundingClientRect() : null;
        return {
            viewport: { width: innerWidth, height: innerHeight },
            root: { width: rect.width, height: rect.height },
            canvas: canvasRect ? { width: canvasRect.width, height: canvasRect.height } : null,
            statusDisplay: getComputedStyle(document.getElementById('status-overlay')).display,
            readyCount: window.__track3dTest.readyCount,
            readyRenderCount: window.__track3dTest.readyRenderCount,
            viewports: window.__track3dTest.viewports,
            errors: window.__track3dTest.errors,
            renderCount: window.__track3dTest.renderCount,
            frame: window.__track3dTest.lastFrame
        };
    });
}

async function sendSnapshot(page) {
    await page.evaluate(({ tracks, state }) => {
        window.__track3dTest.bridge.snapshotChanged.emit(tracks, state);
    }, { tracks: snapshotTracks, state: snapshotState });
}

async function waitForFullCanvas(page) {
    await page.waitForFunction(() => {
        const root = document.getElementById('scene-root');
        const canvas = root.querySelector('canvas');
        const frame = window.__track3dTest.lastFrame;
        return canvas && frame && root.clientWidth === innerWidth && root.clientHeight === innerHeight
            && canvas.width === innerWidth && canvas.height === innerHeight
            && frame.width === innerWidth && frame.height === innerHeight;
    }, null, { timeout: 10000 });
}

function assertRendered(result) {
    assert.equal(result.errors.length, 0, result.errors.join('\n'));
    assert.equal(result.statusDisplay, 'none', 'Initialization overlay must be removed.');
    assert(result.readyRenderCount > 0, 'pageReady must follow a real first render.');
    assert.equal(result.root.width, result.viewport.width);
    assert.equal(result.root.height, result.viewport.height);
    assert.equal(result.canvas.width, result.viewport.width);
    assert.equal(result.canvas.height, result.viewport.height);
    assert(result.frame.lineObjects >= 30, 'Real axes/grid line objects must exist.');
    assert(result.frame.sprites >= 10, 'Physical axis/range labels must exist.');
    assert(result.frame.nonBackgroundPixels > 1000, 'Axes/grid must produce real framebuffer pixels.');
    assert.equal(result.frame.points.reduce((sum, bucket) => sum + bucket.count, 0), 2);
    assert(result.frame.magentaPixels > 3, 'Known magenta snapshot target must produce framebuffer pixels.');
    const originTarget = result.frame.points.find(bucket => bucket.positions[0] === 0);
    assert(originTarget && Math.abs(originTarget.positions[1] - 0.5) < 1e-6);
}

async function run() {
    fs.mkdirSync(artifactDirectory, { recursive: true });
    // Fulfill all page requests from local memory: no HTTP listener, DNS lookup,
    // external request, or network permission is needed for these offline assets.
    const origin = 'http://track3d.test';
    let browser;
    const report = {
        browser: executablePath,
        limitation: 'Modern headless browser with CSS inset declarations removed; not actual Qt WebEngine 77.',
        network: 'All URLs fulfilled from local files in memory; no sockets, DNS, DispCtrl, or UDP.'
    };
    try {
        browser = await playwright.chromium.launch({
            executablePath, headless: true,
            args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist']
        });
        report.browserVersion = browser.version();
        const context = await browser.newContext({ viewport: { width: 1024, height: 768 }, deviceScaleFactor: 1 });
        await context.route('**/*', route => {
            if (route.request().url().startsWith(origin + '/')) {
                return route.fulfill(localResponse(route.request().url()));
            }
            return route.abort('blockedbyclient');
        });
        const legacy = await context.newPage();
        await legacy.goto(origin + '/legacy/index.html');
        await legacy.waitForFunction(() => window.__track3dTest && window.__track3dTest.renderCount > 0);
        report.legacy = await metrics(legacy);
        assert(report.legacy.root.width < report.legacy.viewport.width * 0.9
            || report.legacy.root.height < report.legacy.viewport.height * 0.9,
            'Inset-only negative fixture should reproduce an undersized root when inset is unsupported.');
        await legacy.screenshot({ path: path.join(artifactDirectory, 'legacy-no-inset.png') });
        await legacy.close();

        const page = await context.newPage();
        const pageErrors = [];
        page.on('pageerror', error => pageErrors.push(String(error)));
        await page.goto(origin + '/current/index.html');
        await page.waitForFunction(() => window.__track3dTest && window.__track3dTest.readyCount > 0);
        await sendSnapshot(page);
        await waitForFullCanvas(page);
        await page.waitForFunction(() => window.__track3dTest.lastFrame.magentaPixels > 3);
        report.initial = await metrics(page);
        assertRendered(report.initial);
        await page.screenshot({ path: path.join(artifactDirectory, 'current-snapshot.png') });

        await page.setViewportSize({ width: 900, height: 540 });
        await waitForFullCanvas(page);
        report.resized = await metrics(page);
        assertRendered(report.resized);

        await page.evaluate(() => {
            window.__track3dTest.bridge.activeChanged.emit(false);
            document.getElementById('scene-root').style.display = 'none';
        });
        await page.waitForTimeout(100);
        const hiddenRenders = (await metrics(page)).renderCount;
        await page.setViewportSize({ width: 800, height: 480 });
        await page.waitForTimeout(100);
        assert.equal((await metrics(page)).renderCount, hiddenRenders, 'Inactive hidden view must not render.');
        await page.evaluate(() => {
            document.getElementById('scene-root').style.display = '';
            window.__track3dTest.bridge.activeChanged.emit(true);
        });
        await sendSnapshot(page);
        await waitForFullCanvas(page);
        await page.waitForFunction(() => window.__track3dTest.lastFrame.magentaPixels > 3);
        report.restored = await metrics(page);
        assertRendered(report.restored);
        assert.equal(report.restored.readyCount, 1, 'Visibility recovery must not reconnect/reload the page.');
        assert.equal(pageErrors.length, 0, pageErrors.join('\n'));
        await page.screenshot({ path: path.join(artifactDirectory, 'current-restored.png') });
        report.passed = true;
        fs.writeFileSync(path.join(artifactDirectory, 'result.json'), JSON.stringify(report, null, 2));
        console.log(JSON.stringify(report, null, 2));
    } finally {
        if (browser) await browser.close();
    }
}

run().catch(error => {
    console.error(error.stack || error);
    process.exitCode = 1;
});
