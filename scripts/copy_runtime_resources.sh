#!/usr/bin/env bash
# The source tree is canonical. Never ship Windows binaries or field recordings.
set -euo pipefail
[[ $# = 2 ]] || { echo 'Usage: copy_runtime_resources.sh SOURCE_ROOT RELEASE_BIN' >&2; exit 2; }
SOURCE=$(cd "$1" && pwd)
mkdir -p "$2"
DEST=$(cd "$2" && pwd)
resources=()
copy_asset() {
    local source="$1" relative="$2"
    mkdir -p "$(dirname "$DEST/$relative")"
    cp -- "$source" "$DEST/$relative"
    [[ "$relative" = config.toml ]] || resources+=("$relative")
}
copy_tree() {
    local source="$1" prefix="$2"
    [[ -d "$source" ]] || { echo "Missing runtime resource directory: $source" >&2; exit 1; }
    while IFS= read -r -d '' file; do
        local relative="${file#"$source"/}"
        copy_asset "$file" "${prefix}${relative}"
    done < <(find "$source" \
        -type d \( -name mapNoL -o -name map16 -o -name map16S -o -name osm \
            -o -name CommandControlRecords -o -name recordings -o -name .git \) -prune -o \
        -type f ! -iname '*.dll' ! -iname '*.exe' ! -iname '*.pdb' ! -iname '*.lnk' \
            ! -name 'data.txt' ! -name 'disp_ctrl_log_*.txt' -print0)
}
copy_asset "$SOURCE/config.toml" config.toml
copy_tree "$SOURCE/htmls" ''
copy_tree "$SOURCE/resources/style" 'resources/style/'
copy_tree "$SOURCE/resources/icon" 'resources/icon/'
for required in config.toml black.html index.html indexNoL.html indexS.html index3d.html \
    maplibre.html qwebchannel.js amap/AMap3.js amap/2.0.1 amap/init.js amap/mapsplugin.js \
    resources/style/darkstyle.qss resources/icon/radararray.png; do
    [[ -s "$DEST/$required" ]] || { echo "Missing runtime resource: $required" >&2; exit 1; }
done
# config.toml is mutable after deployment; only immutable resources are verified.
(cd "$DEST"; printf '%s\0' "${resources[@]}" | sort -zu | xargs -0 sha256sum) \
    > "$DEST/../RUNTIME-RESOURCES.sha256"
echo 'Canonical HTML/AMap, styles, icons and config copied; field data excluded.'
