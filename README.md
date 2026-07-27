# DispCtrl Ship-Radar

Qt 5/C++17 marine radar display and control application.

## Runtime scope

- UDP marine control frames and radar status reception.
- Fragmented FFT echo reassembly and sweep statistics.
- PPI echo renderer with configurable range, color map, and retained sweep count.
- SIMRAD-style navigation and radar-control panel.
- Echo A-scope and color bar.

## Source layout

- `Basic/`: configuration, logging, display scaling, and the marine protocol.
- `Controller/`: ship-radar controller and UDP protocol manager.
- `PolarDisp/`: PPI axis, grid, echo renderer, color bar, and A-scope.
- `mainPanel/`: the active ship-radar window layout and controls.
- `cusWidgets/`: the message box used by the active UI.
- `docs/SHIP_RADAR_EXTERNAL_PROTOCOL.md`: current external protocol reference.

The `ship-radar` branch intentionally does not include the old X576 map, internal protocol, detection/track, parameter-dialog, or legacy UDP stacks.

## Build

Required Qt modules: `Core`, `Gui`, `Widgets`, and `Network`.

Linux helper scripts:

```bash
scripts/build_linux.sh Release
scripts/package_linux.sh Release
```

The package script produces `deploy/DispCtrl-linux-x64.tar.gz`.
