#!/bin/bash
##############################################################################
# DispCtrl Docker 一键编译+打包脚本
#
# 用法:
#   ./docker/docker_build.sh          # 默认: Ubuntu 22.04 目标 (glibc ≥2.35)
#   ./docker/docker_build.sh 1804     # Ubuntu 18.04+ 目标 (glibc ≥2.27)
#   ./docker/docker_build.sh 2204     # 显式: Ubuntu 22.04+ 目标
#
# 前提: WSL 中已安装 Docker
#   sudo apt install docker.io
#   sudo usermod -aG docker $USER
#   (重启 WSL 后生效)
#
# 说明:
#   1. 根据目标选择对应 Dockerfile
#   2. 构建 Docker 镜像 (含完整 Qt 5.15 + WebEngine)
#   3. 在容器中执行 cmake 编译 + package_linux.sh 打包
#   4. 将 DispCtrl-linux-x64.tar.gz 输出到 deploy/ 目录
#
# 目标对比:
#   1804 — Ubuntu 18.04+, aqtinstall Qt 5.15.2, GCC 9, 捆绑 libstdc++
#   2204 — Ubuntu 22.04+, 系统 Qt 5.15.x, GCC 11, 更小体积
##############################################################################
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# ---- 选择目标平台 ----
TARGET="${1:-2204}"

case "$TARGET" in
    1804|18.04)
        DOCKERFILE="${PROJECT_DIR}/docker/Dockerfile.ubuntu1804"
        IMAGE_TAG="dispctrl-builder-1804"
        COMPAT_DESC="Ubuntu 18.04+ (glibc ≥ 2.27)"
        ;;
    2204|22.04|*)
        DOCKERFILE="${PROJECT_DIR}/docker/Dockerfile"
        IMAGE_TAG="dispctrl-builder"
        COMPAT_DESC="Ubuntu 22.04+ (glibc ≥ 2.35)"
        ;;
esac

echo "============================================"
echo "  DispCtrl Docker Build"
echo "============================================"
echo "  项目目录: ${PROJECT_DIR}"
echo "  目标平台: ${COMPAT_DESC}"
echo "  Dockerfile: $(basename ${DOCKERFILE})"
echo ""

# ---- 检查 Docker ----
if ! command -v docker &>/dev/null; then
    echo "错误: Docker 未安装"
    echo ""
    echo "安装步骤:"
    echo "  sudo apt update"
    echo "  sudo apt install -y docker.io"
    echo "  sudo usermod -aG docker \$USER"
    echo "  # 重启 WSL (PowerShell: wsl --shutdown)"
    exit 1
fi

# 检查 Docker daemon 是否运行
if ! docker info &>/dev/null 2>&1; then
    echo "Docker daemon 未运行，尝试启动..."
    sudo service docker start
    sleep 2
    if ! docker info &>/dev/null 2>&1; then
        echo "错误: 无法启动 Docker daemon"
        echo "请手动执行: sudo service docker start"
        exit 1
    fi
fi

# ---- 创建输出目录 ----
mkdir -p "${PROJECT_DIR}/deploy"

# ---- 构建 Docker 镜像 ----
echo "[1/3] 构建 Docker 编译环境 (首次需要下载，约5-10分钟)..."
docker build \
    -f "${DOCKERFILE}" \
    -t "${IMAGE_TAG}" \
    "${PROJECT_DIR}"

# ---- 在容器中编译+打包 ----
echo ""
echo "[2/3] 在 Docker 容器中编译+打包..."
docker run --rm \
    -v "${PROJECT_DIR}:/src:rw" \
    -v "${PROJECT_DIR}/deploy:/output:rw" \
    "${IMAGE_TAG}"

# ---- 检查产物 ----
echo ""
echo "[3/3] 检查产物..."
TARBALL="${PROJECT_DIR}/deploy/DispCtrl-linux-x64.tar.gz"
if [ -f "$TARBALL" ]; then
    TARBALL_SIZE=$(du -h "$TARBALL" | cut -f1)
    echo ""
    echo "============================================"
    echo "  Docker 编译打包成功!"
    echo "============================================"
    echo "  发布包: deploy/DispCtrl-linux-x64.tar.gz"
    echo "  大小:   ${TARBALL_SIZE}"
    echo "  兼容:   ${COMPAT_DESC}"
    echo ""
    echo "  部署到目标机:"
    echo "    scp deploy/DispCtrl-linux-x64.tar.gz user@target:~/"
    echo "    ssh user@target 'tar xzf DispCtrl-linux-x64.tar.gz'"
    echo "    ssh user@target 'cd DispCtrl-linux-x64 && ./run.sh'"
    echo "============================================"
else
    echo "错误: 未找到产物 ${TARBALL}"
    echo "请检查 Docker 编译日志"
    exit 1
fi
