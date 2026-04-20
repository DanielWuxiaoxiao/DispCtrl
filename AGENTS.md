# AGENTS.md

This file is the Codex-facing working guide for the `DispCtrl` repository. It is derived primarily from [Claude.md](./Claude.md), then cross-checked against the current tree, `CMakeLists.txt`, `README.md`, `main.cpp`, and recent source files such as `Controller/MarineRadarManager.cpp` and `PolarDisp/echolinechart.cpp`.

## 1. Scope and Priority

When working in this repo, use the following priority order:

1. User request
2. This `AGENTS.md`
3. `Claude.md`
4. Actual source code and build files
5. Other docs under `docs/`

If documentation and code disagree, trust the real code/tree after verifying the affected files.

## 2. Project Snapshot

`DispCtrl` is a radar display and control application built with:

- C++17
- Qt 5.14
- Qt Widgets, Network, WebEngineWidgets, WebChannel
- CMake 3.16+ as the primary build system
- qmake (`DispCtrl.pro`) as a secondary entry

The application is a real-time radar display/control system for the X576 radar family. Major features include:

- PPI polar display and sector display
- Detection/track visualization and tables
- UDP-based subsystem communication
- WebEngine map overlay
- Parameter/config dialogs
- External radar-control link
- Newer marine-radar echo/control protocol support in `Basic/MarineProtocol.h`

## 3. Ground Truth About the Current Repo

Important repository facts verified from the current tree:

- The main agent spec file is `Claude.md` at repo root. The user may refer to `/claude.md`, but the actual file name is `Claude.md`.
- Primary startup path is `main.cpp` -> `FramelessMainWindow` -> controller/init/UI setup.
- Main build file is `CMakeLists.txt`.
- There is a second protocol family in `Basic/MarineProtocol.h`.
- `Controller/MarineRadarManager.cpp` and `PolarDisp/echolinechart.*` exist in the tree.
- `Controller/MarineRadarManager.cpp` currently includes `MarineRadarManager.h`, but that header is not present in the visible tree.
- `CMakeLists.txt` does not currently list `Controller/MarineRadarManager.cpp` or `PolarDisp/echolinechart.cpp` in `SRC_FILES`.

Implication: before modifying or extending marine-radar-related code, verify whether the feature is intentionally incomplete, locally omitted, or simply not yet wired into the build.

## 4. Architecture Summary

High-level layering:

1. UI/view layer
   - `PolarDisp/`
   - `mainPanel/`
   - `paramWidget/`
   - `cusWidgets/`
2. Controller/data-dispatch layer
   - `Controller/`
3. Data object / display object layer
   - `PointManager/`
4. UDP/network layer
   - `UDP/`
5. Shared basics/protocol/config/logging
   - `Basic/`
6. Web map frontend
   - `htmls/`

Key design patterns already in use:

- Singletons: `ConfigManager`, `Controller`, `RadarDataManager`
- Qt signal/slot observer flow
- Manager-based parsing/dispatch between sockets and UI
- Packed binary protocol structs in `Basic/Protocol.h`

## 5. Important Directories

- `Basic/`: protocol structs, config access, logging, math, auth, shared constants
- `Controller/`: managers for inbound/outbound subsystem traffic and business orchestration
- `PolarDisp/`: PPI, sector, tooltip, range-azimuth, zoom, echo-line views
- `PointManager/`: detection/track item managers
- `mainPanel/`: main overlay layout, health/status widgets, recording
- `paramWidget/`: parameter dialogs and downlink command UIs
- `mapDisp/`: Qt <-> JS bridge for web maps
- `UDP/`: threaded UDP socket wrapper
- `docs/`: protocol notes, feature writeups, project book
- `resources/style/darkstyle.qss`: global dark theme
- `config.toml`: runtime configuration

## 6. Core Runtime Facts

- UI must remain on the main thread.
- UDP sockets/managers may run off-thread; UI updates should flow back via signals/slots.
- The codebase relies heavily on Qt parent ownership for widgets/objects.
- Protocol structs are little-endian and typically `#pragma pack(1)`.
- There are known unit differences:
  - detection angles are already in degrees
  - DBT/TBD track angles require radian-to-degree conversion

## 7. Build and Run Facts

Primary build expectations from `CMakeLists.txt`:

- `CMAKE_AUTOMOC`, `AUTOUIC`, and `AUTORCC` are enabled
- Qt 5 components required:
  - `Widgets`
  - `Core`
  - `Gui`
  - `Network`
  - `WebEngineWidgets`
  - `WebChannel`
- Output is under `build/bin/<Config>/`
- `config.toml` is copied to output directories if missing
- `osm/` data is copied to output directories

Agent rule for this repo from `Claude.md`:

- Do not run automatic compile/build commands unless the user explicitly asks.

That means Codex should normally stop at code/document edits and clearly state that build verification was not run.

## 8. Mandatory Working Style for Codex

Before changing code:

1. Read the relevant files first.
2. Verify real call sites, signal connections, and build inclusion.
3. Prefer the smallest correct change.
4. Preserve local style instead of imposing repo-wide refactors.

When changing code:

- Use new-style Qt signal/slot syntax only.
- Keep naming consistent:
  - classes: `PascalCase`
  - members: `m_` prefix
  - functions: `camelCase`
  - macros/constants: `UPPER_SNAKE`
- Prefer Doxygen-style comments when adding non-trivial API or protocol declarations.
- In headers, prefer forward declarations where practical.
- In `.cpp` files, include the full headers actually needed.

After changing code:

1. Check whether `CMakeLists.txt` must be updated.
2. Check whether a `.ui` change implies regenerated `ui_*` files on the user side.
3. Check whether `Claude.md` and/or docs need a sync update for major functional changes.

## 9. Hard Constraints and Prohibitions

From the repository instructions in `Claude.md`:

- Do not run build commands automatically.
- Do not run git commands unless the user explicitly asks.
- Do not auto-commit, auto-push, auto-pull, auto-reset, or auto-checkout.
- Do not perform large unrelated refactors.
- Do not manipulate QWidget/QGraphicsItem from non-UI threads.

Also follow these repo-specific safety checks:

- If adding a new `.cpp`, update `CMakeLists.txt`.
- If editing singleton-to-child signal connections, review destructor disconnect safety.
- If touching detachable/floating windows, review destruction order and guard flags.
- If modifying parameter dialogs, keep the established button behavior:
  - OK sends data and usually does not close
  - Cancel closes the parent `CusWindow`

## 10. Known Pitfalls

These issues are explicitly called out by the project docs and should be assumed relevant:

- Track angle units: DBT/TBD angles may arrive in radians and must be converted to degrees.
- QSS overrides: local dialog styles can unintentionally override `darkstyle.qss`.
- Destructor safety: parent objects must explicitly disconnect singleton signals bound to child objects.
- `DetachableWidget` teardown can trigger reattach/close-order bugs if not guarded.
- `.ui` changes require user-side rebuild/regeneration.
- Config accessors may be missing for new config sections and must be added in `Basic/ConfigManager.h`.
- `CMakeLists.txt` can lag behind the source tree.

Current-tree pitfall verified during this pass:

- Marine radar files exist in source form but appear partially integrated. Validate headers, build entries, and usage before extending them.

## 11. Key Entry Points

Use these as first-stop navigation points:

- App startup: `main.cpp`
- Global controller: `Controller/controller.h`, `Controller/controller.cpp`
- Protocols: `Basic/Protocol.h`, `Basic/MarineProtocol.h`
- Config: `Basic/ConfigManager.h`
- Main UI composition: `mainPanel/mainoverlayout.*`
- PPI scene/view: `PolarDisp/ppiview.*`, `PolarDisp/ppisscene.*`
- Range/azimuth charting: `PolarDisp/rangeazimuthchart.*`, `PolarDisp/rangeazimuthwidget.*`
- Detection manager: `PointManager/detmanager.*`
- Track manager: `PointManager/trackmanager.*`
- External control link: `Controller/ExternalCtrlManager.*`
- Marine echo/control work-in-progress: `Controller/MarineRadarManager.cpp`, `PolarDisp/echolinechart.*`
- UDP transport: `UDP/threadudpsocket.*`

## 12. Typical Task Playbooks

### Add a new inbound data type

1. Define or extend protocol structs in `Basic/Protocol.h` if needed.
2. Add config keys to `config.toml` if new IP/port/ID is required.
3. Create or extend a manager under `Controller/` to parse and emit signals.
4. Expose/relay signals through `Controller`.
5. Connect relevant UI/view consumers.
6. Update `CMakeLists.txt`.

### Modify a parameter dialog

1. Read both `.ui` and `.cpp`.
2. Preserve the established button-box pattern.
3. Keep OK as send/apply without auto-close unless the existing dialog already differs.
4. If persistence is needed, wire `ConfigManager` read/save methods and `config.toml`.

### Add a new config item

1. Add it to `config.toml`.
2. Add an accessor or save helper in `Basic/ConfigManager.h`.
3. Use `CF_INS` consistently.

### Investigate display anomalies

Check, in order:

1. config IP/port mismatch
2. protocol unit mismatch
3. missing signal connections
4. scene/item visibility logic
5. FIFO limits or max-point settings

## 13. Documentation Maintenance

For significant changes, update the relevant docs, especially:

- `Claude.md` for project-wide guidance changes
- feature-specific docs under `docs/`
- `config_documentation.md` when config schema changes

If the change materially affects how future agents should work in this repo, update this `AGENTS.md` too.

## 14. Preferred Response Style for Future Codex Runs

When working in this repository, Codex should:

- be direct and technical
- explain assumptions when code and docs diverge
- cite exact files being changed
- call out unverified build/test status explicitly
- mention relevant pitfalls when they are likely to matter

## 15. One-Line Operating Rule

Read first, trust the actual tree over stale docs, make the smallest correct Qt/C++ change, and always verify whether build wiring, signal lifetimes, units, and config access need to move with the edit.
