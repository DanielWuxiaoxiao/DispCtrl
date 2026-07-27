# DispCtrl Ship-Radar Guide

## Scope

`ship-radar` is a Qt 5/C++17 marine radar display and control application. The active runtime path is intentionally small:

```
Marine UDP -> MarineRadarManager -> Controller::marineEchoLine
          -> PPIScene -> EchoRenderer -> PPIView

MainOverLayOut controls -> MarineRadarManager -> 16-byte control UDP frame
```

Only these source areas are part of the active product:

- `Basic/`: `ConfigManager`, `DispBasci`, `log`, `MarineProtocol`.
- `Controller/`: `controller`, `MarineRadarManager`.
- `PolarDisp/`: PPI axis/grid/view/scene, echo renderer, color bar, A-scope.
- `mainPanel/mainoverlayout.*`: SIMRAD-style marine UI.
- `cusWidgets/custommessagebox.*`.

The project does not include the old X576 map/WebEngine layer, internal `Protocol`, target display/track pipeline, parameter dialogs, or generic threaded UDP wrapper.

## Engineering rules

- Preserve physical units and the marine protocol byte layout exactly.
- Keep UI changes in `mainPanel/mainoverlayout.ui` and wire them in `mainoverlayout.cpp`.
- Keep range switching synchronized through `EchoRenderer`, `PolarAxis`, and the marine control frame.
- Do not automatically compile; provide code changes for the user to build and validate.
- Use git commands only after the user explicitly requests them.
- Update `docs/SHIP_RADAR_EXTERNAL_PROTOCOL.md` whenever the external protocol changes.

## Build and deployment

- CMake and qmake use only Qt `Core`, `Gui`, `Widgets`, and `Network`.
- `scripts/build_linux.sh` builds a selected configuration.
- `scripts/package_linux.sh` creates `deploy/DispCtrl-linux-x64.tar.gz`.
