# DispCtrl 项目 - AI Agent 专用说明文档（纲领版）

> **文档用途**: 本文档专为 AI 开发助手（Claude、GitHub Copilot、Cursor 等）提供项目全局上下文，整合了项目**所有 MD 文档**的核心知识，便于快速理解项目架构、代码规范、常见任务与注意事项。
>
> **最后更新**: 2026-07-15（补充 Linux 18.04+/20.04+ 部署与验证说明）
>
> **信息来源**: README.md / config_documentation.md / docs/internal_protocol.md / docs/external_protocol_notes.md / docs/parameter_save_feature.md / docs/data_storage_feature.md / docs/health_window_realtime_update.md / docs/track_display_fix.md / docs/track_angle_debug.md / docs/rangeazimuth_*.md（6篇）/ docs/changes_2025-12-25.md / docs/book/*.md（15章）/ .azure/dataflow_fix_summary.md
>
> **📌 重要提示**:
> - **每次重大修改后**，AI Agent 应主动更新本文档的相关章节（版本历史、注意事项等）
> - **修改位置**: 在对应章节直接更新，保持文档与代码同步
> - **更新内容**: 新功能、Bug修复、编译错误解决方案、新的注意事项等

---

## 项目概览

**DispCtrl** 是一个基于 **Qt 5.14 + C++17** 的雷达显示与控制系统（**X576雷达型号**），用于实时展示雷达数据（检测点、航迹、目标）、地图叠加、参数配置与外部链路控制。

### 核心技术栈
| 项目 | 技术 |
|------|------|
| 语言 | C++17 |
| 框架 | Qt 5.14 (Widgets, WebEngine, WebChannel, Network) |
| 构建 | CMake >= 3.16（主要）+ qmake（备用） |
| 编译器 | MSVC 2019+（Windows），GCC 9+（Linux） |
| 网络 | UDP 多线程收发（ThreadedUdpSocket） |
| 渲染 | QGraphicsView/Scene（PPI极坐标），Qt WebEngine（地图） |
| 地图 | Qt WebEngine + 高德地图（2D/3D/卫星/无图） |
| 样式 | 自定义暗色主题 QSS（resources/style/darkstyle.qss） |
| 配置 | TOML 格式（config.toml） |

### Docker 构建目标
- `docker/docker_build.sh` 支持 `1804/2004`、`2204`、`2404` 和 `auto/current` 目标；`auto/current` 会按当前 WSL/宿主 Ubuntu 版本选择对应容器基础镜像，Ubuntu 20.04 宿主会回退到 18.04+ 兼容目标。
- `1804` 使用 `Dockerfile.ubuntu1804`，用于生成 Ubuntu 18.04+ 兼容产物；`2204`/`2404` 使用 `docker/Dockerfile` 的 `UBUNTU_VERSION` build arg。
- `Dockerfile.ubuntu1804` 使用固定 Ubuntu 18.04 快照、GCC 9 和 Qt 5.15.2 WebEngine；`scripts/package_linux.sh` 会递归收集 Qt/插件依赖并捆绑 ICU、libstdc++ 和必要的 xcb 库，发布包根目录包含 `readme.txt`。开发机的 WSL + Docker 构建命令见 `docker/README.md`。
- Dockerfile 中的 Ubuntu 版本是容器编译环境和目标运行兼容性，不要求与 WSL 宿主版本一致。Docker Hub 拉取 `ubuntu:*` 超时属于 Docker registry 网络/镜像加速器问题，不是 apt 源或 WSL 版本不匹配问题。

---

## 项目结构（完整目录，已对照实际文件核实）

```
DispCtrl/
├── Basic/              # 基础设施
│   ├── Protocol.h/cpp      # 协议结构体定义（小端、1B对齐）
│   ├── ConfigManager.h     # TOML 配置单例（CF_INS宏访问）
│   ├── authmanager.h       # 认证管理
│   ├── bindThread.h        # 线程绑定工具
│   ├── log.h/cpp           # 统一日志系统
│   ├── mathUtil.h          # 数学工具（极坐标转换等）
│   └── DispBasci.h         # 全局常量（颜色、范围、最大值等）
│
├── Controller/         # 控制层：业务逻辑与数据分发
│   ├── controller.h/cpp              # 总控制器（CON_INS单例）
│   ├── RadarDataManager.h/cpp        # 统一数据缓存与订阅分发
│   ├── sig2dispmanager.h/cpp         # 信号处理 → 显示（检测点）
│   ├── data2dispmanager.h/cpp        # DBT数据处理 → 显示（DBT航迹）
│   ├── tbd2dispmanager.h/cpp         # TBD数据处理 → 显示（TBD航迹）
│   ├── mon2dispmanager.h/cpp         # 监控 → 显示（状态/BIT）
│   ├── res2dispmanager.h/cpp         # 资源调度 → 显示
│   ├── targetdispmanager.h/cpp       # 目标分类 → 显示
│   ├── disp2sigmanager.h/cpp         # 显示 → 信号处理（下行控制）
│   ├── disp2datamanager.h/cpp        # 显示 → 数据处理（下行控制）
│   ├── disp2monmanager.h/cpp         # 显示 → 监控（下行）
│   ├── disp2resmanager.h/cpp         # 显示 → 资源调度（下行）
│   ├── disp2photomanager.h/cpp       # 显示 → 光电设备（下行）
│   ├── ExternalCtrlManager.h/cpp     # 外部雷控/伺服链路（512B/32B/AD帧）
│   └── ErrorHandler.h/cpp            # 错误处理策略
│
├── PolarDisp/          # PPI极坐标及扇区显示模块
│   ├── ppiview.h/cpp               # PPI视图主容器
│   ├── ppisscene.h/cpp             # PPI场景（含Det/Track Manager）
│   ├── polaraxis.h/cpp             # 极坐标轴与刻度绘制
│   ├── polargrid.h/cpp             # 极坐标网格
│   ├── ppivisualsettings.h/cpp/ui  # 右下角设置（距离、地图、检测点数）
│   ├── mousepositioninfo.h/cpp/ui  # 鼠标位置与点迹大小控制SpinBox
│   ├── scanlayer.h/cpp             # 扫描动画层
│   ├── sectorwidget.h/cpp          # 扇区显示容器
│   ├── sectorscene.h/cpp           # 扇区场景
│   ├── sectorpolargrid.h/cpp       # 扇区极坐标网格
│   ├── rangeazimuthchart.h/cpp     # 距离-方位直角坐标图表（继承CustomLineChart）
│   ├── rangeheightchart.h/cpp      # 距离-高度直角坐标图表（继承CustomLineChart）
│   ├── rangeazimuthwidget.h/cpp    # 距离-方位显示组件（工具栏+图表）
│   ├── tooltip.h/cpp               # 鼠标悬停提示气泡
│   ├── pointinfow.h/cpp/ui         # 点信息窗口
│   ├── pviewtopleft.h/cpp/ui       # PPI左上角信息叠加
│   └── zoomview.h/cpp              # 缩略图
│
├── PointManager/       # 检测点与航迹数据对象管理
│   ├── detmanager.h/cpp            # 检测点管理（FIFO数量限制）
│   ├── trackmanager.h/cpp          # DBT航迹管理
│   ├── sectordetmanager.h/cpp      # 扇区检测点管理
│   ├── sectortrackmanager.h/cpp    # 扇区航迹管理
│   └── point.h/cpp                 # 点图形对象基类
│
├── UDP/                # 网络通信层
│   └── threadudpsocket.h/cpp       # 多线程UDP Socket封装
│
├── mapDisp/            # WebEngine 地图集成
│   └── mapprox.h/cpp               # 地图代理，Qt↔JS双向通信
│
├── mainPanel/          # 主界面布局与功能
│   ├── mainoverlayout.h/cpp/ui     # 顶层UI组织（主面板）
│   ├── azelrangewidget.h/cpp       # 方位/仰角/距离实时显示控件
│   ├── screenrecorderwidget.h/cpp  # 屏幕录制功能控件
│   └── simpleavi.h/cpp             # AVI文件录制实现
│
├── paramWidget/        # 参数配置对话框（所有下发参数面板）
│   ├── servocontrol.h/cpp/ui       # 伺服控制（参考实现，其他对话框参照此基准）
│   ├── batterycontrol.h/cpp/ui     # 电池/波束控制
│   ├── waveandsample.h/cpp/ui      # 波形与采样参数（BeamControl 0xAA05）
│   ├── freqcontrolui.h/cpp/ui      # 频率控制
│   ├── sigparamui.h/cpp/ui         # 信号处理参数（16个参数）
│   ├── dataprocessui.h/cpp/ui      # 数据处理参数（15个参数）
│   ├── datasaveui.h/cpp/ui         # 数据存储控制（0xCC01/CC02/CC03）
│   ├── scanrangeui.h/cpp/ui        # 扫描范围配置
│   ├── photoelectricparam.h/cpp/ui # 光电设备参数
│   ├── tranrecvui.h/cpp/ui         # 收发参数
│   ├── tasmodedialog.h/cpp/ui      # TAS工作模式对话框
│   └── twsmodedialog.h/cpp/ui      # TWS工作模式对话框
│
├── cusWidgets/         # 自定义基础控件库
│   ├── cuswindow.h/cpp             # 可拖拽浮动窗口基类
│   ├── detachablewidget.h/cpp      # 可分离/重附加窗口容器
│   ├── customcombobox.h/cpp        # 自定义下拉框
│   ├── custommessagebox.h/cpp      # 自定义消息框（确认/信息/警告）
│   ├── customspinbox.h/cpp         # 自定义数值输入框
│   ├── customspinboxstyle.h/cpp    # 数值框样式代理
│   ├── customlinechart.h/cpp       # 通用直角坐标图表基类（RangeAzimuth用）
│   └── frozentablewidget.h/cpp     # 冻结列表格（航迹表格用）
│
├── htmls/              # Web地图前端页面（高德地图）
│   ├── index.html      # 标准2D路网地图
│   ├── index3d.html    # 3D地图
│   ├── indexNoL.html   # 无路网图
│   ├── indexS.html     # 卫星图
│   └── black.html      # 纯黑（无地图模式）
│
├── resources/
│   ├── style/darkstyle.qss  # 全局暗色主题样式表
│   ├── icon/                # 图标资源（工具栏、窗口等）
│   └── images/
│
├── docs/               # 完整项目文档（34个MD文件）
│   ├── book/                         # 技术专著（15章）
│   │   ├── 01_introduction.md        # 前言与项目目标
│   │   ├── 02_requirements.md        # 需求分析
│   │   ├── 03_architecture.md        # 总体架构
│   │   ├── 04_protocols.md           # 协议详解
│   │   ├── 05_core_modules.md        # 核心模块详解
│   │   ├── 06_data_flow.md           # 数据流与处理流程
│   │   ├── 07_configuration.md       # 配置说明
│   │   ├── 08_build_deploy.md        # 构建与部署
│   │   ├── 09_testing.md             # 测试策略
│   │   ├── 10_extensibility.md       # 扩展与二次开发
│   │   ├── 11_ops_troubleshooting.md # 运维与故障排查
│   │   ├── 12_security_reliability.md# 安全性与可靠性
│   │   ├── 13_code_guidelines.md     # 代码规范与贡献流程
│   │   ├── 14_appendix.md            # 附录
│   │   └── 15_change_log.md          # 更新历史
│   ├── internal_protocol.md          # 内部协议完整说明（v5.1）
│   ├── external_protocol_notes.md    # 外部协议笔记（AD帧/系统控制512B）
│   ├── parameter_save_feature.md     # 参数保存功能实现指南
│   ├── data_storage_feature.md       # 数据存储管理（开/停/删除/离线）
│   ├── health_window_realtime_update.md  # 健康管理窗口实时更新功能
│   ├── track_display_fix.md          # 航迹显示修复（坐标单位、颜色、样式）
│   ├── track_angle_debug.md          # 航迹角度调试（弧度→度转换记录）
│   ├── rangeazimuth_summary.md       # 距离-方位显示功能总结
│   ├── rangeazimuth_chart_migration.md  # 图表架构重构（极坐标→直角坐标）
│   ├── rangeazimuth_integration_guide.md # 集成到MainOverLayOut指南
│   ├── rangeazimuth_display_logic_fix.md # 显示逻辑修复（隐藏/显示）
│   ├── rangeazimuth_style_update.md  # 深色主题样式与工具栏升级
│   ├── rangeazimuth_km_unit_and_style.md # 单位与样式说明
│   └── changes_2025-12-25.md        # 外部雷控链路变更摘要
│
├── .azure/
│   └── dataflow_fix_summary.md      # 数据流连接修复报告（2025-10-24）
│
├── scripts/            # 开发辅助脚本（如更新头文件注释）
├── tests/              # 测试框架头文件
├── config.toml         # 主配置文件（启动自动加载到build输出目录）
├── CMakeLists.txt      # CMake 构建配置
├── DispCtrl.pro        # Qt Creator qmake 项目文件
└── Claude.md           # 本AI Agent纲领文档
```

---

## 架构设计原则

### 1. 分层架构
```
[Web/HTML地图层]   Qt WebEngine + 高德地图（htmls/*.html）
      ↑↓
[UI 视图层]        PPIView / SectorWidget / RangeAzimuthWidget / 参数对话框 / MainOverLayOut
      ↑↓（Qt信号槽）
[Controller层]     controller.h/cpp（总控，CON_INS单例）
      ↑↓
[Manager数据层]    RadarDataManager + Sig2/Data2/Tbd2/Mon2/Res2/Target... Manager
      ↑↓
[UDP 网络层]       ThreadedUdpSocket（每路独立接收线程）
```

### 2. 核心设计模式
| 模式 | 应用 |
|------|------|
| 单例 | ConfigManager（CF_INS）、RadarDataManager（RADAR_DATA_MGR）、Controller（CON_INS）|
| 观察者 | Qt 信号槽：Manager发射信号 → UI订阅更新 |
| 工厂 | 点对象创建（DetPoint, TrackPoint, TBDPoint）|
| 策略 | 错误处理（ErrorHandler），数据可见性策略 |
| 桥接 | ExternalCtrlManager 封装外部链路透传，屏蔽业务字段 |

### 3. 典型数据流（检测点）
```
信号处理子系统 —UDP→ ThreadedUdpSocket(端口8003)
    → sig2dispmanager::handleDatagram()
    → Controller::detInfoProcess 信号
    → PPIScene::DetManager::addDetPoint()           # PPI显示（绿色）
    → SectorDetManager::addDetPoint()              # 扇区显示（绿色）
    → RangeAzimuthChart::addDetectionPoint()       # 距离-方位图（绿色，鼠标悬停Tooltip）
    → MainOverLayOut::updateTrackList()            # 航迹表格（航迹同理，不含检测点）
```

### 4. 线程模型
- **主线程**: 全部 UI 操作与 Qt 事件循环
- **UDP线程**: 每个 ThreadedUdpSocket 独立线程（接收+初步解析）
- **原则**: 数据处理在后台，UI更新通过信号槽自动 marshal 到主线程（AutoConnection）

---

## 通信协议体系（docs/internal_protocol.md v5.1）

### 基本约定
- **字节序**: 小端（Little-Endian）
- **对齐**: 1字节对齐（`#pragma pack(1)`）
- **帧头**: 0xFA55FA55 / **帧尾**: 0x55FA55FA

### 帧格式
| 字节 | 字段 | 说明 |
|------|------|------|
| 1-4 | 帧头 | 0xFA55FA55 |
| 5-6 | 源设备ID | 分系统编号（见下表）|
| 7-8 | 目的设备ID | 分系统编号 |
| 9-12 | 通信次数 | 循环计数 |
| 13-14 | 消息长度 | 含ID+payload字节数 |
| 15-16 | 消息ID | 具体消息类型 |
| 17... | 消息体 | 按ID变长 |
| N | 校验码 | 头到尾XOR |
| N+1~N+4 | 帧尾 | 0x55FA55FA |

### 分机代号
| 软件 | 代号 |
|------|------|
| 资源调度 | 0xBB01 |
| 信号处理 | 0xBB02 |
| 数据处理 | 0xBB03 |
| **显控（本系统）** | **0xBB04** |
| 目标识别 | 0xBB05 |
| 自启动监控 | 0xBB06 |
| **外部雷控** | **0xBB07** |

### 关键消息ID速查
| 消息ID | 方向 | 内容 | 处理文件 |
|--------|------|------|----------|
| 0xAA01 | 显控→资调 | 象限电源控制 | disp2resmanager |
| 0xAA05 | 显控→资调 | 波束控制（系统控制表512B）| waveandsample |
| 0xCC01 | 显控→信处 | 数据存储开关 | datasaveui |
| 0xCC02 | 显控→信处 | 数据删除 | datasaveui |
| 0xCC03 | 显控→信处 | 离线处理控制 | datasaveui |
| 0xCF01 | 监控→显控 | MonitorParam软件状态（4项）| mon2dispmanager |
| 0xDD02 | 信处→显控 | 数据存储成功回执 | Controller信号 |
| 0xDD03 | 信处→显控 | 数据删除成功回执 | Controller信号 |
| 0xDD04 | 信处→显控 | 离线处理状态回执 | Controller信号 |
| 0xDE02 | 资调→显控 | BIT硬件状态上报（10项+温度+角度）| mon2dispmanager |

### 协议单位注意（重要！已有Bug修复记录）
| 数据类型 | 字段 | 协议单位 | 显控内部单位 | 转换 |
|---------|------|---------|-------------|------|
| 检测点 detInfo | azi/ele | **度** | 度 | 无需转换 |
| DBT航迹 trackInfo | azi/ele | **弧度** | 度 | ×(180/π) |
| TBD航迹 TBDPoint | azi/ele | **弧度** | 度 | ×(180/π) |
| 距离所有类型 | dis | 米 | 米 | 无需转换 |
| BeamControl freqID | freqID | 0~80(→9.0~9.8GHz) | ComboBox index 0~8 | index×10 |

**修复位置**: `Controller/data2dispmanager.cpp`（DBT），`Controller/tbd2dispmanager.cpp`（TBD）

---

## 外部雷控链路（ExternalCtrlManager）

### 消息类型
| 消息 | 方向 | 长度 | 备注 |
|------|------|------|------|
| 系统控制 | 显控→雷控 | 512B | 含帧头尾，原样透传；显控不改字段不计算校验 |
| 控制回执 | 雷控→显控 | 64B | 按长度区分类型 |
| 伺服控制 | 显控→雷控 | 32B | 调用方负责填充并保证长度 |
| 伺服回执 | 雷控→显控 | 32B | 按长度区分类型 |
| AD数据帧 | 双向 | 可变 | 头0x7FFFBC1C/尾0x7FFF5A5A；beamCount×samplesPerPulse复采样（I/Q各2字节）|

### 默认网络配置（config.toml可覆盖）
| 参数 | 键名 | 默认值 |
|------|------|--------|
| 显控发送IP | SCHED_IP | 192.168.1.5 |
| 雷控接收IP | RADAR_CTRL_IP | 192.168.1.16 |
| 发送源端口 | EXT_SYSCTRL_SRC | 6001 |
| 雷控目标端口 | EXT_SYSCTRL_DST | 8001 |
| 回执接收端口 | EXT_ACK_DST | 8002 |

### 关键接口
```cpp
// 发送（Controller层暴露）
Controller::sendExternalSystemControl(data512B);  // 512B完整帧（含头尾）
Controller::sendExternalServoControl(data32B);    // 32B伺服帧

// 发送（ExternalCtrlManager直接调用）
mgr->sendSystemControlPayload(payload504B);       // 504B内容→自动加头尾
mgr->sendAdFrame(frame);                          // AD数据帧
mgr->sendSystemControlWithAd(ctrl504B, adFrame);  // 组合发送

// 接收信号
Controller::externalSystemCtrlAck(QByteArray)     // 64B系统控制回执
Controller::externalServoAck(ExternalServoAck32)  // 32B伺服回执
ExternalCtrlManager::systemControlFrameReceived(ExternalSystemControl512)
ExternalCtrlManager::adFrameReceived(ExternalAdFrame)
ExternalCtrlManager::externalCtrlLog(QString)     // 日志
```

---

## 网络拓扑与端口

### IP体系
```
192.168.64.x（内部主系统网段）
├── 192.168.64.3  数据处理/信号处理/资源调度/监控/目标识别
└── 192.168.64.4  显示控制（本系统）

192.168.101.x（光电设备专用网段）
├── 192.168.101.10  光电跟踪设备
└── 192.168.101.14  显控对光电专用接口

192.168.1.x（外部雷控链路）
├── 192.168.1.5   显控/调度本机（发送方）
└── 192.168.1.16  外部雷控采集端（接收方）
```

### 关键端口（内部 192.168.64 网段）
| 端口键名 | 端口号 | 方向 | 数据内容 |
|---------|--------|------|----------|
| SIG_2_DISP_PORT1 / DISP_GET_SIG_PORT1 | 8003 | 信处→显控 | **检测点数据** |
| SIG_2_DISP_PORT2 / DISP_GET_SIG_PORT2 | 8004 | 信处→显控 | 系统状态 |
| DATA_PRO_2_DISP / DISP_GET_DATA_PORT | 8006 | 数处→显控 | **DBT航迹** |
| DATA_PRO_2_DISP2 / DISP_GET_DATA_PORT2 | 8010 | 数处→显控 | **TBD航迹** |
| TARGET_2_DISP / DISP_GET_TARGET_PORT | 8017 | 目标识别→显控 | 识别结果 |
| MONITOR_2_DISP / DISP_GET_MONITOR_PORT | 8019 | 监控→显控 | 监控指令 |
| DISP_2_SIG_PORT / SIG_GET_DISP_PORT | 6002/8002 | 显控→信处 | 信处控制指令 |
| DISP_2_DATA / DATA_GET_DISP | 6008/8008 | 显控→数处 | 数处控制指令 |
| DISP_2_RES_PORT / RES_GET_DISP_PORT | 6012/8012 | 显控→资调 | 资源申请 |
| DISP_2_MONITOR / MONITOR_GET_DISP_PORT | 6018/8018 | 显控→监控 | 状态上报 |

---

## 关键功能特性与实现记录

> 仅记录"做了什么 + 在哪改"，具体代码以源码为准（避免文档代码漂移）。

### 1. 检测点数量限制与P显一键清除（2026-01-19）
- 问题：检测点无限增长导致内存/卡顿。
- `DetManager` FIFO 淘汰（超 `m_maxPoints` 删最旧）；`PPIVisualSettings` 加"检测点"输入框（默认1000，100~1000000）；"显清"按钮经 `CustomMessageBox::showConfirm` 清空检测点+航迹。
- 文件：`PolarDisp/ppivisualsettings.*`、`PointManager/detmanager.*`、`PolarDisp/ppiview.*`（`onClearDisplayRequested`）。

### 2. 航迹角度单位修复（统一按度，2026-05-15）
- 现象：常规/TBD/协同航迹方位角被放大数千度，触发 `DATA_INVALID_TRACK`。根因：历史代码误把 `azi/ele` 当弧度做 `×180/π`；联调确认三类航迹角度字段均为**度**，不再转换。
- 文件：`Controller/data2dispmanager.cpp`(DBT)、`tbd2dispmanager.cpp`(TBD)、`collabtrack2dispmanager.cpp`(协同)，均 `info.azimuth/elevation = pt->azi/ele`。

### 3. 航迹显示样式（颜色、线宽、关注态）
- 颜色（`Basic/DispBasci.h`）：检测点绿 `(0,255,0)`、DBT 橙 `(255,128,0)`、TBD 黄 `(255,255,0)`、协同青蓝 `CO_TRACK_COLOR`。
- 连线（`PointManager/trackmanager.cpp`）：宽2、实线、圆端点/圆角、透明度0.8。
- 关注态（2026-05）：右键 `DraggableLabel` 菜单 `关注/取消关注`；被关注批次在 P显 为更大空心三角、层级提到最高；`statMethod==2` 消批或取消关注后恢复普通样式。

### 4. 数据流连接修复（2025-10-24，详见 `.azure/dataflow_fix_summary.md`）
- PPIScene：`Controller::detInfoProcess/traInfoProcess → DetManager/TrackManager::add*Point`（`ppisscene.cpp`）。
- 航迹表格：`traInfoProcess → MainOverLayOut::updateTrackList/updateDroneTrackList`（`mainoverlayout.cpp`）。
- 扇区：`det/traInfoProcess → Sector{Det,Track}Manager::add*Point`（`mainoverlayout.cpp`）。

### 5. 数据存储管理（datasaveui，2026-01-26）
- 下行 显控→信处：`DataSave 0xCC01`(saveSwitch,dataID)、`DataDel 0xCC02`(dataID)、`OfflineDel 0xCC03`(onSwitch,dataID)。
- 上行 信处→显控：`DataSaveOK 0xDD02`(dataID,dataSize/GB)、`DataDelOK 0xDD03`(dataID)、`OfflineStat 0xDD04`(delStat,dataID)。
- 结构体定义见 `Basic/Protocol.h`。

### 6. 健康管理窗口实时更新（2026-01-26）
- 窗口持久化（不重建）、数据实时刷新。数据源：`MonitorParam 0xCF01`（4 软件模块状态，状态码 0正常/1异常/2启动成功/3启动失败/4关闭成功/5关闭失败）、`BITReport 0xDE02`（10 项硬件 BIT + FPGA温度 + 阵面温度 + 偏航角 + 扫描角）。
- 成员见 `mainoverlayout.h`（`m_healthWindow`、各状态按钮、`m_tempLabel/m_angleLabel`）；逻辑见 `MainOverLayOut::updateHealthWindow()`。

### 7. 距离-方位图 / 距离-高度图（RangeAzimuth/RangeHeight，2026-01~05）
- 架构：直角坐标基类 `CustomLineChart`（cusWidgets）→ `RangeAzimuthChart`（X=方位0~360°，Y=距离）→ `RangeAzimuthWidget`（工具栏+图表）。`RangeHeightChart/Widget` 为 `B显` 后的 `高显`（X=距离 km 同步主PPI量程，Y=高度 m，默认0~500 可配）。
- 关键接口（`rangeazimuthchart.h`）：`addDetectionPoint/addTrackPoint/addPointInfo`、`setDetection/TrackVisible`、`setDetection/TrackSizeRatio`、`setRangeFromMain`、`setAzimuthRange`(支持跨0°)、`clearRadarData`、`setMaxDetectionPoints`。
- 同步：主PPI `PolarAxis::rangeChanged → setRangeFromMain`；`MousePositionInfo` spinbox/checkbox → 三路视图统一大小/显隐；`DetManager::setMaxPoints` → 各路 `setMaxDetectionPoints`。
- 配置 `[rangeAzimuthDisp.angle] min/max`；QSS 对象名 `#RangeAzimuthChartToolBar`、`#RangeAzimuthChart`。
- `3D显`：`Track3DWidget` 使用 Qt WebEngine + WebChannel 承载随程序资源打包的 Three.js r128，仅保留 `(PointInfo.type, batch)` 对应的最新航迹点；普通/TBD/协同、无人机筛选、点大小、显清和离线 RAE 直投均与 P/B/H 显同步，不接收检测点、不保存历史轨迹线。
- 三维坐标：水平距离为 `range*cos(elevation)`，X=东向、Z=北向、Y=`altitute`；水平范围同步 PPI，垂直范围同步 H显。Web 端独立归一化三轴但保持真实单位标签，范围外航迹仍保留在 C++ 最新点模型中。
- 3D 性能/稳定性：C++ 以 50ms 合并 WebChannel 增量，隐藏页仅维护最新模型并在恢复时发送快照；JS 按颜色批量使用 `THREE.Points/BufferGeometry` 且按需渲染。WebEngine 页面/渲染进程首次异常限次重载，再次失败显示占位并写文件日志。

### 8. 多通道航迹显示开关（2026-05-09）
- 通道：TBD `0xEE02`(`6010→8010`)、协同 `0xEE03`(`6020→8020`，复用 `TBDTrackHead+TBDTrackInfo+TBDPoint`)；`PointInfo.type` 新增 `CooperativeTrackPointType=4`。
- 开关 `[displayConfig] iftbd/ifxietong`（默认 false）：false 时不创建管理器/不监听端口/不显示 UI。`MousePositionInfo` 动态加复选框、`MainOverLayOut` 动态加表页、`RangeAzimuthChart`/`TrackManager`/`SectorTrackManager` 按类型着色与显隐。

### 9. 参数对话框统一规范（2026-01-15，参考 `paramWidget/servocontrol.cpp`）
- "确定下发"：打包结构体发信号，**不关窗**（便于连续下发）；"取消"：向上找 `CusWindow` 父指针 `close()`。
- 构造中 `disconnect` `buttonBox` 默认 accepted/rejected，改连自定义 `onAccept/onCancel`。

### 10. 参数保存到 config.toml（2026-01-21）
- "保存参数"按钮 → `CustomMessageBox::showConfirm` → `CF_INS.saveXxxParam(...)` + `CF_INS.save()` → 成功/失败弹窗；下次启动构造中用 `CF_INS.xxxParam(默认值)` 恢复。
- 已支持段落 `[params.*]`：`servo`(cmd,speed,az)、`scanrange`(workMode)、`beamcontrol`(freqID,type,3波形)、`sigpro`(16参)、`datapro`(15参)、`datasave`(saveSwitch,dataID)。
- `.ui` 在 `QDialogButtonBox` 前加 `QPushButton name="saveButton"`。

---

## 配置系统（config.toml）

### 配置访问宏
```cpp
#define CF_INS ConfigManager::getInstance()

CF_INS.ip("RADAR_CTRL_IP", "192.168.1.16");       // IP地址
CF_INS.port("SIG_2_DISP_PORT1", 8003);             // 端口
CF_INS.polarDispRange("max", 5.0);                 // PPI显示距离
CF_INS.displayConfig("max_points", 1000);          // 显示配置
CF_INS.rangeAzimuthAngle("max", 360.0);            // 距离-方位角配置
CF_INS.servoParam(默认值);                          // 参数读取
CF_INS.saveServoParam(值...); CF_INS.save();       // 参数写入
```

### 完整配置结构速览
```toml
[network.ips]
DISP_CTRL_IP = "192.168.64.4"
DATA_PRO_IP  = "192.168.64.3"
RADAR_CTRL_IP = "192.168.1.16"     # 外部雷控
SCHED_IP     = "192.168.1.5"       # 外部链路本机

[network.ports]
SIG_2_DISP_PORT1 = 8003            # 检测点
DATA_PRO_2_DISP  = 8006            # DBT航迹
DATA_PRO_2_DISP2 = 8010            # TBD航迹
EXT_SYSCTRL_SRC  = 6001
EXT_SYSCTRL_DST  = 8001
EXT_ACK_DST      = 8002

[network.ids]
DISP_CTRL_ID = 47876               # 0xBB04

[polarDisp.range]
min = 1
max = 5

[sectorDisp.angle]
min = -30
max = 30

[rangeAzimuthDisp.angle]
min = 0
max = 360

[map]
mode = "standard"                  # standard|satellite|none|noroad|3d

[displayConfig]
max_points = 1000                  # 检测点FIFO上限
max_track_points = 200            # 单批航迹FIFO上限

[params.servo]                     # 参数保存功能写入的段落
# ... 运行时写入
```

---

## 代码规范与约定

### 命名规范
| 类别 | 规范 | 示例 |
|------|------|------|
| 类名 | PascalCase | `PPIView`, `DetManager`, `RangeAzimuthChart` |
| 成员变量 | `m_` + camelCase | `m_scene`, `m_maxPoints`, `m_detVisible` |
| 函数名 | camelCase | `addDetPoint()`, `setRangeFromMain()` |
| 常量/宏 | UPPER_SNAKE | `DET_COLOR`, `CF_INS`, `CON_INS` |
| 信号 | camelCase描述性 | `detInfoProcess`, `rangeChanged` |
| 槽函数 | `on` + 描述 | `onAccept()`, `onMaxPointsChanged()` |

### 注释（Doxygen风格）
```cpp
/**
 * @brief 添加一个检测点并渲染到场景
 * @param info 检测点信息（azi单位：度；dis单位：米）
 * @details 超出 m_maxPoints 时自动淘汰最旧数据（FIFO队列）
 */
void addDetPoint(const PointInfo& info);
```

### 信号槽（必须使用新式语法）
```cpp
// 推荐（类型安全，编译期检查）
connect(sender, &SenderClass::signal, receiver, &ReceiverClass::slot);

// 禁止（字符串形式，无编译期检查）
connect(sender, SIGNAL(signal(int)), receiver, SLOT(slot(int)));
```

### 内存管理原则
- Qt对象优先指定 parent（自动随父对象销毁）
- 非Qt对象使用 `std::unique_ptr`/`std::shared_ptr`
- 容器使用 Qt 容器（`QVector`, `QList`, `QMap`）
- 禁止在非主线程操作任何 QWidget/QGraphicsItem

---

## 常见开发任务

### 任务1: 添加新数据类型与显示
1. `Basic/Protocol.h` — 定义消息结构体与消息ID
2. `config.toml` — 添加端口/IP配置
3. 新建 `XxxManager`（参考 `sig2dispmanager`）：监听UDP → 解析 → 发射信号
4. `Controller/controller.h` — 添加中转信号与注册
5. `PPIScene`/`MainOverLayOut` — 订阅信号更新视图
6. `CMakeLists.txt` — 添加新的 `.cpp/.h` 文件

### 任务2: 修改参数对话框
1. `.ui` 文件修改（Qt Designer）
2. 断开 `QDialogButtonBox` 默认连接，连接 `onAccept()/onCancel()`
3. `onAccept()`: 读取UI → 填充结构体 → 发射信号 → 不关闭窗口
4. `onCancel()`: 查找 `CusWindow` 父指针 → 关闭
5. 若需参数持久化：参照"参数保存"实现模式

### 任务3: 添加新配置项
1. `config.toml` 添加配置项
2. `Basic/ConfigManager.h` 添加读写方法（仿已有方法）
3. 代码中 `CF_INS.xxx(key, default)` 使用

### 任务4: 调试数据显示异常
1. 确认端口与 `config.toml` 一致
2. 确认角度单位（弧度/度）转换（DBT/TBD需×180/π）
3. 确认信号连接：`CON_INS` 相关信号是否已连接到Manager/视图
4. `qDebug()` 打印原始协议值与转换后值
5. 检查 FIFO `maxPoints` 是否设置过小

### 任务5: 修复析构崩溃
参见注意事项第3、3.1、4条，关键检查：
- 单例信号连接到子对象 → 父对象析构函数中显式 `disconnect`
- `DetachableWidget` `m_isDestroying` 标志位
- `CusWindow` 浮动窗口的析构顺序

---

## 注意事项与已知陷阱

### 1. 不要执行自动编译
**重要**: AI 只提供代码修改，不执行编译任务，用户自行编译验证。
- 不调用 `run_task`、`cmake --build`、构建相关命令

### 0. Git 命令限制（最高优先级）
**严格禁止**: 未经用户明文指定，AI 绝不执行任何 git 命令。
- 禁止自动执行: `git commit`、`git push`、`git pull`、`git merge`、`git rebase`、`git reset`、`git checkout` 等所有 git 操作
- **仅当用户明确说出"执行 git commit"、"push"、"提交代码"等明确指令时**，才可执行对应命令
- 代码修改完成后，只告知用户"修改完毕，可执行 git commit"，不自动触发

### 2. QSS 样式覆盖问题
`darkstyle.qss` 中特定对话框的 `QDialogButtonBox QPushButton` 规则会覆盖全局按钮样式。
- 解决：删除特定对话框的内联QSS覆盖，继承全局规则

### 3. 析构函数信号断开
```cpp
~MyWidget() {
    disconnect(CON_INS, nullptr, this, nullptr);
}
```

### 3.1 单例→子对象连接的析构（2026-01-22已修复）
```cpp
// MainOverLayOut::~MainOverLayOut()
if (CON_INS) {
    disconnect(CON_INS, nullptr, this, nullptr);
    if (m_sectorWidget && m_sectorWidget->scene()) {
        disconnect(CON_INS, nullptr, m_sectorWidget->scene()->detManager(), nullptr);
        disconnect(CON_INS, nullptr, m_sectorWidget->scene()->trackManager(), nullptr);
    }
}
disconnect(&RADAR_DATA_MGR, nullptr, this, nullptr);
delete ui;
```
**规则**: 将单例信号连接到子对象时，父对象析构函数必须显式断开该连接。

### 4. DetachableWidget 安全析构
```cpp
~DetachableWidget() {
    m_isDestroying = true;   // 阻止浮动窗口关闭时触发reattach
    if (m_floatingWindow) { m_floatingWindow->close(); delete m_floatingWindow; }
}
```

### 5. .ui 文件修改后需重新构建
修改 `.ui` 后需 cmake 重新构建才能生成 `ui_xxx.h`，让用户自行执行。

### 6. Linux 跨平台注意
- 固定 `geometry` 导致控件在左上角堆叠 → 删除固定geometry，使用Qt布局管理器
- overlay初始化时机 → 不在构造函数用 `width()/height()`，改用 `QTimer::singleShot(0, ...)`
- 文件路径大小写敏感 → 检查 `#include` 路径

### 7. 配置方法缺失（编译错误）
在 `Basic/ConfigManager.h` 添加缺失的访问方法（参照已有方法模式）:
```cpp
int displayConfig(const QString& key, int def = 1000) const {
    return getValue("displayConfig." + key, def).toInt();
}
```

### 8. 头文件前向声明 vs 完整包含
- `.h` 文件：尽量前向声明 `class XxxManager;`（减少编译依赖）
- `.cpp` 文件：必须 `#include` 完整头文件才能调用成员方法

### 9. BeamControl 频点映射
- 协议 freqID 范围: 0~80（对应 9.0~9.8 GHz）
- UI ComboBox index: 0~8
- **转换**: `freqID = comboBox->currentIndex() * 10`

### 10. 外部系统控制帧注意
- 传 504B 内容 → 用 `sendSystemControlPayload`（自动加头尾、不需调用方计算校验）
- 传 512B 完整帧 → 用 `sendSystemControl`（调用方需确保头尾完整且校验通过）
- 显控对外部链路**不做字段解析，不计算业务校验**，原样透传

### 11. CMakeLists.txt 同步
每新增 `.cpp` 文件，必须同步添加到 `CMakeLists.txt` 的源文件列表，否则 MOC/编译不会包含。

---

## 快速定位代码

### 功能 → 关键文件
| 功能 | 关键文件 |
|------|---------|
| PPI主视图 | `PolarDisp/ppiview.cpp`, `ppisscene.cpp` |
| 扇区显示 | `PolarDisp/sectorwidget.cpp`, `sectorscene.cpp` |
| 距离-方位图 | `PolarDisp/rangeazimuthchart.cpp`, `rangeazimuthwidget.cpp` |
| 检测点管理（FIFO）| `PointManager/detmanager.cpp` |
| DBT航迹管理 | `PointManager/trackmanager.cpp` |
| DBT角度转换 | `Controller/data2dispmanager.cpp` |
| TBD角度转换 | `Controller/tbd2dispmanager.cpp` |
| 外部雷控/AD帧 | `Controller/ExternalCtrlManager.cpp` |
| 数据存储UI | `paramWidget/datasaveui.cpp` |
| 健康管理窗口 | `mainPanel/mainoverlayout.cpp`（`updateHealthWindow()`）|
| 地图JS通信 | `mapDisp/mapprox.cpp` |
| 参数对话框参考 | `paramWidget/servocontrol.cpp` |
| 全局颜色常量 | `Basic/DispBasci.h` |
| 协议结构体 | `Basic/Protocol.h` |
| 配置管理 | `Basic/ConfigManager.h` |
| 暗色主题样式 | `resources/style/darkstyle.qss` |
| 主面板布局 | `mainPanel/mainoverlayout.cpp` |
| UDP收发 | `UDP/threadudpsocket.cpp` |
| 屏幕录制 | `mainPanel/screenrecorderwidget.cpp` |

### 常用搜索关键词
```
signals:                # 信号定义
slots: / void onXxx(   # 槽函数
CF_INS.                # 配置读取
CON_INS                # Controller单例
RADAR_DATA_MGR         # RadarDataManager单例
addDetPoint            # 检测点添加入口
addTrackPoint          # 航迹添加入口
handleDatagram         # UDP解析入口
0xAA05 / 0xCC01        # 协议ID搜索
180.0f / 3.14159       # 弧度度转换
setMaxPoints           # FIFO限制
dataToScene            # RangeAzimuth坐标转换
```

---

## AI Agent 工作建议

### 修改原则
1. **读先于写**: 先读相关文件，理解上下文再修改
2. **保持风格**: `m_`前缀、新式信号槽、Doxygen注释
3. **最小范围**: 只改必要部分，不引入无关重构
4. **评估影响**: 修改Manager/Controller信号前搜索所有订阅点
5. **检查包含**: 新增调用后确认 `#include` 完整
6. **同步构建**: 新增 `.cpp` 后提醒用户更新 `CMakeLists.txt`

### 回答策略
1. 用 `grep_search`/`semantic_search` 精准定位代码与行号
2. 说明"为什么"，不只是"怎么做"
3. 给出含上下文的完整代码片段
4. 主动提醒本文档中相关的已知陷阱

---

## 版本历史（整合全部文档）

> 早期版本（v1.0–v5.16）精简为一版一行；详细变更见 git 历史。最近版本（v5.17+）保留要点。

- **v1.0**(2025-08-18) 初版协议。
- **v4.0**(2025-09-18) 控制表512B对齐；BIT 增偏航与子阵电源。
- **v5.0/5.1**(2025-10~12) 协议v5.0 控制表512B 对齐外部协议、目标识别状态上报；数据流连接修复；新增 `ExternalCtrlManager`；外部链路默认参数写入 config.toml。
- **v5.1-UI**(2026-01-15) 参数对话框按钮样式/行为统一；移除特定对话框 QSS 覆盖。
- **v5.2**(2026-01-19) 检测点 FIFO 上限 + "显清"；补 `ConfigManager::displayConfig()`。
- **v5.3**(2026-01-21) 参数保存到 config.toml（servo/scanrange/beamcontrol/sigpro/datapro/datasave）。
- **v5.4**(2026-01-26) 数据存储管理（0xCC01~03/0xDD02~04）；健康管理窗口实时更新；距离-方位图表（`RangeAzimuthChart`+`CustomLineChart`）。
- **v5.5**(2026-01-28) B显超范围点隐藏而非删除；方位角跨0°支持。
- **v5.6**(2026-02-04) 航迹角度修复（当时 DBT/TBD ×180/π，后于 v5.18 改回全按度）；航迹橙色/线宽2/圆端点/0.8 透明。
- **v5.7**(2026-03-23) 外部协议 AD 数据帧；CLAUDE.md 全面整合重写。
- **v5.8**(2026-05-09) 多通道航迹开关 `iftbd`/`ifxietong`；协同航迹通道 `0xEE03`(6020→8020)；动态表页/可见性/按类型着色。
- **v5.9**(2026-05-12) 高显 TAB（`RangeHeightChart`，X=距离km/Y=高度m）。
- **v5.10**(2026-05-13) 航迹关注功能（右键关注/取消关注，空心三角+置顶）。
- **v5.11**(2026-05-14) 单批航迹点数上限 `displayConfig.max_track_points`(200)，按 batch 独立 FIFO。
- **v5.12**(2026-05-15) 压测性能开关（标签限频刷新、图表标签可关、增量刷新、道路点下发可关）。
- **v5.13**(2026-05-18) P显/B显/高显批量绘制（`DetBatchItem`/`TrackBatchItem`/batch item + `drawPoints/drawLines`），16ms 合并 repaint，增量 bounds，命中回退 `pointInfoAt`。
- **v5.14**(2026-05-18) 扇区数据流降载（`sector_display_enabled`/`sector_display_data_enabled`），隐藏图元移出 scene。
- **v5.15**(2026-05-22) `伺服控制` 移到"参数设置"tab + `伺服归北`（归北0°→4s后寻位0°）；TWS/TAS 下发后待机态自动进入工作。
- **v5.16**(2026-05-29) 旧 GCS `0x52` 上报开关 `network.gcs.target_report_enabled`(默认false)；新增 `EdgeRadarReporter`（UDP JSON `type=target`/`heartbeat`，默认 `192.168.1.100:9001`）。

### v5.17 (2026-06-15)
- **边缘终端目标上报节拍**：`EdgeRadarReporter` 按协议维护普通航迹最新点缓存，定时遍历当前普通航迹并逐目标发送单包 `type=target` JSON；无目标时不发送目标包，仅保留心跳。
- **联调IP约定**：边缘终端默认对端为 `192.168.1.100:9001`；显控本机网卡需在同一 `192.168.1.x` 网段，通常使用外部链路本机地址 `192.168.1.5/24`。多网卡环境建议将 `local_ip` 显式设为该网卡地址。

### v5.18 (2026-06-17)
- **边缘目标上报配置化**：`network.edge_radar_report.target_report_interval_ms` 控制目标上报周期，默认 `4000ms`；`max_target_distance_m` 控制上报距离上限，默认 `2000m`。
- **边缘识别结果回传**：新增 `EdgeRadarResultReceiver`，默认监听 `network.edge_radar_result.local_ip/local_port`（`0.0.0.0:9002`），接收边缘终端回传的 `type=recognition_result` JSON 光电识别结果，解析 `track_id/is_drone/count/detections/timestamp` 并保留完整调试日志。
- **总控MQTT上报**：新增 `TotalControlMqttClient`，使用 QtNetwork 实现 MQTT 3.1.1 QoS0 发布；默认 topic `x576/target/result`，按 `track_id` 将雷达侧结果放入 `radar` 对象、光电侧结果放入 `optical` 对象，不做本地融合，由总控端自行融合。
- **总控协议文档**：新增 `docs/total_control_mqtt_protocol.md`，说明 MQTT Broker、topic、JSON 字段、`radar/optical` 对象和总控端处理建议。

### v5.19 (2026-06-28)
- **地图偏移补偿改为坐标空间无关方案**：彻底修复"P显圆圈相对真实位置右偏，且改量程约 1 秒后漂到错误位置"的外场问题。
  - **根因**：旧 `PPIView::calculateMapDisplayParameters` 用 **PPI 子区域自身比例**（`qMin(ppiW,ppiH)/2` + 量程算出的 `metersPerPixel`）把"PPI 中心相对地图中心的像素偏移"换算成米→经纬度，再在 C++ 侧减到地图中心；但地图最终是按 **整窗地图(centralWidget)的比例** 在 WebEngine(CSS 像素)中渲染。两个坐标空间（Qt 逻辑像素 vs WebEngine CSS 像素、PPI 比例 vs 地图比例）不一致，叠加 `AA_EnableHighDpiScaling` 的文本缩放/不同分辨率，导致补偿后的目标中心算错；`map.setBounds` 的平移动画把地图在约 1 秒内"漂"到这个错误目标，表现为先对后偏。
  - **修复**：C++ 只输出 **PPI 中心占地图容器尺寸的比例**（`pixelOffset / centralWidget尺寸`，无量纲、DPI/分辨率自动抵消）；HTML 端用 **地图自身比例**（`range×1000/containerWidth`）把该比例还原为像素→米→经纬度偏移，反推地图中心，使雷达(GCJ02)精确钉在 PPI 中心像素，任意量程/分辨率/DPI 均对齐。
  - **接口变更**：`PPIView::calculateMapDisplayParameters` / `radarCenterChanged` 增加 `offsetRatioX/Y`；`MapProxyWidget` 新增 `syncRadarToMapWithOffset` 并存储 `m_offsetRatioX/Y`，`centerOn`/`setCenterOn` 增加偏移比例参数；`chooseMap` 通过 URL `offx/offy` 把偏移透传给首帧。
  - **北斗实时上报路径修正**：`geoLocationUpdated → syncRadarToMap` 旧版用原始经纬度居中、丢失偏移补偿（有北斗时每秒覆盖一次正确位置）；现复用已存储偏移比例，与 `onGeoLocationChanged` 路径一致。
  - **4 图层统一 GCJ02**：`indexNoL.html`/`index.html`/`index3d.html` 补齐原本缺失的 WGS84→GCJ02 转换（此前仅 `indexS.html` 有），避免切换图层后偏移行为不一致。
  - **关闭 setBounds 动画**：4 个 html 的 `new AMap.Map(...)` 增加 `animateEnable: false`，改量程瞬间到位，消除动画期间的可见漂移。
- **注意事项**：程序从 `applicationDirPath()` 加载 html，构建后须确保更新的 `htmls/*.html` 已复制到运行目录；`maplibre.html`（OSM 引擎，已禁用）使用 WGS84 瓦片、无需 GCJ02 转换，保持原样。

### v5.20 (2026-06-28)
- **主界面版本号显示**：主界面标题区原英文名 `Radar Control Platform`（`SubtitleLabel`）改为显示软件版本号。
  - 版本号常量集中定义在 `Basic/DispBasci.h` 的 `APP_VERSION_STR`，发布新版只改这一处（同时建议同步 `mainoverlayout.ui` 占位文本）。
  - `MainOverLayOut` 构造中 `ui->SubtitleLabel->setText(APP_VERSION_STR)`，tooltip 改为“软件版本号”；`mainoverlayout.ui` 的 `SubtitleLabel` 文本同步为占位版本号（运行时由 setText 覆盖）。
  - 中文标题 `TitleLabel`（雷达控制平台）保持不变。

### v5.21 (2026-06-29)
- **渲染后端可配置（修外场两类显示故障）**：
  - **故障**：① 打开某些录屏软件后显控黑屏（切桌面再切回仍黑）；② 部分电脑持续黑屏闪烁。根因均为 **QtWebEngine(Chromium) 的 GPU 加速渲染与显卡驱动/桌面合成(DWM)、录屏软件钩取冲突**。
  - **方案**：新增 `[webengine]` 配置项，逐台调试、无需重新编译；相关设置在 `QApplication` 创建前生效（故 `main.cpp` 提前 `ConfigManager::load`）。
    - `gl_backend`：`desktop`(默认,`AA_UseDesktopOpenGL`+Core3.3) | `angle`/`gles`(`AA_UseOpenGLES`,更兼容) | `software`(`AA_UseSoftwareOpenGL`,最兼容)。
    - `disable_gpu`：true 时给 Chromium 加 `--disable-gpu --disable-gpu-compositing`（经 `qputenv("QTWEBENGINE_CHROMIUM_FLAGS", ...)`），关闭 WebEngine GPU 加速/合成（地图为2D瓦片，软件渲染足够）。
    - `extra_chromium_flags`：追加自定义 Chromium 参数（空格分隔），高级调优用。
  - **排障顺序**：先 `disable_gpu=true`；仍异常 → `gl_backend="angle"`；再不行 → `gl_backend="software"`。
  - **改动**：`main.cpp`（QApplication 前按配置设 `AA_Use*OpenGL` 与 Chromium flags，`setupOpenGL` 仅 desktop 后端调用）、`Basic/ConfigManager.h`（`webEngineGlBackend/webEngineDisableGpu/webEngineExtraChromiumFlags`）、`config.toml` `[webengine]`。

### v5.22 (2026-06-29)
- **黑屏闪烁修复定稿（双显卡笔记本，如联想 Y9000P）**：v5.21 的 `disable_gpu` 方案不适用本工程——离线高德 AMap 2.0 基于 **WebGL** 渲染，关 GPU 会导致**地图整块空白**。真正根因是 **NVIDIA Optimus 双显卡（Intel 核显 ↔ NVIDIA 独显）切换使 WebEngine 共享 GL 上下文失效**，在"切软件 / 点窗口 / 切地图"时整屏黑屏闪烁。
  - **默认改为 ANGLE**：`gl_backend` 默认 `"angle"`（`AA_UseOpenGLES` → Direct3D11），Windows/双显卡上最稳，且 WebGL 地图正常显示。`config.toml`、`ConfigManager` 默认、`main.cpp` 默认均改为 `angle`。
  - **强制独显**：`main.cpp` 顶部导出 `NvOptimusEnablement=1` 与 `AmdPowerXpressRequestHighPerformance=1`（仅 `Q_OS_WIN`），把双显卡笔记本钉在独显上，消除核显↔独显切换这个根因。
  - **disable_gpu 警示**：`disable_gpu=true` 会让 WebGL 地图空白，本工程一般保持 `false`；config 注释已标注。
  - **排障顺序**：`angle`(默认) → 不行 `desktop` → 再不行 `software`。
  - **部署注意**：使用 `angle` 需随程序部署 ANGLE 运行库（`libEGL.dll`/`libGLESv2.dll`/`d3dcompiler_47.dll`，windeployqt 默认会带）。
  - 现场无需重新编译也可临时缓解：NVIDIA 控制面板 → 将本程序 exe 指定为"高性能 NVIDIA 处理器"。

### v5.23 (2026-06-29)
- **渲染诊断日志（外场黑屏/闪烁排查）**：`main.cpp` 启动时打印实际 GL 后端、Chromium flags、以及真正在用的 GPU/驱动（`GL_VENDOR/RENDERER/VERSION`，经临时 `QOpenGLContext` 查询）——可判断 ANGLE 是否走 D3D11、独显是否生效（NVIDIA vs Intel）、是否落到软件渲染。
- **WebEngine 渲染进程崩溃监控**：`mapprox.cpp` 连接 `QWebEnginePage::renderProcessTerminated`，记录终止状态/退出码（黑屏的直接信号）。

### v5.24 (2026-06-29)
- **激光侦察上报（独立模块，仅"激光终端"功能）**：按 `docs/光电跟踪与激光上报协议.md` 第4节实现，**不含**光电转台引导(模式2)与手动跟踪。
  - **触发**：右键航迹 →「激光上报[批次]」对该单一目标按周期（默认1s）持续发送侦察帧给激光控制终端；再次右键「关闭激光上报」或目标消批则停止（消批补发一帧 `cancelFlag=1`）。单目标，切到新目标会自动停旧目标。
  - **本地存档**：每次"下发一个新目标"时把当时信息（时间/批次/类型/距离/方位/俯仰/速度）追加保存到 txt（UTF-8，按天文件，`[laser].save_dir`），格式同 `GuideTrack_CN` 样例。
  - **状态帧心跳(0x0200)**：模块启用后 1s 周期常驻发送状态帧（`LaserStatusData(13)`，隐含心跳，workState/faultState=0x0F、workMode=0、scanAreaCount=0），与是否有活动目标无关；有活动目标的那一拍再附带侦察帧(0x0300)。与 RadarAPP `sendLaserReport` 行为一致。
  - **协议结构**：`Basic/Protocol.h` 新增 `LaserDataTime/LaserFrameHeader/LaserFrameTail/LaserReconData/LaserTargetInfo/LaserStatusData/LaserScanRangeInfo`（`#pragma pack(1)`）；侦察帧=Header(18)+ReconData(12)+N×TargetInfo(64)+Tail(4)，`contentLen`=N×64、`dataLen`=12+N×64；校验=frameType起按字节累加低16位；类型映射 targetRecResult(0/1)→激光类型(0普通/1无人机)，trackQuality 由 statMethod(0→7,1→3,else0)。**已对照 RadarAPP `util/Protocol.h` 校准字节级一致（Header 实为18字节，文档“20”系笔误）。**
  - **网络**：本地绑定 `192.168.101.9:9009`，发往激光终端 `192.168.101.10:9009`（单端口双向）；sender=2100/receiver=5100。
  - **开关与独立性**：`config.toml [laser].enabled`（默认 false）。**关闭时不创建模块、右键菜单不出现该项、不占网络资源**；与 GCS/边缘/MQTT 等其它上报下发互不关联。
  - **模块**：新增 `Controller/laserreportmanager.h/.cpp`（镜像 `EdgeRadarReporter`：`reportTrackPoint` 缓存最新点、`removeTrackPoint` 消批、`start/stopReport` 由右键驱动、1s 定时发送）；`PPIView` 注入 `setLaserReportManager` 并在 `trackPointAdded/trackRemoved` 喂数据、`onTrackLabelRightClicked` 加菜单项；`mainwindow.cpp` 创建并接日志；已同步 `CMakeLists.txt` / `DispCtrl.pro`。
  - **（v5.24 时）暂未实现**：模式1 自动10目标上报、激光端控制指令接收——已在 v5.25 补齐。

### v5.25 (2026-06-29)
- **激光模块补齐"激光终端"全部对应功能**（在 v5.24 单目标侦察+状态心跳基础上）：
  - **模式1 自动上报（全部目标）**：右键航迹「开启/关闭激光自动上报(全部目标)」全局开关；开启后每拍上报当前全部普通航迹（最多 `LASER_MAX_TARGETS=10`），无目标也发空侦察帧（`targetCount=0`）。优先级：自动上报开 → 走全部目标；否则 → 单目标（`m_activeBatch`），与协议第6节一致。侦察帧构建改为多目标向量版 `buildReconFrame(QVector<PointInfo>, cancelFlag)`。
  - **激光端控制指令接收 + 控制响应**：`m_socket` 绑定端口(9009)接收激光端(sender=5100)控制帧 `0x0100`；校验帧头/尾/校验和/发送方后解析 `LaserControlHeader(6)+内容`：
    - `0x0101 授时`(`LaserTimeSync`)、`0x0201 工作状态`(`LaserWorkStateSet`，Bit0..3；更新本地 `m_workState` 并反映到状态帧)、`0x0104 搜索范围`(`LaserSearchRange`，**float 字段按大端解析** `bswapFloat`；更新本地 `m_scanRanges` 列表并反映到状态帧 `scanAreaCount`+`LaserScanRangeInfo`)；
    - 每条已知指令回 `0x0101` 控制响应 `LaserControlResponse(cmdSeq,result=1)`，未知类别回 `result=0`；
    - 同时 `emit laserTimeSyncCommand/laserWorkStateCommand/laserSearchRangeCommand` 供外部驱动雷达。
  - **控制指令真正驱动雷达**（`mainwindow.cpp` 接线，受 `[laser].apply_control` 门控，默认 true）：
    - **工作状态(0x0201)** Bit0 → 发射开关：`laserWorkStateCommand` → `TranRecControl{recv=1, tran=Bit0}` → `CON_INS->sendTRParam`（复用主界面"阵面发射"同一下发通道）。
    - **授时(0x0101)**：雷达时间来自北斗，无外部授时下发通道，仅回响应+日志，不驱动。
    - **搜索范围(0x0104)/波形下发**：按需求暂不驱动（信号已抛出，未接线）。
    - `apply_control=false` 时：控制指令仍正常接收+回响应+日志，但不改变雷达状态。
  - **协议结构补充**：`Basic/Protocol.h` 新增 `LaserControlHeader/LaserTimeSync/LaserWorkStateSet/LaserSearchRange/LaserControlResponse` 及常量 `LASER_FT_CONTROL=0x0100 / LASER_FT_CTRL_RESP=0x0101 / LASER_CT_TIME_SYNC=0x0101 / LASER_CT_SEARCH_RANGE=0x0104 / LASER_CT_WORK_STATE=0x0201`。均对照 RadarAPP `parseLaserFrame`/`sendLaserControlResponse` 校准。
  - **独立性**：全部仍受 `[laser].enabled` 控制，关闭时不接收/不发送/右键无项；自动上报默认关(右键手动开)；驱动雷达额外受 `[laser].apply_control` 门控。

### v5.26 (2026-07-05)
- **固定DPI策略**：新增 `ui.dpi_policy`，默认 `fixed`。fixed 模式在 `QApplication` 创建前禁用 Qt 高DPI缩放、固定 `QT_SCALE_FACTOR=1`，并通过 `AA_Use96Dpi`/`QT_FONT_DPI=96` 固定字体DPI，使显控界面不跟随 Windows 125/150/175% 显示缩放和分辨率切换后的系统DPI变化，避免固定布局在高文本缩放下挤压重叠。
- **内部UI缩放轻量版**：新增 `ui.ui_scale`，默认 `1.0`，启动时读取并限制在 `0.70~1.60`。当前覆盖主界面/左右面板、PPI设置面板、P显工具栏、B/H工具栏和图表字体等显控关键控件；不影响雷达坐标、地图坐标和目标数据。启动日志会同时输出配置值和实际应用值，便于确认运行目录配置是否生效。
- **DPI兼容回退**：`ui.dpi_policy="system"` 时保留原 `AA_EnableHighDpiScaling` 行为，用于高DPI/地图现场兼容回退。地图偏移补偿仍使用 PPI 中心相对容器比例，不依赖绝对DPI。
- **DPI诊断日志**：启动日志记录 `dpiPolicy`、Qt逻辑尺寸、估算物理尺寸、devicePixelRatio、ScaleHelper布局因子和渲染属性，便于对比 100/125/175% 缩放现场表现。

### v5.27 (2026-07-08)
- **离线RAE点迹绘制**：新增 `[offline_rae]` 配置和 `RaeDatasetReader`，显控读取 MATLAB 导出的 `RAE1` little-endian 二进制文件并转换为 `PointInfo`，不直接解析内部 `.mat`。离线数据标记为 `statMethod=250`，直投 PPI/B显/H显显示对象，避免触发航迹表、总控 MQTT、激光上报等实时链路。
- **两类脚本数据入口**：`offline_rae.enabled=true` 时在右侧功能按钮区动态显示 `ADSB点迹` 和 `对比点迹` 两个按钮，分别读取 `adsb_file` / `compare_file`。默认点击前显清，按数据最大距离自动扩大量程，日志写入文件并同步到显控日志区。
- **MATLAB导出脚本**：新增 `matlabScript/export_rae_bins.m`，按 `adsb.m` 和 `compare.m` 的筛选逻辑导出 `adsb_rae.bin` / `compare_rae.bin`。二进制记录包含 `timestamp_ms/type/batch/range_m/azimuth_deg/elevation_deg/snr/speed/amp/targetRecResult`，距离默认从 km 转 m；离线轨迹统一按航迹绘制，颜色码与 MATLAB 一致：101=青色、102=红色、103=蓝色。离线轨迹跳过 batch 标签和 `max_track_points` 截断。

### v5.28 (2026-07-10)
- **3D最新航迹显示**：B显/H显后新增可分离的`3D显`，使用本地 Three.js r128 + `QWebEngineView/QWebChannel` 显示当前量程和H显高度范围内的最新航迹点，支持拖动旋转、滚轮缩放、视角复位和悬停详情。
- **统一数据语义**：以`type+batch`为航迹键，`statMethod==2`精确消批；颜色、显隐、无人机模式、点大小、显清和离线RAE直投与现有显示同步。三维物理坐标采用水平投影距离与协议目标高度，Web端只负责归一化和批量渲染。
- **限频和容错**：航迹增量最多20批/秒，隐藏页停止跨进程推送，WebGL资源全部由qrc离线打包；渲染进程异常采用一次重载后降级占位策略。新增可选`DISPCTRL_BUILD_TESTS`坐标换算QTest。

---

## 联系与贡献

- **项目仓库**: GitHub — `DanielWuxiaoxiao/DispCtrl`，当前分支 `x576`
- **问题反馈**: GitHub Issues
- **贡献流程**: 参见 `docs/book/13_code_guidelines.md`
  - 统一 clang-format 风格
  - 关键模块需有单元测试
  - 提交信息：简短主题 + 关键说明

---

### v5.29 (2026-07-14)
- **X576 internal protocol 2026-07-14**: `0xDD01` now decodes the documented 56-byte point record tail as `targetConfidence` plus `targetRecResult`; the frame-level `radarId` (`0..3`) is carried into `PointInfo::radarId`. `0xDE02` adds the documented `radarId` before a 21-byte reserve area while preserving the packed 39-byte report size.
- **Track-frame compatibility**: `0xEE01/EE02/EE03` remain `mesID + trackNum + N * trackInfo` because the updated tables do not specify a radar/array ID field. The table's 58-byte note conflicts with the field sum and the implementation layout of 61 bytes. The receiver does not guess an ID position; track multi-array separation requires a follow-up protocol definition.
- **External control compatibility**: the 2026-07-03 additions are inside the existing opaque 512-byte system-control table, so `ExternalSystemControl512` remains pass-through and the outer frame does not change.
- **Validation**: `Basic/Protocol.h` has compile-time size assertions for DD01 point records, EE track records, and DE02 BIT reports; receive logs identify DD01/DE02 radar IDs and reject out-of-range DD01 IDs with a warning.

### v5.30 (2026-07-14)
- **Four-array health management**: `MainOverLayOut` caches `BITReport` by `radarId` and the health window now contains four array tabs (`0..3`). Software status remains shared, while each tab has independent hardware BIT buttons, temperatures, yaw/scan angles, and last-report time. Tab switching rebinds the existing update path to the selected array cache.

### v5.31 (2026-07-14)
- **Four-array PPI scan overlay**: `PolarDisp/ScanLayer` now caches the latest `BITReport` for each `radarId` and draws an independent dashed scan-sector boundary, solid scan line, and colored current-angle label for arrays `0..3`.
  - The configured scan range remains in each array's local coordinates. At render time, each boundary and scan line is rotated by that array's `yaw` into the PPI north-referenced azimuth system (`global = yaw + local`).
  - Array colors are used only by the scan overlay. Detection/track point colors, filtering, batch statistics, and table behavior remain unchanged.

### v5.32 (2026-07-14)
- **Four-array power control restored**: the existing `BatteryControl` dialog is visible from the parameter-settings tab. Its four checkboxes build one `BatteryControlM` command for arrays 1-4, send through `Controller::sendBCParam`, and write the four requested states to the command log. The button remains in the compact parameter-settings grid.

### v5.33 (2026-07-14)
- **Track table initial fill**: the hidden drone-track tab performs its first column-width fill after the tab becomes visible and its viewport has a real width. The last column also stretches to the viewport edge, while the other columns remain interactive for manual resizing.

### v5.34 (2026-07-15)
- **Multi-array BIT diagnostics and PPI rendering**: every received `BITReport` now logs all raw fields, decoded temperatures/angles, normalized angles, sub-array power bytes, and reserved bytes. PPI scan-range rendering uses the original shared translucent sector/afterglow; received arrays add separate color-coded scan lines using `yaw + scanAngle`, normalized to the north-referenced `0..360` range.

### v5.35 (2026-07-15)
- **Track table equal-width layout and scan labels**: normal and drone track tables initialize all columns with equal widths while keeping interactive manual resizing. Multi-array PPI scan lines again show the array ID, panel-local scan angle, and north-referenced angle in the corresponding array color.

### v5.36 (2026-07-15)
- **Display-only array numbering**: protocol and internal array indexes remain `radarId=0..3`, while all user-facing array labels use `1..4`.

### v5.37 (2026-07-15)
- **Linux deployment package**: `docker/docker_build.sh 1804/2004` now produces one x86_64 package targeting Ubuntu 18.04+ and Ubuntu 20.04. The helper uses the Ubuntu 18.04 snapshot baseline and no longer mounts the same `deploy` directory twice, so a successful package command exits cleanly.

- **Offline runtime dependencies and instructions**: `scripts/package_linux.sh` recursively collects executable, Qt, WebEngine, and plugin dependencies, explicitly bundles Qt ICU SONAME libraries plus the existing libstdc++/xcb runtime set, and copies `scripts/deploy_readme.txt` to package-root `readme.txt`. `docker/docker_build.sh 1804` compiles one Ubuntu 18.04-baseline binary and additionally ships version-matched `.deb` closures in `offline-deps/ubuntu1804`, `ubuntu2004`, `ubuntu2204`, and `ubuntu2404`; target operators run `sudo ./install_offline_deps.sh` once without network access. The installer preserves target-owned glibc, dynamic loader, X11 display service, and GPU driver.
- **Offline package validation**: `docker/test_offline_deps.sh` installs each bundled closure in clean `--network none` Ubuntu 18.04/20.04/22.04/24.04 containers and checks `DispCtrl` plus `QtWebEngineProcess` with `ldd`. Ubuntu 18.04/20.04 bundles use `libssl1.1`; Ubuntu 22.04/24.04 bundles use `libssl3`. This validates package runtime-library installation; GUI/X11 and hardware-driver behavior still require validation on the real Ubuntu desktop target.
- **Developer build instructions**: `docker/README.md` records the Windows -> WSL -> Docker -> deploy artifact workflow, exact `1804`/`2004` command behavior, artifact checks, and the distinction between bundled application libraries and target-owned graphics/system runtime dependencies.

### v5.38 (2026-07-16)
- **Enabled-array PPI scan coverage**: a TAS/TWS scan range is treated as a single panel-local angular range. `ScanLayer` now renders that translucent range and green afterglow once for every enabled panel, with fixed installation rotations of `0/90/180/270` degrees for panel IDs `0/1/2/3`. The sector fill/boundary and scan line use the panel color, while every panel keeps the same green afterglow. Thus a local `-15..15` range becomes `345..15`, `75..105`, `165..195`, and `255..285` as panels are enabled.
- **Control/data separation**: `BatteryControlM` is connected directly to `ScanLayer` so the PPI immediately hides disabled panel sectors. Per-panel BIT data still determines only the color-coded real-time scan line and its displayed local/north-referenced angles; it does not move the configured sector.

### v5.39 (2026-07-20)
- **Ubuntu 24.04 offline-install compatibility**: the offline dependency builder now uses `--no-install-recommends`, so new bundles do not contain optional ALSA UCM/topology packages. The installer always skips those host-managed packages, and preserves an existing `libasound.so.2` rather than unpacking the release's `libasound2`/`libasound2t64` package over it. When ALSA is absent, the minimal ALSA runtime remains available for Qt WebEngine. It configures only the release's unpacked closure instead of invoking global `dpkg --configure -a`, so a pre-existing target package error is not retried or misreported as a deployment failure.

### v5.40 (2026-07-26)
- **Legacy GCS multi-track reporting**: with `network.gcs.target_report_enabled=true`, each PPI right-click `目标下发` adds (or refreshes) that batch in the active `0x52` reporting set. It no longer replaces previously selected batches, so several manually selected tracks continue sending their latest points concurrently. A batch is removed only when its own `statMethod==2` cancellation reaches `TrackManager`; a local display clear retains the active GCS subscription set. Subscription, refresh, retention, and cancellation logs include the active batch list for field verification.

### v5.41 (2026-07-28)
- **Default panel count**: `displayConfig.default_panel_count` selects the first contiguous `1..4` panels enabled when the display starts; the default is `1`, so only panel 1 has an initial PPI scan sector. Missing, unreadable, non-numeric, or out-of-range configuration falls back to `1` and records a warning. The same value initializes the panel-enable dialog checkboxes. It is an operator-interface default only and does not automatically send an `AA01` panel power command at startup.

### v5.42 (2026-08-20)
- **TBD/协同同批号隔离**：P显 `TrackManager`、扇区 `SectorTrackManager` 与 `RadarDataManager` 的航迹历史缓存统一以 `(PointInfo.type, PointInfo.batch)` 作为唯一键。TBD (`type=3`) 与协同 (`type=4`) 即使使用相同批号并交替到达，也分别维护历史点、最新点、标签和连线；任一路 `statMethod==2` 消批只清理本类型的同批航迹，不影响另一路。B显/H显/3D显原本已按该组合键管理。
- **运行开关**：`[displayConfig].iftbd` 和 `ifxietong` 分别决定 TBD/协同接收管理器、端口监听、P/B/H/3D 显示连接及对应 UI 是否启用；当前默认配置均为 `true`，压测场景可按需关闭。

---

**文档结束**

*本文档整合了 DispCtrl 项目全部 34 个 MD 文档的核心知识与最新实现状态，为 AI 开发助手提供准确的项目全局上下文。重大修改后请同步更新版本历史与注意事项章节。*
