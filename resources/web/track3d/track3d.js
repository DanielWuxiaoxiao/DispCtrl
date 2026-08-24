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
    var tracks = new Map();
    var pointObjects = [];
    var gridGroup = null;
    var pointsGroup = null;
    var axesSignature = '';

    var viewState = {
        minRangeM: 0,
        maxRangeM: 5000,
        minHeightM: 0,
        maxHeightM: 500,
        trackVisible: true,
        tbdTrackVisible: true,
        cooperativeTrackVisible: true,
        onlyRecognizedDroneTracksVisible: false,
        pointSizeRatio: 1
    };

    var scene;
    var camera;
    var renderer;
    var controls;
    var raycaster;
    var pointer = new THREE.Vector2();

    function reportError(message) {
        var text = String(message || 'unknown JavaScript error');
        if (bridge) {
            bridge.reportError(text);
        } else {
            pendingErrors.push(text);
        }
    }

    function showError(message) {
        statusOverlay.textContent = message;
        statusOverlay.classList.add('error');
        statusOverlay.style.display = 'flex';
    }

    function requestRender() {
        if (renderPending || !renderer) {
            return;
        }
        renderPending = true;
        window.requestAnimationFrame(function () {
            renderPending = false;
            if (renderer) {
                renderer.render(scene, camera);
            }
        });
    }

    function refreshRendererAfterActivation() {
        // QWebEngineView may receive the tab signal before its final visible size.
        // Resize once immediately and once after Qt has completed the tab layout.
        resizeRenderer();
        window.setTimeout(function () {
            if (active && renderer) {
                resizeRenderer();
            }
        }, 0);
        window.setTimeout(function () {
            if (active && renderer) {
                resizeRenderer();
            }
        }, 120);
    }

    function resetCamera() {
        camera.position.set(1.7, 1.35, -1.7);
        controls.target.set(0, 0.35, 0);
        controls.update();
        requestRender();
    }

    function resizeRenderer() {
        if (!renderer) {
            return;
        }
        var width = Math.max(1, root.clientWidth);
        var height = Math.max(1, root.clientHeight);
        camera.aspect = width / height;
        camera.updateProjectionMatrix();
        renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
        renderer.setSize(width, height, false);
        requestRender();
    }

    function disposeMaterial(material) {
        if (!material) {
            return;
        }
        if (material.map) {
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
        var width = Math.max(height * 1.3, height * Math.min(5.0, 0.62 * text.length));
        sprite.scale.set(width, height, 1);
        sprite.renderOrder = 10;
        return sprite;
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

        var majorColor = 0x00503c;
        var minorColor = 0x003226;
        var axisColor = 0x00ff88;
        var labelColor = '#66ffcc';
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
            var spokeAngle = spoke * Math.PI * 2 / 12;
            gridGroup.add(createLine([
                new THREE.Vector3(0, 0, 0),
                new THREE.Vector3(Math.sin(spokeAngle), 0, Math.cos(spokeAngle))
            ], spoke % 3 === 0 ? majorColor : minorColor, spoke % 3 === 0 ? 0.75 : 0.5, false));
        }

        gridGroup.add(createLine([
            new THREE.Vector3(-1, 0, 0), new THREE.Vector3(1, 0, 0)
        ], 0xff4f4f, 0.85, false));
        gridGroup.add(createLine([
            new THREE.Vector3(0, 0, -1), new THREE.Vector3(0, 0, 1)
        ], 0x00aaff, 0.85, false));

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
        ], axisColor, 0.9, false));
        for (var tick = 0; tick <= 5; ++tick) {
            var y = tick / 5;
            gridGroup.add(createLine([
                new THREE.Vector3(heightAxisX - 0.025, y, heightAxisZ),
                new THREE.Vector3(heightAxisX + 0.025, y, heightAxisZ)
            ], axisColor, 0.8, false));
            var heightValue = minHeight + (maxHeight - minHeight) * y;
            var heightLabel = createTextSprite(heightValue.toFixed(0) + ' m', labelColor, 0.065);
            heightLabel.position.set(heightAxisX - 0.09, y, heightAxisZ);
            gridGroup.add(heightLabel);
        }

        [
            ['N', 0, 0.02, 1.1], ['S', 0, 0.02, -1.1],
            ['E', 1.1, 0.02, 0], ['W', -1.1, 0.02, 0]
        ].forEach(function (labelData) {
            var sprite = createTextSprite(labelData[0], '#00ff88', 0.085);
            sprite.position.set(labelData[1], labelData[2], labelData[3]);
            gridGroup.add(sprite);
        });

        axisRange.textContent = '量程 ' + maxRangeKm.toFixed(1) + ' km | 高度 '
            + minHeight.toFixed(0) + '–' + maxHeight.toFixed(0) + ' m';
        requestRender();
    }

    function trackVisible(track) {
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

        var rangeM = Number(track.rangeM);
        var heightM = Number(track.heightM);
        return rangeM >= Number(viewState.minRangeM)
            && rangeM <= Number(viewState.maxRangeM)
            && heightM >= Number(viewState.minHeightM)
            && heightM <= Number(viewState.maxHeightM);
    }

    function rebuildPoints() {
        disposeGroup(pointsGroup);
        pointsGroup = new THREE.Group();
        scene.add(pointsGroup);
        pointObjects = [];

        var buckets = new Map();
        tracks.forEach(function (track) {
            if (!trackVisible(track)) {
                return;
            }
            var color = String(track.color || '#ff0000').toLowerCase();
            if (!buckets.has(color)) {
                buckets.set(color, []);
            }
            buckets.get(color).push(track);
        });

        var maxRangeM = Math.max(1, Number(viewState.maxRangeM));
        var heightSpanM = Math.max(1, Number(viewState.maxHeightM) - Number(viewState.minHeightM));
        var pointSize = 0.032 * Math.max(0.5, Math.min(3, Number(viewState.pointSizeRatio)));

        buckets.forEach(function (items, color) {
            var positions = new Float32Array(items.length * 3);
            items.forEach(function (track, index) {
                positions[index * 3] = Number(track.eastM) / maxRangeM;
                positions[index * 3 + 1] = (Number(track.heightM) - Number(viewState.minHeightM)) / heightSpanM;
                positions[index * 3 + 2] = Number(track.northM) / maxRangeM;
            });

            var geometry = new THREE.BufferGeometry();
            geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
            geometry.computeBoundingSphere();
            var material = new THREE.PointsMaterial({
                color: color,
                size: pointSize,
                sizeAttenuation: true,
                transparent: true,
                opacity: 0.96,
                depthWrite: true
            });
            var points = new THREE.Points(geometry, material);
            points.userData.trackItems = items;
            pointsGroup.add(points);
            pointObjects.push(points);
        });

        raycaster.params.Points.threshold = pointSize * 1.45;
        requestRender();
    }

    function applyState(nextState) {
        var oldAxesSignature = axesSignature;
        Object.keys(viewState).forEach(function (key) {
            if (Object.prototype.hasOwnProperty.call(nextState, key)) {
                viewState[key] = nextState[key];
            }
        });
        axesSignature = [viewState.minRangeM, viewState.maxRangeM,
                         viewState.minHeightM, viewState.maxHeightM].join('|');
        if (oldAxesSignature !== axesSignature) {
            rebuildAxes();
        }
        rebuildPoints();
    }

    function applySnapshot(snapshotTracks, state) {
        tracks.clear();
        (snapshotTracks || []).forEach(function (track) {
            tracks.set(String(track.key), track);
        });
        applyState(state || {});
    }

    function applyDelta(upserts, removals) {
        (removals || []).forEach(function (key) {
            tracks.delete(String(key));
        });
        (upserts || []).forEach(function (track) {
            tracks.set(String(track.key), track);
        });
        rebuildPoints();
    }

    function clearTracks() {
        tracks.clear();
        rebuildPoints();
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
        if (!active || !pointObjects.length) {
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
        new QWebChannel(qt.webChannelTransport, function (channel) {
            bridge = channel.objects.track3dBridge;
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
                    refreshRendererAfterActivation();
                }
            });

            pendingErrors.forEach(function (message) {
                bridge.reportError(message);
            });
            pendingErrors = [];
            bridge.pageReady();
            statusOverlay.style.display = 'none';
        });
    }

    function initialize() {
        scene = new THREE.Scene();
        scene.background = new THREE.Color(0x0a1010);

        camera = new THREE.PerspectiveCamera(42, 1, 0.01, 100);
        renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false });
        renderer.setClearColor(0x0a1010, 1);
        renderer.outputEncoding = THREE.sRGBEncoding;
        root.appendChild(renderer.domElement);

        controls = new THREE.OrbitControls(camera, renderer.domElement);
        controls.enableDamping = false;
        controls.enablePan = false;
        controls.minDistance = 0.8;
        controls.maxDistance = 8;
        controls.maxPolarAngle = Math.PI * 0.49;
        controls.addEventListener('change', requestRender);
        controls.addEventListener('start', hideTooltip);

        raycaster = new THREE.Raycaster();
        renderer.domElement.addEventListener('mousemove', onPointerMove, { passive: true });
        renderer.domElement.addEventListener('mouseleave', hideTooltip, { passive: true });
        renderer.domElement.addEventListener('webglcontextlost', function (event) {
            event.preventDefault();
            showError('WebGL上下文已丢失');
            reportError('WebGL context lost');
        }, false);
        renderer.domElement.addEventListener('webglcontextrestored', function () {
            statusOverlay.style.display = 'none';
            reportError('WebGL context restored');
            rebuildAxes();
            rebuildPoints();
        }, false);

        pointsGroup = new THREE.Group();
        scene.add(pointsGroup);
        resetCamera();
        rebuildAxes();
        resizeRenderer();

        if (window.ResizeObserver) {
            new ResizeObserver(resizeRenderer).observe(root);
        } else {
            window.addEventListener('resize', resizeRenderer, { passive: true });
        }
        connectBridge();
    }

    window.addEventListener('error', function (event) {
        reportError(event.message + ' @ ' + event.filename + ':' + event.lineno);
    });
    window.addEventListener('unhandledrejection', function (event) {
        reportError('Unhandled promise rejection: ' + event.reason);
    });

    try {
        initialize();
    } catch (error) {
        showError('3D渲染初始化失败');
        reportError(error && error.stack ? error.stack : String(error));
    }
}());
