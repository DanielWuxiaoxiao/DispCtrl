#!/usr/bin/env bash
# Create/remove the current user's DispCtrl desktop, application-menu and
# optional desktop-session autostart entries. Run from an extracted release.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
APP_NAME="DispCtrl"
if [[ -f "${SCRIPT_DIR}/VERSION" ]]; then
    APP_VERSION=$(tr -d '\r\n' < "${SCRIPT_DIR}/VERSION")
    if [[ "$APP_VERSION" =~ ^V?[0-9]+\.[0-9]+(\.[0-9]+)?$ ]]; then
        APP_NAME="DispCtrl ${APP_VERSION}"
    else
        echo "Invalid release VERSION: ${APP_VERSION}" >&2
        exit 1
    fi
fi

APP_COMMENT="X576 radar display and control system"
DESKTOP_FILE="dispctrl.desktop"
DESKTOP_DIR="${XDG_DATA_HOME:-${HOME}/.local/share}/applications"
AUTOSTART_DIR="${XDG_CONFIG_HOME:-${HOME}/.config}/autostart"
AUTOSTART_FILE="${AUTOSTART_DIR}/${DESKTOP_FILE}"

case "${1:-}" in
    ''|--auto|--uninstall|-u) ;;
    *) echo "Usage: bash $0 [--auto|--uninstall]" >&2; exit 2 ;;
esac

DESKTOP_PATH=""
if command -v xdg-user-dir >/dev/null 2>&1; then
    DESKTOP_PATH=$(xdg-user-dir DESKTOP 2>/dev/null || true)
fi
if [[ -z "$DESKTOP_PATH" ]]; then
    [[ -d "$HOME/桌面" ]] && DESKTOP_PATH="$HOME/桌面" || DESKTOP_PATH="$HOME/Desktop"
fi
[[ "$DESKTOP_PATH" = /* ]] || { echo "Desktop directory must be absolute: $DESKTOP_PATH" >&2; exit 1; }

if [[ "${1:-}" = --uninstall || "${1:-}" = -u ]]; then
    rm -f "${DESKTOP_DIR}/${DESKTOP_FILE}" "$AUTOSTART_FILE"
    rm -f "$HOME/Desktop/${DESKTOP_FILE}" "$HOME/桌面/${DESKTOP_FILE}"
    [[ "$DESKTOP_PATH" = "$HOME" ]] || rm -f "${DESKTOP_PATH}/${DESKTOP_FILE}"
    command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
    echo "DispCtrl desktop entries removed."
    exit 0
fi

RUN_SCRIPT="${SCRIPT_DIR}/run.sh"
[[ -f "$RUN_SCRIPT" ]] || { echo "Run this script from the extracted DispCtrl release directory." >&2; exit 1; }

ICON_PATH="utilities-system-monitor"
for candidate in "${SCRIPT_DIR}/bin/resources/icon/radararray.png" \
                 "${SCRIPT_DIR}/bin/resources/icon/logo.png"; do
    if [[ -f "$candidate" ]]; then ICON_PATH="$candidate"; break; fi
done

mkdir -p "$DESKTOP_DIR"
# Desktop values and the Exec command use separate escaping rules.
desktop_value() {
    local value="$1"
    value=${value//\\/\\\\}
    value=${value//$'\n'/\\n}
    value=${value//$'\r'/\\r}
    value=${value//$'\t'/\\t}
    printf '%s' "$value"
}
exec_path=${RUN_SCRIPT//\\/\\\\}
exec_path=${exec_path//\"/\\\"}
exec_path=${exec_path//\$/\\\$}
exec_path=${exec_path//\`/\\\`}
exec_path=${exec_path//%/%%}
entry_tmp=$(mktemp "${DESKTOP_DIR}/.dispctrl.XXXXXX")
trap 'rm -f -- "$entry_tmp"' EXIT
cat > "$entry_tmp" <<DESKTOP_EOF
[Desktop Entry]
Type=Application
Name=${APP_NAME}
Name[zh_CN]=雷达显控 ${APP_VERSION:-}
Comment=${APP_COMMENT}
Exec=$(desktop_value "\"${exec_path}\"")
Icon=$(desktop_value "$ICON_PATH")
Terminal=false
Categories=Science;Engineering;Qt;
StartupNotify=true
StartupWMClass=DispCtrl
DESKTOP_EOF
if ! cmp -s "$entry_tmp" "${DESKTOP_DIR}/${DESKTOP_FILE}"; then
    mv -f -- "$entry_tmp" "${DESKTOP_DIR}/${DESKTOP_FILE}"
fi

if [[ "$DESKTOP_PATH" != "$HOME" ]]; then
    mkdir -p "$DESKTOP_PATH"
    if ! cmp -s "${DESKTOP_DIR}/${DESKTOP_FILE}" "${DESKTOP_PATH}/${DESKTOP_FILE}"; then
        cp "${DESKTOP_DIR}/${DESKTOP_FILE}" "${DESKTOP_PATH}/"
        chmod +x "${DESKTOP_PATH}/${DESKTOP_FILE}"
        if command -v gio >/dev/null 2>&1 && ! gio set "${DESKTOP_PATH}/${DESKTOP_FILE}" metadata::trusted true 2>/dev/null; then
            echo "Desktop entry created; if required, right-click it and choose Allow Launching."
        fi
    fi
fi

# First-run registration never prompts or enables/disables login autostart.
if [[ "${1:-}" = --auto ]]; then
    if [[ -f "$AUTOSTART_FILE" ]] && ! cmp -s "${DESKTOP_DIR}/${DESKTOP_FILE}" "$AUTOSTART_FILE"; then
        cp "${DESKTOP_DIR}/${DESKTOP_FILE}" "$AUTOSTART_FILE"
    fi
    command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
    exit 0
fi

# Autostart uses the normal desktop user/session, never a system service.
while true; do
    echo "Start ${APP_NAME} automatically after this user logs into the desktop? [y/N]"
    if ! IFS= read -r answer; then
        [[ -f "$AUTOSTART_FILE" ]] && cp "${DESKTOP_DIR}/${DESKTOP_FILE}" "$AUTOSTART_FILE"
        echo "No interactive answer; existing autostart setting was kept."
        break
    fi
    case "$answer" in
        y|Y|yes|YES|是) mkdir -p "$AUTOSTART_DIR"; cp "${DESKTOP_DIR}/${DESKTOP_FILE}" "$AUTOSTART_FILE"; echo "Desktop-session autostart enabled."; break ;;
        ''|n|N|no|NO|否) rm -f "$AUTOSTART_FILE"; echo "Desktop-session autostart disabled."; break ;;
        *) echo "Please enter y or n." ;;
    esac
done

command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
echo "Installed ${APP_NAME}. Search the application menu, use the desktop entry, or run: ${RUN_SCRIPT}"
