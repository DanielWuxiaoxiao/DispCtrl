# DispCtrl - Radar Display Control System

## 项目简介

DispCtrl 是一个基于 Qt5 的雷达显示控制系统，提供实时的雷达数据可视化和管理功能。系统支持 PPI（平面位置指示器）和扇区显示模式，适用于雷达监控、目标跟踪和态势感知应用。

## 主要功能

### 🎯 雷达显示
- **PPI 显示**: 传统的极坐标雷达显示界面
- **扇区显示**: 可定制角度范围的扇形显示区域
- **实时更新**: 支持动态数据刷新和显示更新
- **缩放平移**: 交互式视图操作

### 📡 数据管理
- **点迹管理**: 实时点迹检测、显示和管理
- **航迹管理**: 目标跟踪和轨迹显示
- **数据同步**: 多模块间的数据同步机制
- **UDP 通信**: 多线程网络数据接收

### 🖥️ 用户界面
- **可分离组件**: 支持窗口分离的灵活界面
- **自定义控件**: 专用的雷达显示控件
- **工具提示**: 智能悬浮信息显示
- **暗色主题**: 专业的雷达操作界面

### 🗺️ 地图集成
- **WebEngine 支持**: 集成 HTML5 地图显示
- **高德地图**: 支持地理信息叠加显示
- **多种视图**: 2D/3D 地图模式切换

## 技术架构

### 开发环境
- **Qt 5.14.2**: 基础 GUI 框架
- **CMake**: 跨平台构建系统
- **C++17**: 现代 C++ 标准
- **MSVC 2017**: Windows 编译器

### 核心模块
```
DispCtrl/
├── Basic/              # 基础工具类（协议、日志、配置）
├── Controller/         # 控制器模块（数据管理、信号处理）
├── PolarDisp/          # 极坐标显示模块
├── PointManager/       # 点迹和航迹管理
├── cusWidgets/         # 自定义 UI 组件
├── UDP/               # 网络通信模块
├── mapDisp/           # 地图显示模块
├── mainPanel/         # 主控制面板
└── resources/         # 资源文件（图标、样式）
```

### 关键组件
- **PolarAxis**: 极坐标轴系统
- **PolarGrid**: 极坐标网格绘制
- **SectorWidget**: 扇区显示控件
- **DetManager**: 点迹检测管理器
- **TrackManager**: 航迹跟踪管理器

## 编译和运行

### 环境要求
- Windows 10/11
- Visual Studio 2017 或更高版本
- Qt 5.14.2 或更高版本
- CMake 3.16 或更高版本

### 编译步骤

#### 使用 VS Code (推荐)
1. 安装 CMake Tools 扩展
2. 打开项目文件夹
3. 按 `Ctrl+Shift+P` 输入 "CMake: Configure"
4. 选择编译器工具链
5. 按 `F7` 或使用 "CMake: Build" 命令

#### 命令行编译
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Debug
```

### 运行程序
```bash
# Debug 模式
./build/bin/Debug/DispCtrl.exe

# Release 模式
./build/bin/Release/DispCtrl.exe
```

## 配置文件

### config.json
系统配置文件，包含：
- 显示参数设置
- 网络连接配置
- 界面布局选项
- 数据处理参数

## 开发指南

### 添加新的显示模块
1. 继承 `QGraphicsItem` 或 `QGraphicsScene`
2. 实现 `paint()` 和 `boundingRect()` 方法
3. 在相应的管理器中注册模块
4. 更新 CMakeLists.txt 文件

### 自定义数据协议
1. 修改 `Basic/Protocol.h` 中的数据结构
2. 更新相应的解析和处理逻辑
3. 确保数据同步机制正常工作

## 许可证

本项目采用 [MIT License](LICENSE) 开源协议。

## 贡献指南

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 联系方式

- 项目维护者: [Your Name]
- 邮箱: [your.email@example.com]
- 项目主页: [GitHub Repository URL]

---

*DispCtrl - 专业的雷达显示控制解决方案*