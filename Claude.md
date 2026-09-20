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
  - **历史说明**：上述入站控制/反向驱动描述已由 v5.43 的“仅9009出站”范围取代；现行代码不连接激光端入站控制，也不存在 `apply_control` 配置。

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

### v5.43 (2026-09-09)
- **引导光电跟踪（持续，9009出站）**：P显航迹右键入口改为「引导光电跟踪(持续)」。启用 `[laser].enabled=true` 后，对选中普通航迹每秒发送状态心跳和该航迹的侦察帧；切换目标只保留新目标，消批补一帧 `cancelFlag=1` 后停止。只使用 `192.168.101.9:9009 → 192.168.101.10:9009`（均可由 `[laser]` 配置覆盖），不接入光电转台引导或其它端口。
- **与当前 RadarAPP 的9009字节协议对齐**：出站状态/侦察 `frameType` 分别为 `0x0002` / `0x0003`（小端在线字节 `02 00` / `03 00`）；默认状态为跟踪模式，携带范围ID=1的 `45..135° / 0..60°` 扫描区域。该版本不连接9009入站处理，因此激光端报文不会反向控制阵面。

### v5.44 (2026-09-09)
- **指控无人机目标上报占位模块**：新增独立 `CommandTargetReporter` 和 `commandtargetreportprotocol.h`。它仅监听普通航迹及其分类结果；每个已识别无人机的航迹点立即上报，TBD/协同航迹不参与。分类结果晚到时会补发该批次缓存的最新点。
- **可配置传输与协议**：`[command_target_report]` 提供启动默认开关、TCP/UDP、本地/对端 IP 端口、JSON/固定 52 字节 binary 载荷及 binary 大小端配置。TCP 自动重连且保留有限待发队列；UDP 状态只表示本机 bind/发送成功，不宣称对端已收到。对端接口确认后优先只调整协议头中的序列化函数，详情见 `docs/command_target_report_placeholder.md`。
- **显控状态**：参数设置区新增本次运行有效的“指控无人机上报：开/关”按钮，不改写配置文件；健康管理窗口增加该链路的即时状态。经纬高沿用 PPI 北向方位的局部切平面换算，不重复叠加阵面偏航。当前 `PointInfo` 没有源航迹时间，故上报中的航迹时间为显控接收时间。

### v5.46 (2026-09-11)
- **总控 UDP 定版协议替换占位实现**：移除 `CommandTargetReporter`、其 TCP/JSON/52 字节占位协议及 `[command_target_report]` 配置，新增独立 `CommandControl/` 目录。目录内按职责拆分小端字节级编解码、鲁棒配置读取、DDA4 JSONL 会话记录、UDP 状态机和独立三页控制窗口；详见 `docs/command_control_protocol.md`。
- **协议与网络**：所有报文使用 18 字节 `0x5AA5`/`0x10` 报头，组播 `224.0.1.2:21505`。专用 `CommandControlTransport` 在工作线程独占 UDP 套接字，GUI 线程只处理已转发的协议状态和 UI。启动即周期发送 DD25、DDA1；DD31 发现/更新总控单播端点，驱动 DD33 登录或退出，DD34 完成结果确认。DD31、DD33、DD34、DD25、DDA4、DDA1 均严格校验报头和固定长度。`CommandControlRecordWriter` 在第二条工作线程处理 JSONL 的序列化、flush 和回放读取，禁止磁盘 I/O 占用 PPI 线程。
- **DDA4/UI**：普通航迹的识别无人机可自动上报；PPI 航迹右键可逐批开启/关闭手动上报，消批自动清理。总控通信窗口提供登录状态与自动开关、组播设备 DD25 在线状态、DDA4 会话记录和 PPI 回放。窗口通过 `CommandControlWindow` 专属 QSS 复用深色雷达主题，主操作为青绿色、停止/退出为红色，样式不影响其它对话框。DDA4 缓存表和设备表均合并为最多每 200 ms 更新一次，防止打开窗口后高频数据触发整表重绘。RAE 与 WGS84/ENU 转换只放在该模块，避免影响现有 GCS、激光或 MQTT 链路。
- **配置与风险**：`[command_control]` 单独剥离嵌套引号并校验数值，默认本机 `192.30.105.13`、总控 `192.30.105.10`、设备 ID `0x11474202`。DD25 协同状态、DDA4 的设备/数据率/目标属性/质量/RCS/干扰/更新方式/相对延时，以及 DDA1 的工作、健康、设备、扫描范围和初始辐射状态均从该节读取，并在启动日志输出最终生效值。DDA1 经纬高和 DDA4 局地坐标换算只使用 DD05 阵面真值；GCS 下发、DDA4 上报和 DDA4 回放共同调用 `Basic/wgs84coordinate.h` 的已验证局地 ENU→WGS84 公式，目标高固定为“雷达高 + ENU 上向分量”，避免两份实现分歧且不改变既有 GCS 语义。未收到 DD05 时不发送 DDA1/DDA4。`dda1_radar_id` 选择提供 BIT 偏航角的阵面，TAS/TWS 的 AA05 波束参数只在配套 AA03 收到 DE01 成功回送后，才与该 BIT 偏航角组合为正北参考的 DDA1 扫描范围。DDA1“搜索雷达”使用 `dda1_device_type = 2`（`0x02`）。`dda4_log_interval_ms` 只控制 DDA4 摘要日志，完整原始 DDA4 仍在 JSONL；设为 `0` 可短时逐条输出。`packet_hex_log_enabled` 默认关闭，只有字节级联调时开启；它只额外打印本机与当前总控间五类控制报文的原始字节，DDA4 原始字节仍只保存到 JSONL。
- **总控字段级调试日志**：DD31、DD33、DD34、DD25、DDA1、DDA4 六类报文均在收发后按字节级解析为结构体字段再写入项目 `LOG_INFO`，每项含 UDP 对端、完整公共报头和完整负载（DD31/DDA1/DDA4 备份字段也保留）。仅本机发出或当前 DD31 确认总控发出的报文进入这组调试日志；其余组播设备仍可更新在线状态或写 DDA4 JSONL，但不污染总控联调日志。主界面日志栏仅显示 DD31 端点变化、DD33/DD34 登录状态及本机 DDA1 经纬高/扫描范围，周期 DD25/DDA4 只落项目日志，避免刷屏；DDA4 的完整项目日志受 `dda4_log_interval_ms` 限速，设为 `0` 可逐条输出，JSONL 始终逐条保存；任何报头、长度、字段或主控 ID 校验失败均写 `LOG_WARNING`。
- **外部链路航迹范围**：`PointType::TBDPointType` 与 `PointType::CooperativeTrackPointType` 仅参与显控显示、表格和回放，禁止进入 GCS、激光、总控 DDA4、边缘雷达及 MQTT 的所有下发/上报路径。PPI 航迹标签右键会携带源类型，特殊航迹不显示外部操作；并且仅普通 DBT 航迹消批才会清理对应的外部上报状态，防止特殊航迹与普通航迹批号相同而误清理。
- **DD25/DDA4 月内时间位域**：4 字节时间字段不是本月累计毫秒数。按小端 32 位位域依次存放 `ms[0..9]`、`sec[10..15]`、`min[16..21]`、`hour[22..26]`、`day[27..31]`；`day` 使用本月自然日号，月初 1 日必须写 `1`，收到 `day=0`、超范围时分秒毫秒即丢弃报文并记录解析失败。`CommandControlProtocol::MonthDayTime` 是 DD25 与 DDA4 唯一共用编解码实现；DDA4 JSONL 同时保存原始压缩值和拆分后的时间分量，便于联调。
- **DDA1 高度与 DDA4 回放原点**：DDA1 高度定版为 2 字节有符号 `short`（二补码，`-32768..32767 m`），故 DDA1 总长度为 44 字节；DD05 高度超范围时拒绝发送而不截断。DDA4 与 GCS 均使用 `Controller::traInfoProcess` 的普通 DBT `PointInfo` 及同一 WGS84/ENU 公式。每条 DDA4 JSONL 记录额外保存写入时的本机 DD05 雷达经纬高，回放严格使用该历史原点反算 PPI RAE，绝不改用回放时当前 DD05；旧记录没有历史原点时跳过并告警。

### v5.47 (2026-09-12)
- **子阵电源可视化**：健康管理窗口的四个阵面页均新增 6×6 子阵电源状态图。数据复用既有 `BITReport (0xDE02)::subArrayPower[5]`，其上游电源状态来自基础雷达/显控 BB01 链路；不新增 UDP 解析或线程。未收到该阵面 BIT 时 36 格均显示黄色“未上报”，收到后 bit0..bit35 分别映射按行编号的 1..36 号电源（1 绿色正常、0 红色故障），且右上角为 1 号、向左递增、下一行右侧从 7 号开始，左下角为 36 号。第 5 字节 D4..D7 始终忽略。
- **全零报文临时防抖**：现场日志显示正常全 1 位图与全 0 位图高频交替。子阵电源显示因此改为：完整正常位图 `FF FF FF FF 0F` 立即全绿；完整全零位图必须连续 100 帧才确认全红，1..99 帧保留最后一次确认状态（尚无确认状态时保持黄色）；含有 0/1 混合位的报文仍立即逐路显示，避免掩盖真实的单路故障。达到阈值和从已确认全零故障恢复时写项目日志。

### v5.48 (2026-09-13)
- **本地测试无人机航迹**：新增 `Controller/TestTrackGenerator`，仅在 `[test_track].enabled=true` 时创建并在参数设置区显示“生成测试航迹”。一次点击并行产生 2 或 3 条普通 DBT `PointInfo` 航迹（默认各 300 点、500 ms、15 m/s），每条均标记为无人机；所有点直接从 `Controller::traInfoProcess` 注入既有 PPI、表格、GCS、激光和总控 DDA4 下游，绝不伪造 UDP 数据或影响 TBD/协同链路。位置按 `QElapsedTimer` 的实际经过时间沿固定 RAE 视线匀速积分，`range/elevation/altitute/speed` 与 DDA4 ENU 三轴速度、经纬高换算保持同一运动模型，避免显示速度与上报速度矛盾。结束时逐批发送 `statMethod=2`，再次点击从第一个点重新开始。
- **无阵面联调原点**：开始本地测试时，生成器仅一次读取 `[radar]` 预存经纬高，并通过专用测试信号提供给总控 DDA4；它不会伪装为 DD05 阵面真值，现场 DD05 真值到达后仍可正常覆盖临时原点。DDA4 与 GCS 均继续复用既有 WGS84/ENU 实现。生成开始、每 50 个采样点、结束及每个消批均写项目日志；出站 DDA4 JSONL 同时保存用于 PPI 的源 `batch/range/azimuth/elevation/relative altitude/speed` 与雷达原点，便于逐条核对生成值、PPI 输入和已编码的经纬高。
- **日志 UTF-8**：统一日志文件通过 `QTextStream::setCodec("UTF-8")` 写入；总控报文类型中文通过 UTF-8 解码，避免总控窗口与日志中出现乱码。旧日志仍为历史本地编码，不自动转换。

### v5.49 (2026-09-15)

- **总控 DD31 与 DDA4 批号纠正**：DD31 明确为 `224.0.1.2:21505` 组播发现报文；总控通信窗口持续显示“已收到 DD31 组播”及其提供的 DD33/DD34 单播端点，避免将发现报文误称为单播。DDA4“本机设置批号”不再二次分配 `1,2,...`，而是严格透传普通 `TRAINFO/PPI PointInfo.batch`，因此真实 DBT 航迹和测试航迹使用完全相同的总控上报路径与业务批号。
- **总控字段日志与健康网段**：公共报头版本/标志、DD25/DDA4 时间位域、DDA4 更新方式/目标属性、DDA1 工作状态和有符号俯仰在结构化日志中按 bit 段和原始值共同输出；原始十六进制打印仍仅由 `packet_hex_log_enabled` 控制。本版本健康管理雷达网络固定显示本机 `192.168.64.4`、阵面对端 `192.168.64.3`，并将对端默认值固化于 `[health_network]`。
- **总控备用 NTP 授时**：新增独立 `CommandControl/NtpTimeSync`，以标准 UDP `192.30.105.10:123` 查询 NTP，在 GUI/PPI 线程中仅用非阻塞 `QUdpSocket` 和定时器等待应答，不阻塞航迹绘制。有效应答按 NTP 四时间戳公式估计 UTC 后调用 Windows `SetSystemTime` 或 Unix `clock_settime` 更新系统时间；NTP 无响应、应答非法或权限不足只写日志和控制窗口状态，绝不阻断总控 UDP、登录、上报或既有显控功能，便于现场继续使用操作系统的手动/自动授时。配置支持周期重校和 DD31 到达后的 60 秒限流补充校时。

### v5.54 (2026-09-16)

- **总控默认网段、校时与网络状态**：`[command_control]` 默认本机/总控地址更新为 `192.30.106.13`/`192.30.106.10`，备用 NTP 服务器独立设为 `192.30.1.1:123`；DDA4 自动上报默认关闭，仍可在总控窗口按本次运行需要开启。通信控制页新增本机 IP、对端主控 IP、NTP 服务器 IP 三个红绿状态块，采用健康管理相同的“本机网卡启用 + 从指定本机 IP 发起异步 Ping + 三次失败去抖”语义；绿色仅代表 ICMP 网络可达，不替代 DD33/DD34 登录或 NTP 授时状态。窗口增加手动 NTP 校时按钮，成功后更新操作系统时间。协议各发送点每次从系统时钟即时生成北京时间，因此用户运行期间通过 Windows 手动校时后，后续 DD25、DDA4、DD33 等报文自动使用新时间，无需重启。地图 `default_type` 及缺省回退调整为 `0`（无地图）。

### v5.55 (2026-09-16)

- **PPI 激光/总控快捷下发**：`[laser].enabled` 与 `[command_control].enabled` 默认均为 `true`；前者开启时 PPI 显示“激光 (F6)”，后者开启时显示“总控主动上报 (F7)”。按钮和 F6/F7 先检查可用性，失败直接弹出深色主题诊断；通过后才输入批号。右键菜单继续保留，因为已选中航迹而不重复输入批号，但三种入口统一调用同一批号校验和发送函数。输入框预填最近更新的真实 `TRAINFO` 普通航迹，优先识别为无人机的批号；它自动获得输入焦点并全选预填值，回放航迹绝不参与候选。激光入口和“引导光电跟踪（持续）”不再因 9009 本地绑定或光电网段异常而隐藏，点击时才依据健康管理的本机激光网口/激光终端状态给出明确弹窗；总控主动上报严格要求 UDP 已就绪、DD31 后 DD33/DD34 登录成功且批号有当前普通航迹，否则不发包并说明原因。TBD、协同、已消批和不存在批号始终不能经此路径下发。已有 `CustomMessageBox` 扩展为深色主题整数输入框，提示和输入不再使用系统原生 `QMessageBox`/`QInputDialog`；快捷入口只在人工触发时分配对话框和做状态查询，不进入高频 PPI 航迹绘制路径。

### v5.56 (2026-09-17)

- **航迹表与上报状态一致性**：航迹管理、无人机、TBD、协同表统一列序为 `ID、距离、方位、高度、速度、类型、SNR、指控、俯仰`。前 8 列铺满可视区，ID 保持较窄；俯仰固定在右侧水平滚动区。普通 DBT 航迹的“指控”由同一状态源显示为 `否 / 手动 / 自动 / 激光`，外部上报航迹置顶、加粗并使用青蓝底色。TBD/协同仍禁止外部下发，因此指控始终为“否”。
- **PPI 外部上报强调**：总控手动/自动和激光单目标/自动上报状态变化会即时传给 PPI，不等待下一帧。正在上报的普通航迹改用协同航迹色，历史点和最新交互点略放大并提高图层；标签增大加粗、带青蓝填充背景且处于更高图层。用户“关注”样式仍独立存在，不覆盖上报状态。
- **测试航迹离线联调**：仅当批号落在 `[test_track].first_batch .. first_batch + track_count - 1` 时，F6/F7 或右键允许在总控未登录、激光 UDP/网段未就绪时进入“待发送”上报状态并触发表格/PPI 高亮；不发送任何离线 UDP 包，通信就绪后沿现有真实航迹路径发送。真实航迹仍保留登录、UDP 和激光网络校验。总控通信控制页额外显示红绿“登录状态：已登录/未登录”，语义与网络状态块分离。

### v5.57 (2026-09-17)

- **测试航迹与真实航迹的约束边界**：本地测试批号在 F6/F7、右键手动下发及自动模式下均可绕过激光网络、总控 UDP 和 DD33/DD34 登录前置条件，以便无设备现场验证。离线时只保存“待发送”状态、表格和 PPI 强调，绝不伪造 UDP 发送；激光端口绑定恢复并重启模块后仍沿用既有激光/DDA4 编码路径。真实普通航迹仍必须满足原有激光网段或总控“UDP 就绪且已登录”条件。激光自动模式离线时只把测试航迹标为上报，避免错误突出现场真实航迹。
- **DDA4 自动上报范围**：`[command_control]` 新增高度上限、距离起止和方位起止五项默认值（默认 `高度<=100m`、`0~3000m`、`0~360°`）。总控通信页可实时修改，所有识别为无人机的普通航迹（含测试航迹）均使用该判据；高度不设下限，距离为闭区间，方位起始大于终止表示跨正北。测试航迹只豁免网络/登录，并不豁免业务范围。修改不写回 TOML，仅作用于本次运行。
- **航迹列表可读性**：深色航迹表对所有单元格显式设置浅色前景，避免空前景回退为系统深灰；高度列始终使用粗体，外部上报行继续保留青蓝底色、置顶和全行粗体。

### v5.58 (2026-09-17)

- **上报标签残影**：外部上报/关注标签的填充背景此前画在 `QGraphicsTextItem` 原始边界之外，拖动后 Qt 没有把超出部分纳入脏矩形，因而留下拖影。`DraggableLabel::boundingRect()` 现包含背景边距，并在切换高亮状态前通知场景几何变更；背景只按文字原始边界扩展一次。
- **测试范围覆盖**：测试航迹不再绕过自动 DDA4 的高度、距离、方位筛选，只绕过网络和登录前置条件。默认测试集扩展为 10 条、101～110 批号、每条 1200 点（500ms 周期约 10 分钟）；其中包含满足默认 `高度<=100m、距离<=3km` 的无人机、距离或高度越界无人机，以及几何条件满足但非无人机的航迹，便于直接验证自动上报判据和列表强调排序。
- **上报表格排序**：表格排序不再依赖异步刷新时可能过期的“指控”单元格文本，而是在排序和重建行时从总控/激光模块实时读取状态；上报行始终置顶，并改用更醒目的深青背景、浅色粗体文字。

### v5.59 (2026-09-17)

- **自动上报范围确认与全量重判**：总控通信页的高度、距离和方位输入改为“编辑后点击 `确认并应用范围`”才写入本次运行判据，避免输入过程中每一位变化触发不完整重算。确认后 `CommandControlModule` 对缓存的全部普通航迹立即重新判定：新满足“识别无人机 + 几何范围”者立即进入/保持自动上报并补发当前点；已不满足者立即停止后续 DDA4 发送，PPI 和两个航迹列表同时撤销“自动上报中”强调。DDA4 协议没有自动上报停止包，停止的定义是不再发送后续航迹点；再次点击确认相同数值也会强制全量同步。
- **上报可视化与表格可读性**：PPI 普通航迹标签在总控手动/自动上报时显示 `batch : xxx` 换行 `手动上报中/自动上报中`（激光为 `激光上报中`），状态变化即时刷新。航迹表的外部上报底色由单元格数据角色绘制，不再被 QSS 默认行背景覆盖；高度列在所有状态下保持绿色粗体。
- **测试航迹启停**：参数区测试按钮在生成中显示“终止并清除测试航迹”且保持可点击。主动终止与自然完成共享 `statMethod=2` 消批通路，清理 PPI、航迹表和外部上报缓存；随后再次点击从 `[test_track].first_batch` 重新开始生成。

### v5.60 (2026-09-17)

- **BIT 与激光心跳日志降噪**：健康管理仍逐帧接收 `BITReport` 并更新温度、扫描线和子阵电源，但 `[logging].bit_report_normal_log_enabled=false`（默认）时不再向显控日志栏追加周期性正常 BIT 摘要；非法阵面 ID、超范围角度和子阵电源全零确认等异常日志不受影响。激光 9009 正常状态心跳新增 `[laser].heartbeat_log_interval_ms`，默认 `0` 表示完全抑制正常心跳日志，发送失败仍立即记录；现场短时确认链路存活可配置为例如 `60000`，每分钟写一条摘要。

### v5.61 (2026-09-17)

- **自动 DDA4 的发送门禁**：范围确认后除更新缓存航迹的自动状态外，`reportTrack(..., manualMode=false)` 在实际组包/发送前再次调用 `isAutoReportActive`。任何未来调用路径均不能绕过“无人机、最高高度、距离闭区间、正北方位区间”的自动筛选；手动上报按定义不受自动范围限制。每次点击范围确认，通信控制页状态和项目日志会明确显示已应用的五个范围值、缓存数量、自动合格数量及保持的手动上报数量，便于现场区分“自动未过滤”与“手动上报仍在发送”。

### v5.62 (2026-09-17)

- **航迹表黑色主题分组强调**：所有航迹表的正常行与交替行分别固定为深黑绿与深墨绿，彻底隔离 Windows 默认白色 `AlternateBase`。高度列由表格代理在 QSS 最终绘制后覆盖为绿色粗体，避免样式表覆盖单元格前景色。普通 DBT 的手动、自动或激光上报航迹继续按实时状态置顶，不改变行底色，而以青色细框和粗体强调；第一条未上报航迹上方绘制青色粗分隔线，将“上报中”与普通航迹分组。选中态仍由原有 QSS 控制。

### v5.63 (2026-09-18)

- **DDA4 真实航迹全过程独立日志**：`[command_control].track_report_log_enabled=true` 时，每次软件启动在 `track_report_log_directory` 创建独立 `command_control_track_report_*.jsonl`。它与 DDA4 收发/回放 JSONL、项目调试日志分离，不参与 PPI 回放。仅真实普通 DBT `TRAINFO` 航迹进入：模块从起批点开始缓存 RAE、速度、类别、时间和当时 DD05 雷达原点；手动或自动 DDA4 实际具备坐标并开始组包发送时补写起批以来全部点，之后逐点记录；手动停止、自动判据失效、模式切换或消批时写结束事件和原因。每点含 RAE、速度/SNR/幅度、识别类别/置信度、雷达 ID、UTC/北京时刻、雷达原点和由相同 ENU→WGS84 公式换算的目标经纬高。未有 DD05 时明确记录 `target_lla_valid=false`。本地测试航迹、TBD、协同及离线 RAE 均不写此日志。JSON 序列化和逐条 flush 沿用独立记录线程，GUI/PPI 线程不执行磁盘 I/O。

### v5.64 (2026-09-20)

- **默认卫星图首次加载稳定性**：`MapProxyWidget` 不再以固定 `500ms` 推测 WebEngine 已布局完成；启动期间先缓存地图类型，待实际 `QWebEngineView` 可见且具有有效尺寸后才加载 exe 同目录的对应 HTML。每次地图 HTML 成功加载后，Qt 直接执行页面内的 `map.resize()` 和 `setCenterOn()`，用当前 WGS84 雷达位置、范围及 PPI 偏移重新设置 GCJ-02 地图边界，不依赖 WebChannel 回调先后。该机制解决默认卫星图已在下拉框选中、但首次启动偶发空白，手动切换后才出现的问题；加载失败仍记录实际 URL。

### v5.65 (2026-09-20)

- **DDA4 正常日志降频，不降发送频率**：手动或自动上报的普通航迹仍在每次 `TRAINFO` 到达时立即发送 DDA4；雷达 2Hz 输入时实际 UDP 仍为 2Hz，`dda4_rate_centi_hz=200` 也继续如实写入协议字段。此前显控日志看到的每秒一条，是 `dda4_log_interval_ms=1000` 对完整正常字段日志的显示限频，不代表发送被限为 1Hz。默认正常字段日志保持关闭；若现场主动开启，默认最小间隔改为 `10000ms`，完整逐帧 DDA4 JSONL 与独立上报全过程 JSONL 均不丢点、不受此项影响。`0` 仅用于短时逐帧字节级联调。

### v5.66 (2026-09-20)

- **激光侦察日志降频**：新增 `[laser].recon_log_interval_ms`：正常自动/持续侦察帧按该间隔最多写一条摘要，附带 `suppressed=N` 表示期间未打印的正常帧数；设为 `0` 完全抑制正常侦察帧日志。消批取消帧和 UDP 发送失败始终立即写入，状态心跳继续使用既有 `heartbeat_log_interval_ms=0` 默认抑制。所有日志降频仅影响显示与落盘，不改变实际 UDP 发包频率；当前默认值与状态/侦察帧各自节拍见 v5.67、v5.68。

### v5.67 (2026-09-20)

- **激光侦察帧与航迹数据率同步**：9009 `0x0200` 状态帧继续按 `[laser].report_interval_ms`（默认 `1000ms`）发送，作为独立心跳；`0x0300` 侦察帧改为在 `LaserReportManager::reportTrackPoint()` 收到每个实际普通 `TRAINFO` 新点时立即发送。手动持续模式仅发送被选批号的新点；自动模式在任一新点到达时发送最多 10 条当前航迹快照，保留既有多目标报文语义。开启自动/手动跟踪时仍立即补发当前缓存快照，消批取消帧和发送错误逻辑不变。定时器不再重复发送旧航迹点，因此雷达为 2Hz 时对应活动航迹的侦察数据也随其 2Hz 更新；日志降频配置只影响显示与落盘，不影响此发送节拍。

### v5.68 (2026-09-20)

- **外部上报显示去干扰**：激光与总控指控模块可对同一普通航迹同时运行，PPI 标签合并显示为“激光上报中 + 手动/自动上报中”，不再因激光状态覆盖指控状态；航迹列表“指控”列在两者同时启用时显示为“手动/激光”或“自动/激光”。外部上报不再改变航迹历史线、最新点的颜色、尺寸和层级；只将 `TraInfo` 标签文字改为青绿色并追加状态文字。外部上报标签不再绘制填充背景，避免相邻运动标签叠加遮挡。标签刷新时依次尝试右上、右下、左上、左下、正上、正下六个位置，避开其他可见标签；全部被占用时选择重叠面积最小的位置。用户可以拖动标签；实时新航迹到达时优先保持该拖动位置，仅在发生遮挡时于其附近避让；地图、量程或坐标轴刷新导致场景重映射时，才恢复到最新航迹点附近并重新避让。正常 9009 侦察帧显控日志默认由 `[laser].recon_log_interval_ms=30000` 控制为每 30 秒一条摘要，取消和发送错误仍立即记录。

### v5.53 (2026-09-16)

- **DDA4 回放失活航迹淘汰**：回放不再只在整段记录结束时统一清理。每个 `(device_id, local_batch)` 都保存其最后一条 DDA4 的 `observed_utc_ms`；后续记录推进回放时间轴时，若该航迹已连续 `replay_track_stale_ms`（默认 3000 ms）没有新点，即仅删除对应的回放内部 PPI 批次。该操作不会调用真实航迹消批路径，也不会影响仍在更新的回放航迹或实时 PPI；设置为 `0` 可关闭自动淘汰。倒退、快进和停止的全量回放清理同时复位这些最后更新时间，避免重建后沿用旧状态。

### v5.52 (2026-09-16)

- **DDA4 回放控制与清理**：DDA4 记录页新增暂停/继续、倒退 10 条、快进 10 条和停止操作。倒退、快进会先仅删除回放专用的内部批次，再用每轮最多 32 条的 GUI 事件循环分段重建至目标位置；因此不会阻塞界面，也不会删除或重置实时 PPI 航迹。正常回放结束、选择新文件或手动停止时同样只清理这些回放批次，回放航迹和来源图例立即消失。回放期间本机设备 ID 改为红色高亮，适合黑色 PPI 背景。

### v5.51 (2026-09-16)

- **DDA4 回放批号与来源颜色**：DDA4 JSONL 的原始 `local_batch` 始终保持协议值。回放层仍以 `(device_id, local_batch)` 映射 `0x80000000` 起始的内部 PPI 批号，避免不同设备同名批号与实时航迹碰撞；该内部值不再作为 PPI 标签显示，标签改为原始 `local_batch`。回放信号同时携带来源设备 ID，PPI 对每个来源分配稳定颜色并显示临时图例；本机设备 ID 使用红色高亮。图例随仍显示的回放航迹保留，在执行显清时复位。自定义回放颜色只影响该少量回放序列，不改变高频普通航迹的默认批量绘制路径。

### v5.50 (2026-09-15)

- **AA06 距离处理上限**：`SigProParam (0xAA06)` 保持 32 字节、1 字节对齐的线上布局不变。原末尾 10 字节预留区的前 2 字节定义为小端 `unsigned short distanceProcessUpperLimitM`，量化 1 m、默认 5200 m；后 8 字节继续保留并由构造函数清零。参数窗口新增“距离处理上限(m)”输入框，启动读取和“保存参数”均使用 `[params.sigpro].distanceProcessUpperLimitM`，输入限制为 `0..65535`。
- **高频正常帧日志控制**：`[logging].bit_report_normal_log_enabled=false` 时，BIT/DE02 仍完整分发到健康管理和扫描线，但不构造并写入冗长的正常字段日志；雷达 ID、角度等协议异常无条件用 WARNING 输出。`[command_control].dda1_normal_log_enabled=false` 与 `dda4_normal_log_enabled=false` 默认抑制 DDA1/DDA4 正常字段日志，异常帧日志保留；DDA4 JSONL 记录和 PPI 回放完全不受影响。开启 DDA4 正常日志后，`dda4_log_interval_ms` 继续用于限速。

### v5.45 (2026-09-10)
- **Ubuntu 发布工作流同步**：`docker/build_ubuntu.ps1` 为 Windows 入口，调用 WSL/Docker；`docker/docker_build.sh` 支持 `1804/2004/2204/2404` 和 `--check`。源码以只读方式挂载到 Docker，容器内部复制到私有 `/src` 再构建，输出写入 `deploy/ubuntu<目标>/`，避免混用 Windows CMake 缓存或旧发布产物。
- **发布包自检与可追溯性**：新增 `check_linux_package.sh`、`BUILD-INFO.txt`、tarball SHA-256 和 `qt.conf`；打包缺失 WebEngineProcess、地图资源或未解析动态库时失败。检查仅覆盖文件/依赖，仍需目标机图形桌面、GPU、地图和雷达网络联调。
- **直接安装步骤**：发布包提供“校验→解压→（兼容包）离线依赖安装→包检查→启动→桌面/登录会话自启动”的清晰流程。桌面安装器按 XDG 目录创建当前用户入口，并可选择图形桌面登录后的自启动；不以 sudo 运行，不创建系统服务。X576 原有 Ubuntu 18.04+ 多版本离线 `.deb` 安装与 ALSA 保护逻辑保留。
- **健康管理网络状态**：新增 `SubsystemNetworkMonitor`，健康窗口显示本机阵面IP、阵面连接、本机激光IP、激光连接。前两项分别检查本机网卡状态和经指定本机IP发起的ICMP可达性（连续三次失败才置不可达）；激光端地址继承 `[laser]`，阵面对端必须在 `[health_network].radar_peer_ip` 明确配置，绝不从 `DATA_PRO_IP` 等处理节点地址猜测。

---

**文档结束**

*本文档整合了 DispCtrl 项目全部 34 个 MD 文档的核心知识与最新实现状态，为 AI 开发助手提供准确的项目全局上下文。重大修改后请同步更新版本历史与注意事项章节。*
