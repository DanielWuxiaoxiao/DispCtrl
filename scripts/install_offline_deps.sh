#!/usr/bin/env bash
# Installs the Ubuntu-version-matched runtime .deb bundle shipped in a release.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [ ! -r /etc/os-release ]; then
    echo "Unsupported target: /etc/os-release is unavailable." >&2
    exit 1
fi

. /etc/os-release
if [ "${ID:-}" != "ubuntu" ]; then
    echo "Unsupported target: this offline dependency bundle is for Ubuntu." >&2
    exit 1
fi

case "${VERSION_ID:-}" in
    18.04) BUNDLE_NAME="ubuntu1804" ;;
    20.04) BUNDLE_NAME="ubuntu2004" ;;
    22.04) BUNDLE_NAME="ubuntu2204" ;;
    24.04) BUNDLE_NAME="ubuntu2404" ;;
    *)
        echo "Unsupported Ubuntu version: ${VERSION_ID:-unknown}." >&2
        echo "Available offline bundles: Ubuntu 18.04, 20.04, 22.04 and 24.04." >&2
        exit 1
        ;;
esac

DEPS_DIR="${SCRIPT_DIR}/offline-deps/${BUNDLE_NAME}"
if ! compgen -G "${DEPS_DIR}/*.deb" >/dev/null; then
    echo "Offline dependency bundle not found: ${DEPS_DIR}" >&2
    exit 1
fi

if [ "$(id -u)" -eq 0 ]; then
    RUN_AS_ROOT=()
elif command -v sudo >/dev/null 2>&1; then
    RUN_AS_ROOT=(sudo)
else
    echo "Administrator privileges are required. Run this script as root." >&2
    exit 1
fi

echo "Installing offline runtime dependencies for Ubuntu ${VERSION_ID}..."
export DEBIAN_FRONTEND=noninteractive

# A few Ubuntu 18.04 X11 packages pre-depend on multiarch-support. Install
# every locally available pre-dependency first, then unpack the whole closure.
pre_dep_names=$(for package in "${DEPS_DIR}"/*.deb; do
    dpkg-deb -f "$package" Pre-Depends 2>/dev/null || true
done | tr ',' '\n' | sed -E 's/^[[:space:]]*([A-Za-z0-9+.-]+).*/\1/' | sort -u)

while IFS= read -r pre_dep_name; do
    [ -z "$pre_dep_name" ] && continue
    for package in "${DEPS_DIR}"/*.deb; do
        if [ "$(dpkg-deb -f "$package" Package)" = "$pre_dep_name" ]; then
            "${RUN_AS_ROOT[@]}" dpkg --unpack "$package"
            "${RUN_AS_ROOT[@]}" dpkg --configure "$pre_dep_name"
            break
        fi
    done
done <<< "$pre_dep_names"

"${RUN_AS_ROOT[@]}" dpkg --unpack "${DEPS_DIR}"/*.deb || true
"${RUN_AS_ROOT[@]}" dpkg --configure -a

echo "Offline runtime dependencies installed successfully."
