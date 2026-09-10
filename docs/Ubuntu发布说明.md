# X576 Ubuntu 发布与安装

本流程使用 Windows 源码目录、WSL 和 Docker 生成 Linux x86_64 发布包；它不会把 Windows 的 `exe` 转换为 Linux 程序。Docker 容器中进行干净编译，Windows/WSL 原有 `build/`、`deploy/` 和 Git 元数据不会混入发布产物。

## Windows 一键入口

在 PowerShell 中执行：

```powershell
cd D:\X576
# 只检查 WSL/Docker 可用性，不编译
.\docker\build_ubuntu.ps1 -Target 1804 -CheckOnly
# 生成 Ubuntu 18.04+ 兼容发布包
.\docker\build_ubuntu.ps1 -Target 1804 -Jobs 4
```

可选目标为 `1804`、`2004`、`2204`、`2404`。`1804` 是默认推荐值，使用 Ubuntu 18.04 / GCC 9 / Qt 5.15.2 构建，适合需要兼容较旧 Ubuntu 的现场；`2004` 复用该兼容基线。`2204` 和 `2404` 生成对应较新基线的包，不能承诺在更旧系统运行。

WSL 中也可直接执行：

```bash
cd /mnt/d/X576
bash docker/docker_build.sh 1804
```

输出位于 `deploy/ubuntu1804/`（目录名随目标变化）：

```text
DispCtrl-linux-x64.tar.gz
DispCtrl-linux-x64.tar.gz.sha256
DispCtrl-linux-x64/
```

包内 `BUILD-INFO.txt` 记录 UTC 构建时间、Qt、GCC、glibc 和源文件摘要。发布入口会检查压缩包 SHA-256；但该检查不能替代目标机 GUI、GPU、地图和雷达网络联调。

对含离线依赖包的 `1804/2004` 产物，可在开发机额外验证四个 Ubuntu 版本的 `.deb` 安装闭包：

```bash
bash docker/test_offline_deps.sh deploy/ubuntu1804/DispCtrl-linux-x64
```

## 目标机安装

将压缩包和 `.sha256` 文件复制到 Ubuntu x86_64 图形桌面机器。在固定安装目录下以日常用户执行：

```bash
sha256sum -c DispCtrl-linux-x64.tar.gz.sha256
tar -xzf DispCtrl-linux-x64.tar.gz
cd DispCtrl-linux-x64

# 18.04/2004 兼容包含 offline-deps 时：离线安装运行时依赖
sudo ./install_offline_deps.sh

# 不启动程序的资源和动态库检查
./check_package.sh

# 图形桌面中启动
./run.sh

# 可选：应用菜单、桌面图标和登录桌面后的自动启动
./install_desktop.sh
```

`install_desktop.sh` 只写入当前用户的 XDG 应用菜单、桌面和可选 autostart 项，**不要用 sudo 运行**。自动启动发生在该用户登录图形桌面后，不是系统服务；移动或删除解压目录后需要重新执行安装。卸载这些入口使用：

```bash
./install_desktop.sh --uninstall
```

离线依赖包只在 `1804` / `2004` 兼容构建中生成，包含面向 Ubuntu 18.04、20.04、22.04、24.04 的匹配 `.deb` 闭包。安装器不会覆盖系统提供的 ALSA 运行库，也不会执行全局 `dpkg --configure -a`。`2204` / `2404` 包依赖目标系统提供的对应基础库。

## 验证边界

`check_package.sh` 检查 WebEngine 资源、地图文件、Qt 插件和 `ldd` 依赖，**不会启动界面，也不会发送雷达报文**。最终验收仍须确认：

- Ubuntu 图形桌面与 X11/XWayland、显卡/OpenGL 驱动；
- Qt WebEngine 地图与本地离线资源；
- 现场网卡地址、UDP 端口、雷达/激光/指控链路；
- 真实航迹显示及协议联调。

默认保留 Chromium sandbox。只有在现场已确认 sandbox 不可用时，才使用 `DISPCTRL_DISABLE_SANDBOX=1 ./run.sh` 作为回退。
