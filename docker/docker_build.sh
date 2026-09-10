#!/usr/bin/env bash
# Windows/WSL -> Docker -> clean Ubuntu release workflow for X576 DispCtrl.
# The source mount is read-only; application builds happen only in /src inside
# the disposable container and output is written to deploy/ubuntu<TARGET>.
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
TARGET="${1:-1804}"
CHECK_ONLY=0
if [[ "$TARGET" = --help || "$TARGET" = -h ]]; then
    echo 'Usage: bash docker/docker_build.sh [1804|2004|2204|2404] [--check]'
    exit 0
fi
case "$TARGET" in
    1804|18.04) TARGET=1804; BASE=18.04; OFFLINE_DEPS_ENABLED=1 ;;
    2004|20.04) TARGET=2004; BASE=18.04; OFFLINE_DEPS_ENABLED=1 ;;
    2204|22.04) TARGET=2204; BASE=22.04; OFFLINE_DEPS_ENABLED=0 ;;
    2404|24.04) TARGET=2404; BASE=24.04; OFFLINE_DEPS_ENABLED=0 ;;
    *) echo "Unknown target: $TARGET" >&2; exit 2 ;;
esac
if [[ $# -gt 2 || ( -n "${2:-}" && "${2:-}" != --check ) ]]; then
    echo 'Only --check is accepted as the second argument.' >&2
    exit 2
fi
[[ "${2:-}" = --check ]] && CHECK_ONLY=1

JOBS="${DISPCTRL_BUILD_JOBS:-4}"
[[ "$JOBS" =~ ^[1-9][0-9]*$ ]] || { echo 'Invalid DISPCTRL_BUILD_JOBS' >&2; exit 2; }
command -v docker >/dev/null || { echo 'Docker is not available in this WSL/Linux environment.' >&2; exit 1; }
docker info >/dev/null
[[ "$(docker info --format '{{.OSType}}')" = linux ]] || { echo 'Docker must use Linux containers.' >&2; exit 1; }

if [[ "$CHECK_ONLY" = 1 ]]; then
    echo "Prerequisites checked: target Ubuntu ${TARGET}, Docker Linux containers, jobs=${JOBS}."
    exit 0
fi

DOCKERFILE="$PROJECT_DIR/docker/Dockerfile"
[[ "$BASE" = 18.04 ]] && DOCKERFILE="$PROJECT_DIR/docker/Dockerfile.ubuntu1804"
IMAGE="dispctrl-builder-${TARGET}"
OUTPUT="$PROJECT_DIR/deploy/ubuntu${TARGET}"
RELEASE_ROOT="$OUTPUT/DispCtrl-linux-x64"
EXPECTED_OUTPUT="$(realpath -m "$PROJECT_DIR")/deploy/ubuntu${TARGET}"
[[ "$(realpath -m "$OUTPUT")" = "$EXPECTED_OUTPUT" && ! -L "$OUTPUT" ]] || {
    echo "Refusing unexpected release output path: $OUTPUT" >&2; exit 1;
}

mkdir -p "$PROJECT_DIR/deploy"
if [[ -d "$OUTPUT" ]]; then
    chmod -R u+w "$OUTPUT" 2>/dev/null || true
    rm -rf "$OUTPUT"
fi
mkdir -p "$OUTPUT"

echo "Source: $PROJECT_DIR"
echo "Target: Ubuntu $TARGET x64 (builder base $BASE); jobs: $JOBS"
docker build --platform linux/amd64 --build-arg "UBUNTU_VERSION=$BASE" \
    -f "$DOCKERFILE" -t "$IMAGE" "$PROJECT_DIR"
docker run --rm --platform linux/amd64 \
    --mount "type=bind,source=$PROJECT_DIR,target=/input,readonly" \
    --mount "type=bind,source=$OUTPUT,target=/output" \
    -e "DISPCTRL_BUILD_JOBS=$JOBS" -e "DISPCTRL_TARGET=$TARGET" "$IMAGE"
test -d "$RELEASE_ROOT"

# The 18.04-compatible release keeps X576's existing multi-version offline
# dependency closure. Newer target baselines intentionally use host packages.
build_offline_deps() {
    local ubuntu_version="$1" bundle_name="$2" openssl_package="$3" alsa_package="$4"
    local image_name="dispctrl-offline-deps-${bundle_name}"
    local output_dir="$RELEASE_ROOT/offline-deps/${bundle_name}"
    mkdir -p "$output_dir"
    docker build --platform linux/amd64 -f "$PROJECT_DIR/docker/Dockerfile.offline-deps" \
        --build-arg "UBUNTU_VERSION=${ubuntu_version}" \
        --build-arg "OPENSSL_PACKAGE=${openssl_package}" \
        --build-arg "ALSA_PACKAGE=${alsa_package}" \
        -t "$image_name" "$PROJECT_DIR"
    docker run --rm --platform linux/amd64 \
        --mount "type=bind,source=$output_dir,target=/output" "$image_name"
    compgen -G "$output_dir/*.deb" >/dev/null || { echo "Offline bundle is empty: $output_dir" >&2; exit 1; }
}

if [[ "$OFFLINE_DEPS_ENABLED" = 1 ]]; then
    rm -rf "$RELEASE_ROOT/offline-deps"
    build_offline_deps 18.04 ubuntu1804 libssl1.1 libasound2
    build_offline_deps 20.04 ubuntu2004 libssl1.1 libasound2
    build_offline_deps 22.04 ubuntu2204 libssl3 libasound2
    build_offline_deps 24.04 ubuntu2404 libssl3 libasound2t64
fi

bash "$RELEASE_ROOT/check_package.sh"
TARBALL="$OUTPUT/DispCtrl-linux-x64.tar.gz"
tar -czf "$TARBALL.tmp" -C "$OUTPUT" DispCtrl-linux-x64
mv "$TARBALL.tmp" "$TARBALL"
(cd "$OUTPUT" && sha256sum DispCtrl-linux-x64.tar.gz > DispCtrl-linux-x64.tar.gz.sha256)
(cd "$OUTPUT" && sha256sum -c DispCtrl-linux-x64.tar.gz.sha256)
echo "Release ready: $TARBALL"
