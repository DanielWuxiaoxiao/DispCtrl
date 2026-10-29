#!/usr/bin/env bash
# Packaged Linux entrypoint. Desktop registration never starts a second application.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Work on the current user's graphical session only, never root's desktop.
# Bound registration so missing desktop services cannot block radar startup.
if [[ "${DISPCTRL_INSTALL_DESKTOP:-1}" = 1 && "$EUID" != 0
      && ( -n "${DISPLAY:-}" || -n "${WAYLAND_DISPLAY:-}" )
      && -f "${SCRIPT_DIR}/install_desktop.sh" ]]; then
    if command -v timeout >/dev/null 2>&1; then
        timeout 5s bash "${SCRIPT_DIR}/install_desktop.sh" --auto </dev/null ||
            echo 'Desktop registration incomplete; run install_desktop.sh manually.' >&2
    else
        echo 'Desktop registration skipped (timeout unavailable); run install_desktop.sh manually.' >&2
    fi
fi

export LD_LIBRARY_PATH="${SCRIPT_DIR}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export QT_PLUGIN_PATH="${SCRIPT_DIR}/plugins"
export QTWEBENGINEPROCESS_PATH="${SCRIPT_DIR}/bin/QtWebEngineProcess"
export QTWEBENGINE_RESOURCES_PATH="${SCRIPT_DIR}/resources"
export QTWEBENGINE_LOCALES_PATH="${SCRIPT_DIR}/translations/qtwebengine_locales"

# Target-machine testing confirmed SIGSYS/159 with the bundled WebEngine sandbox.
# Default to the requested compatibility mode; set DISPCTRL_DISABLE_SANDBOX=0
# to restore sandboxing on a verified-compatible system. This reduces isolation.
SANDBOX_FLAGS=""
if [[ "${DISPCTRL_DISABLE_SANDBOX:-1}" = 1 ]]; then
    export QTWEBENGINE_DISABLE_SANDBOX=1
    SANDBOX_FLAGS="--no-sandbox --disable-gpu-sandbox"
else
    export QTWEBENGINE_DISABLE_SANDBOX=0
fi
GPU_FLAGS="--ignore-gpu-blocklist --enable-gpu-rasterization"
export QTWEBENGINE_CHROMIUM_FLAGS="${QTWEBENGINE_CHROMIUM_FLAGS:-} ${SANDBOX_FLAGS} ${GPU_FLAGS}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
cd "${SCRIPT_DIR}/bin"
exec ./DispCtrl "$@"
