#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
TARGET="${1:-2204}"

case "$TARGET" in
    1804|18.04)
        DOCKERFILE="$PROJECT_DIR/docker/Dockerfile.ubuntu1804"
        IMAGE_TAG="dispctrl-builder-1804"
        ;;
    2204|22.04|*)
        DOCKERFILE="$PROJECT_DIR/docker/Dockerfile"
        IMAGE_TAG="dispctrl-builder"
        ;;
esac

command -v docker >/dev/null || {
    echo "Docker is required." >&2
    exit 1
}
docker info >/dev/null 2>&1 || {
    echo "Docker daemon is not available." >&2
    exit 1
}

mkdir -p "$PROJECT_DIR/deploy"
docker build -f "$DOCKERFILE" -t "$IMAGE_TAG" "$PROJECT_DIR"
docker run --rm \
    -v "$PROJECT_DIR:/src:rw" \
    -v "$PROJECT_DIR/deploy:/output:rw" \
    "$IMAGE_TAG"

test -f "$PROJECT_DIR/deploy/DispCtrl-linux-x64.tar.gz"
echo "Package created: $PROJECT_DIR/deploy/DispCtrl-linux-x64.tar.gz"
