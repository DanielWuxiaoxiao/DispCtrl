# DispCtrl 项目 - AI Agent 专用说明文档（纲领版）

> **文档用途**: 本文档专为 AI 开发助手（Claude、GitHub Copilot、Cursor 等）提供项目全局上下文，整合了项目**所有 MD 文档**的核心知识，便于快速理解项目架构、代码规范、常见任务与注意事项。
>
> **最后更新**: 2026-03-23（全面整合，基于全部34个MD文件重写）
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
- `docker/docker_build.sh` 支持 `1804`、`2204`、`2404` 和 `auto/current` 目标；`auto/current` 会按当前 WSL/宿主 Ubuntu 版本选择对应容器基础镜像，Ubuntu 20.04 宿主会回退到 18.04+ 兼容目标。
- `1804` 使用 `Dockerfile.ubuntu1804`，用于生成 Ubuntu 18.04+ 兼容产物；`2204`/`2404` 使用 `docker/Dockerfile` 的 `UBUNTU_VERSION` build arg。
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

### 1. 检测点数量限制与P显一键清除（2026-01-19）

**问题**: 长时间运行检测点无限增长 → 内存溢出/界面卡死。

**解决**:
- `PPIVisualSettings` 添加"检测点"输入框（默认1000，范围100~1000000）
- `DetManager` 实现 FIFO：超出 `m_maxPoints` 自动淘汰最旧点
- "显清"按钮：点击 → `CustomMessageBox::showConfirm()` → 清除所有检测点和航迹点

```cpp
// DetManager::addDetPoint() 中的FIFO淘汰
while (mNodes.size() > m_maxPoints) {
    DetNode& oldNode = mNodes.first();
    if (oldNode.point) { mScene->removeItem(oldNode.point); delete oldNode.point; }
    mNodes.removeFirst();
}

// PPIView::onClearDisplayRequested()
if (m_scene->det()) m_scene->det()->clear();
if (m_scene->tra()) m_scene->tra()->clear();
```

**相关文件**: `PolarDisp/ppivisualsettings.*`, `PointManager/detmanager.*`, `PolarDisp/ppiview.*`

---

### 2. 航迹角度单位修复（统一按度，2026-05-15）

**现象**: 常规/TBD/协同航迹在联调时出现方位角被放大到数千度，触发 `DATA_INVALID_TRACK`。
**根因**: 历史代码将航迹 `azi/ele` 误按弧度处理并执行 `×(180/π)`；当前联调协议确认三类航迹角度字段均为**度**。

```cpp
// Controller/data2dispmanager.cpp（DBT）
info.azimuth   = traPointInfo->azi;
info.elevation = traPointInfo->ele;

// Controller/tbd2dispmanager.cpp（TBD）
info.azimuth   = pt->azi;
info.elevation = pt->ele;

// Controller/collabtrack2dispmanager.cpp（协同）
info.azimuth   = pt->azi;
info.elevation = pt->ele;
```

---

### 3. 航迹显示样式（颜色、线宽）

**颜色方案**（`Basic/DispBasci.h` `TRA_COLOR`）:
- 检测点: **绿色** `QColor(0, 255, 0)`
- DBT航迹: **橙色** `QColor(255, 128, 0)`（由红色改为橙色）
- TBD航迹: **黄色** `QColor(255, 255, 0)`

**连线样式**（`PointManager/trackmanager.cpp`）:
```cpp
QPen pen(s.color);
pen.setWidth(2);               // 线宽从1改为2
pen.setStyle(Qt::SolidLine);
pen.setCapStyle(Qt::RoundCap); // 圆形端点
pen.setJoinStyle(Qt::RoundJoin);
line->setPen(pen);
line->setOpacity(0.8);        // 80%不透明度
```

**关注态显示**（2026-05）:
- 右键 `DraggableLabel` 菜单新增 `关注 / 取消关注`
- 被关注批次在 `P显` 中显示为**较大的空心三角点**
- 被关注批次的点、标签与连线层级提升到最高，便于在密集点迹中持续观察
- `statMethod==2` 消批或手动 `取消关注` 后恢复普通显示样式

---

### 4. 数据流连接修复（2025-10-24）

修复内容（详见 `.azure/dataflow_fix_summary.md`）:

```cpp
// ppisscene.cpp 构造函数：PPIScene数据流
connect(CON_INS, &Controller::detInfoProcess, m_det, &DetManager::addDetPoint);
connect(CON_INS, &Controller::traInfoProcess, m_track, &TrackManager::addTrackPoint);

// mainoverlayout.cpp：航迹表格实时更新
connect(CON_INS, &Controller::traInfoProcess, this, &MainOverLayOut::updateTrackList);
connect(CON_INS, &Controller::traInfoProcess, this, &MainOverLayOut::updateDroneTrackList);

// mainoverlayout.cpp：扇区显示数据流
connect(CON_INS, &Controller::detInfoProcess,
        m_sectorWidget->scene()->detManager(), &SectorDetManager::addDetPoint);
connect(CON_INS, &Controller::traInfoProcess,
        m_sectorWidget->scene()->trackManager(), &SectorTrackManager::addTrackPoint);
```

---

### 5. 数据存储管理（datasaveui，2026-01-26）

**协议（下行 显控→信处）**:
```cpp
struct DataSave   { ushort mesID=0xCC01; uchar saveSwitch/*0:停 1:开*/; uchar dataID; };
struct DataDel    { ushort mesID=0xCC02; uchar dataID; };
struct OfflineDel { ushort mesID=0xCC03; uchar onSwitch/*0:正常 1:离线*/; uchar dataID; };
```

**协议（上行 信处→显控）**:
```cpp
struct DataSaveOK  { ushort mesID=0xDD02; uchar dataID; ushort dataSize/*GB*/; };
struct DataDelOK   { ushort mesID=0xDD03; uchar dataID; };
struct OfflineStat { ushort mesID=0xDD04; uchar delStat/*0:正常 1:离线*/; uchar dataID; };
```

---

### 6. 健康管理窗口实时更新（2026-01-26）

**改进**: 窗口持久化（不再每次重建），数据实时刷新。

**数据源**:
- `MonitorParam (0xCF01)`: 4个软件模块状态
  - 状态码: 0正常(绿)、1异常(红)、2启动成功(绿)、3启动失败(红)、4关闭成功(黄)、5关闭失败(红)
- `BITReport (0xDE02)`: 10项硬件状态位 + FPGA温度 + 阵面温度 + 偏航角 + 扫描角

**窗口成员** (`mainoverlayout.h`):
```cpp
CusWindow*   m_healthWindow;   // 持久化窗口指针
QPushButton* m_sigProBtn;      // 信号处理状态按钮
QPushButton* m_dataProBtn;     // 数据处理状态按钮
QPushButton* m_beamConBtn;     // 波束调度状态按钮
QPushButton* m_targetRecBtn;   // 目标识别状态按钮
// 硬件BIT: m_btnTxOpen/DutyCycle/PulseWidth/RxOpen/FreqSrc/DigBoard/Servo/Beidou/Bluetooth/PowerBoard
QLabel*      m_tempLabel;      // 温度信息标签
QLabel*      m_angleLabel;     // 角度信息标签
```

---

### 7. 距离-方位图表（RangeAzimuthChart/Widget，2026-01）

**架构**: 直角坐标系（X=方位角 0~360°，Y=距离 m），取代旧极坐标版本。

**类层次**:
```
CustomLineChart（cusWidgets/customlinechart.h）-- 通用直角坐标图表基类
    └── RangeAzimuthChart（PolarDisp/rangeazimuthchart.h）-- 雷达专用图表
            └── （集成到）RangeAzimuthWidget（含工具栏+图表）
```

**核心接口** (`RangeAzimuthChart`):
```cpp
void addDetectionPoint(const PointInfo& info);      // 检测点（绿色，含Tooltip）
void addTrackPoint(const trackInfo& info);           // DBT航迹（黄色，含Tooltip）
void addPointInfo(const PointInfo& info);            // 统一接口（航迹/TBD）

void setDetectionVisible(bool);                      // 可见性控制
void setTrackVisible(bool);
void setDetectionSizeRatio(double);                  // 大小比例 0.5x~3.0x
void setTrackSizeRatio(double);

void setRangeFromMain(double minRange, double maxRange); // 与主PPI距离同步
void setAzimuthRange(double min, double max);        // 方位角过滤（支持跨0°）
void clearRadarData();                               // 清除所有数据
void setMaxDetectionPoints(int);                     // FIFO最大数量
```

**同步机制**:
```
主PPI PolarAxis::rangeChanged → RangeAzimuthWidget::setRangeFromMain()
MousePositionInfo spinbox valueChanged → 三路视图统一setDetectionSizeRatio()
MousePositionInfo checkbox → 三路视图统一setAllVisible()
主PPI DetManager::setMaxPoints() → 同步调用各路setMaxDetectionPoints()
```

**工具栏布局**:
```
[标题] [方位范围: <最小>° ~ <最大>°] [检测点:N | 航迹:M] [清除] [重置]
```

**配置** (`config.toml`):
```toml
[rangeAzimuthDisp.angle]
min = 0
max = 360
```

**QSS对象名（用于darkstyle.qss）**:
- 工具栏: `#RangeAzimuthChartToolBar`
- 图表主体: `#RangeAzimuthChart`

**扩展显示**（2026-05）:
- 新增 `RangeHeightChart/Widget`，作为 `B显` 后的 `高显` TAB
- 坐标系：X=距离（km，与主PPI量程同步），Y=高度（m，默认0~500，可配置）
- 复用 `CustomLineChart`、点迹Tooltip、显隐控制、大小控制、FIFO与无人机过滤逻辑

---

### 10. 多通道航迹显示开关（2026-05-09）

**新增通道**:
- TBD航迹：`0xEE02`，`6010 -> 8010`
- 协同航迹：`0xEE03`，`6020 -> 8020`

**协议约定**:
- 协同航迹通道复用 `TBDTrackHead + TBDTrackInfo + TBDPoint` 帧体定义
- `PointInfo.type` 新增 `CooperativeTrackPointType = 4`

**配置开关** (`config.toml` `[displayConfig]`):
- `iftbd = false`
- `ifxietong = false`

**行为规则**:
- 开关为 `false` 时：不创建对应接收管理器，不监听对应端口，不显示对应UI控件/表页
- `MousePositionInfo` 按开关动态添加 `TBD航迹` / `协同航迹` 可见性复选框
- `MainOverLayOut` 动态添加对应航迹表页
- `RangeAzimuthChart`、`TrackManager`、`SectorTrackManager` 按类型分别控制颜色与可见性

**颜色方案**:
- 常规航迹：`TRA_COLOR`（红）
- TBD航迹：`TBD_COLOR`（黄）
- 协同航迹：`CO_TRACK_COLOR`（青蓝）

---

### 8. 参数对话框统一规范（2026-01-15）

**参考实现**: `paramWidget/servocontrol.cpp`

**按钮行为标准**:
- "确定下发": 打包协议结构体发送信号，**不关闭窗口**（便于连续下发）
- "取消": 查找 `CusWindow` 父指针并关闭

```cpp
// 构造函数：断开默认连接，绑定自定义槽
disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
connect(ui->buttonBox->button(QDialogButtonBox::Ok),
        &QPushButton::clicked, this, &YourDialog::onAccept);
connect(ui->buttonBox->button(QDialogButtonBox::Cancel),
        &QPushButton::clicked, this, &YourDialog::onCancel);

// onCancel：向上查找CusWindow父窗口关闭
void YourDialog::onCancel() {
    QWidget* p = parentWidget();
    while (p && !qobject_cast<CusWindow*>(p)) p = p->parentWidget();
    if (p) p->close();
}
```

---

### 9. 参数保存到 config.toml（2026-01-21）

**功能**: "保存参数"按钮 → 确认弹窗 → 写入 config.toml → 下次启动自动恢复。

**已支持段落** (`[params.*]`):
- `params.servo` — 伺服控制（cmd, speed, az）
- `params.scanrange` — 扫描范围（workMode）
- `params.beamcontrol` — 波形控制（freqID, type，3个波形配置）
- `params.sigpro` — 信号处理（16个参数）
- `params.datapro` — 数据处理（15个参数）
- `params.datasave` — 数据保存（saveSwitch, dataID）

**实现模式**:
```cpp
// .h
private slots:
    void onSaveToConfig();

// .cpp 构造函数（加载默认值）
ui->fieldXxx->setValue(CF_INS.xxxParam(硬编码默认值));
connect(ui->saveButton, &QPushButton::clicked, this, &类名::onSaveToConfig);

// .cpp 实现
void 类名::onSaveToConfig() {
    if (!CustomMessageBox::showConfirm(this, tr("确认保存"),
                                       tr("是否将当前参数保存到配置文件？"))) return;
    CF_INS.saveXxxParam(UI读取的值...);
    if (CF_INS.save()) {
        CustomMessageBox::showInfo(this, tr("保存成功"), tr("参数已保存！\n下次启动将自动加载。"));
    } else {
        CustomMessageBox::showWarning(this, tr("保存失败"), tr("无法保存配置文件，请检查文件权限。"));
    }
}
```

**.ui 文件片段**（在 `QDialogButtonBox` 之前）:
```xml
<item>
 <widget class="QPushButton" name="saveButton">
  <property name="text"><string>保存参数</string></property>
 </widget>
</item>
```

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

### v1.0 (2025-08-18)
- 初版协议（internal_protocol.md v1.0）

### v4.0 (2025-09-18)
- 控制表512B对齐；BIT新增偏航与子阵电源BIT

### v5.0 (2025-10-24 + 2025-12-11)
- 协议v5.0：控制表512B与外部协议一致；新增目标识别状态上报
- **数据流修复**：PPIScene、航迹表格、扇区显示数据流连接修复
- 新增 `ExternalCtrlManager` 外部雷控/伺服链路
- 协议v5.1（2025-12-25）：外部链路默认参数写入config.toml

### v5.1-UI (2026-01-15)
- 修复参数对话框按钮样式与行为不统一
- 移除 darkstyle.qss 中特定对话框的QSS覆盖

### v5.2 (2026-01-19)
- 检测点数量限制（`DetManager` FIFO + `PPIVisualSettings`输入框）
- P显一键清除（"显清"按钮 + 确认对话框）
- 修复 `ConfigManager::displayConfig()` 方法缺失

### v5.3 (2026-01-21)
- 参数保存到 config.toml（servo/scanrange/beamcontrol/sigpro/datapro/datasave）

### v5.4 (2026-01-26)
- **数据存储管理**（datasaveui，0xCC01~03/0xDD02~04）
- **健康管理窗口实时更新**（持久化 + 6状态 + 10项BIT + 温度/角度）
- **距离-方位图表**（`RangeAzimuthChart`，直角坐标系）
  - `CustomLineChart` 通用基类（cusWidgets/）
  - Tooltip、范围同步、大小同步、FIFO
  - 深色主题QSS（`#RangeAzimuthChartToolBar`）

### v5.5 (2026-01-28)
- 距离-方位图超出范围点隐藏而非删除（显示逻辑修复）
- 方位角跨0°范围支持（如330°~30°）

### v5.6 (2026-02-04)
- **航迹角度单位修复**：DBT/TBD 弧度→度（×180/π）
- 航迹颜色：橙色(255,128,0)，线宽2，圆形端点，80%透明度
- 移除 TrackManager 重复信号连接

### v5.7 (2026-03-23)
- **外部协议扩展**：AD数据帧（`sendAdFrame`/`adFrameReceived`/`sendSystemControlWithAd`）
- **Claude.md 全面整合重写**（基于所有34个MD文档）

### v5.8 (2026-05-09)
- **多通道航迹开关**：新增 TBD / 协同航迹配置开关 `iftbd`、`ifxietong`
- **协同航迹通道**：新增 `0xEE03`、`6020 -> 8020` 接收链路
- **UI扩展**：动态新增 TBD / 协同航迹表页与可见性开关
- **显示扩展**：P显、扇区、B显按航迹类型分别着色与显隐控制

### v5.9 (2026-05-12)
- **高显TAB**：新增 `RangeHeightChart/Widget`，位于 `B显` 之后
- **坐标定义**：X轴为距离（km，主PPI量程同步），Y轴为高度（m，默认0~500，可设置）
- **联动扩展**：检测点/航迹数据流、显清、点数上限、点大小、普通无人机过滤同步接入高显

### v5.10 (2026-05-13)
- **航迹关注功能**：`DraggableLabel` 右键菜单新增 `关注 / 取消关注`
- **关注态样式**：被关注批次在 `P显` 中显示为更大的空心三角点，且层级提升到最高
- **生命周期**：手动取消关注或批次消批后，自动恢复普通航迹样式

### v5.11 (2026-05-14)
- **单批航迹点数限制**：新增 `displayConfig.max_track_points`，默认 `200`
- **配置入口**：`PPIVisualSettings` 新增“航迹点”输入框，修改后立即同步到 `P显`、扇区、`B显`、`高显`
- **显示策略**：`TrackManager` / `SectorTrackManager` 对每个批次独立执行 FIFO 裁剪，避免普通航迹批次历史无限增长
- **图表对齐**：`B显` / `高显` 的航迹点上限也改为按 `batch` 独立裁剪，保留每批最新 `N` 个点，不再按全局总数裁剪

### v5.12 (2026-05-15)
- **压测性能开关**：新增 `displayConfig.track_label_refresh_ms`、`chart_track_labels_enabled`、`chart_track_label_refresh_ms`、`send_road_points_to_datapro`
- **主P显优化**：`TrackManager` 的批号标签改为限频刷新，仍保持可见性与锚线更新，减少每点 `setPlainText/setPos` 开销
- **B显/高显优化**：图表批号标签支持直接关闭，并支持最小刷新间隔，降低全量 `refreshTrackLabels()` 的文本绘制成本
- **B显/高显增量刷新**：新航迹点到来时只刷新当前批次的 FIFO 与显隐状态，避免每点触发全图航迹遍历；标签关闭时不再清空 latest-index 缓存，保证无人机过滤逻辑可用
- **道路点下发优化**：`PPIView` 启动/地理位置变化时的道路点下发改为可配置关闭，便于纯航迹压测时剔除无关负载

### v5.13 (2026-05-18)
- **P显检测点批量绘制**：`DetManager` 不再为每个检测点创建 `DetPoint` 图元，改为单个 `DetBatchItem` 保存 FIFO 点列并用 `QPainter::drawPoints()` 批量绘制，减少 `QGraphicsScene` item 创建/删除压力
- **P显航迹批量绘制**：`TrackManager` 每个 batch 使用一个 `TrackBatchItem` 绘制历史点和连线，只保留每批最新 `TrackPoint` 作为交互锚点/关注态图元，避免每点一个 `TrackPoint` 加一条 `QGraphicsLineItem`
- **交互保持**：`PPIScene::mousePressEvent()` 在旧 `Point*` 命中失败时回退调用 `TrackManager::pointInfoAt()` / `DetManager::pointInfoAt()`，批量绘制后的历史航迹点和检测点仍可点击并发出 `trackPointClicked`
- **UI刷新限速**：检测点与航迹批量 item 使用 16ms 单次定时器合并 repaint 请求；数据接收继续入队，GUI 正常负载下接近 60 FPS，积压时跳过中间重复 repaint
- **Bounds增量优化**：检测点/航迹新点到来时只扩展批量 item 的 boundingRect，不再每点全量扫描历史点；量程变化、显隐切换、清空和上限调整时仍会完整重建 bounds
- **航迹绘制降载**：普通航迹按颜色分桶后批量 `drawLines()` / `drawPoints()`，关闭普通态抗锯齿；关注态保留原空心三角样式
- **B显/高显批量绘制**：`RangeAzimuthChart` / `RangeHeightChart` 改为单个 batch item 批量 `drawPoints()`，新增检测点/航迹点不再创建 `QGraphicsEllipseItem`，显隐判断延迟到 paint 阶段，避免每点扫描整张图表

### v5.14 (2026-05-18)
- **扇区数据流降载**：新增 `displayConfig.sector_display_enabled` 与 `sector_display_data_enabled`，默认不创建/不连接隐藏扇区显示，避免 `SectorDetManager` / `SectorTrackManager` 的 per-point item 路径进入压测热路径
- **扇区隐藏图元优化**：扇区检测点、航迹点、连线、标签在不可见时从 `QGraphicsScene` 移除但保留对象和业务数据；重新可见时再加入 scene，减少 invisible item 对 scene 索引、命中测试和遍历的压力

### v5.15 (2026-05-22)
- **显控入口调整**：`伺服控制` 按钮从“雷达控制”tab 移至“参数设置”tab，并新增 `伺服归北` 快捷按钮。
- **伺服归北序列**：点击 `伺服归北` 后立即下发 `方位归北 0°`，4 秒后自动下发 `方位寻位 0°`，两条命令均写入显控命令日志。
- **模式下发联动**：TWS/TAS 模式对话框完成参数下发后，如果雷达仍处于待机状态，自动触发现有“进入工作”按钮逻辑；已在工作态时不反向切回待机。

---

## 联系与贡献

- **项目仓库**: GitHub — `DanielWuxiaoxiao/DispCtrl`，当前分支 `x576`
- **问题反馈**: GitHub Issues
- **贡献流程**: 参见 `docs/book/13_code_guidelines.md`
  - 统一 clang-format 风格
  - 关键模块需有单元测试
  - 提交信息：简短主题 + 关键说明

---

**文档结束**

*本文档整合了 DispCtrl 项目全部 34 个 MD 文档的核心知识与最新实现状态，为 AI 开发助手提供准确的项目全局上下文。重大修改后请同步更新版本历史与注意事项章节。*
