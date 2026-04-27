# DispCtrl - 雷达显示控制系统

Qt 5.14 / C++17 实现的雷达显示与管控应用，提供 PPI / 扇区可视化、航迹管理、地图叠加、多通道 UDP 通信以及外部雷控链路。

## 项目概览
- 支持实时雷达数据展示，覆盖 PPI、扇区和表格/提示等形态。
- 融合信号处理、数据处理、目标分类/监控/资源调度等多源数据流，统一在 Controller 层分发。
- 集成 Qt WebEngine 地图（高德 2D/3D/无图/暗色），支持地理叠加与坐标转换。
- 自定义 UI 组件与可拆分窗口，适配多显示器和专业暗色主题。
- 可配置的网络/IP/端口/ID、扫描范围、显示半径与地图样式，便于部署到不同现场。

## 功能特性
- 雷达显示：PPI 极坐标显示、可设扇区显示、扫描层动画、缩放/平移/鼠标位置提示、局部缩略图。
- 数据与控制：检测点/航迹管理（扇区与全局）、多线程 UDP 收发、外部雷控/伺服 512B/32B 链路、错误处理与日志。
- 地图与叠加：HTML5 地图渲染、2D/3D/无图模式切换、雷达中心/方位/姿态参数配置、地图/雷达半径联动。
- UI 与交互：可拆分窗口、自定义控件（ComboBox/MessageBox）、参数面板（波形、信号、波控、光电、存储、扫描范围等）、暗色 QSS。
- 扩展性：协议定义集中在 `Basic/Protocol.h`，新数据流可通过对应 *Manager* 接口分发到视图或表格；显示模块可按 Qt Graphics 扩展。

## 项目结构
```
DispCtrl/
├── Basic/            # 协议、配置、日志、数学工具、线程绑定
├── Controller/       # 控制器与数据分发：Data/Sig/Mon/Res/Photo/TBD/Target/ExternalCtrl
├── PolarDisp/        # PPI/扇区渲染：坐标轴、网格、场景、缩略图、提示等
├── PointManager/     # 检测点与航迹管理（含扇区版本）
├── UDP/              # 多线程 UDP Socket 封装
├── mapDisp/          # WebEngine 地图代理，Qt 与 JS 互通
├── mainPanel/        # 主界面布局与顶层面板
├── paramWidget/      # 参数配置对话框（信号、波控、扫描、光电、存储等）
├── cusWidgets/       # 可拆分窗口与自定义基础控件
├── htmls/            # Web 地图前端（2D/3D/无图/暗色）
├── resources/        # 图标、样式、QSS、翻译
├── docs/             # 协议与变更文档
├── scripts/          # 开发辅助脚本（如更新头文件注释）
├── tests/            # 轻量测试框架头
├── CMakeLists.txt    # CMake 构建入口（默认 Qt5 Widgets/WebEngine/WebChannel）
├── DispCtrl.pro      # Qt Creator/qmake 构建入口
└── config.toml       # 默认配置（启动时自动复制到构建输出目录）
```

## 核心模块与组件
- 控制与数据流：`Controller/controller.*`（总控），`RadarDataManager`（统一缓冲），`Data2DispManager`/`Sig2DispManager`/`TargetDispManager` 等分发到视图，`ExternalCtrlManager` 支持外部雷控/伺服链路。
- 雷达显示：`PolarDisp` 下的 `PpiView/PpiScene/ScanLayer/SectorWidget/PolarGrid/PolarAxis` 等实现极坐标网格、扇区渲染、缩放与提示。
- 点迹与航迹：`PointManager` 下 `DetManager/TrackManager` 及扇区版本负责数据维护与 UI 对接。
- UI 布局：`mainPanel/mainoverlayout.*` 组织主面板，`paramWidget/*` 提供各类参数对话框，`cusWidgets/*` 支持窗口分离与定制控件。
- 网络通信：`UDP/threadudpsocket.*` 封装多线程 UDP 收发、缓冲与回调。

## 数据流与协议
- 协议定义：详见 `docs/internal_protocol.md`（v5.0，小端、1B 对齐；帧头/尾兼容 512B 控制链路）。
- 默认数据流（部分）：检测点 `SIG_2_DISP_PORT1`、状态 `SIG_2_DISP_PORT2`、航迹 `DATA_PRO_2_DISP`、目标分类 `TAR_2_DISP`、监控 `MONITOR_GET_DISP_PORT`、扩展外部雷控 `EXT_SYSCTRL_SRC/EXT_SYSCTRL_DST/EXT_ACK_DST`。完整键值与端口参见 `config.toml`/`config_documentation.md`。
- 新增链路（`docs/changes_2025-12-25.md`）：外部雷控/伺服 512B/32B 占位发送与 ACK，接口暴露在 `ExternalCtrlManager` 与 `Controller::sendExternalSystemControl` 等。
- 待办/TBD 数据：`Tbd2DispManager` 已接入扩展数据通道，可分发到 PPI/扇区/表格。
- UI 可根据 `controller` 暴露的信号（如 `externalSystemCtrlAck`、`externalServoAck`、`bitReport` 等）展示状态。

## 配置
应用启动时加载根目录 `config.toml`（失败则使用默认值，并打印日志），同时 CMake 会复制到 `build/bin/<Config>/config.toml`。
关键段落：
- `network.ips/ports/ids`：各子系统 IP、端口与 ID（含外部雷控链路）。
- `radar`：雷达中心经纬高、姿态角。
- `polarDisp` 与 `sectorDisp`：PPI/扇区显示的距离、方位、俯仰范围。
- `map`：默认地图模式（无图/路网/标准/卫星）。
- `system`：日志级别、自动保存间隔；`webengine`：远程调试开关与端口。

示例（节选）：
```toml
[network.ports]
DATA_PRO_2_DISP = 8007      # 航迹数据流
SIG_2_DISP_PORT1 = 8008     # 检测点流
TAR_2_DISP = 8011           # 目标分析结果
EXT_SYSCTRL_SRC = 6001      # 外部雷控/伺服发送
EXT_SYSCTRL_DST = 8001      # 外部雷控接收
EXT_ACK_DST = 8002          # 外部雷控/伺服回执

[polarDisp.range]
min = 1
max = 5

[sectorDisp.angle]
min = -30
max = 30
```

## 构建与运行

### Windows 开发构建
依赖：Windows 10/11，Qt 5.14+（Widgets、Network、WebEngine、WebChannel），CMake ≥ 3.16，MSVC 2017+。
- VS Code（推荐）：安装 CMake Tools，`Ctrl+Shift+P` 选择 `CMake: Configure`，选好工具链后执行 `CMake: Build`。
- 命令行：
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Debug
```
- 运行：`./build/bin/Debug/DispCtrl.exe` 或对应 `Release` 目录；确保输出目录下存在 `config.toml` 与资源文件（通过 CMake 自定义目标已自动复制）。

---

## Linux 部署完整流程

完成 Windows 端编码后，按以下任一方式生成可在 Linux 上直接运行的部署包（`deploy/DispCtrl-linux-x64.tar.gz`）。

### 方式一：WSL 原生编译（推荐，无 Docker）

**前提：WSL2 + Ubuntu 已安装 Qt 5 开发库**

```bash
# 1. 安装编译依赖（首次）
sudo apt update
sudo apt install -y build-essential cmake ninja-build \
    qtbase5-dev qt5-qmake qtwebengine5-dev \
    libqt5webchannel5-dev patchelf \
    libgl1-mesa-dev libxkbcommon-dev libfontconfig1-dev

# 2. 进入项目目录
cd /mnt/d/DispCtrl/DispCtrl

# 3. 修复脚本换行符（首次只需执行一次）
find scripts docker -name "*.sh" -exec sed -i 's/\r//' {} \; -exec chmod +x {} \;

# 4. 编译（Release）
./scripts/build_linux.sh Release

# 5. 打包为独立部署目录 + tar.gz
./scripts/package_linux.sh Release
```

**产物：**
```
deploy/
├── DispCtrl-linux-x64/        # 独立目录（含 Qt 库、插件、资源）
│   ├── bin/DispCtrl           # 可执行文件
│   ├── lib/                   # Qt & xcb 动态库
│   ├── plugins/               # Qt 平台/图像插件
│   ├── resources/             # QSS、图标
│   ├── run.sh                 # 一键启动脚本（设置 LD_LIBRARY_PATH）
│   └── install_desktop.sh     # 安装到系统桌面快捷方式
└── DispCtrl-linux-x64.tar.gz  # 打包好的离线部署压缩包
```

---

### 方式二：Docker 编译（兼容旧系统 / 无需本地安装 Qt）

适用于需要兼容 **Ubuntu 18.04+（glibc ≥ 2.27）** 或本机没有安装 Qt 的场景。

**前提：WSL2 中已安装 Docker**

```bash
# 安装 Docker（首次）
sudo apt install -y docker.io
sudo usermod -aG docker $USER
# 重启 WSL：在 PowerShell 执行 wsl --shutdown，再重新打开 WSL

# 进入项目目录
cd /mnt/d/DispCtrl/DispCtrl

# 修复脚本换行符（首次）
find scripts docker -name "*.sh" -exec sed -i 's/\r//' {} \; -exec chmod +x {} \;

# 一键构建（Ubuntu 22.04 目标）
./docker/docker_build.sh

# 或构建 Ubuntu 18.04 兼容版（glibc 2.27，适合更旧的目标机器）
./docker/docker_build.sh 1804
```

Docker 脚本会自动完成：镜像构建 → cmake 编译 → package_linux.sh 打包 → 输出 `deploy/DispCtrl-linux-x64.tar.gz`。

---

### 部署到目标 Linux 机器

```bash
# 将 tar.gz 拷贝到目标机器后：
tar -xzf DispCtrl-linux-x64.tar.gz
cd DispCtrl-linux-x64

# 直接运行
./run.sh

# 或安装桌面快捷方式（可选）
chmod +x install_desktop.sh
./install_desktop.sh
```

> **注意：** 目标机器需要 X11 或 Wayland 图形环境（libxcb 等已打包在 `lib/` 中，无需额外安装 Qt）。

---

### 脚本说明汇总

| 脚本 | 用途 |
|------|------|
| `scripts/build_linux.sh [Release\|Debug]` | WSL 原生 cmake 编译 |
| `scripts/package_linux.sh [Release\|Debug]` | 打包编译产物 + Qt 依赖为独立目录 + tar.gz |
| `scripts/install_desktop.sh` | 在目标机器创建桌面快捷方式 |
| `docker/docker_build.sh [1804\|2204]` | Docker 一键编译+打包（无需本地 Qt） |
| `docker/Dockerfile` | Ubuntu 22.04 构建镜像定义 |
| `docker/Dockerfile.ubuntu1804` | Ubuntu 18.04 构建镜像（兼容旧 glibc） |

## 开发指引
- 新增显示模块：继承 `QGraphicsItem` 或 `QGraphicsScene`，实现 `paint()`/`boundingRect()`，在对应 Manager 注册，并更新 `CMakeLists.txt`。
- 扩展协议/数据流：修改 `Basic/Protocol.h`，在相关 *Manager* 增加解析与分发逻辑，必要时补充 `config.toml` 的端口/ID。
- 地图前端：`htmls/` 下为 WebEngine 页面，可根据需要调整地图样式或坐标转换逻辑。
- 日志与错误：`Basic/log.*`、`Controller/ErrorHandler.*` 提供统一日志入口和错误处理，主程序通过 `qInstallMessageHandler` 挂载。

## 文档与参考
- 协议：`docs/internal_protocol.md`
- 配置说明：`config_documentation.md`
- 最近变更：`docs/changes_2025-12-25.md`

## 许可证与贡献
- 许可证：MIT License（若仓库缺少 `LICENSE` 文件，请补充后分发）。
- 贡献流程：Fork -> 创建分支 (`git checkout -b feature/...`) -> 提交 (`git commit -m "..."`) -> Push -> 提 PR。
