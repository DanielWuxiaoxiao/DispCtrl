#!/usr/bin/env bash
# Build in the container's private filesystem so Windows/WSL build caches and
# old deploy artefacts cannot enter a release.
set -euo pipefail
test -f /input/CMakeLists.txt
test -d /output

tar --exclude='./build' --exclude='./build-*' --exclude='./deploy' \
    --exclude='./.git' --exclude='./.vs' --exclude='./.codex' --exclude='./.agents' \
    --exclude='*.pro.user*' --exclude='*.exe' --exclude='*.dll' \
    -C /input -cf - . | tar -C /src -xf -
cd /src
find scripts docker -type f -name '*.sh' -exec sed -i 's/\r$//' {} +
bash scripts/build_linux.sh Release
bash scripts/package_linux.sh Release

RELEASE_ROOT="/src/deploy/DispCtrl-linux-x64"
test -d "$RELEASE_ROOT"
{
    echo 'Application: DispCtrl'
    echo "Target: Ubuntu ${DISPCTRL_TARGET:-unknown} amd64"
    echo "Built UTC: $(date -u +%FT%TZ)"
    echo "Qt: $(qmake -query QT_VERSION)"
    g++ --version | head -n 1
    ldd --version | head -n 1
    echo 'Source file SHA256:'
    find . -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.ui' \
        -o -name '*.qrc' -o -name '*.html' -o -name '*.js' -o -name 'CMakeLists.txt' \) \
        -not -path './build/*' -not -path './deploy/*' -print0 | sort -z | xargs -0 sha256sum
} > "$RELEASE_ROOT/BUILD-INFO.txt"

rm -rf /output/DispCtrl-linux-x64
cp -a "$RELEASE_ROOT" /output/
