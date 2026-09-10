# X576 Linux Build and Package Guide

This document is for the developer workstation. It describes the local WSL +
Docker build flow. The `readme.txt` inside the generated package is for the
target machine operator.

## Build Model

The source tree remains on Windows at `D:\X576`. WSL accesses it at
`/mnt/d/X576`. Docker runs the compiler, Qt 5.15.2, and packaging tools inside
a clean Ubuntu container. Source is mounted read-only, copied to the
container's private filesystem for compilation, and only final deployment
output is written back to the local tree:

```text
Windows D:\X576
  -> WSL /mnt/d/X576
  -> Docker Ubuntu build container (/src is disposable)
  -> D:\X576\deploy\ubuntu1804\DispCtrl-linux-x64.tar.gz
```

This is the preferred workflow: target compatibility is controlled by the
container image instead of the developer workstation's Qt, compiler, or WSL
version. The target machine only receives the release package and does not
need build tools.

## Prerequisites

Run the build from a WSL Ubuntu distribution with Docker available:

```bash
wsl -l -v
wsl -d <your-wsl-distribution>
docker info
```

If Docker is not installed in that WSL distribution:

```bash
sudo apt update
sudo apt install -y docker.io
sudo usermod -aG docker $USER
```

Restart WSL after changing the Docker group membership. Docker, Qt, GCC, CMake,
and Ninja are installed inside the build container; they do not need to be
installed in the project directory or on the target machine.

## Recommended Build Command

From a WSL terminal:

```bash
cd /mnt/d/X576
./docker/docker_build.sh 1804
```

From Windows PowerShell:

```powershell
.\docker\build_ubuntu.ps1 -Target 1804 -CheckOnly
.\docker\build_ubuntu.ps1 -Target 1804 -Jobs 4
```

`1804` is the recommended target for one package that supports both Ubuntu
18.04, Ubuntu 20.04, Ubuntu 22.04, and Ubuntu 24.04. It also builds matching offline `.deb` runtime bundles
into the release package. `2004` is an alias for the same 18.04-compatible
build; do not build both unless a separate test record is needed.

Other targets:

```bash
./docker/docker_build.sh 2204
./docker/docker_build.sh 2404
./docker/docker_build.sh 1804 --check
```

## Output and Checks

The successful build generates:

```text
deploy/ubuntu1804/DispCtrl-linux-x64/
deploy/ubuntu1804/DispCtrl-linux-x64.tar.gz
deploy/ubuntu1804/DispCtrl-linux-x64.tar.gz.sha256
```

Useful local checks:

```bash
sha256sum -c deploy/ubuntu1804/DispCtrl-linux-x64.tar.gz.sha256
bash deploy/ubuntu1804/DispCtrl-linux-x64/check_package.sh
bash docker/test_offline_deps.sh deploy/ubuntu1804/DispCtrl-linux-x64
```

`check_package.sh` checks package resources and dynamic libraries without
launching the GUI or sending packets.

`docker/test_offline_deps.sh` starts clean Ubuntu 18.04, 20.04, 22.04, and 24.04 containers
without a network, installs the matching bundled `.deb` closure, then checks
both `DispCtrl` and `QtWebEngineProcess` with `ldd`.

## Target Machine and Offline Deployment

The package includes DispCtrl, Qt, Qt WebEngine resources, ICU, bundled
libstdc++, xcb extensions, launch scripts, and two offline dependency bundles:

```text
offline-deps/ubuntu1804/
offline-deps/ubuntu2004/
offline-deps/ubuntu2204/
offline-deps/ubuntu2404/
```

On the target machine, run `sudo ./install_offline_deps.sh` after extraction.
The installer selects the matching core runtime bundle and uses `dpkg` only;
it does not need a network connection. Qt WebEngine may require `libasound`,
but the installer preserves a target-provided ALSA runtime and always skips
optional ALSA UCM/topology packages, preventing avoidable Ubuntu 24.04 package
conflicts. It configures only packages from its release closure and does not
attempt to repair unrelated target-machine package state.

The package intentionally does not bundle glibc, the dynamic loader, X11
display services, or OpenGL/DRM GPU drivers. Those pieces must match the
target operating system and graphics hardware.

The package-root `README.md` and `readme.txt` contain the target-side sequence:
verify checksum, extract, optionally install offline dependencies, check the
package, start it, then optionally install application-menu/desktop/autostart
entries. The desktop installer is current-user only and must not be run with
`sudo`.

## Notes

- The first Docker build downloads the Ubuntu toolchain and Qt WebEngine, so it
  takes noticeably longer. Later builds reuse Docker cache.
- Building under `/mnt/d/X576` is convenient because output is immediately
  visible from Windows. For very frequent clean builds, copying the workspace
  into the WSL ext4 filesystem can improve file-system performance.
- A successful headless smoke test only validates process startup and library
  loading. Final release validation still needs a real Ubuntu desktop session
  with the target graphics driver and X11 display available.
