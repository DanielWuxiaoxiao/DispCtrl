DispCtrl Linux installation
===========================

1. Verify and unpack
   sha256sum -c DispCtrl-linux-x64.tar.gz.sha256
   tar -xzf DispCtrl-linux-x64.tar.gz
   cd DispCtrl-linux-x64

2. Install offline runtime dependencies (only when offline-deps/ exists)
   sudo ./install_offline_deps.sh

3. Verify packaged resources and libraries
   ./check_package.sh

4. Start as your normal user in an Ubuntu graphical desktop session
   ./run.sh
   First launch automatically creates this user's application-menu and desktop
   entries with the radar icon and release version. Later launches refresh them
   if the release path/version changed. No prompt and no automatic login startup.
   Search for "DispCtrl" / "雷达显控", or use the desktop icon next time.
   If the desktop asks, right-click the icon and choose "Allow Launching".

5. Optional repair or login-session autostart configuration
   ./install_desktop.sh

Do not run install_desktop.sh with sudo. The autostart option begins the
application after this user logs into the graphical desktop; it is not a
system service. Keep the extracted directory in a fixed location after
installing the desktop entry (or run run.sh once again after moving it).
To opt out of automatic registration, use DISPCTRL_INSTALL_DESKTOP=0 ./run.sh.
To remove those entries:
   ./install_desktop.sh --uninstall

The package checks file/resource and dynamic-library availability only. Final
acceptance still requires desktop launch, GPU/WebEngine map and radar-network
verification on the target machine.

Linux WebEngine compatibility: run.sh now defaults to --no-sandbox plus
QTWEBENGINE_DISABLE_SANDBOX=1, as requested after target-machine exitCode=159
testing. This reduces web-content isolation; do not load untrusted content.
On compatible systems, use DISPCTRL_DISABLE_SANDBOX=0 ./run.sh to opt out of
the default workaround (remove externally supplied --no-sandbox flags too).
Missing/empty mapNoL, map16 or map16S for the selected map loads black.html;
another map's directory does not count. Existing but incomplete tile coverage
still requires the appropriate tile data.
