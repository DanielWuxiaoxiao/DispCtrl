#!/usr/bin/env bash
# Test desktop/autostart installation in a temporary home without launching DispCtrl.
set -euo pipefail
SOURCE="$(cd "$(dirname "$0")" && pwd)/install_desktop.sh"
ROOT=$(mktemp -d)
trap 'rm -rf -- "$ROOT"' EXIT
export HOME="$ROOT/home"
export XDG_CONFIG_HOME="$ROOT/config"
export XDG_DATA_HOME="$ROOT/data"
APP="$ROOT/release with spaces"
mkdir -p "$HOME/Desktop" "$APP" "$ROOT/tools"
printf '#!/bin/sh\nprintf "%%s\\n" "$HOME/custom desktop"\n' > "$ROOT/tools/xdg-user-dir"
chmod +x "$ROOT/tools/xdg-user-dir"
export PATH="$ROOT/tools:$PATH"
cp "$SOURCE" "$APP/install_desktop.sh"
cp /bin/true "$APP/run.sh"
printf 'V5.27\n' > "$APP/VERSION"
AUTO="$XDG_CONFIG_HOME/autostart/dispctrl.desktop"
MENU="$XDG_DATA_HOME/applications/dispctrl.desktop"
printf 'y\n' | bash "$APP/install_desktop.sh"
cmp "$MENU" "$AUTO"
cmp "$MENU" "$HOME/custom desktop/dispctrl.desktop"
grep -Fx 'Name=DispCtrl V5.27' "$MENU"
grep -Fx "Exec=\"$APP/run.sh\"" "$AUTO"
printf 'n\n' | bash "$APP/install_desktop.sh"
test ! -e "$AUTO"
printf '是\n' | bash "$APP/install_desktop.sh"
test -s "$AUTO"
bash "$APP/install_desktop.sh" --uninstall
test ! -e "$AUTO" && test ! -e "$MENU" && test ! -e "$HOME/custom desktop/dispctrl.desktop"
echo 'Desktop installer tests passed.'
