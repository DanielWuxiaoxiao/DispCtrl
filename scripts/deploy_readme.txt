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

4. Start in an Ubuntu graphical desktop session
   ./run.sh

5. Optional application-menu, desktop and login-session autostart entry
   ./install_desktop.sh

Do not run install_desktop.sh with sudo. The autostart option begins the
application after this user logs into the graphical desktop; it is not a
system service. Keep the extracted directory in a fixed location after
installing the desktop entry. To remove those entries:
   ./install_desktop.sh --uninstall

The package checks file/resource and dynamic-library availability only. Final
acceptance still requires desktop launch, GPU/WebEngine map and radar-network
verification on the target machine.
