#!/usr/bin/env bash
# Resource and dynamic-library verification only. It never launches DispCtrl.
set -euo pipefail
ROOT="${1:-$(cd "$(dirname "$0")" && pwd)}"
for file in bin/DispCtrl bin/QtWebEngineProcess bin/qt.conf bin/config.toml \
    bin/index.html bin/indexNoL.html bin/indexS.html bin/index3d.html bin/qwebchannel.js \
    bin/amap/AMap3.js resources/qtwebengine_resources.pak \
    resources/qtwebengine_resources_100p.pak resources/qtwebengine_resources_200p.pak \
    translations/qtwebengine_locales/en-US.pak plugins/platforms/libqxcb.so; do
    test -s "$ROOT/$file" || { echo "Missing required file: $ROOT/$file" >&2; exit 1; }
done
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
