#!/usr/bin/env bash
# Verifies that the packaged offline runtime dependencies install without a network.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PACKAGE_DIR="${1:-${PROJECT_DIR}/deploy/ubuntu1804/DispCtrl-linux-x64}"

if [ ! -x "${PACKAGE_DIR}/install_offline_deps.sh" ]; then
    echo "Package is missing install_offline_deps.sh: ${PACKAGE_DIR}" >&2
    exit 1
fi

for ubuntu_version in 18.04 20.04 22.04 24.04; do
    echo "Testing offline dependency install on Ubuntu ${ubuntu_version}..."
    docker run --rm --network none \
        -v "${PACKAGE_DIR}:/package:ro" \
        "ubuntu:${ubuntu_version}" \
        bash -lc '
            set -e
            cd /package
            ./install_offline_deps.sh
            export LD_LIBRARY_PATH=/package/lib
            if ldd bin/DispCtrl | grep -q "not found"; then
                ldd bin/DispCtrl
                exit 1
            fi
            if ldd bin/QtWebEngineProcess | grep -q "not found"; then
                ldd bin/QtWebEngineProcess
                exit 1
            fi
        '
done

echo "Offline dependency validation passed for Ubuntu 18.04, 20.04, 22.04 and 24.04."
