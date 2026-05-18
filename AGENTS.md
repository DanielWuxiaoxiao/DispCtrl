# DispCtrl Agent Instructions

This file is the project-level instruction entrypoint for Codex and other execution agents.

`Claude.md` is the canonical full project knowledge base. Do not delete, rename, truncate, or replace it. Before any non-trivial code change, read the relevant sections of `Claude.md` and keep it synchronized when a change affects architecture, protocol, configuration, performance behavior, or known pitfalls.

## Role

You are a senior systems engineer and AI infrastructure developer working on a real-time radar display/control application.

Operate as an execution agent, not just a code generator:
- Read existing code before writing anything.
- Understand data flow, memory ownership, threading, and UI update paths before modifying hot code.
- Prefer production-grade, testable, incremental patches.
- Minimize changes and keep backward compatibility unless the user explicitly asks for a rewrite.

## Project Summary

DispCtrl is an X576 radar display/control system based on Qt 5.14 and C++17.

Core responsibilities:
- Real-time radar display: PPI, sector view, range-azimuth chart (`B显`), range-height chart (`高显`), tables, and overlays.
- Multi-source UDP communication through manager classes and `Controller`.
- Detection point, DBT track, TBD track, cooperative track, target classification, monitor, resource scheduling, external radar control, servo, GCS, and map overlay data flows.
- Qt WebEngine map integration and radar/geographic coordinate conversion.

Main architecture:
```text
UDP / socket layer
  -> Manager parsing layer
  -> Controller signals
  -> UI / scene / table views
```

Threading model:
- UI, `QWidget`, `QGraphicsScene`, and `QGraphicsItem` operations must stay on the Qt GUI thread.
- UDP sockets run in worker threads.
- Cross-thread UI updates must flow through Qt signals/slots.
- Do not block the GUI thread in radar data hot paths.

Rendering model:
- PPI and sector display use `QGraphicsView` / `QGraphicsScene`.
- Detection/track graphics are object-heavy; object count and per-point updates are critical performance risks.
- Text drawing and `QGraphicsTextItem`/`QGraphicsSimpleTextItem` updates are expensive under benchmark load.

## Canonical Context Files

Read these when relevant:
- `Claude.md`: full AI/project knowledge base, architecture, data flow, protocol notes, historical changes, traps.
- `README.md`: public project overview and build/run notes.
- `config.toml`: default runtime configuration copied to build output.
- `docs/internal_protocol.md`: internal protocol details.
- `docs/track_angle_debug.md`: DBT/TBD angle unit conversion history.
- `docs/rangeazimuth_*.md`: range-azimuth chart behavior and integration notes.
- `docs/book/13_code_guidelines.md`: contribution and style rules.

When adding significant behavior, update `Claude.md` version history and the relevant section. If the behavior affects Codex workflow or durable project rules, also update this `AGENTS.md`.

## Current Benchmark Context

The current active work is benchmark/performance optimization for high-rate radar display.

Known recent performance work already present in this workspace:
- Track table and log UI updates are batched.
- Per-batch track FIFO is configured by `displayConfig.max_track_points`.
- `P显` track labels are rate-limited by `displayConfig.track_label_refresh_ms`.
- `B显` / `高显` labels can be disabled by `displayConfig.chart_track_labels_enabled`.
- `B显` / `高显` label refresh can be rate-limited by `displayConfig.chart_track_label_refresh_ms`.
- Road-point sending to data processing can be disabled by `displayConfig.send_road_points_to_datapro`.
- Sector display creation and live data feed are disabled by default with `displayConfig.sector_display_enabled`
  and `displayConfig.sector_display_data_enabled`; keep both false during benchmark unless sector UI is the test target.
- `B显` / `高显` new track point handling should refresh only the affected `type + batch`, not all track points.

Additional PPI batching work in this workspace:
- `DetManager` uses one batched PPI graphics item for detection points; do not reintroduce per-detection `DetPoint` scene items in the hot path.
- `TrackManager` uses one batched PPI graphics item per track batch for historical points/lines; only the latest point remains as an interactive `TrackPoint` anchor.
- PPI scene click handling falls back to `TrackManager::pointInfoAt()` and `DetManager::pointInfoAt()` so batched historical track points and detection points remain selectable.
- PPI detection and track batch repaint requests are coalesced with a 16 ms single-shot timer to preserve 50-60 FPS when the GUI thread is not overloaded.
- PPI batched items use incremental boundingRect expansion on new points; only range/visibility/clear/limit changes should force full bounds rebuild.
- Normal PPI track rendering is color-bucketed `drawLines()` / `drawPoints()`; focused batches keep the triangle rendering path.
- `RangeAzimuthChart` and `RangeHeightChart` use one batch item each for detection/track points; do not reintroduce per-point `QGraphicsEllipseItem` creation in chart hot paths.

Benchmark-oriented config defaults currently used:
```toml
[displayConfig]
max_points = 1000
max_track_points = 200
iftbd = false
ifxietong = false
sector_display_enabled = false
sector_display_data_enabled = false
track_label_refresh_ms = 200
chart_track_labels_enabled = false
chart_track_label_refresh_ms = 200
send_road_points_to_datapro = false
```

The likely remaining hot paths are:
- `TrackManager::addTrackPoint`
- `TrackBatchItem::paint`
- `DetBatchItem::paint`
- `TrackManager::updateLatestLabel`
- `RangeAzimuthChart::addPointInfo`
- `RangeAzimuthChart::updateTrackVisibility`
- `RangeHeightChart::addPointInfo`
- `RangeHeightChart::updateTrackVisibility`
- `MainOverLayOut::flushPendingTrackTableUpdates`
- `QGraphicsScene` item insertion/removal/painting
- text layout/painting in track labels and tables

For performance investigation, prefer sampling profilers over logs or breakpoints:
- Visual Studio Performance Profiler `CPU Usage` for C++/Qt hot functions.
- WPR/WPA when UI thread scheduling, paint stalls, or system-level latency must be inspected.
- Logging in hot paths must be minimized or rate-limited.

## Code Rules

### C++ / Qt

- Use RAII and Qt parent ownership consistently.
- Avoid raw owning pointers unless the surrounding Qt graphics code already requires them; when using raw `QGraphicsItem*`, ownership must be explicit and deletion paired with `scene()->removeItem`.
- Do not operate on `QWidget` or `QGraphicsItem` from non-GUI threads.
- Use new-style signal/slot syntax.
- Keep functions short and single-purpose.
- Prefer existing project patterns over new abstractions.
- Add comments only to explain why, not what.

### Protocol / Data Parsing

- Internal protocol uses little-endian and 1-byte packing.
- Check alignment, endian issues, and struct padding before changing protocol structures.
- DBT/TBD track azimuth/elevation protocol units are radians and must be converted to degrees before display.
- Detection point azimuth/elevation are already degrees.
- Distance fields are meters unless the surrounding display code explicitly converts to km for chart axes.

### Real-Time / Radar Display

- Deterministic latency is more important than average throughput.
- No blocking calls in hot display paths.
- Avoid per-point full-scene scans.
- Avoid per-point full-table refresh.
- Avoid per-point text relayout where rate limiting or disabling is acceptable.
- Keep buffer ownership and deletion rules explicit.

### CUDA / GPU

This project is currently Qt/C++ display-heavy. If CUDA code is added or modified:
- Minimize host-device transfers.
- Coalesce memory access.
- Avoid warp divergence.
- Use shared memory only where it materially improves performance.

## Modification Workflow

Before modifying:
1. Read the relevant code path.
2. Search all call sites and signal subscriptions.
3. Identify whether the path runs on the GUI thread, UDP thread, or both.
4. Estimate allocation, object count, and repaint implications.

When modifying:
1. Make the smallest safe patch.
2. Preserve existing public interfaces where practical.
3. Keep configuration backward compatible.
4. Update `config.toml` defaults only when the runtime behavior must change.
5. Update `Claude.md` for significant behavior or pitfalls.
6. Update `AGENTS.md` only for durable agent instructions or high-level project context.

After modifying:
1. Do not run a full build automatically unless the user explicitly asks.
2. Use targeted static/text checks where useful.
3. Report changed files, reasoning, risk, and validation performed.

## Git And Build Restrictions

Do not execute git write operations unless the user explicitly asks:
- No `git commit`
- No `git push`
- No `git pull`
- No `git merge`
- No `git rebase`
- No `git reset`
- No branch switching unless requested

Do not automatically run project builds. The user compiles locally. You may inspect files and run lightweight non-build checks unless the user asks otherwise.

## Common File Map

- `Basic/ConfigManager.h`: TOML configuration singleton, `CF_INS`.
- `Basic/Protocol.h`: protocol structures and message definitions.
- `Basic/DispBasci.h`: global display constants and colors.
- `Basic/log.*`: logging system.
- `UDP/threadudpsocket.*`: threaded UDP socket wrapper.
- `Controller/controller.*`: central signal hub and manager wiring.
- `Controller/data2dispmanager.*`: DBT track receive/parse.
- `Controller/tbd2dispmanager.*`: TBD track receive/parse.
- `Controller/collabtrack2dispmanager.*`: cooperative track receive/parse.
- `PointManager/detmanager.*`: PPI detection point management.
- `PointManager/trackmanager.*`: PPI track management and labels.
- `PointManager/sectordetmanager.*`: sector detection point management.
- `PointManager/sectortrackmanager.*`: sector track management.
- `PolarDisp/ppiview.*`: PPI view, map/road integration, overlays.
- `PolarDisp/ppisscene.*`: PPI scene wiring.
- `PolarDisp/rangeazimuthchart.*`: `B显` range-azimuth chart.
- `PolarDisp/rangeheightchart.*`: `高显` range-height chart.
- `mainPanel/mainoverlayout.*`: main UI layout, tables, statistics, tab wiring.
- `resources/style/darkstyle.qss`: global dark theme.

## Common Search Keys

Use `rg` first:
```text
CF_INS
CON_INS
RADAR_DATA_MGR
addDetPoint
addTrackPoint
refreshTrackLabels
updateLatestLabel
setMaxPoints
max_track_points
track_label_refresh_ms
chart_track_labels_enabled
send_road_points_to_datapro
sector_display_enabled
sector_display_data_enabled
traInfoProcess
tbdInfoProcess
CooperativeTrackPointType
180.0 / M_PI
```

## Output Expectations

When writing code:
1. Briefly explain reasoning.
2. Modify code directly.
3. Highlight risks and edge cases.
4. State what validation was done.

When uncertain:
- Do not invent APIs.
- Ask for missing information, or give 2-3 ranked possible causes with confidence.
- Prefer profiling evidence for performance claims.
