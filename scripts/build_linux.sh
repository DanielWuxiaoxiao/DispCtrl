#!/bin/bash
##############################################################################
# DispCtrl Linux 编译脚本
# 用法: ./scripts/build_linux.sh [Release|Debug]
# 说明: 在 WSL 或 Linux 系统中使用 cmake 编译项目
##############################################################################
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_TYPE="${1:-Release}"
case "$BUILD_TYPE" in Release|Debug) ;; *) echo 'Expected Release or Debug' >&2; exit 2 ;; esac
BUILD_DIR="${PROJECT_DIR}/build/linux-${BUILD_TYPE,,}"

echo "============================================"
echo "  DispCtrl Linux Build"
echo "============================================"
echo "  项目目录: ${PROJECT_DIR}"
echo "  构建目录: ${BUILD_DIR}"
echo "  构建类型: ${BUILD_TYPE}"
echo "============================================"

# ---- 依赖检查 ----
MISSING=""
for cmd in cmake g++ qmake; do
    if ! command -v "$cmd" &>/dev/null; then
        MISSING="${MISSING} ${cmd}"
    fi
done

if [ -n "$MISSING" ]; then
    echo ""
    echo "错误: 缺少以下工具:${MISSING}"
    echo ""
    echo "请先安装编译依赖:"
    echo "  sudo apt update"
    echo "  sudo apt install -y build-essential cmake ninja-build \\"
    echo "      qtbase5-dev qt5-qmake qtwebengine5-dev \\"
    echo "      libqt5webchannel5-dev patchelf \\"
    echo "      libgl1-mesa-dev libxkbcommon-dev libfontconfig1-dev"
    exit 1
fi

echo ""
echo "系统 Qt 版本: $(qmake -query QT_VERSION 2>/dev/null || echo '未知')"
echo "GCC 版本: $(g++ --version | head -1)"
echo ""

# ---- 选择生成器 ----
GENERATOR="Unix Makefiles"
if command -v ninja &>/dev/null; then
    GENERATOR="Ninja"
    echo "使用 Ninja 生成器"
else
    echo "使用 Make 生成器 (安装 ninja-build 可加速编译)"
fi

# ---- Configure ----
echo ""
echo "[1/2] CMake Configure..."
cmake -S "${PROJECT_DIR}" \
      -B "${BUILD_DIR}" \
      -G "${GENERATOR}" \
      -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

# ---- Build ----
echo ""
echo "[2/2] CMake Build..."
cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}" --parallel "${DISPCTRL_BUILD_JOBS:-4}"

# ---- 查找编译产物 ----
BINARY=""
for p in \
    "${BUILD_DIR}/bin/${BUILD_TYPE}/DispCtrl" \
    "${BUILD_DIR}/bin/DispCtrl" \
    "${BUILD_DIR}/DispCtrl"; do
    if [ -f "$p" ]; then
        BINARY="$p"
        break
    fi
done

echo ""
echo "============================================"
if [ -n "$BINARY" ]; then
    echo "  编译成功!"
    echo "  可执行文件: ${BINARY}"
    echo "  文件大小: $(du -h "$BINARY" | cut -f1)"
    echo ""
    echo "  下一步: 运行打包脚本"
    echo "  ./scripts/package_linux.sh ${BUILD_TYPE}"
else
    echo "  编译完成，但未找到可执行文件"
    echo "  请检查 ${BUILD_DIR}/bin/ 目录"
    exit 1
fi
echo "============================================"
