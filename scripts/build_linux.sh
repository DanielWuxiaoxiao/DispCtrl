#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_TYPE="${1:-Release}"
BUILD_DIR="${PROJECT_DIR}/build/linux-${BUILD_TYPE,,}"

for tool in cmake g++ qmake; do
    command -v "$tool" >/dev/null || {
        echo "Missing required tool: $tool" >&2
        exit 1
    }
done

echo "Required packages: build-essential cmake ninja-build qtbase5-dev qt5-qmake patchelf"

GENERATOR="Unix Makefiles"
BUILD_ARGS=()
if command -v ninja >/dev/null; then
    GENERATOR="Ninja"
else
    BUILD_ARGS=(-- -j"$(nproc)")
fi

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" "${BUILD_ARGS[@]}"
