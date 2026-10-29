#!/usr/bin/env bash
# No radar GUI, sockets or real desktop changes: all entries live in a temporary HOME.
set -euo pipefail
SOURCE="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
ROOT=$(mktemp -d /tmp/dispctrl-desktop-test.XXXXXX)
[[ "$ROOT" = /tmp/dispctrl-desktop-test.* ]] || exit 1
trap 'rm -rf -- "$ROOT"' EXIT
export HOME="$ROOT/home" XDG_CONFIG_HOME="$ROOT/config" XDG_DATA_HOME="$ROOT/data"
export DISPLAY=:test
export LAUNCH_RECORD="$ROOT/launched"
APP="$ROOT/release with spaces 100%"
mkdir -p "$HOME" "$ROOT/tools" "$APP/bin/resources/icon"
sed 's/\r$//' "$SOURCE/scripts/run_linux.sh" > "$APP/run.sh"
sed 's/\r$//' "$SOURCE/scripts/install_desktop.sh" > "$APP/install_desktop.sh"
cp "$SOURCE/resources/icon/radararray.png" "$APP/bin/resources/icon/"
printf 'V6.0\n' > "$APP/VERSION"
cat > "$APP/bin/DispCtrl" <<'EOF'
#!/bin/bash
printf '%s\n%s\n' "$PWD" "${1:-}" > "$LAUNCH_RECORD"
printf '%s\n%s\n' "${QTWEBENGINE_DISABLE_SANDBOX:-}" "${QTWEBENGINE_CHROMIUM_FLAGS:-}" > "$LAUNCH_RECORD.env"
EOF
cat > "$ROOT/tools/xdg-user-dir" <<'EOF'
#!/bin/bash
printf '%s\n' "${TEST_DESKTOP:-$HOME/custom desktop}"
EOF
for tool in gio update-desktop-database; do
    printf '#!/bin/sh\nexit 0\n' > "$ROOT/tools/$tool"
done
chmod +x "$APP/bin/DispCtrl" "$ROOT/tools/"*
export PATH="$ROOT/tools:$PATH"
MENU="$XDG_DATA_HOME/applications/dispctrl.desktop"
DESKTOP="$HOME/custom desktop/dispctrl.desktop"
AUTO="$XDG_CONFIG_HOME/autostart/dispctrl.desktop"

if [[ "$EUID" = 0 ]]; then
    echo 'Run this test as a non-root user to cover automatic first-run registration.' >&2
    exit 1
fi
env -u DISPCTRL_DISABLE_SANDBOX -u QTWEBENGINE_DISABLE_SANDBOX \
    QTWEBENGINE_CHROMIUM_FLAGS='--lang=en-US' bash "$APP/run.sh" --test-sentinel
test "$(head -n 1 "$LAUNCH_RECORD.env")" = 1
grep -F -- '--no-sandbox' "$LAUNCH_RECORD.env"
grep -F -- '--lang=en-US' "$LAUNCH_RECORD.env"
grep -Fx "$APP/bin" "$LAUNCH_RECORD"
grep -Fx -- '--test-sentinel' "$LAUNCH_RECORD"
grep -Fx 'Name=DispCtrl V6.0' "$MENU"
grep -Fx 'Name[zh_CN]=雷达显控 V6.0' "$MENU"
grep -Fx "Exec=\"${APP//%/%%}/run.sh\"" "$MENU"
grep -Fx "Icon=$APP/bin/resources/icon/radararray.png" "$MENU"
cmp "$MENU" "$DESKTOP"
test -x "$DESKTOP"
test ! -e "$AUTO"
# Explicit opt-out restores sandbox policy; direct Qt env cannot override the choice.
DISPCTRL_DISABLE_SANDBOX=0 QTWEBENGINE_DISABLE_SANDBOX=1 QTWEBENGINE_CHROMIUM_FLAGS='' bash "$APP/run.sh"
test "$(head -n 1 "$LAUNCH_RECORD.env")" = 0
if grep -q -- '--no-sandbox' "$LAUNCH_RECORD.env"; then
    echo 'Sandbox opt-out unexpectedly kept --no-sandbox' >&2; exit 1
fi
DISPCTRL_DISABLE_SANDBOX=1 QTWEBENGINE_DISABLE_SANDBOX=0 bash "$APP/run.sh"
test "$(head -n 1 "$LAUNCH_RECORD.env")" = 1
grep -F -- '--no-sandbox' "$LAUNCH_RECORD.env"
inode=$(stat -c %i "$MENU")
bash "$APP/run.sh"
test "$(stat -c %i "$MENU")" = "$inode"

printf 'y\n' | bash "$APP/install_desktop.sh"
cmp "$MENU" "$AUTO"
printf 'V6.1\n' > "$APP/VERSION"
bash "$APP/run.sh"
grep -Fx 'Name=DispCtrl V6.1' "$MENU"
cmp "$MENU" "$AUTO"
printf 'V6.2\n' > "$APP/VERSION"
bash "$APP/run.sh"
grep -Fx 'Name=DispCtrl V6.2' "$MENU"
cmp "$MENU" "$AUTO"
printf 'n\n' | bash "$APP/install_desktop.sh"
test ! -e "$AUTO"
bash "$APP/install_desktop.sh" --uninstall
test ! -e "$MENU"
test ! -e "$DESKTOP"
DISPCTRL_INSTALL_DESKTOP=0 bash "$APP/run.sh"
test ! -e "$MENU"

# A registration error must not prevent starting the application.
printf 'not a directory\n' > "$ROOT/unwritable"
XDG_DATA_HOME="$ROOT/unwritable" bash "$APP/run.sh" --still-starts
grep -Fx -- '--still-starts' "$LAUNCH_RECORD"

# No dedicated desktop configured: menu-only installation, never dump an icon into HOME.
TEST_DESKTOP="$HOME" bash "$APP/run.sh"
test -s "$MENU"
test ! -e "$HOME/dispctrl.desktop"
bash "$APP/install_desktop.sh" --uninstall
printf 'invalid\n' > "$APP/VERSION"
if bash "$APP/install_desktop.sh" --auto; then
    echo 'Invalid version was not rejected' >&2; exit 1
fi
test ! -e "$MENU"
echo 'Desktop installer and first-run launcher tests passed.'
