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

# 以日常用户在图形桌面中首次启动，自动注册桌面/应用菜单及雷达图标
./run.sh

# 可选：手动修复入口或配置登录桌面后的自动启动
./install_desktop.sh
```

V6.0起，解压后在图形桌面首次运行`./run.sh`即自动创建当前用户的应用菜单和桌面入口（显示版本号及雷达图标），以后可以搜索“DispCtrl”/“雷达显控”或双击桌面图标运行。不弹安装询问，不默认开启登录自启动。部分桌面环境仍需右键图标选择“允许启动”；桌面图标是否可见也受系统桌面扩展设置影响，应用菜单入口不依赖该扩展。直接运行`bin/DispCtrl`不会自动注册，也不会配置发布包依赖路径。

`install_desktop.sh` 只写入当前用户的 XDG 应用菜单、桌面和可选 autostart 项，**不要用 sudo 运行**。正常启动自动注册最多等待5秒，失败不阻断程序；root或无图形会话不自动注册。可用`DISPCTRL_INSTALL_DESKTOP=0 ./run.sh`关闭自动注册。自动启动发生在该用户登录图形桌面后，不是系统服务；移动目录后重新运行新目录的`run.sh`可刷新入口，删除目录前建议卸载入口。卸载这些入口使用：

```bash
./install_desktop.sh --uninstall
```

离线依赖包只在 `1804` / `2004` 兼容构建中生成，包含面向 Ubuntu 18.04、20.04、22.04、24.04 的匹配 `.deb` 闭包。安装器不会覆盖系统提供的 ALSA 运行库，也不会执行全局 `dpkg --configure -a`。`2204` / `2404` 包依赖目标系统提供的对应基础库。

## V6.1运行资源来源

每次发布从代码目录的`config.toml`、`htmls/`（含`amap/`全部文件）、`resources/style/`及`resources/icon/`生成`bin/`资源，不直接依赖开发机`E:/DPRelease`。`copy_runtime_resources.sh`检查页面、WebChannel、高德脚本及图标是否完整，并生成`RUNTIME-RESOURCES.sha256`；`check_package.sh`检查静态资源摘要，防止漏包/改坏。部署后允许修改`config.toml`，它不参与静态资源摘要。

不打包CommandControlRecords、recordings、OSM数据、mapNoL/map16/map16S瓦片、运行日志、data.txt录制索引，也不混入Windows EXE/DLL或Windows版WebEngine资源。Qt库、Qt插件和WebEngine资源仍来自Linux构建环境。录制索引由运行期按需生成。

2026-10-28对比Windows发布目录：7个HTML/WebChannel文件一致，4个amap文件规范化换行后也一致；配置同步Windows已有字段值，保留源码新增字段及正常中文注释。默认卫星页需要另配map16S，排除瓦片不代表发布包自带离线底图。在线脚本/服务仍受网络及既有服务授权约束；已禁用的MapLibre入口仅保留原HTML，不增加Windows包也没有的MapLibre/PMTiles依赖。

## 实机验收

V6.1起1920×1200采用紧凑布局，固定字号统一跟随`bin/config.toml`中的`[ui].ui_scale`（0.70～1.60，默认1.0），修改后重启。启动文件日志`[UI_CONFIG]`记录实际读取路径、缩放和屏幕信息，可用于排除修改了另一份配置的问题。

“无地图”仍实际加载`bin/black.html`，不要求瓦片目录；该文件本轮明确设置html/body为黑底。通过文件日志`[MAP][LOAD_FINISHED] ok=1`和`[MAP][DOM]`的`ready=complete`、黑色背景及页面尺寸判断HTML/JS是否运行；仅看到黑色可能是Qt兜底，不能据此证明WebEngine/GPU正常。缺文件、加载失败、10秒超时和渲染进程退出均记录错误。离屏测试验证了HTML/JS，真实GPU首帧仍需现场检查。

路网/标准/卫星图分别需要`bin/mapNoL`、`bin/map16`、`bin/map16S`。所选目录缺失或为空时也实际加载`black.html`，保留下拉选择，不显示AMap灰绿色空底；其他类型有瓦片不能代替所选目录。已有目录但瓦片覆盖范围/缩放级别不完整时，仍需补齐数据。

`check_package.sh` 检查 WebEngine 资源、地图文件、Qt 插件和 `ldd` 依赖，**不会启动界面，也不会发送雷达报文**。最终验收仍须确认：

- Ubuntu 图形桌面与 X11/XWayland、显卡/OpenGL 驱动；
- Qt WebEngine 地图与本地离线资源；
- 现场网卡地址、UDP 端口、雷达/激光/指控链路；
- 真实航迹显示及协议联调。

V6.2（2026-10-28）用户确认临时关闭沙箱后exitCode=159和切图闪白消失，要求发布入口默认使用`--no-sandbox`。因此`run.sh`默认同时设置`QTWEBENGINE_DISABLE_SANDBOX=1`，不再需要额外输入命令；这会降低网页隔离保护，属于旧版WebEngine兼容绕过，不应加载不可信外部内容。兼容的目标机可用`DISPCTRL_DISABLE_SANDBOX=0 ./run.sh`恢复默认沙箱策略（若外部另设了`--no-sandbox`须先清除）。不关闭GPU，Windows策略不变。文件日志`[WEBENGINE_CONFIG]`记录实际系统、Qt、沙箱环境和最终Chromium参数。
