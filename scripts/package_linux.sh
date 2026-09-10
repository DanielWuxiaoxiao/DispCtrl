#!/bin/bash
##############################################################################
# DispCtrl Linux 打包脚本
# 用法: ./scripts/package_linux.sh [Release|Debug]
# 说明: 将编译产物及所有 Qt 依赖打包为独立可部署目录
#       类似 Windows 上 windeployqt 的功能
#
# 输出:
#   deploy/DispCtrl-linux-x64/          -- 独立部署目录
#   deploy/DispCtrl-linux-x64.tar.gz    -- 压缩包
##############################################################################
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_TYPE="${1:-Release}"
case "$BUILD_TYPE" in Release|Debug) ;; *) echo 'Expected Release or Debug' >&2; exit 2 ;; esac
BUILD_DIR="${PROJECT_DIR}/build/linux-${BUILD_TYPE,,}"
DEPLOY_DIR="${PROJECT_DIR}/deploy/DispCtrl-linux-x64"
APP_NAME="DispCtrl"

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "错误: 缺少命令 $1。请先安装 Linux 构建/打包依赖。" >&2
        exit 1
    fi
}

for command_name in qmake ldd tar gzip; do
    require_command "$command_name"
done

echo "============================================"
echo "  DispCtrl Linux Packaging"
echo "============================================"

# ---- 查找可执行文件 ----
APP_BINARY=""
for p in \
    "${BUILD_DIR}/bin/${BUILD_TYPE}/${APP_NAME}" \
    "${BUILD_DIR}/bin/${APP_NAME}" \
    "${BUILD_DIR}/${APP_NAME}"; do
    if [ -f "$p" ]; then
        APP_BINARY="$p"
        break
    fi
done

if [ -z "$APP_BINARY" ]; then
    echo "错误: 未找到编译产物。请先运行 build_linux.sh"
    exit 1
fi

if [ ! -f "${PROJECT_DIR}/config.toml" ]; then
    echo "错误: 未找到配置文件 ${PROJECT_DIR}/config.toml" >&2
    exit 1
fi

if [ ! -f "${PROJECT_DIR}/scripts/deploy_readme.txt" ]; then
    echo "错误: 未找到部署说明 ${PROJECT_DIR}/scripts/deploy_readme.txt" >&2
    exit 1
fi

if [ ! -f "${PROJECT_DIR}/scripts/install_offline_deps.sh" ]; then
    echo "错误: 未找到离线依赖安装脚本 ${PROJECT_DIR}/scripts/install_offline_deps.sh" >&2
    exit 1
fi

echo "  可执行文件: ${APP_BINARY}"
echo "  输出目录:   ${DEPLOY_DIR}"

# ---- 获取 Qt 路径 ----
QT_LIBS="$(qmake -query QT_INSTALL_LIBS)"
QT_PLUGINS="$(qmake -query QT_INSTALL_PLUGINS)"
QT_LIBEXECS="$(qmake -query QT_INSTALL_LIBEXECS)"
QT_DATA="$(qmake -query QT_INSTALL_DATA)"
QT_TRANSLATIONS="$(qmake -query QT_INSTALL_TRANSLATIONS)"

echo "  Qt libs:    ${QT_LIBS}"
echo "  Qt plugins: ${QT_PLUGINS}"
echo "  Qt libexec: ${QT_LIBEXECS}"
echo "  Qt data:    ${QT_DATA}"
echo "============================================"
echo ""

# ---- 清理并创建目录结构 ----
# Keep deletion constrained to the known deploy child even when invoked from Docker/WSL.
EXPECTED_DEPLOY_DIR="$(realpath -m "${PROJECT_DIR}")/deploy/DispCtrl-linux-x64"
if [[ "$(realpath -m "${DEPLOY_DIR}")" != "${EXPECTED_DEPLOY_DIR}" || -L "${DEPLOY_DIR}" ]]; then
    echo "Refusing unexpected deployment output path: ${DEPLOY_DIR}" >&2
    exit 1
fi
if [[ -d "${DEPLOY_DIR}" ]]; then
    chmod -R u+w "${DEPLOY_DIR}" 2>/dev/null || true
    rm -rf "${DEPLOY_DIR}"
fi
mkdir -p "${DEPLOY_DIR}"/{bin,lib,plugins/{platforms,imageformats,xcbglintegrations},resources,translations}

##############################################################################
# 1. 复制可执行文件
##############################################################################
echo "[1/8] 复制可执行文件..."
cp "${APP_BINARY}" "${DEPLOY_DIR}/bin/"
chmod +x "${DEPLOY_DIR}/bin/${APP_NAME}"

##############################################################################
# 2. 复制项目资源文件（config、HTML地图、QSS等）
##############################################################################
echo "[2/8] 复制项目资源..."

# config.toml
cp "${PROJECT_DIR}/config.toml" "${DEPLOY_DIR}/bin/"

# HTML 地图文件。缺失其中任意一个会导致对应地图页空白，打包时直接失败。
for web_asset in index.html indexNoL.html indexS.html index3d.html qwebchannel.js; do
    if [[ ! -f "${PROJECT_DIR}/htmls/${web_asset}" ]]; then
        echo "Missing required map asset: htmls/${web_asset}" >&2
        exit 1
    fi
done
[[ -d "${PROJECT_DIR}/htmls/amap" ]] || { echo "Missing required map asset directory: htmls/amap" >&2; exit 1; }
cp -r "${PROJECT_DIR}/htmls/"* "${DEPLOY_DIR}/bin/"
echo "  ✓ htmls/"

# QSS 样式表
if [ -d "${PROJECT_DIR}/resources/style" ]; then
    mkdir -p "${DEPLOY_DIR}/bin/resources/style"
    cp "${PROJECT_DIR}/resources/style/"*.qss "${DEPLOY_DIR}/bin/resources/style/" 2>/dev/null || true
    echo "  ✓ resources/style/"
fi

# 图标资源
if [ -d "${PROJECT_DIR}/resources/icon" ]; then
    mkdir -p "${DEPLOY_DIR}/bin/resources/icon"
    cp -r "${PROJECT_DIR}/resources/icon/"* "${DEPLOY_DIR}/bin/resources/icon/" 2>/dev/null || true
    echo "  ✓ resources/icon/"
fi

# 瓦片地图文件夹 (如果存在于项目目录)
for tiledir in mapNoL map16 map16S; do
    if [ -d "${PROJECT_DIR}/${tiledir}" ]; then
        cp -r "${PROJECT_DIR}/${tiledir}" "${DEPLOY_DIR}/bin/"
        echo "  ✓ ${tiledir}/"
    fi
done

##############################################################################
# 3. 复制 Qt 共享库
##############################################################################
echo "[3/8] 复制 Qt 共享库..."

# 需要的 Qt 模块列表
QT_MODULES=(
    Core Gui Widgets Network
    WebEngineCore WebEngineWidgets WebChannel
    DBus XcbQpa
    Quick Qml QmlModels QuickWidgets
    Positioning PrintSupport
    OpenGL Svg
)

COPIED_QT=0

# Qt 5.15.2 on Linux links QtCore/QtNetwork against its bundled ICU 56
# libraries. They are not guaranteed to be present on the target system, so
# keep the Qt ICU runtime beside the other bundled libraries. Only copy the
# SONAME entries (for example libicuuc.so.56); cp -L stores each target once.
COPIED_ICU=0
for lib in "${QT_LIBS}"/libicu*.so*; do
    if [ -f "$lib" ]; then
        bn=$(basename "$lib")
        if [[ ! "$bn" =~ \.so\.[0-9]+$ ]]; then
            continue
        fi
        if [ ! -f "${DEPLOY_DIR}/lib/${bn}" ]; then
            cp -L "$lib" "${DEPLOY_DIR}/lib/"
            COPIED_ICU=$((COPIED_ICU + 1))
        fi
    fi
done
echo "  Qt ICU libraries copied: ${COPIED_ICU}"

for mod in "${QT_MODULES[@]}"; do
    # Qt5 库文件名模式: libQt5XXX.so.5.x.x, 我们复制 .so.5 符号链接指向的实际文件
    for lib in "${QT_LIBS}/libQt5${mod}.so.5"*; do
        if [ -f "$lib" ]; then
            bn=$(basename "$lib")
            if [ ! -f "${DEPLOY_DIR}/lib/${bn}" ]; then
                cp -L "$lib" "${DEPLOY_DIR}/lib/"
                COPIED_QT=$((COPIED_QT + 1))
            fi
        fi
    done
done
echo "  ✓ 复制了 ${COPIED_QT} 个 Qt 库文件"

##############################################################################
# 4. 复制 Qt 插件
##############################################################################
echo "[4/8] 复制 Qt 插件..."

# platforms 插件 (xcb 是 Linux 桌面必需)
for plugin in libqxcb.so libqminimal.so libqoffscreen.so; do
    if [ -f "${QT_PLUGINS}/platforms/${plugin}" ]; then
        cp "${QT_PLUGINS}/platforms/${plugin}" "${DEPLOY_DIR}/plugins/platforms/"
        echo "  ✓ platforms/${plugin}"
    fi
done

# imageformats 插件
for f in "${QT_PLUGINS}/imageformats/"*.so; do
    if [ -f "$f" ]; then
        cp "$f" "${DEPLOY_DIR}/plugins/imageformats/"
    fi
done
echo "  ✓ imageformats/"

# xcbglintegrations 插件
if [ -d "${QT_PLUGINS}/xcbglintegrations" ]; then
    for f in "${QT_PLUGINS}/xcbglintegrations/"*.so; do
        if [ -f "$f" ]; then
            cp "$f" "${DEPLOY_DIR}/plugins/xcbglintegrations/"
        fi
    done
    echo "  ✓ xcbglintegrations/"
fi

# bearer 插件 (网络用)
if [ -d "${QT_PLUGINS}/bearer" ]; then
    mkdir -p "${DEPLOY_DIR}/plugins/bearer"
    for f in "${QT_PLUGINS}/bearer/"*.so; do
        [ -f "$f" ] && cp "$f" "${DEPLOY_DIR}/plugins/bearer/"
    done
    echo "  ✓ bearer/"
fi

##############################################################################
# 5. 复制 WebEngine 资源（关键！）
##############################################################################
echo "[5/8] 复制 WebEngine 资源..."

# QtWebEngineProcess 辅助进程
if [ -f "${QT_LIBEXECS}/QtWebEngineProcess" ]; then
    cp "${QT_LIBEXECS}/QtWebEngineProcess" "${DEPLOY_DIR}/bin/"
    chmod +x "${DEPLOY_DIR}/bin/QtWebEngineProcess"
    echo "  ✓ QtWebEngineProcess"
else
    echo "Missing QtWebEngineProcess: ${QT_LIBEXECS}" >&2
    exit 1
fi

# .pak 资源文件和 ICU 数据
RESOURCES_DIR="${QT_DATA}/resources"
if [ -d "${RESOURCES_DIR}" ]; then
    for f in "${RESOURCES_DIR}"/*.pak "${RESOURCES_DIR}"/icudtl.dat; do
        if [ -f "$f" ]; then
            cp "$f" "${DEPLOY_DIR}/resources/"
        fi
    done
    echo "  ✓ WebEngine .pak 资源"
else
    # 部分发行版将 WebEngine 资源放在 lib 目录下
    ALT_RES="${QT_LIBS}/qt5/resources"
    if [ -d "$ALT_RES" ]; then
        cp "$ALT_RES"/*.pak "${DEPLOY_DIR}/resources/" 2>/dev/null || true
        cp "$ALT_RES"/icudtl.dat "${DEPLOY_DIR}/resources/" 2>/dev/null || true
        echo "  ✓ WebEngine .pak 资源 (备用路径)"
    else
        echo "  ⚠ WebEngine 资源未找到"
    fi
fi

# WebEngine 本地化文件
LOCALES_DIR="${QT_TRANSLATIONS}/qtwebengine_locales"
if [ -d "${LOCALES_DIR}" ]; then
    mkdir -p "${DEPLOY_DIR}/translations/qtwebengine_locales"
    cp "${LOCALES_DIR}"/*.pak "${DEPLOY_DIR}/translations/qtwebengine_locales/"
    echo "  ✓ WebEngine 本地化数据"
else
    # 备用路径
    ALT_LOC="${QT_DATA}/translations/qtwebengine_locales"
    if [ -d "$ALT_LOC" ]; then
        mkdir -p "${DEPLOY_DIR}/translations/qtwebengine_locales"
        cp "$ALT_LOC"/*.pak "${DEPLOY_DIR}/translations/qtwebengine_locales/"
        echo "  ✓ WebEngine 本地化数据 (备用路径)"
    fi
fi

##############################################################################
# 6. 收集系统库依赖
##############################################################################
echo "[6/8] 收集库依赖..."

# 不需要打包的系统基础库（目标系统上一定存在）
SKIP_PATTERN="^(linux-vdso|ld-linux|libc\.so|libm\.so|libdl\.so|libpthread\.so"
SKIP_PATTERN+="|librt\.so|libresolv\.so"
# DISPCTRL_BUNDLE_STDCPP=1 时捆绑 libstdc++ (用于 18.04 目标:
#   GCC 9 的 libstdc++ GLIBCXX_3.4.28 > Ubuntu 18.04 系统自带的 3.4.25)
if [ "${DISPCTRL_BUNDLE_STDCPP:-0}" != "1" ]; then
    SKIP_PATTERN+="|libstdc\+\+|libgcc_s"
else
    echo "  (DISPCTRL_BUNDLE_STDCPP=1: 将捆绑 libstdc++.so)"
fi
# X11/xcb 核心库（目标机装了桌面环境就一定有）
SKIP_PATTERN+="|libX11\.so|libX11-xcb\.so"
# 注意: libxcb-*.so 和 libxkbcommon*.so 不跳过，
# 这些 xcb 扩展库在最小安装的 Ubuntu 上可能缺失
SKIP_PATTERN+="|libwayland|libEGL|libGL\.so|libGLX|libGLdispatch|libdrm"
SKIP_PATTERN+="|libfontconfig|libfreetype"
SKIP_PATTERN+="|libexpat|libz\.so|libbz2|libpng|libjpeg"
SKIP_PATTERN+="|libffi|libdbus|libsystemd"
SKIP_PATTERN+="|libglib|libgobject|libgio|libgmodule|libgthread"
SKIP_PATTERN+="|libnss|libnspr|libsmime|libssl|libcrypto"
SKIP_PATTERN+="|libpulse|libasound)"

collect_deps() {
    local binary="$1"
    local dependencies
    dependencies=$(ldd "$binary") || { echo "ldd failed: ${binary}" >&2; return 1; }
    if [[ "$dependencies" == *"not found"* ]]; then
        echo "Unresolved dependencies: ${binary}" >&2
        echo "$dependencies" >&2
        return 1
    fi
    while IFS= read -r lib; do
        local bn
        bn=$(basename "$lib")
        if echo "$bn" | grep -qE "$SKIP_PATTERN"; then
            continue
        fi
        if [ -f "$lib" ] && [ ! -f "${DEPLOY_DIR}/lib/${bn}" ]; then
            cp -L "$lib" "${DEPLOY_DIR}/lib/"
            echo "    + ${bn}"
        fi
    done < <(printf '%s\n' "$dependencies" | awk '/=> \/[[:alnum:]_.+\/-]+/ {print $3}' | sort -u)
}

# 收集主程序、WebEngine 子进程、Qt 库和插件的递归依赖。
# 每轮重新扫描，直到没有新增库，避免遗漏“依赖的依赖”。
collect_deps "${DEPLOY_DIR}/bin/${APP_NAME}"
if [ -f "${DEPLOY_DIR}/bin/QtWebEngineProcess" ]; then
    collect_deps "${DEPLOY_DIR}/bin/QtWebEngineProcess"
fi

while true; do
    before_count=$(find "${DEPLOY_DIR}/lib" -type f -name '*.so*' | wc -l)

    while IFS= read -r dependency_file; do
        collect_deps "$dependency_file"
    done < <(find "${DEPLOY_DIR}/lib" -type f -name '*.so*' -print)

    while IFS= read -r plugin_file; do
        collect_deps "$plugin_file"
    done < <(find "${DEPLOY_DIR}/plugins" -type f -name '*.so' -print)

    after_count=$(find "${DEPLOY_DIR}/lib" -type f -name '*.so*' | wc -l)
    if [ "$after_count" -le "$before_count" ]; then
        break
    fi
done

# ---- 显式收集 xcb 扩展库 (部分通过 dlopen 加载，ldd 扫不到) ----
echo "  收集 xcb 扩展库 (离线部署必需)..."
XCB_LIBS=(
    libxcb-xinerama.so.0
    libxcb-cursor.so.0
    libxcb-icccm.so.4
    libxcb-image.so.0
    libxcb-keysyms.so.1
    libxcb-render-util.so.0
    libxcb-shape.so.0
    libxcb-shm.so.0
    libxcb-sync.so.1
    libxcb-xfixes.so.0
    libxcb-randr.so.0
    libxcb-render.so.0
    libxcb-glx.so.0
    libxcb-xkb.so.1
    libxkbcommon.so.0
    libxkbcommon-x11.so.0
    libxcb.so.1
    libxcb-util.so.1
    libxcb-util.so.0
)
XCB_SEARCH_DIRS=("/usr/lib/x86_64-linux-gnu" "/usr/lib64" "/lib/x86_64-linux-gnu")
XCB_COPIED=0
for xcblib in "${XCB_LIBS[@]}"; do
    [ -f "${DEPLOY_DIR}/lib/${xcblib}" ] && continue
    for dir in "${XCB_SEARCH_DIRS[@]}"; do
        if [ -f "${dir}/${xcblib}" ]; then
            cp -L "${dir}/${xcblib}" "${DEPLOY_DIR}/lib/"
            XCB_COPIED=$((XCB_COPIED + 1))
            break
        fi
    done
done
echo "  ✓ 额外收集了 ${XCB_COPIED} 个 xcb 库"

echo "  ✓ 依赖收集完成"

##############################################################################
# 7. 修补 RPATH
##############################################################################
echo "[7/8] 修补 RPATH..."

if command -v patchelf &>/dev/null; then
    # 主程序: 从 bin/ 查找 ../lib
    patchelf --set-rpath '$ORIGIN/../lib' "${DEPLOY_DIR}/bin/${APP_NAME}"
    echo "  ✓ ${APP_NAME}"

    # QtWebEngineProcess: 同样从 bin/ 查找 ../lib
    if [ -f "${DEPLOY_DIR}/bin/QtWebEngineProcess" ]; then
        patchelf --set-rpath '$ORIGIN/../lib' "${DEPLOY_DIR}/bin/QtWebEngineProcess"
        echo "  ✓ QtWebEngineProcess"
    fi

    # lib/ 下的库: 从 lib/ 查找自身目录
    for f in "${DEPLOY_DIR}/lib/"*.so*; do
        if [ -f "$f" ] && [ ! -L "$f" ]; then
            patchelf --set-rpath '$ORIGIN' "$f" 2>/dev/null || true
        fi
    done
    echo "  ✓ 所有库文件"
else
    echo "  ⚠ patchelf 未安装，跳过 RPATH 修补"
    echo "    安装: sudo apt install patchelf"
    echo "    不修补 RPATH 时需依赖 run.sh 中的 LD_LIBRARY_PATH"
fi

# 将部署说明放入发布目录根部，便于用户解压后直接查看。
cp "${PROJECT_DIR}/scripts/deploy_readme.txt" "${DEPLOY_DIR}/readme.txt"
echo "  ✓ readme.txt"
cp "${PROJECT_DIR}/scripts/check_linux_package.sh" "${DEPLOY_DIR}/check_package.sh"
chmod +x "${DEPLOY_DIR}/check_package.sh"
if [[ -f "${PROJECT_DIR}/docs/Ubuntu发布说明.md" ]]; then
    cp "${PROJECT_DIR}/docs/Ubuntu发布说明.md" "${DEPLOY_DIR}/README.md"
fi
APP_VERSION=$(sed -n 's/^#define APP_VERSION_STR "\([^"]*\)"/\1/p' "${PROJECT_DIR}/Basic/DispBasci.h")
[[ "$APP_VERSION" =~ ^V?[0-9]+\.[0-9]+(\.[0-9]+)?$ ]] || { echo "Cannot read APP_VERSION_STR" >&2; exit 1; }
printf '%s\n' "$APP_VERSION" > "${DEPLOY_DIR}/VERSION"
cp "${PROJECT_DIR}/scripts/install_offline_deps.sh" "${DEPLOY_DIR}/"
chmod +x "${DEPLOY_DIR}/install_offline_deps.sh"
echo "  ✓ install_offline_deps.sh"

##############################################################################
# 8. 创建启动脚本
##############################################################################
echo "[8/8] 创建启动脚本..."

cat > "${DEPLOY_DIR}/run.sh" << 'EOF'
#!/bin/bash
##############################################################################
# DispCtrl 启动脚本
# 设置正确的运行环境变量后启动程序
##############################################################################
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 动态库搜索路径
export LD_LIBRARY_PATH="${SCRIPT_DIR}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

# Qt 插件路径
export QT_PLUGIN_PATH="${SCRIPT_DIR}/plugins"

# WebEngine 关键路径
export QTWEBENGINEPROCESS_PATH="${SCRIPT_DIR}/bin/QtWebEngineProcess"
export QTWEBENGINE_RESOURCES_PATH="${SCRIPT_DIR}/resources"
export QTWEBENGINE_LOCALES_PATH="${SCRIPT_DIR}/translations/qtwebengine_locales"

# ---------- WebEngine Sandbox (Linux 必需) ----------
# Keep Chromium sandbox enabled by default. Set DISPCTRL_DISABLE_SANDBOX=1 only
# when the target desktop has explicitly been verified not to support it.
SANDBOX_FLAGS=""
if [ "${DISPCTRL_DISABLE_SANDBOX:-0}" = 1 ]; then
    SANDBOX_FLAGS="--no-sandbox --disable-gpu-sandbox"
fi

# ---------- WebEngine GPU 兼容性 ----------
# 如果目标机器没有独立显卡或显卡驱动不完整，WebGL 可能被禁用。
# 默认启用忽略 GPU 黑名单（适合大多数场景）。
# 如仍有问题，可切换到方案2或方案3。
#
# 方案1: 忽略 GPU 黑名单 (默认启用，大多数情况下足够)
GPU_FLAGS="--ignore-gpu-blocklist --enable-gpu-rasterization"
#
# 方案2: 完全禁用 GPU 加速 (最安全，但地图性能略低)
# GPU_FLAGS="--disable-gpu --disable-gpu-compositing"
#
# 方案3: 使用软件 OpenGL 渲染 (取消注释以下两行)
# export QT_QUICK_BACKEND=software
# export LIBGL_ALWAYS_SOFTWARE=1
# ------------------------------------------

export QTWEBENGINE_CHROMIUM_FLAGS="${QTWEBENGINE_CHROMIUM_FLAGS:-} ${SANDBOX_FLAGS} ${GPU_FLAGS}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"

# 切换到 bin 目录（确保 config.toml 和 html 资源的相对路径正确）
cd "${SCRIPT_DIR}/bin"
exec ./DispCtrl "$@"
EOF
chmod +x "${DEPLOY_DIR}/run.sh"

cat > "${DEPLOY_DIR}/bin/qt.conf" << 'EOF'
[Paths]
Prefix=..
Libraries=lib
Plugins=plugins
Translations=translations
LibraryExecutables=bin
Data=.
EOF

# ---- install_desktop.sh (桌面快捷方式安装脚本) ----
if [ -f "${PROJECT_DIR}/scripts/install_desktop.sh" ]; then
    cp "${PROJECT_DIR}/scripts/install_desktop.sh" "${DEPLOY_DIR}/"
    chmod +x "${DEPLOY_DIR}/install_desktop.sh"
    echo "  ✓ install_desktop.sh"
fi

bash "${DEPLOY_DIR}/check_package.sh"

# ---- 统计 ----
TOTAL_SIZE=$(du -sh "${DEPLOY_DIR}" | cut -f1)
LIB_COUNT=$(find "${DEPLOY_DIR}/lib" -name "*.so*" -type f | wc -l)
PLUGIN_COUNT=$(find "${DEPLOY_DIR}/plugins" -name "*.so" -type f | wc -l)

echo ""
echo "============================================"
echo "  打包完成!"
echo "============================================"
echo "  部署目录: ${DEPLOY_DIR}"
echo "  总大小:   ${TOTAL_SIZE}"
echo "  库文件:   ${LIB_COUNT} 个"
echo "  插件:     ${PLUGIN_COUNT} 个"
echo ""

# ---- 创建 tar.gz ----
TARBALL="${PROJECT_DIR}/deploy/DispCtrl-linux-x64.tar.gz"
echo "创建压缩包: ${TARBALL}"
cd "${PROJECT_DIR}/deploy"
tar czf "DispCtrl-linux-x64.tar.gz" "DispCtrl-linux-x64/"
sha256sum "DispCtrl-linux-x64.tar.gz" > "DispCtrl-linux-x64.tar.gz.sha256"
TARBALL_SIZE=$(du -h "${TARBALL}" | cut -f1)

echo ""
echo "============================================"
echo "  发布包: ${TARBALL} (${TARBALL_SIZE})"
echo ""
echo "  部署到目标机:"
echo "    scp ${TARBALL} user@target:~/"
echo "    ssh user@target 'tar xzf DispCtrl-linux-x64.tar.gz'"
echo "    ssh user@target 'cd DispCtrl-linux-x64 && ./run.sh'"
echo ""
echo "  创建桌面快捷方式 (可选):"
echo "    ssh user@target 'cd DispCtrl-linux-x64 && bash install_desktop.sh'"
echo "============================================"
