(function () {
    'use strict';

    var root = document.getElementById('scene-root');
    var tooltip = document.getElementById('tooltip');
    var statusOverlay = document.getElementById('status-overlay');
    var axisRange = document.getElementById('axis-range');

    var bridge = null;
    var pendingErrors = [];
    var active = true;
    var renderPending = false;
    var sceneInitialized = false;
    var firstFrameRendered = false;
    var readyReported = false;
    var startupFailed = false;
    var viewportWidth = 0;
    var viewportHeight = 0;
    var lastViewportSignature = '';
    var viewportDetailsReported = false;
    var tracks = new Map();
    var pointObjects = [];
    var gridGroup = null;
    var pointsGroup = null;
    var trackHeadTexture;
    var trailLines;
    var trailPosition;
    var trailColor;
    var segmentEnds = [];
    var dirtySegmentStart = Infinity;
    var dirtySegmentEnd = 0;
    var trailRebuildPending = false;
    var headsDirty = false;
    var axesSignature = '';
    var defaultFov = 42;
    var pathStride = 5; // east, north, height, slant range, endpoint RGB
    var palettes = {
        graphite: { background: 0x12171c, major: 0x45616b, minor: 0x303b44,
            axis: 0x78cbd8, label: '#a6b2bd', x: 0xea8c93, y: 0x78d4a5, z: 0x78cbd8 },
        classic: { background: 0x0a1010, major: 0x00503c, minor: 0x003226,
            axis: 0x00ff88, label: '#66ffcc', x: 0xff4f4f, y: 0x00ff88, z: 0x00aaff }
    };
    var theme = String(window.location.href).indexOf('theme=classic') >= 0 ? 'classic' : 'graphite';
    var palette = palettes[theme];

    var viewState = {
        theme: theme,
        minRangeM: 0,
        maxRangeM: 5000,
        minHeightM: 0,
        maxHeightM: 500,
        trackVisible: true,
        tbdTrackVisible: true,
        cooperativeTrackVisible: true,
        onlyRecognizedDroneTracksVisible: false,
        pointSizeRatio: 1,
        maxTrackPoints: 200
    };
    var horizontalScaleM = viewState.maxRangeM;
    var verticalBaseM = viewState.minHeightM;
    var verticalScaleM = viewState.maxHeightM - verticalBaseM;

    var scene;
    var camera;
    var renderer;
    var controls;
    var raycaster;
    var pointer;

    function reportError(message) {
        var text = String(message || 'unknown JavaScript error');
        if (bridge) {
            bridge.reportError(text);
        } else if (pendingErrors.length < 10) {
            pendingErrors.push(text);
        }
        // The native page console also captures failures before WebChannel exists.
        console.error('[Track3D] ' + text);
    }

    function showError(message) {
        if (!statusOverlay) return;
        statusOverlay.textContent = message;
        statusOverlay.classList.add('error');
        statusOverlay.style.display = 'flex';
    }

    function failInitialization(error) {
        if (startupFailed) return;
        startupFailed = true;
        active = false;
        showError('3D渲染初始化或执行失败');
        reportError(error && error.stack ? error.stack : String(error));
    }

    function reportViewport() {
        if (!bridge || !bridge.reportViewport || !renderer) return;
        var canvas = renderer.domElement;
        var signature = [root.clientWidth, root.clientHeight, canvas.width, canvas.height].join('|');
        if (signature === lastViewportSignature) return;
        lastViewportSignature = signature;
        var details = '';
        if (!viewportDetailsReported) {
            details = 'userAgent=' + navigator.userAgent;
            try {
                var gl = renderer.getContext();
                details += '; WebGL=' + gl.getParameter(gl.VERSION)
                    + '; renderer=' + gl.getParameter(gl.RENDERER);
            } catch (ignored) {
                details += '; WebGL details unavailable';
            }
            viewportDetailsReported = true;
        }
        bridge.reportViewport(root.clientWidth, root.clientHeight, canvas.width, canvas.height, details);
    }

    function notifyPageReady() {
        if (!bridge || readyReported || !firstFrameRendered || startupFailed) return;
        readyReported = true;
        reportViewport();
        bridge.pageReady();
        statusOverlay.style.display = 'none';
    }

    function requestRender() {
        if (!active || renderPending || !renderer || !sceneInitialized || startupFailed
            || viewportWidth <= 0 || viewportHeight <= 0) {
            return;
        }
        renderPending = true;
        window.requestAnimationFrame(function () {
            renderPending = false;
            if (active && renderer && !startupFailed && viewportWidth > 0 && viewportHeight > 0) {
                try {
                    if (trailRebuildPending) rebuildTrailLines();
                    uploadTrailChanges();
                    if (headsDirty) rebuildTrackHeads();
                    renderer.render(scene, camera);
                    firstFrameRendered = true;
                    notifyPageReady();
                } catch (error) {
                    failInitialization(error);
                }
            }
        });
    }

    function resetCamera() {
        // From the south, +Z (north) is ahead and +X (east) projects to the left.
        camera.position.set(0, 1.35, -2.4);
        controls.target.set(0, 0.35, 0);
        controls.update();
        requestRender();
    }

    function resizeRenderer() {
        if (!renderer || !camera) {
            return;
        }
        var width = root.clientWidth;
        var height = root.clientHeight;
        viewportWidth = width;
        viewportHeight = height;
        if (width <= 0 || height <= 0) {
            // Hidden tabs can have no layout yet. Never claim a 1px canvas is ready.
            reportViewport();
            return;
        }
        camera.aspect = width / height;
        // Keep east and west in frame in the narrow dock. OrbitControls uses FOV for panning.
        var fitScale = Math.min(1, camera.aspect / 1.15);
        camera.fov = THREE.MathUtils.radToDeg(2 * Math.atan(
            Math.tan(THREE.MathUtils.degToRad(defaultFov) / 2) / fitScale));
        camera.updateProjectionMatrix();
        renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
        renderer.setSize(width, height, false);
        reportViewport();
        requestRender();
    }

    function disposeMaterial(material) {
        if (!material) {
            return;
        }
        if (material.map && material.map !== trackHeadTexture) {
            material.map.dispose();
        }
        material.dispose();
    }

    function disposeGroup(group) {
        if (!group) {
            return;
        }
        while (group.children.length) {
            var child = group.children.pop();
            child.traverse(function (node) {
                if (node.geometry) {
                    node.geometry.dispose();
                }
                if (Array.isArray(node.material)) {
                    node.material.forEach(disposeMaterial);
                } else if (node.material) {
                    disposeMaterial(node.material);
                }
            });
        }
        scene.remove(group);
    }

    function createLine(points, color, opacity, loop) {
        var geometry = new THREE.BufferGeometry().setFromPoints(points);
        var material = new THREE.LineBasicMaterial({
            color: color,
            transparent: opacity < 1,
            opacity: opacity
        });
        return loop ? new THREE.LineLoop(geometry, material) : new THREE.Line(geometry, material);
    }

    function createTextSprite(text, color, height) {
        var canvas = document.createElement('canvas');
        canvas.width = 512;
        canvas.height = 128;
        var context = canvas.getContext('2d');
        context.clearRect(0, 0, canvas.width, canvas.height);
        context.font = '44px Microsoft YaHei, Segoe UI, sans-serif';
        context.textAlign = 'center';
        context.textBaseline = 'middle';
        context.fillStyle = color;
        context.fillText(text, canvas.width / 2, canvas.height / 2);

        var texture = new THREE.CanvasTexture(canvas);
        texture.minFilter = THREE.LinearFilter;
        var material = new THREE.SpriteMaterial({
            map: texture,
            transparent: true,
            depthWrite: false,
            depthTest: false
        });
        var sprite = new THREE.Sprite(material);
        sprite.name = text;
        var width = Math.max(height * 1.3, height * Math.min(5.0, 0.62 * text.length));
        sprite.scale.set(width, height, 1);
        sprite.renderOrder = 10;
        return sprite;
    }

    function createTrackHeadTexture() {
        var canvas = document.createElement('canvas');
        canvas.width = canvas.height = 64;
        var context = canvas.getContext('2d');
        context.fillStyle = '#ffffff';
        context.beginPath();
        context.moveTo(32, 5);
        context.lineTo(7, 50);
        context.lineTo(57, 50);
        context.closePath();
        context.fill();
        return new THREE.CanvasTexture(canvas);
    }

    function formatAxisHeight(value) {
        return String(Number(value.toFixed(2)));
    }

    function circlePoints(radius, y) {
        var points = [];
        var segments = 96;
        for (var i = 0; i < segments; ++i) {
            var angle = i * Math.PI * 2 / segments;
            points.push(new THREE.Vector3(radius * Math.sin(angle), y, radius * Math.cos(angle)));
        }
        return points;
    }

    function rebuildAxes() {
        disposeGroup(gridGroup);
        gridGroup = new THREE.Group();
        scene.add(gridGroup);

        var majorColor = palette.major;
        var minorColor = palette.minor;
        var axisColor = palette.axis;
        var labelColor = palette.label;
        var maxRangeKm = Math.max(0.001, Number(viewState.maxRangeM) / 1000);
        var minHeight = Number(viewState.minHeightM);
        var maxHeight = Number(viewState.maxHeightM);

        for (var ring = 1; ring <= 5; ++ring) {
            var radius = ring / 5;
            gridGroup.add(createLine(circlePoints(radius, 0), ring === 5 ? axisColor : majorColor,
                                     ring === 5 ? 0.9 : 0.72, true));

            var rangeLabel = createTextSprite((maxRangeKm * radius).toFixed(1) + ' km', labelColor, 0.065);
            rangeLabel.position.set(radius, 0.025, 0.035);
            gridGroup.add(rangeLabel);
        }

        for (var spoke = 0; spoke < 12; ++spoke) {
            if (spoke % 3 === 0) continue; // The cardinal axes are drawn separately.
            var spokeAngle = spoke * Math.PI * 2 / 12;
            gridGroup.add(createLine([
                new THREE.Vector3(0, 0, 0),
                new THREE.Vector3(Math.sin(spokeAngle), 0, Math.cos(spokeAngle))
            ], minorColor, 0.5, false));
        }

        gridGroup.add(createLine([
            new THREE.Vector3(-1, 0, 0), new THREE.Vector3(1, 0, 0)
        ], palette.x, 0.85, false));
        gridGroup.add(createLine([
            new THREE.Vector3(0, 0, -1), new THREE.Vector3(0, 0, 0)
        ], palette.z, 0.85, false));
        var northLine = createLine([
            new THREE.Vector3(0, 0, 0), new THREE.Vector3(0, 0, 1)
        ], axisColor, 1, false);
        northLine.name = 'north-azimuth-zero';
        gridGroup.add(northLine);

        gridGroup.add(createLine(circlePoints(1, 1), majorColor, 0.42, true));
        var verticalLocations = [
            new THREE.Vector3(1, 0, 0), new THREE.Vector3(-1, 0, 0),
            new THREE.Vector3(0, 0, 1), new THREE.Vector3(0, 0, -1)
        ];
        verticalLocations.forEach(function (base) {
            gridGroup.add(createLine([
                base.clone(), new THREE.Vector3(base.x, 1, base.z)
            ], majorColor, 0.48, false));
        });

        var heightAxisX = -0.72;
        var heightAxisZ = -0.72;
        gridGroup.add(createLine([
            new THREE.Vector3(heightAxisX, 0, heightAxisZ),
            new THREE.Vector3(heightAxisX, 1, heightAxisZ)
        ], palette.y, 0.9, false));
        for (var tick = 0; tick <= 5; ++tick) {
            var y = tick / 5;
            gridGroup.add(createLine([
                new THREE.Vector3(heightAxisX - 0.025, y, heightAxisZ),
                new THREE.Vector3(heightAxisX + 0.025, y, heightAxisZ)
            ], palette.y, 0.8, false));
            var heightValue = minHeight + (maxHeight - minHeight) * y;
            var heightLabel = createTextSprite(formatAxisHeight(heightValue) + ' m', labelColor, 0.065);
            heightLabel.position.set(heightAxisX - 0.09, y, heightAxisZ);
            gridGroup.add(heightLabel);
        }

        [
            ['0° 正北', 0, 0.05, 1.12, 0.13], ['S', 0, 0.02, -1.1, 0.085],
            ['E', 1.1, 0.02, 0, 0.085], ['W', -1.1, 0.02, 0, 0.085]
        ].forEach(function (labelData) {
            var sprite = createTextSprite(labelData[0], '#' + axisColor.toString(16).padStart(6, '0'),
                labelData[4]);
            sprite.position.set(labelData[1], labelData[2], labelData[3]);
            gridGroup.add(sprite);
        });

        axisRange.textContent = '量程 ' + maxRangeKm.toFixed(1) + ' km | 高度 '
            + formatAxisHeight(minHeight) + '–' + formatAxisHeight(maxHeight) + ' m';
        requestRender();
    }

    function trackTypeVisible(track) {
        var type = Number(track.type);
        if (type === 2 && !viewState.trackVisible) {
            return false;
        }
        if (type === 3 && !viewState.tbdTrackVisible) {
            return false;
        }
        if (type === 4 && !viewState.cooperativeTrackVisible) {
            return false;
        }
        if (viewState.onlyRecognizedDroneTracksVisible
            && type === 2 && Number(track.targetRecResult) !== 1) {
            return false;
        }

        return true;
    }

    function inView(rangeM, heightM) {
        return rangeM >= viewState.minRangeM && rangeM <= viewState.maxRangeM
            && heightM >= viewState.minHeightM && heightM <= viewState.maxHeightM;
    }

    function markSegmentDirty(index) {
        dirtySegmentStart = Math.min(dirtySegmentStart, index);
        dirtySegmentEnd = Math.max(dirtySegmentEnd, index + 1);
    }

    function reserveSegments(count) {
        var capacity = trailPosition ? trailPosition.count / 2 : 0;
        if (count <= capacity) return;
        capacity = Math.max(128, capacity * 2, count);
        var positions = new Float32Array(capacity * 6);
        var colors = new Uint8Array(capacity * 6);
        if (trailPosition) {
            positions.set(trailPosition.array);
            colors.set(trailColor.array);
        }

        trailLines.geometry.dispose();
        trailLines.geometry = new THREE.BufferGeometry();
        trailPosition = new THREE.BufferAttribute(positions, 3).setUsage(THREE.DynamicDrawUsage);
        trailColor = new THREE.BufferAttribute(colors, 3, true).setUsage(THREE.DynamicDrawUsage);
        trailLines.geometry.setAttribute('position', trailPosition);
        trailLines.geometry.setAttribute('color', trailColor);
    }

    function writeSegment(from, to, evicted) {
        var index = evicted ? evicted.segment : -1;
        if (index < 0) {
            index = segmentEnds.length;
            reserveSegments(index + 1);
        } else {
            // Reuse the expired segment's slot so a FIFO update can upload just one segment.
            evicted.segment = -1;
        }
        to.segment = index;
        segmentEnds[index] = to;

        var positions = trailPosition.array;
        var colors = trailColor.array;
        var offset = index * 6;
        positions[offset] = from.east / horizontalScaleM;
        positions[offset + 1] = (from.height - verticalBaseM) / verticalScaleM;
        positions[offset + 2] = from.north / horizontalScaleM;
        positions[offset + 3] = to.east / horizontalScaleM;
        positions[offset + 4] = (to.height - verticalBaseM) / verticalScaleM;
        positions[offset + 5] = to.north / horizontalScaleM;
        colors[offset] = colors[offset + 3] = (to.rgb >>> 16) & 255;
        colors[offset + 1] = colors[offset + 4] = (to.rgb >>> 8) & 255;
        colors[offset + 2] = colors[offset + 5] = to.rgb & 255;
        markSegmentDirty(index);
    }

    function removeSegment(sample) {
        var index = sample.segment;
        if (index < 0) return;
        var last = segmentEnds.pop();
        if (last !== sample) {
            // Keep the draw range dense when an entire batch or a clipped segment disappears.
            var from = last.segment * 6;
            var to = index * 6;
            trailPosition.array.copyWithin(to, from, from + 6);
            trailColor.array.copyWithin(to, from, from + 6);
            segmentEnds[index] = last;
            last.segment = index;
            markSegmentDirty(index);
        }
        sample.segment = -1;
    }

    function uploadTrailChanges() {
        trailLines.visible = segmentEnds.length > 0;
        trailLines.geometry.setDrawRange(0, segmentEnds.length * 2);
        if (dirtySegmentStart < dirtySegmentEnd) {
            // Three r128 accepts a single contiguous dirty range per attribute.
            var offset = dirtySegmentStart * 6;
            var count = (dirtySegmentEnd - dirtySegmentStart) * 6;
            trailPosition.updateRange.offset = trailColor.updateRange.offset = offset;
            trailPosition.updateRange.count = trailColor.updateRange.count = count;
            trailPosition.needsUpdate = trailColor.needsUpdate = true;
            dirtySegmentStart = Infinity;
            dirtySegmentEnd = 0;
        }
    }

    function sampleAt(track, index) {
        return track.samples[(track.firstSample + index) % track.samples.length];
    }

    function addVisibleSegments(track) {
        if (!trackTypeVisible(track)) return;
        var previous = null;
        for (var i = 0; i < track.samples.length; ++i) {
            var sample = sampleAt(track, i);
            var visible = inView(sample.range, sample.height);
            if (visible && previous) writeSegment(previous, sample, null);
            previous = visible ? sample : null;
        }
    }

    function rebuildTrailLines() {
        segmentEnds.length = 0;
        tracks.forEach(function (track) {
            track.samples.forEach(function (sample) { sample.segment = -1; });
            addVisibleSegments(track);
        });
        trailRebuildPending = false;
    }

    function appendSamples(track, incoming, draw) {
        var samples = track.samples;
        var limit = viewState.maxTrackPoints;
        for (var i = Math.max(0, incoming.length - limit * pathStride); i < incoming.length; i += pathStride) {
            var previous = samples.length ? sampleAt(track, samples.length - 1) : null;
            var sample;
            var evicted = null;
            if (samples.length === limit) {
                var next = (track.firstSample + 1) % limit;
                // The new oldest sample must lose its incoming line.
                evicted = samples[next];
                sample = samples[track.firstSample];
                track.firstSample = next;
            } else {
                sample = {};
                samples.push(sample);
            }
            sample.east = incoming[i];
            sample.north = incoming[i + 1];
            sample.height = incoming[i + 2];
            sample.range = incoming[i + 3];
            sample.rgb = incoming[i + 4];
            sample.segment = -1;
            if (draw && limit > 1 && previous && inView(previous.range, previous.height)
                && inView(sample.range, sample.height)) {
                writeSegment(previous, sample, evicted);
            } else if (evicted && !trailRebuildPending) {
                removeSegment(evicted);
            }
        }
    }

    function removeTrack(key) {
        var track = tracks.get(key);
        if (!track) return;
        if (!trailRebuildPending) track.samples.forEach(removeSegment);
        tracks.delete(key);
    }

    function rebuildTrackHeads() {
        disposeGroup(pointsGroup);
        pointsGroup = new THREE.Group();
        scene.add(pointsGroup);
        pointObjects = [];
        headsDirty = false;

        var pointBuckets = new Map();
        var pointSize = 0.06 * Math.max(0.5, Math.min(3, Number(viewState.pointSizeRatio)));

        tracks.forEach(function (track) {
            if (!trackTypeVisible(track)) return;
            if (inView(track.rangeM, track.heightM)) {
                var pointColor = String(track.color || '#ea8c93').toLowerCase();
                if (!pointBuckets.has(pointColor)) pointBuckets.set(pointColor, []);
                pointBuckets.get(pointColor).push(track);
            }
        });

        pointBuckets.forEach(function (items, color) {
            var positions = new Float32Array(items.length * 3);
            items.forEach(function (track, index) {
                positions[index * 3] = Number(track.eastM) / horizontalScaleM;
                positions[index * 3 + 1] = (Number(track.heightM) - verticalBaseM) / verticalScaleM;
                positions[index * 3 + 2] = Number(track.northM) / horizontalScaleM;
            });

            var geometry = new THREE.BufferGeometry();
            geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            geometry.computeBoundingSphere();
            var material = new THREE.PointsMaterial({
                color: color,
                map: trackHeadTexture,
                size: pointSize,
                sizeAttenuation: true,
                transparent: true,
                alphaTest: 0.15,
                opacity: 0.96,
                depthWrite: true
            });
            var points = new THREE.Points(geometry, material);
            points.userData.trackItems = items;
            pointsGroup.add(points);
            pointObjects.push(points);
        });

        raycaster.params.Points.threshold = pointSize * 0.9;
    }

    function applyState(nextState) {
        var oldAxesSignature = axesSignature;
        var filterChanged = false;
        Object.keys(viewState).forEach(function (key) {
            if (!Object.prototype.hasOwnProperty.call(nextState, key) || viewState[key] === nextState[key]) return;
            if (key !== 'theme' && key !== 'pointSizeRatio') filterChanged = true;
            if (key === 'maxTrackPoints') {
                tracks.forEach(function (track) {
                    var start = Math.max(0, track.samples.length - nextState.maxTrackPoints);
                    if (start === 0 && track.firstSample === 0) return;
                    var tail = [];
                    for (var i = start; i < track.samples.length; ++i) {
                        tail.push(sampleAt(track, i));
                    }
                    track.samples = tail;
                    track.firstSample = 0;
                });
            }
            viewState[key] = nextState[key];
        });
        palette = palettes[viewState.theme] || palettes.graphite;
        document.documentElement.setAttribute('data-theme', viewState.theme === 'classic' ? 'classic' : 'graphite');
        scene.background.setHex(palette.background);
        renderer.setClearColor(palette.background, 1);
        axesSignature = [viewState.minRangeM, viewState.maxRangeM,
                         viewState.minHeightM, viewState.maxHeightM, viewState.theme].join('|');
        if (oldAxesSignature !== axesSignature) {
            horizontalScaleM = Math.max(1, Number(viewState.maxRangeM));
            verticalBaseM = Number(viewState.minHeightM);
            var heightSpan = Number(viewState.maxHeightM) - verticalBaseM;
            verticalScaleM = heightSpan > 0 && Number.isFinite(heightSpan) ? heightSpan : 1;
            rebuildAxes();
        }
        if (filterChanged) trailRebuildPending = true;
        headsDirty = true;
        requestRender();
    }

    function applySnapshot(snapshotTracks, state) {
        tracks.clear();
        segmentEnds.length = 0;
        applyState(state || {});
        // A snapshot replaces every path, so build each visible line while loading its samples.
        trailRebuildPending = false;
        applyDelta(snapshotTracks, []);
    }

    function applyDelta(upserts, removals) {
        (removals || []).forEach(function (key) { removeTrack(String(key)); });
        (upserts || []).forEach(function (track) {
            var key = String(track.key);
            var oldTrack = tracks.get(key);
            track.samples = oldTrack ? oldTrack.samples : [];
            track.firstSample = oldTrack ? oldTrack.firstSample : 0;
            var visible = trackTypeVisible(track);
            var rebuildBatch = !trailRebuildPending && oldTrack && trackTypeVisible(oldTrack) !== visible;
            if (rebuildBatch) {
                track.samples.forEach(removeSegment);
            }
            appendSamples(track, track.path || [], visible && !trailRebuildPending && !rebuildBatch);
            track.path = null;
            if (rebuildBatch) addVisibleSegments(track);
            tracks.set(key, track);
        });
        headsDirty = true;
        requestRender();
    }

    function clearTracks() {
        tracks.clear();
        segmentEnds.length = 0;
        trailLines.geometry.dispose();
        trailLines.geometry = new THREE.BufferGeometry();
        trailPosition = null;
        trailColor = null;
        dirtySegmentStart = Infinity;
        dirtySegmentEnd = 0;
        trailRebuildPending = false;
        headsDirty = true;
        requestRender();
        hideTooltip();
    }

    function formatNumber(value, digits) {
        var numeric = Number(value);
        return Number.isFinite(numeric) ? numeric.toFixed(digits) : '--';
    }

    function trackTypeLabel(type) {
        if (Number(type) === 3) return 'TBD航迹';
        if (Number(type) === 4) return '协同航迹';
        return '跟踪点';
    }

    function recognitionLabel(track) {
        if (track.offline) {
            return '离线颜色码 ' + track.targetRecResult;
        }
        return Number(track.targetRecResult) === 1 ? '无人机' : '其它';
    }

    function showTooltip(track, clientX, clientY) {
        tooltip.innerHTML =
            '<div class="tooltip-title">' + trackTypeLabel(track.type) + '</div>'
            + '<div><span class="tooltip-key">批号</span>' + track.batch + '</div>'
            + '<div><span class="tooltip-key">距离</span>' + formatNumber(track.rangeM, 1) + ' m</div>'
            + '<div><span class="tooltip-key">方位</span>' + formatNumber(track.azimuthDeg, 1) + '°</div>'
            + '<div><span class="tooltip-key">俯仰</span>' + formatNumber(track.elevationDeg, 1) + '°</div>'
            + '<div><span class="tooltip-key">高度</span>' + formatNumber(track.heightM, 1) + ' m</div>'
            + '<div><span class="tooltip-key">速度</span>' + formatNumber(track.speedMps, 1) + ' m/s</div>'
            + '<div><span class="tooltip-key">SNR</span>' + formatNumber(track.snrDb, 1) + ' dB</div>'
            + '<div><span class="tooltip-key">幅度</span>' + formatNumber(track.amp, 1) + '</div>'
            + '<div><span class="tooltip-key">识别</span>' + recognitionLabel(track) + '</div>';
        tooltip.style.display = 'block';

        var left = clientX + 14;
        var top = clientY + 14;
        var maxLeft = Math.max(4, root.clientWidth - tooltip.offsetWidth - 4);
        var maxTop = Math.max(4, root.clientHeight - tooltip.offsetHeight - 4);
        tooltip.style.left = Math.min(left, maxLeft) + 'px';
        tooltip.style.top = Math.min(top, maxTop) + 'px';
    }

    function hideTooltip() {
        tooltip.style.display = 'none';
    }

    function onPointerMove(event) {
        if (!active || event.buttons || !pointObjects.length) {
            hideTooltip();
            return;
        }
        var rect = renderer.domElement.getBoundingClientRect();
        pointer.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
        pointer.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;
        raycaster.setFromCamera(pointer, camera);
        var intersections = raycaster.intersectObjects(pointObjects, false);
        if (!intersections.length) {
            hideTooltip();
            return;
        }
        var hit = intersections[0];
        var items = hit.object.userData.trackItems || [];
        var track = items[hit.index];
        if (!track) {
            hideTooltip();
            return;
        }
        showTooltip(track, event.clientX, event.clientY);
    }

    function connectBridge() {
        if (typeof QWebChannel === 'undefined' || typeof qt === 'undefined'
            || !qt.webChannelTransport) {
            throw new Error('Qt WebChannel transport or local qwebchannel.js is unavailable');
        }
        new QWebChannel(qt.webChannelTransport, function (channel) {
            try {
                bridge = channel.objects.track3dBridge;
                if (!bridge) throw new Error('track3dBridge is not registered');
                bridge.snapshotChanged.connect(applySnapshot);
                bridge.trackDeltaChanged.connect(applyDelta);
                bridge.sceneStateChanged.connect(applyState);
                bridge.clearRequested.connect(clearTracks);
                bridge.resetViewRequested.connect(resetCamera);
                bridge.activeChanged.connect(function (nextActive) {
                    active = Boolean(nextActive);
                    if (!active) {
                        hideTooltip();
                    } else {
                        resizeRenderer();
                        requestRender();
                    }
                });

                pendingErrors.forEach(function (message) {
                    bridge.reportError(message);
                });
                pendingErrors = [];
                // Connect first so even a missing Three.js/WebGL failure reaches C++.
                initialize();
            } catch (error) {
                failInitialization(error);
            }
        });
    }

    function initialize() {
        if (typeof THREE === 'undefined') throw new Error('Local Three.js resource did not load');
        if (typeof THREE.OrbitControls !== 'function') throw new Error('Local OrbitControls resource did not load');
        pointer = new THREE.Vector2();
        scene = new THREE.Scene();
        document.documentElement.setAttribute('data-theme', theme);
        scene.background = new THREE.Color(palette.background);

        camera = new THREE.PerspectiveCamera(defaultFov, 1, 0.01, 100);
        renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false });
        renderer.setClearColor(palette.background, 1);
        renderer.outputEncoding = THREE.sRGBEncoding;
        root.appendChild(renderer.domElement);

        controls = new THREE.OrbitControls(camera, renderer.domElement);
        controls.enableDamping = false;
        controls.enablePan = true;
        controls.screenSpacePanning = true;
        controls.minDistance = 0.8;
        controls.maxDistance = 8;
        controls.maxPolarAngle = Math.PI * 0.49;
        controls.addEventListener('change', requestRender);
        controls.addEventListener('start', hideTooltip);

        raycaster = new THREE.Raycaster();
        trackHeadTexture = createTrackHeadTexture();
        renderer.domElement.addEventListener('mousemove', onPointerMove, { passive: true });
        renderer.domElement.addEventListener('mouseleave', hideTooltip, { passive: true });
        renderer.domElement.addEventListener('webglcontextlost', function (event) {
            event.preventDefault();
            failInitialization(new Error('WebGL context lost'));
        }, false);
        renderer.domElement.addEventListener('webglcontextrestored', function () {
            console.info('[Track3D] WebGL context restored');
            if (startupFailed) return; // Native one-shot recovery owns a lost context.
            rebuildAxes();
            trailRebuildPending = true;
            headsDirty = true;
            resizeRenderer();
        }, false);

        pointsGroup = new THREE.Group();
        scene.add(pointsGroup);
        trailLines = new THREE.LineSegments(new THREE.BufferGeometry(),
            new THREE.LineBasicMaterial({ vertexColors: true }));
        // The dynamic geometry contains unused capacity and no cached bounding sphere.
        trailLines.frustumCulled = false;
        trailLines.visible = false;
        scene.add(trailLines);
        resetCamera();
        rebuildAxes();
        sceneInitialized = true;
        resizeRenderer();

        if (window.ResizeObserver) {
            new ResizeObserver(resizeRenderer).observe(root);
        }
        window.addEventListener('resize', resizeRenderer, { passive: true });
    }

    window.addEventListener('error', function (event) {
        failInitialization(event.message + ' @ ' + event.filename + ':' + event.lineno);
    });
    window.addEventListener('unhandledrejection', function (event) {
        failInitialization('Unhandled promise rejection: ' + event.reason);
    });

    try {
        connectBridge();
    } catch (error) {
        failInitialization(error);
    }
}());
