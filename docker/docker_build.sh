#!/usr/bin/env bash
##############################################################################
# DispCtrl Docker build/package helper
#
# Usage:
#   ./docker/docker_build.sh              # default: Ubuntu 22.04 target
#   ./docker/docker_build.sh auto         # use the current WSL/host Ubuntu version
#   ./docker/docker_build.sh current      # same as auto
#   ./docker/docker_build.sh 1804         # Ubuntu 18.04+ compatible target
#   ./docker/docker_build.sh 2204         # Ubuntu 22.04+ target
#   ./docker/docker_build.sh 2404         # Ubuntu 24.04+ target
#
# Notes:
#   The Docker base image version controls the target runtime compatibility. It
#   does not need to match the WSL host version. Use 1804 when you need binaries
#   that can run on Ubuntu 18.04 and newer systems.
##############################################################################
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

TARGET="${1:-2204}"
DOCKER_BUILD_ARGS=()

show_usage() {
    cat <<EOF
Usage:
  ./docker/docker_build.sh              Build Ubuntu 22.04+ target
  ./docker/docker_build.sh auto         Build target matching current WSL/host Ubuntu
  ./docker/docker_build.sh current      Same as auto
  ./docker/docker_build.sh 1804         Build Ubuntu 18.04+ compatible target
  ./docker/docker_build.sh 2204         Build Ubuntu 22.04+ target
  ./docker/docker_build.sh 2404         Build Ubuntu 24.04+ target

The Docker base image version controls target runtime compatibility; it does
not need to match the WSL host version.
EOF
}

case "$TARGET" in
    -h|--help|help)
        show_usage
        exit 0
        ;;
esac

detect_host_ubuntu_version() {
    if [ -r /etc/os-release ]; then
        . /etc/os-release
        printf '%s' "${VERSION_ID:-}"
    fi
}

case "$TARGET" in
    auto|current|host)
        HOST_VERSION="$(detect_host_ubuntu_version)"
        case "$HOST_VERSION" in
            18.04) TARGET="1804" ;;
            20.04)
                echo "Ubuntu 20.04 host target is not configured separately."
                echo "Using Ubuntu 18.04+ compatible target instead."
                TARGET="1804"
                ;;
            22.04) TARGET="2204" ;;
            24.04) TARGET="2404" ;;
            *)
                echo "Unsupported or unknown host Ubuntu version: ${HOST_VERSION:-unknown}"
                echo "Falling back to Ubuntu 22.04 target. Pass 1804/2204/2404 explicitly if needed."
                TARGET="2204"
                ;;
        esac
        ;;
esac

case "$TARGET" in
    1804|18.04)
        DOCKERFILE="${PROJECT_DIR}/docker/Dockerfile.ubuntu1804"
        IMAGE_TAG="dispctrl-builder-1804"
        COMPAT_DESC="Ubuntu 18.04+ (glibc >= 2.27)"
        ;;
    2204|22.04)
        DOCKERFILE="${PROJECT_DIR}/docker/Dockerfile"
        IMAGE_TAG="dispctrl-builder-2204"
        COMPAT_DESC="Ubuntu 22.04+ (glibc >= 2.35)"
        DOCKER_BUILD_ARGS+=(--build-arg UBUNTU_VERSION=22.04)
        ;;
    2404|24.04)
        DOCKERFILE="${PROJECT_DIR}/docker/Dockerfile"
        IMAGE_TAG="dispctrl-builder-2404"
        COMPAT_DESC="Ubuntu 24.04+ (glibc >= 2.39)"
        DOCKER_BUILD_ARGS+=(--build-arg UBUNTU_VERSION=24.04)
        ;;
    *)
        echo "Unknown target: $TARGET"
        echo "Supported targets: auto/current, 1804, 2204, 2404"
        exit 1
        ;;
esac

echo "============================================"
echo "  DispCtrl Docker Build"
echo "============================================"
echo "  Project:    ${PROJECT_DIR}"
echo "  Target:     ${COMPAT_DESC}"
echo "  Dockerfile: $(basename "$DOCKERFILE")"
echo "  Image:      ${IMAGE_TAG}"
echo ""

if ! command -v docker >/dev/null 2>&1; then
    echo "Error: Docker is not installed."
    echo ""
    echo "Install in WSL:"
    echo "  sudo apt update"
    echo "  sudo apt install -y docker.io"
    echo "  sudo usermod -aG docker \$USER"
    echo "  # Restart WSL from PowerShell: wsl --shutdown"
    exit 1
fi

if ! docker info >/dev/null 2>&1; then
    echo "Docker daemon is not running; trying to start it..."
    sudo service docker start
    sleep 2
    if ! docker info >/dev/null 2>&1; then
        echo "Error: failed to start Docker daemon."
        echo "Run manually: sudo service docker start"
        exit 1
    fi
fi

mkdir -p "${PROJECT_DIR}/deploy"

echo "[1/3] Building Docker compile environment..."
docker build \
    -f "${DOCKERFILE}" \
    "${DOCKER_BUILD_ARGS[@]}" \
    -t "${IMAGE_TAG}" \
    "${PROJECT_DIR}"

echo ""
echo "[2/3] Building and packaging DispCtrl inside Docker..."
docker run --rm \
    -v "${PROJECT_DIR}:/src:rw" \
    -v "${PROJECT_DIR}/deploy:/output:rw" \
    "${IMAGE_TAG}"

echo ""
echo "[3/3] Checking artifact..."
TARBALL="${PROJECT_DIR}/deploy/DispCtrl-linux-x64.tar.gz"
if [ -f "$TARBALL" ]; then
    TARBALL_SIZE="$(du -h "$TARBALL" | cut -f1)"
    echo ""
    echo "============================================"
    echo "  Docker build/package succeeded"
    echo "============================================"
    echo "  Artifact: deploy/DispCtrl-linux-x64.tar.gz"
    echo "  Size:     ${TARBALL_SIZE}"
    echo "  Target:   ${COMPAT_DESC}"
    echo "============================================"
else
    echo "Error: artifact not found: ${TARBALL}"
    echo "Please check Docker build logs."
    exit 1
fi
