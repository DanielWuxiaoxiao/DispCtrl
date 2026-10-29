#!/usr/bin/env bash
# Resource and dynamic-library verification only. It never launches DispCtrl.
set -euo pipefail
ROOT="${1:-$(cd "$(dirname "$0")" && pwd)}"
for file in run.sh install_desktop.sh VERSION RUNTIME-RESOURCES.sha256 bin/resources/icon/radararray.png \
    bin/DispCtrl bin/QtWebEngineProcess bin/qt.conf bin/config.toml \
    bin/black.html bin/index.html bin/indexNoL.html bin/indexS.html bin/index3d.html bin/maplibre.html bin/qwebchannel.js \
    bin/amap/AMap3.js bin/amap/2.0.1 bin/amap/init.js bin/amap/mapsplugin.js resources/qtwebengine_resources.pak \
    resources/qtwebengine_resources_100p.pak resources/qtwebengine_resources_200p.pak \
    translations/qtwebengine_locales/en-US.pak plugins/platforms/libqxcb.so; do
    test -s "$ROOT/$file" || { echo "Missing required file: $ROOT/$file" >&2; exit 1; }
done
(cd "$ROOT/bin"; sha256sum --quiet -c ../RUNTIME-RESOURCES.sha256)
command -v ldd >/dev/null
export LD_LIBRARY_PATH="$ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
if [[ ! -s "$ROOT/resources/icudtl.dat" ]]; then
    icu_dependencies=$(ldd "$ROOT/lib/libQt5WebEngineCore.so.5")
    [[ "$icu_dependencies" == *libicudata.so* && "$icu_dependencies" != *"not found"* ]] || {
        echo 'Missing WebEngine ICU data (icudtl.dat or resolved libicudata).' >&2; exit 1;
    }
fi
while IFS= read -r -d '' binary; do
    dependencies=$(ldd "$binary") || { echo "ldd failed: $binary" >&2; exit 1; }
    [[ "$dependencies" != *"not found"* ]] || { echo "Missing runtime dependency: $binary" >&2; echo "$dependencies" >&2; exit 1; }
done < <(find "$ROOT/bin" "$ROOT/lib" "$ROOT/plugins" -type f \
    \( -name DispCtrl -o -name QtWebEngineProcess -o -name '*.so' -o -name '*.so.*' \) -print0)
echo 'Package resource/dependency checks passed; GUI, GPU and radar-network checks remain required.'
