#!/usr/bin/env bash
# Package only resources into a temporary fixture; never compile or run DispCtrl.
set -euo pipefail
SOURCE="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
ROOT=$(mktemp -d /tmp/dispctrl-resources-test.XXXXXX)
[[ "$ROOT" = /tmp/dispctrl-resources-test.* ]] || exit 1
trap 'rm -rf -- "$ROOT"' EXIT
FIXTURE="$ROOT/source with spaces"
mkdir -p "$FIXTURE/resources" "$ROOT/release/bin"
cp "$SOURCE/config.toml" "$FIXTURE/"
cp -a "$SOURCE/htmls" "$FIXTURE/"
cp -a "$SOURCE/resources/style" "$SOURCE/resources/icon" "$FIXTURE/resources/"
sed 's/\r$//' "$SOURCE/scripts/copy_runtime_resources.sh" > "$ROOT/copy.sh"
for name in mapNoL map16 map16S osm CommandControlRecords recordings; do
    mkdir -p "$FIXTURE/$name" "$FIXTURE/htmls/$name"
    printf 'excluded\n' > "$FIXTURE/$name/field-data"
    printf 'excluded\n' > "$FIXTURE/htmls/$name/field-data"
done
printf 'excluded\n' > "$FIXTURE/htmls/Qt5Core.DLL"
printf 'excluded\n' > "$FIXTURE/htmls/data.txt"
printf 'excluded\n' > "$FIXTURE/htmls/disp_ctrl_log_0.txt"
bash "$ROOT/copy.sh" "$FIXTURE" "$ROOT/release/bin"
cmp "$FIXTURE/config.toml" "$ROOT/release/bin/config.toml"
cmp "$FIXTURE/htmls/indexNoL.html" "$ROOT/release/bin/indexNoL.html"
cmp "$FIXTURE/htmls/amap/2.0.1" "$ROOT/release/bin/amap/2.0.1"
cmp "$FIXTURE/htmls/amap/mapsplugin.js" "$ROOT/release/bin/amap/mapsplugin.js"
for name in mapNoL map16 map16S osm CommandControlRecords recordings Qt5Core.DLL data.txt disp_ctrl_log_0.txt; do
    test ! -e "$ROOT/release/bin/$name"
done
(cd "$ROOT/release/bin"; sha256sum --quiet -c ../RUNTIME-RESOURCES.sha256)
# Deployed settings are editable without failing immutable-resource checks.
printf '\n# operator edit\n' >> "$ROOT/release/bin/config.toml"
(cd "$ROOT/release/bin"; sha256sum --quiet -c ../RUNTIME-RESOURCES.sha256)
printf '\nchanged\n' >> "$ROOT/release/bin/indexNoL.html"
if (cd "$ROOT/release/bin"; sha256sum --quiet -c ../RUNTIME-RESOURCES.sha256) 2>/dev/null; then
    echo 'Resource corruption was not detected' >&2; exit 1
fi
rm -- "$FIXTURE/htmls/qwebchannel.js"
if bash "$ROOT/copy.sh" "$FIXTURE" "$ROOT/incomplete/bin"; then
    echo 'Missing required resource was not detected' >&2; exit 1
fi
echo 'Runtime resource copy, exclusion, config and integrity tests passed.'
