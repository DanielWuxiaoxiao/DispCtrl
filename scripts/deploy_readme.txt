DispCtrl Linux 安装说明
========================

适用系统：Ubuntu 18.04 / 20.04 / 22.04 / 24.04 x86_64。

1. 解压安装包：

   tar -xzf DispCtrl-linux-x64.tar.gz
   cd DispCtrl-linux-x64

2. 首次安装：

   chmod +x install_offline_deps.sh run.sh install_desktop.sh
   sudo ./install_offline_deps.sh

   此步骤不需要联网，只需执行一次。

3. 启动软件：

   ./run.sh

4. 可选：创建桌面和应用菜单快捷方式：

   ./install_desktop.sh

   之后可在应用菜单中搜索 DispCtrl，或双击桌面图标启动。

请在 Ubuntu 图形桌面登录后运行软件。
