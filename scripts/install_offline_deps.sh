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

echo "Installing core offline runtime dependencies for Ubuntu ${VERSION_ID}..."
export DEBIAN_FRONTEND=noninteractive

# Keep UCM/topology packages owned by the target operating system. They are
# optional ALSA recommendations and can conflict with a newer Ubuntu desktop.
packages=("${DEPS_DIR}"/*.deb)
filtered_packages=()
target_has_alsa=0
if ldconfig -p 2>/dev/null | grep -q 'libasound\.so\.2'; then
    target_has_alsa=1
    echo "Target already provides libasound.so.2; preserving its ALSA packages."
fi

for package in "${packages[@]}"; do
    package_name=$(dpkg-deb -f "$package" Package)
    case "$package_name" in
        alsa-ucm-conf|alsa-topology-conf)
            echo "Skipping optional host-managed audio package: ${package_name}"
            continue
            ;;
        libasound2|libasound2t64|libasound2-data)
            if [ "$target_has_alsa" -eq 1 ]; then
                echo "Skipping target-provided audio package: ${package_name}"
                continue
            fi
            ;;
    esac
    filtered_packages+=("$package")
done
packages=("${filtered_packages[@]}")
package_names=()
for package in "${packages[@]}"; do
    package_names+=("$(dpkg-deb -f "$package" Package)")
done

# A few Ubuntu 18.04 X11 packages pre-depend on multiarch-support. Install
# every locally available pre-dependency first, then unpack the whole closure.
pre_dep_names=$(for package in "${packages[@]}"; do
    dpkg-deb -f "$package" Pre-Depends 2>/dev/null || true
done | tr ',' '\n' | sed -E 's/^[[:space:]]*([A-Za-z0-9+.-]+).*/\1/' | sort -u)

while IFS= read -r pre_dep_name; do
    [ -z "$pre_dep_name" ] && continue
    for package in "${packages[@]}"; do
        if [ "$(dpkg-deb -f "$package" Package)" = "$pre_dep_name" ]; then
            "${RUN_AS_ROOT[@]}" dpkg --unpack "$package"
            "${RUN_AS_ROOT[@]}" dpkg --configure "$pre_dep_name"
            break
        fi
    done
done <<< "$pre_dep_names"

"${RUN_AS_ROOT[@]}" dpkg --unpack "${packages[@]}" || true

# Do not use "dpkg --configure -a": it also retries unrelated packages that
# may already be broken on the target, such as a previously failed ALSA UCM
# installation. Configure only packages selected from this release closure.
while true; do
    pending=0
    progress=0
    for package_name in "${package_names[@]}"; do
        status=$(dpkg-query -W -f='${db:Status-Status}' "$package_name" 2>/dev/null || true)
        if [ "$status" = "installed" ]; then
            continue
        fi

        pending=1
        if "${RUN_AS_ROOT[@]}" dpkg --configure "$package_name" >/dev/null 2>&1; then
            progress=1
        fi
    done

    if [ "$pending" -eq 0 ]; then
        break
    fi
    if [ "$progress" -eq 0 ]; then
        echo "Failed to configure the release runtime dependency closure." >&2
        echo "Packages still pending:" >&2
        for package_name in "${package_names[@]}"; do
            status=$(dpkg-query -W -f='${db:Status-Status}' "$package_name" 2>/dev/null || true)
            if [ "$status" != "installed" ]; then
                echo "  ${package_name}: ${status:-unknown}" >&2
            fi
        done
        exit 1
    fi
done

echo "Core offline runtime dependencies installed successfully."
