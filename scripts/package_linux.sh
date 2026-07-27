#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_TYPE="${1:-Release}"
BUILD_DIR="${PROJECT_DIR}/build/linux-${BUILD_TYPE,,}"
DEPLOY_DIR="${PROJECT_DIR}/deploy/DispCtrl-linux-x64"
APP_NAME="DispCtrl"

APP_BINARY=""
for candidate in "$BUILD_DIR/bin/$BUILD_TYPE/$APP_NAME" "$BUILD_DIR/bin/$APP_NAME" "$BUILD_DIR/$APP_NAME"; do
    if [ -f "$candidate" ]; then
        APP_BINARY="$candidate"
        break
    fi
done

if [ -z "$APP_BINARY" ]; then
    echo "Missing build output. Run scripts/build_linux.sh first." >&2
    exit 1
fi

QT_LIBS="$(qmake -query QT_INSTALL_LIBS)"
QT_PLUGINS="$(qmake -query QT_INSTALL_PLUGINS)"

rm -rf "$DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR/bin" "$DEPLOY_DIR/lib" "$DEPLOY_DIR/plugins/platforms" "$DEPLOY_DIR/plugins/imageformats"
cp "$APP_BINARY" "$DEPLOY_DIR/bin/$APP_NAME"
cp "$PROJECT_DIR/config.toml" "$DEPLOY_DIR/bin/"
chmod +x "$DEPLOY_DIR/bin/$APP_NAME"

for module in Core Gui Widgets Network; do
    for library in "$QT_LIBS/libQt5${module}.so.5"*; do
        [ -f "$library" ] && cp -L "$library" "$DEPLOY_DIR/lib/"
    done
done

for plugin in libqxcb.so libqminimal.so libqoffscreen.so; do
    [ -f "$QT_PLUGINS/platforms/$plugin" ] && cp "$QT_PLUGINS/platforms/$plugin" "$DEPLOY_DIR/plugins/platforms/"
done
for plugin in "$QT_PLUGINS/imageformats/"*.so; do
    [ -f "$plugin" ] && cp "$plugin" "$DEPLOY_DIR/plugins/imageformats/"
done

collect_deps() {
    ldd "$1" 2>/dev/null | awk '/=> \/[^ ]+/ {print $3}' | while read -r library; do
        [ -f "$library" ] || continue
        case "$(basename "$library")" in
            libc.so*|libm.so*|libdl.so*|libpthread.so*|librt.so*|ld-linux*) continue ;;
        esac
        cp -Ln "$library" "$DEPLOY_DIR/lib/" 2>/dev/null || true
    done
}

collect_deps "$DEPLOY_DIR/bin/$APP_NAME"
for library in "$DEPLOY_DIR/lib/"*.so* "$DEPLOY_DIR/plugins/"*/*.so; do
    [ -f "$library" ] && collect_deps "$library"
done

if command -v patchelf >/dev/null; then
    patchelf --set-rpath '$ORIGIN/../lib' "$DEPLOY_DIR/bin/$APP_NAME" || true
    for library in "$DEPLOY_DIR/lib/"*.so*; do
        [ -f "$library" ] && patchelf --set-rpath '$ORIGIN' "$library" 2>/dev/null || true
    done
fi

cat > "$DEPLOY_DIR/run.sh" <<'EOF'
#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="${SCRIPT_DIR}/lib:${LD_LIBRARY_PATH:-}"
export QT_PLUGIN_PATH="${SCRIPT_DIR}/plugins"
cd "${SCRIPT_DIR}/bin"
exec ./DispCtrl "$@"
EOF
chmod +x "$DEPLOY_DIR/run.sh"

mkdir -p "$PROJECT_DIR/deploy"
tar -C "$PROJECT_DIR/deploy" -czf "$PROJECT_DIR/deploy/DispCtrl-linux-x64.tar.gz" "DispCtrl-linux-x64"
echo "Package created: $PROJECT_DIR/deploy/DispCtrl-linux-x64.tar.gz"
