DispCtrl Linux 安装说明
========================

适用系统：Ubuntu 18.04 / 20.04 / 22.04 / 24.04 x86_64。

1. 将 DispCtrl-linux-x64.tar.gz 复制到目标机后执行：

   tar -xzf DispCtrl-linux-x64.tar.gz
   cd DispCtrl-linux-x64

2. 首次安装运行库：

   chmod +x install_offline_deps.sh run.sh
   sudo ./install_offline_deps.sh

   该步骤使用发布包内的离线依赖，不需要联网。只需首次执行一次。

3. 启动软件：

   ./run.sh

目标机需要已安装 Ubuntu 图形桌面，并正确安装显卡驱动。请在图形桌面登录后的终端中启动软件。

如启动失败，请保留终端输出和日志文件，并联系软件提供方。
