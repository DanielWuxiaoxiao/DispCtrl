# DispCtrl 项目 - AI Agent 专用说明文档（纲领版）

> **文档用途**: 本文档专为 AI 开发助手（Claude、GitHub Copilot、Cursor 等）提供项目全局上下文，整合了项目**所有 MD 文档**的核心知识，便于快速理解项目架构、代码规范、常见任务与注意事项。
>
> **最后更新**: 2026-10-29（V6.5 CC01存储选项扩展）
>
> **当前软件版本**: `V6.5`，唯一运行期版本常量为`Basic/DispBasci.h::APP_VERSION_STR`；UI占位文本、发布包VERSION、桌面入口均须同步或从该常量派生。后续版本迭代必须在本文版本历史记录日期、前版、变更及验证边界，不只修改标题数字。
>
> **版本确认规则（用户要求）**：每轮新的实现/发布修改开始前，先读取当前软件版本，询问“保留 / 小版本 / 大版本”，给出实际候选值，例如V6.0→保留V6.0、小版本V6.1、大版本V7.0；未经确认不自行升级。用户已指定版本的同一轮修改/重打包不反复询问，只读排查不询问。等待选择时可继续实现但保持原版本，正式打包前必须确认。同步规则见AGENTS.md；版本确认不等于授权构建或提交推送。
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
│   ├── authmanager.h/cpp   # 用户/开发者模式与认证通知
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
- 当前航迹管理/无人机/TBD/协同表共用列序：`ID、距离、方位、高度、速度、类型、SNR、幅度、俯仰`。幅度直接显示上报`PointInfo.amp`（1位小数，不追加换算）；暂不显示“指控”列，外部上报排序/行强调仍从模块实时状态获取，不得覆盖幅度单元格。
- 扇区：`det/traInfoProcess → Sector{Det,Track}Manager::add*Point`（`mainoverlayout.cpp`）。

### 5. 数据存储管理（datasaveui，2026-01-26）
- 下行 显控→信处：`DataSave 0xCC01`(saveSwitch,dataID；V6.5增加确认/跟踪/搜索波束和检测点迹四项0/1存储标志及8B预留，负载16B)、`DataDel 0xCC02`(dataID)、`OfflineDel 0xCC03`(onSwitch,dataID)。
- 上行 信处→显控：`DataSaveOK 0xDD02`(dataID,dataSize/GB)、`DataDelOK 0xDD03`(dataID)、`OfflineStat 0xDD04`(delStat,dataID)。
- 结构体定义见 `Basic/Protocol.h`。

### 6. 健康管理窗口实时更新（2026-01-26）
- 窗口持久化（不重建）、数据实时刷新。数据源：`MonitorParam 0xCF01`（4 软件模块状态，状态码 0正常/1异常/2启动成功/3启动失败/4关闭成功/5关闭失败）、`BITReport 0xDE02`（10 项硬件 BIT + FPGA温度 + 阵面温度 + 偏航角 + 扫描角）。
- 成员见 `mainoverlayout.h`（`m_healthWindow`、各状态按钮、合并信息行`m_tempLabel`）；逻辑见 `MainOverLayOut::updateHealthWindow()`。
- 当前健康页的FPGA温度、阵面温度、阵面偏航合并到`m_tempLabel`一行，移除扫描角度及`m_angleLabel`。Controller用`HealthStateWindow`固定数组逐帧聚合，独立5000ms时钟一次发布完整健康窗口；MainOverLayOut不再订阅原始bitReport/monitorParamSend，只接收五秒快照，所有健康控件及主入口样式在窗口发布后统一刷新，开窗/切阵面只展示已发布快照。网络/外设状态事件只置脏，不自行触发控件刷新；隐藏窗口不刷整页。
- 五秒窗口内异常锁存：占空比/脉宽报警按位OR，频率源/收发板/伺服/北斗/蓝牙/波控电源正常位按AND，软件1/3/5及非法码按故障锁存。发射/接收开启位使用最新开关值，关闭不记故障；温度/偏航保留最新值，不凭空添加温度阈值。窗口全部正常才在下一次快照恢复；没收到新报文不伪造正常。36路电源非全零位图沿用原规则，**连续1000帧**全部36位为0才确认全零故障，忽略第5字节保留高4位，计数跨五秒窗口延续并饱和；窗口内已确认的任一路故障不被随后正常覆盖。
- 正常健康报文不写UI/文件日志、不存逐帧历史，仅保留每阵面一个最新显示值、故障位及防抖计数（聚合模型<1KB）。即使旧bit_report_normal_log_enabled=true也不恢复正常BIT日志；协议校验异常文件日志最少5秒限频。每窗口每阵面/软件只写一条异常摘要，伺服仅窗口内失败写一条，正常/成功/恢复不写健康日志；TAS等人工命令日志不变。实时扫描、姿态及非健康协议业务仍接收原始BIT，不能为健康降频改变绝对扫描角或丢弃实时源。
- 电磁周期发送频率仍沿用原配置，取最新已发布的五秒UI状态快照，保持UI与对外状态一致；不改其独立原点和目标上报。健康UI中显示的时间是状态汇总时间，不再冒称最后一帧接收时间。

### 7. 距离-方位图 / 距离-高度图（RangeAzimuth/RangeHeight，2026-01~05）
- 架构：直角坐标基类 `CustomLineChart`（cusWidgets）→ `RangeAzimuthChart`（X=方位0~360°，Y=距离）→ `RangeAzimuthWidget`（工具栏+图表）。`RangeHeightChart/Widget` 为 `B显` 后的 `高显`（X=距离 km 同步主PPI量程，Y=高度 m，默认0~500 可配）。
- 关键接口（`rangeazimuthchart.h`）：`addDetectionPoint/addTrackPoint/addPointInfo`、`setDetection/TrackVisible`、`setDetection/TrackSizeRatio`、`setRangeFromMain`、`setAzimuthRange`(支持跨0°)、`clearRadarData`、`setMaxDetectionPoints`。
- 同步：主PPI `PolarAxis::rangeChanged → setRangeFromMain`；`MousePositionInfo` spinbox/checkbox → 三路视图统一大小/显隐；`DetManager::setMaxPoints` → 各路 `setMaxDetectionPoints`。
- 配置 `[rangeAzimuthDisp.angle] min/max`；QSS 对象名 `#RangeAzimuthChartToolBar`、`#RangeAzimuthChart`。
- `3D显`：`Track3DWidget` 使用 Qt WebEngine + WebChannel 承载随程序资源打包的 Three.js r128，仅保留 `(PointInfo.type, batch)` 对应的最新航迹点；普通/TBD/协同、无人机筛选、点大小、显清和离线 RAE 直投均与 P/B/H 显同步，不接收检测点、不保存历史轨迹线。
- 三维坐标：水平距离为 `range*cos(elevation)`，X=东向、Z=北向、Y=`altitute`；水平范围同步 PPI，垂直范围同步 H显。Web 端独立归一化三轴但保持真实单位标签，范围外航迹仍保留在 C++ 最新点模型中。
- 3D 性能/稳定性：C++ 以 50ms 合并 WebChannel 增量，隐藏页仅维护最新模型并在恢复时发送快照；JS 按颜色批量使用 `THREE.Points/BufferGeometry` 且按需渲染。WebEngine 页面/渲染进程首次异常限次重载，再次失败显示占位并写文件日志。
- 3D 兼容/就绪：Qt 5.14 基于 Chromium 77，场景和状态覆盖层使用显式四边定位，不能仅依赖较新 CSS `inset`，图例间距也不依赖 flex `gap`。WebChannel 先连接再初始化 Three.js；只有有效容器尺寸下首帧渲染成功才通知 C++ 就绪，隐藏页零尺寸等待重新布局，不能钳成1像素并假报成功。尺寸变化写 `[Track3DWidget][VIEWPORT]`，可见未就绪10秒触发一次受限重载，重复初始化错误合并，控制台错误/警告按级别5秒限频。

### 8. 多通道航迹显示开关（2026-05-09）
- 通道：TBD `0xEE02`(`6010→8010`)、协同 `0xEE03`(`6020→8020`，复用 `TBDTrackHead+TBDTrackInfo+TBDPoint`)；`PointInfo.type` 新增 `CooperativeTrackPointType=4`。
- 开关 `[displayConfig] iftbd/ifxietong`（默认 false）：false 时不创建管理器/不监听端口/不显示 UI。`MousePositionInfo` 动态加复选框、`MainOverLayOut` 动态加表页、`RangeAzimuthChart`/`TrackManager`/`SectorTrackManager` 按类型着色与显隐。

### 9. 参数对话框统一规范（2026-01-15，参考 `paramWidget/servocontrol.cpp`）
- "确定下发"：打包结构体发信号，**不关窗**（便于连续下发）；"取消"：向上找 `CusWindow` 父指针 `close()`。
- 构造中 `disconnect` `buttonBox` 默认 accepted/rejected，改连自定义 `onAccept/onCancel`。

### 10. 参数保存到 config.toml（2026-01-21）
- "保存参数"按钮 → `CustomMessageBox::showConfirm` → `CF_INS.saveXxxParam(...)` + `CF_INS.save()` → 成功/失败弹窗；下次启动构造中用 `CF_INS.xxxParam(默认值)` 恢复。
- 已支持段落 `[params.*]`：`servo`(cmd,speed,az)、`scanrange`(workMode)、`beamcontrol`(freqID,type,3波形)、`sigpro`(原16参及距离上限/高度下界)、`datapro`(15参)、`datasave`(saveSwitch,dataID)。
- AA06 负载保持32字节：距离上限为第23～24字节 unsigned short，高度处理下界为第25～26字节 signed short，均小端、1 m量化，剩余6字节填0。`heightProcessLowerLimitM` 默认0，允许负值，UI保存/下发校验 -32768～32767；配置缺省或非法时回退0。该值仅控制信号处理算法，不改变显示高度范围，不在显控侧追加姿态或地理坐标转换。
- `.ui` 在 `QDialogButtonBox` 前加 `QPushButton name="saveButton"`。

### 11. 用户/开发者模式（2026-10-22）
- 每次启动直接进入用户模式，不再弹出模式选择或登录框。版本号右侧显示当前模式按钮；用户模式点击后打开深色用户名/密码验证框，沿用原有本地凭据；验证成功才升级，错误或取消保持用户模式。开发者模式点击直接降为用户模式。
- `Basic/authmanager.*` 为 GUI 线程的 `QObject` 单例，保留 `isAdminMode()` 兼容接口，以 `modeChanged(bool)` 通知已打开的界面。CMake/qmake 必须编译新增实现并对该头文件执行 MOC，不能再按纯头文件类处理。
- 模式变化即时重新排布主界面参数按钮，并刷新 TAS/TWS 的方位步进及波形高级项；重新升级时恢复原布局和原输入值，不移动已打开窗口。降为用户模式时关闭开发者专用的“波形及采样控制”宿主，保存/下发入口再次检查权限。嵌套确认框采用堆对象及 `QPointer` 防止宿主销毁后继续访问。
- 切换只影响本次进程的界面权限，不写配置、不保存密码、不自动下发设备控制或重置参数；下次启动仍是用户模式。这是沿用现有本地界面权限机制，不替代操作系统级安全隔离。

### 12. 经纬高/姿态统一配置（2026-10-26）
- 左上信息区按实际字体度量预留完整`-180.000000000`经度宽度，六项标签/数值对齐，行距6px、列距10px、外边距10px；操作按钮15px字、最小36px高，面板按布局内容调整大小，不再挤入原236×213设计尺寸。仅创建/模块切换时计算布局，不在GPS逐帧刷新中反复度量/resize。
- PPI左上角“配置雷达经纬高/姿态”不依赖用户/开发者或TAS/TWS模式。六项为经纬高/偏航/倾角/横滚，均对应ID0阵面。开机读取默认值；经纬高按最新有效UI/GPS事件更新，手动姿态保持，不被BIT覆盖。显式0有效。通用弹窗填入当前值，无placeholder、说明行或恢复自动按钮；各项不能为空，非法输入原子拒绝。UI高度-1000～5000m、倾角/横滚±90°、偏航0～360°。“确定”仅本次运行；“保存本地”写入实际加载config.toml的[radar] longitude/latitude/altitude/azimuth/pitch/roll并应用，失败不生效。
- **四阵面下发/锁定窗口（2026-10-29，保持V6.3）**：左上“下发雷达阵面经纬高姿态”改为打开四行编辑窗口，不再点击即下发。每行固定ID0～3、六项可编辑，首次取当前ID0有效参考值，其他行航向+90/+180/+270并归一化；之后按上次AA08下发请求的实际量化值恢复，不被背景BIT改写表单。锁定行优先显示冻结值并只读，每行独立“锁定/解除锁定”。锁定/解锁立即本地生效（关闭或取消不回滚），只有“确定下发”提交AA08。发送请求未获设备ACK，不显示为设备已确认。窗口布局/字号接入ui_scale，尺寸按宿主屏幕限制，窄屏允许滚动。
- **锁定的数据语义**：RadarPanelState是Controller GUI线程的四阵面规范状态，ID0别名继续供现有统一原点消费者使用。每阵面冻结完整经纬高/姿态，BIT仍在固定五样本过滤器及一份有效缓存中处理，不发布锁定值之外的变化，不新增逐帧日志/历史队列。解锁显式清除该阵面手动覆盖，恢复最近有效上报；没有上报则开机默认值。ID0锁定/解锁同步左上角、地图及既有统一解算原点；其他阵面独立缓存，不覆盖ID0。电磁原有独立手动原点入口/优先级保持。普通配置入口在ID0锁定时拒绝应用和保存，避免绕过锁；AA08构建也强制使用冻结行。锁定及上次下发记忆只在本次运行有效，不写config.toml，重启后重新按配置初始化。
- `Basic/radarreference.h`保存固定大小的默认/上报/UI状态，Controller在GUI线程管理。V6.3单面阵DD05、多面阵ID0新版DE02经统一验证和平滑后发布`geoLocationUpdated`，地图/PPI/GCS、总控、光电V2及目指共用最新有效原点，消费者绑定后发布启动快照。电磁保持独立手动原点：订阅`filteredGeoLocationReported`接收同样验证/平滑的上报位置，不受通用UI手动位置覆盖。
- DD05仅含经纬高，无姿态/阵面ID，单面阵按ID0解释。V6.3多面阵新版DE02增加经纬高和高精度航向/倾角/横滚，仅ID0用于统一参考；有效新姿态优先于旧BIT yaw。其他阵面不覆盖原点/姿态；手动姿态后上报仅缓存、不替换当前姿态。单面阵继续取旧BIT yaw或控制表航向，倾角/横滚用手动/开机默认。姿态不额外旋转RAE、目标经纬高或BIT绝对扫描角，避免重复补偿。
- 手动偏航对应第一个物理阵面（ID0）的扇区中心法线；其余固定为+90/+180/+270°并归一化。关闭阵面不压缩编号、不改变其他阵面方向。手动时扇区起点为`normal-span/2`，范围修改仍改变跨度；无UI覆盖时保留原TAS本地范围+固定安装角规则。电磁模块enabled时，左上角按钮路由到原“手动配置经纬高”窗口，通用六项窗口与电磁专用窗口保持独立；专用窗口保留既有局部原点/恢复自动/重编码规则，不写全局UI参考状态。该配置入口不因电磁ui_enabled关闭而不可操作。
- PPI扫描指示（2026-10-27）：单/多阵面均只在外圈方位刻度带绘制红色短线（向内36场景像素、5设备像素粗、圆端），去掉中心贯穿线、60°渐变余晖和跟随角度文字。各启用阵面扫描区域统一青绿`(0,255,136)`，填充alpha30、边界alpha120；用WindingFill合并一次填充避免重叠加深。主PPI的红标由鼠标穿透的`ScanMarkerOverlay`绘制在PPI内部其他QWidget上方，场景仍只画底层扇区；不靠抬高整个扫描层z值来遮挡目标。其他场景视图保留场景红标绘制。标记刷新16ms合并，仅更新旧/新标记矩形区域，调整窗口/滚动时同步投影。BIT绝对角、手动法线及阵面开启规则不变，不增加逐点item或周期日志。
- `ConfigManager::saveRadarReference`专门保存六个[radar]键，保留其他键/段/注释和换行，补齐缺失键及段；旧五参入口保留现有roll。double以17位有效数字写出，使用QSaveFile原子提交，仅成功后更新配置内存。不得调用只更新params/displayConfig的旧save()来假报雷达参数保存成功。落盘值作为下次开机默认值，不持久化运行期来源。
- 开机配置按字段校验，非法值写警告并退回对应内置默认值，不将解析失败悄悄当作0。老协议头含全局`pack(1)`，新参考状态头用push/pop隔离标准库及状态布局，不改变已有线上结构。模型QTest源码覆盖最新位置源、手动姿态保持、零值、非法值原子拒绝、缺省及跨360°阵面顺序；完整工程、GUI/外场运行仍需本机验收。

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

- 分离窗口共用 `CusWindow`：最大化按当前窗口与各屏幕的重叠面积选屏，并使用目标屏幕的 `availableGeometry`；不可直接强制主屏。坐标全程使用 Qt 全局逻辑坐标（可能为负），不要额外乘屏幕缩放倍数。还原矩形在最大化前保存，分离/重附加所有权不变。
- 跨兄弟对象的UI信号必须以实际UI接收对象为连接上下文，不能以仍存活的父窗口为上下文再捕获已销毁的子UI。模块析构停止工作不得重新通知UI就绪状态；停止记录/线程仍需完整执行。健康/存储窗口的destroyed回调会访问覆盖层成员，必须在覆盖层派生成员析构前同步销毁窗口。

### 5. .ui 文件修改后需重新构建
修改 `.ui` 后需 cmake 重新构建才能生成 `ui_xxx.h`，让用户自行执行。

### 6. Linux 跨平台注意
- V6.1每次打包必须执行copy_runtime_resources.sh，以源码config.toml、htmls（含amap全部文件）、resources/style及icon为唯一运行资源来源，必需HTML/WebChannel/高德文件缺失即失败；静态资源生成RUNTIME-RESOURCES.sha256并由check_package校验，配置可编辑不参与该摘要。排除瓦片、OSM、录制记录/索引、日志及Windows平台文件，Linux Qt/插件/.pak来自builder而非Windows发布目录。无瓦片仍完成WebChannel/HTML初始化；所选离线地图的对应目录缺失或为空时实际加载black.html，不初始化带浅色底/绿色蒙版的AMap页。底图数据及在线服务依赖仍需另行满足。
- **无地图必须加载HTML（2026-10-28）**：类型0仍由QWebEngineView实际加载bin/black.html，不依赖瓦片、不用隐藏网页代替正常无地图显示。black.html的html/body均设黑底；Qt page背景和中央容器黑底只覆盖加载期间或明确失败后的空白，看到黑色不等于WebEngine已验证成功。日志记录[MAP][LOAD_STARTED/LOAD_FINISHED/DOM]及文件缺失、10秒加载超时和渲染进程退出；DOM记录readyState、实际页面尺寸和html/body背景色。网页warning/error按级别5秒限频，不转为逐帧UI日志。缺文件/加载失败/超时/进程退出时隐藏失效视图，用户可重新切图触发加载，无自动重启循环。普通地图仍等待有效可见尺寸后初始化。
- V6.0发布入口使用`scripts/run_linux.sh`复制为包根run.sh。普通用户在图形桌面首次运行时以`install_desktop.sh --auto`注册当前用户桌面/应用菜单，默认不启用登录自启动；5秒超时/失败不阻塞应用，root及无图形会话跳过。重复运行按内容更新，不反复覆盖同版本入口；已启用autostart只刷新路径/版本、不变更选择。`DISPCTRL_INSTALL_DESKTOP=0`禁用自动注册，手动安装器仍可配置登录自启动/卸载。包检查要求雷达PNG图标、VERSION和安装/启动脚本，Qt设置desktopFileName=dispctrl匹配入口，保留原GL/sandbox策略。GNOME可信标记与桌面扩展由系统控制，不能保证免除“允许启动”。
- **Linux沙箱兼容（2026-10-28用户明确要求）**：现场地图5次、3D两次status=2/exitCode=159；临时QTWEBENGINE_DISABLE_SANDBOX=1后用户确认不闪白，属于沙箱相关兼容路径，不可凭旧日志中的泛化GPU提示定性显卡故障。新发布入口默认同时设置QTWEBENGINE_DISABLE_SANDBOX=1和--no-sandbox/--disable-gpu-sandbox，可用DISPCTRL_DISABLE_SANDBOX=0显式取消默认禁用（外部已有--no-sandbox参数需自行清除）。这是降低网页隔离保护的兼容绕过，不是修好了底层WebEngine沙箱。main.cpp追加配置参数时必须保留启动器已有Chromium flags；[WEBENGINE_CONFIG]在文件日志安装后记录Qt/系统/内核/后端/沙箱环境与最终flags。Windows渲染策略不改，直接运行bin/DispCtrl不经过Linux入口策略。
- Docker发布默认复用已有本地builder及离线依赖镜像，仍使用当前工作区源码在私有/src全新编译；`-RebuildImage`/`--rebuild-image`才显式重建环境，缺失镜像自动构建。运行挂载源码中的最新container_build.sh，兼容旧镜像无ENTRYPOINT。Qt5.15.2的qtconnectivity/qtserialport/qt3d属于aqt默认包，不放进-m扩展列表；保持下载校验，不绕过校验失败。PowerShell策略受限时使用单次进程ExecutionPolicy Bypass。
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
- TAS在索引9追加“随机跳频”，发送原始`freqID=90/0x5A`，与固定频点0/10/.../80连续；保存/恢复均使用index*10映射。固定频点编码保持不变，TWS/独立波形窗口未新增随机项。TAS参数下发前用`Basic/tascontrolformat.h`从实际AA04/AA05/AA03结构体格式化UI日志及`LOG_INFO [TAS][SUBMIT]`文本日志，完整显示场景raw值、波形使能/码、采样/俯仰、方位及伺服；仅表示提交请求，不代表UDP成功或硬件执行。场景当前显示序号=raw+1，不能隐去原始值或把方位起点写成中心。
- 多面阵界面按`displayConfig.default_panel_count > 1`判断，不用临时阵面电源开关数判断安装模式：主界面隐藏TWS模式设置/伺服归北；TAS隐藏伺服组和积累脉冲数（保留原值参与保存/发送），标题移除≤90°，中心角预览及确定下发不执行90°跨度限制。权限切换不能重新显示这些控件。单面阵保持旧显示及90°规则。用户明确要求仍发送原AA04→AA05→AA03，不因隐藏伺服组而禁发寻位。方位配置保存参数和读取接口均使用int，与AA05 int字段一致，防止36000截断为short。

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
- **V6.1统一字体缩放修复（2026-10-28，用户要求保留版本号）**：1920×1200与1920×1080同为compact，不再因16:10屏幕多120px高度切到24px大标题/宽侧栏。Basic/uifontscale.h只缩放font-size及font简写里的px/pt，不改padding/边框/坐标；全局QSS和所有局部setStyleSheet入口接入一次缩放，复制已有样式不再缩放，健康状态样式比较使用已缩放结果，避免重复抛光。手绘A显/波束/统计/健康图、日志条目、测距/扇区/Tooltip等显式字体统一按96DPI基准转像素并应用ui_scale；原已使用uiScaled的图表字体保留，不重复缩放。QObject QWidget无全局逐帧扫描/事件过滤器。网页地图/3D正常场景内的地理/渲染字体不通过Qt样式改写；3D不可用提示按UI字号缩放。
- **Linux字体与诊断**：全局字体优先已安装Noto Sans CJK SC/WenQuanYi Micro Hei/Noto Sans/DejaVu Sans，而非只请求Windows微软雅黑。fixed策略清除继承的QT_SCREEN_SCALE_FACTORS/QT_DEVICE_PIXEL_RATIO，其他系统策略不强制清除。UI缩放非法/非有限值回退默认，正常值限制0.70～1.60；修改发布目录bin/config.toml后必须重启。文件日志新增[UI_CONFIG]包含真实配置路径/版本/scale/compact/逻辑尺寸/DPR/字体。离屏Qt5.15.2 QTest已通过9项，覆盖1920×1200判定、五档缩放、px/pt/简写、几何不变、样式重复应用/复制及手绘字号。
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

### v5.69 (2026-09-20)

- **本地测试航迹三维机动**：`TestTrackGenerator` 不再把所有测试航迹限制为固定方位、固定俯仰的径向运动。每条预设航迹现在具有独立的初始 RAE、地面航向和爬升/下降角；生成器以配置的真实速度在局地东北天坐标系按实际经过时间匀速积分，再统一反算 `range/azimuth/elevation/altitute`。因此 PPI 方位与俯仰会连续变化，DDA4/GCS 的 WGS84 坐标换算仍读取同一组 RAE，速度和经纬高不会产生模型分歧；测试集继续保留无人机、非无人机、量程与高度边界样本。

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

### v5.63 (2026-09-24)

- **合作方显控协议定版**：`docs/显控协议.md` 将“DBT 航迹”统一简称为“航迹”，检测点和航迹的经纬高改为与总控 DDA4 共用的 DD05 雷达原点及 `Basic/wgs84coordinate.h` RAE → ENU → WGS84 口径；经纬度使用 `int32 × 1e-7°`，高度使用 `int32 × 1 m`。检测点记录由 56 字节扩为 64 字节，航迹复用原 8 字节预留区后仍为 61 字节；无有效 DD05 原点或坐标非法时不发送该记录。
- **网络与文档导航**：双方 UDP 参数定为本端显控 `192.168.64.4:8022 / 0xBB04`、合作方显控 `192.168.64.8:6022 / 0xBB08`，双向均使用固定源端口。协议源稿增加显式 ASCII 锚点目录，便于 Markdown 转 PDF 后保留文内跳转；同时纠正公共帧数据长度公式中重复计算信息编号的问题。

---

### v5.64 (2026-10-08)

- **电磁设备模拟交互**：新增独立 `ElectromagneticDevice/` 模块，按 `docs/显控协议.md` 的既有方向接受模拟设备编号 1/2/3/4/8 控制，通过主界面现有 AA01/AA02/AA04/AA05/DA01 入口执行并同步显示；向设备上报编号 5/6/7/9。`[electromagnetic_device]` 控制启用、UI、独立网络/设备 ID、接收控制和各类数据发送；默认关闭，与总控、激光资源和状态独立。
- **数据正确性与性能**：增加 Controller 完整 DD01/普通 EE01 帧信号，保留 PointInfo 中没有的 CPI/时间/通道等字段。共享 DD05/WGS84 公式，检测/航迹角度遵循当前“度”约定，拒绝非法坐标；消批复用有效缓存。UDP 工作线程、有限检测队列、批号最新点合并和单批在途限制，单包不超过 1472 字节。
- **UI 与联调边界**：参数区“电磁设备通信”及健康管理独立入口，显示网卡、ICMP、UDP、本协议接收、检测/航迹数据源和原点状态。协议无心跳/应答，Ping 或本机发送成功不代表对端应用已收到。新增模拟端脚本与协议 QTest；接入细节见 `docs/电磁设备接入说明.md`。

### v5.65 (2026-10-08)

- **电磁设备选择式下发**：发送入口统一到 `Controller::traInfoProcess/detInfoProcess`；完整 DD01/EE01 信号仅作协议辅助字段缓存，不再自动发送。PPI 航迹/标签右键支持持续手动下发和停止，检测点右键单次下发；通信窗口新增独立的全部普通航迹/检测点自动开关，TOML `auto_report_tracks/auto_report_detections` 默认 false。自动关闭保留手动订阅，消批仅发送曾选中批次的最后有效坐标；原始字段不匹配时拒绝发送。PointInfo/DREC 二进制布局保持不变。
- **模拟数据共用入口**：既有测试航迹接入电磁设备；新增 `TestDetectionGenerator` 与“生成测试点迹”独立启停按钮，由 `[test_detection]` 配置，每采样注入 10 个 PointInfo 到 detInfoProcess，不伪造 DD01/DD05。`testPointGenerated` 在公共点信号前仅登记测试来源/合成字段，不发送，不借用检测点批号作为测试标记；测试原点仅用于已登记且匹配的采样，真实 DD05 优先，真实原始帧撤销冲突测试标记，禁止仅凭批号判断测试来源。测试航迹协议时间来自生成时间，CPI/加速度为 0、空间速度与固定视线径向速度一致；测试检测缺失字段为明确合成值 0。真实航迹仍保留原 CPI/时间，真实检测字段缓存有界，过期手动选择明确失败。
- **验证入口**：新增 `electromagnetic_reporting_test`，覆盖公共信号与原始帧去重、手动/自动选择、消批及检测单次下发；qmake 补齐现有测试生成器清单。禁止自动完整构建，实际 UDP 和鼠标交互仍需本机联调。

### v5.66 (2026-10-08)

- **电磁调试日志**：复用 UTF-8 `disp_ctrl_log_0..4.txt` 滚动日志；新增配置路径/最终值、公共入口连接、原点来源、人工选中/停止/失败、运行期开关、字段采样、报文计数/本机套接字结果/耗时、链路状态变化和周期统计。`[electromagnetic_device]` 的 `data_log_enabled=true`、`data_log_interval_ms=1000`、`log_summary_interval_ms=5000` 控制正常采样/摘要，错误按固定类别各自限频 5 秒并记录抑制次数，原先静默的航迹编码拒绝现在区分原点、字段缓存、样本错配和非法值并累计计数。高频周期日志只进文件，不刷显控日志栏；发送成功与对端应答严格区分，消息组包/入队计数不当作已发目标数。
- **旧功能隔离复核**：测试检测点新增 `testDetectionFallbackRadarPositionReady`，不再复用旧总控订阅的测试航迹原点通知；原测试航迹仍保持每点一次 traInfoProcess 和原通知。电磁模块/UI 未启用时 PPI contextMenuEvent 完全委托原链路；电磁标签追加而非覆盖旧激光/总控状态。指控/激光协议、端点、选择和筛选规则、PointInfo/DREC 布局不变；手动触发测试点迹仍会正常进入公共点迹显示/录制等订阅者，设备有效控制按协议更新既有雷达参数，这不是发送链路隔离能禁止的行为。尚未做完整构建/运行回归，不能据静态检查声称完全无影响。

### v5.67 (2026-10-10)

- **电磁设备联调字节样本**：新增 `docs/电磁设备上报帧字节流示例.md`，按当前线上编码顺序提供编号 5/6/7/9 四类完整 HEX 数据报及消批、预测、零数量边界帧，共 8 帧；附 0 起始完整帧字段偏移、典型值、长度、XOR、设备 ID 与固定端口约定。数据为虚构确定性样本，经纬高用共享局地 ENU 口径计算；阵面正常 BIT 用 0x9F，温度/角度分别按 0.1°C/0.01°量化。明确空航迹数量不代表清场，当前业务不周期发送空记录，样本固定计数不代表运行计数。
- **样本验证**：`scripts/check_electromagnetic_examples.py` 仅读取 Markdown，使用已有模拟端编码/解析器检查全部完整帧、字段、几何量化、长度、校验和损坏拒绝；不发网络报文、不改业务代码、不运行工程构建。

### v5.68 (2026-10-14)

- **电磁状态周期占位**：按联调需求，编号7/9在启用状态上报且UDP就绪时每1秒发送（status_interval_ms默认1000）；模块局部缓存用BITReport/MonitorParam默认构造值初始化，阵面负载15字节全0，软件4字节全0。各自收到有效真实数据后替换缓存，后续即使超过receive_timeout_ms仍发最新值，不停发、不回退。STATUS_TX分类标记DEFAULT/REAL_LATEST/REAL_CACHED_STALE并限频，默认/旧值不代表设备实测健康；不伪造Controller信号，不改变四面健康管理或旧指控状态。保留report_status开关和单批在途限制，实际发送周期受UDP就绪/线程调度影响。

### v5.69 (2026-10-14)

- **电磁软件状态对齐UI**：修正v5.68直接采用MonitorParam默认全0导致“UI异常、上报正常”的错误。模块防御默认四项1；MainOverLayOut::currentSoftwareStatus从UI实际模型读取四项状态，绑定模块时初始化缓存，monitorParamRef更新完成后发softwareStatusChanged供电磁模块更新，不再重复直连Controller原始监控信号。保持每秒发送最新值与超时不停发，不改变旧UI/指控状态或MonitorParam协议结构默认值。
- **状态映射定版**：用户确认外部协议保留0～3，UI的4关闭成功/5关闭失败映射为异常1；其余0～3透传，超过5拒绝更新。STATUS_UI日志并列记录UI原值和线上值；补充初始化、最新更新及4/5映射QTest用例，文档正常全0样例保留但不再标作启动默认。

### v5.70 (2026-10-14)

- **电磁子阵电源与UI防抖一致**：编号7不再直连Controller原始BIT，改由MainOverLayOut在更新阵面0（界面阵面1）的SubArrayPowerVisualState后发electromagneticPanelStatusChanged。currentElectromagneticPanelStatus拷贝原BIT并仅以UI已确认状态重写36个有效电源位，保留其余字段和高4个保留位；未确认仍使用全零占位，不增加协议状态。沿用原100帧防抖，不复制第二套计数，健康窗口隐藏/切页不影响同步，不改原始BIT缓存或其他旧订阅者。首次忽略全零写SUBARRAY_UI日志，测试覆盖正常保留、已确认故障、逐位编号及保留字段。

### v5.71 (2026-10-14)

- **功能关闭时UI隐藏**：修复参数区无条件加入“总控通信”以及健康窗口无条件创建激光双端IP状态的问题。setCommandControlModule拒绝disabled模块并重新排布按钮，仅enabled总控进入可见布局；清理隐藏入口留下的行高/拉伸。PPI F6/F7开关重新按配置/模块enabled校验，右键总控同样加enabled门禁，关闭时禁用快捷键和隐藏入口。开启但UDP/登录异常仍保留入口用于诊断，不将“故障”误当“配置关闭”。
- **激光网络检测隔离**：laser.enabled=false时不创建健康管理的激光IP/连接行，SubsystemNetworkMonitor跳过激光网口及Ping并取消其在途检测，阵面检测仍保留。光电network.photoelectric.tx_enabled、旧GCS target_report_enabled与总控/激光是独立开关，不混用；现有PPI激光配置显隐本已生效，运行时仍须核对实际加载配置路径。UI_FEATURES日志记录配置路径和入口开关，新增subsystem_feature_flags_test针对性用例；不自动完整构建，实际UI需本机重启验证。

### v5.72 (2026-10-14)

- **电磁真实目标备用原点**：启动校验[radar]的longitude/latitude/altitude，完整有效时可供真实和测试检测点/普通航迹解算；优先级DD05→CONFIG_FALLBACK→仅已登记测试采样的TEST_FALLBACK。缺失、非法、非有限或越界经纬高不偷偷转为0，不影响已有DD05接收。日志和通信窗口显示配置备用来源，增加真实检测/航迹备用解算与DD05优先测试。用户澄清只增加经纬高备用，现有RAE角度不额外应用azimuth/pitch/roll旋转；不改旧指控/激光坐标路径。

### v5.73 (2026-10-14)

- **PPI电磁手动原点**：enabled/ui_enabled均true时，在mainviewTopLeft动态增加“手动配置经纬高”按钮和独立电磁原点文本；深色弹窗输入WGS84经纬高，数值范围校验，确认立即使用本次运行手动原点，恢复自动回到DD05/配置备用顺序，取消不修改。不改旧左上角经纬高字段及其GCS消费者，不保存TOML、不做姿态旋转。DD05持续缓存但不覆盖手动原点，日志标注MANUAL。手动切换重编码活动航迹最新缓存并清理未派发旧原点检测点，单批在途不可撤回。增加模型优先级、输入边界与重编码测试；动态尺寸变化重排PPI覆盖层。

### v5.74 (2026-10-15)

- **双阵面现场卡顿排查**：2026-10-15 用户提供的5份日志中，`disp_ctrl_log_1.txt`是`disp_ctrl_log_1(1).txt`的前缀，不能重复统计。去重后130686条带时间戳日志里125780条是Controller的BIT角度越界WARNING（96.25%，峰值139条/秒）；末次启动约19.45秒有2262条该WARNING。现场电磁模块`enabled=false`，不能归因电磁发送。正常BIT日志开关未失效，遗漏的是异常日志未限频；文件日志在调用线程逐条同步写入。日志无崩溃堆栈，不能宣称已证明唯一崩溃原因。
- **BIT日志有界限频**：Controller采用单调时钟及固定四阵面+非法ID槽，按消息ID/阵面ID/角度异常类别分别记录首条，再每5秒记录当前原始字段和`suppressed`计数；角度变化不重置限频键，不改变协议角度单位/符号/归一化语义。正常详情默认关闭，开启后也每阵面限频。新增`[logging].bit_report_normal_log_interval_ms`与`bit_report_warning_log_interval_ms`，缺省5000ms、限制1000~60000ms，旧配置无需补键也安全生效；间隔启动缓存。UI不再重复输出非法阵面ID WARNING。
- **健康窗口按需刷新**：MainOverLayOut逐帧保存BIT、更新100帧子阵电源防抖、向电磁分发已确认状态；仅可见健康窗口的控件刷新合并到100ms，覆盖BIT/软件/网络/电磁状态。打开和切阵面页立即显示最新缓存，重复QSS不再重新设置，隐藏/销毁窗口取消无效刷新；全零计数达到100后饱和，不改变第100帧确认及恢复语义。扫描线和其它业务订阅者仍逐帧接收，没有在入口合并/丢弃BIT。
- **非法数据防护**：DD01曾出现`range=-2.77759e+35`，旧路径虽被RadarDataManager拒绝，仍会向PPI/B/H及业务入口发出。现在在逐点公开分发前验证RAE和所有渲染浮点字段，沿用0..1000km、0≤方位<360°、±90°俯仰，非法点只按帧聚合错误，合法同帧后续点继续处理，不额外旋转坐标或改变合法数值；完整原始帧仍供补充字段缓存。DE02在memcpy前验证完整39B负载与公共帧头尾长度，短帧拒绝分发，避免越界读。
- **错误去重缓存与线程安全**：ErrorHandler在完整字段格式化前按错误代码/类别/级别限频，最多256键，不再永久保存每个不同RAE的完整消息；Critical/Fatal不抑制。统计、最近1000条历史及处理器表均加锁，锁外发信号/调用处理器，处理器以shared_ptr快照保证替换不影响在途调用；自定义处理器仍需自行保证回调并发安全。DD01非法阵面ID同样复用固定错误代码限频，错误计数饱和而不溢出。
- **验证边界**：新增`diagnostic_log_window_test`、`error_log_limiter_test`、`point_info_validation_test`源码目标，覆盖20分钟2kHz模型、独立槽、容量/并发、异常坐标与边界。按新5秒规则对现场日志时间线做回放估算，125780条BIT角度WARNING将归并为215条；这不是实际程序运行或性能实测。本次完成差异/分发顺序/定时器静态检查、TOML解析和日志复核，未自动构建、未运行新增QTest，真实接收/GUI/GPU/长时间运行仍需本地验收。

### v5.75 (2026-10-15)

- **3D黑屏兼容修复**：现场日志已有 `WebGL page and WebChannel ready`，但页面 `#scene-root` 与状态层仅靠 `inset:0` 铺满；Qt 5.14 的 Chromium 77 不支持该简写（Chrome 87才支持），可使空绝对定位容器无有效尺寸。改用 `top/right/bottom/left:0`，图例 flex gap 改相邻元素margin，保留 Three.js r128、资源内嵌与现有全局ANGLE/GPU配置，不影响地图及雷达坐标口径。
- **真实首帧就绪与可观测性**：桥接建立后才执行WebGL初始化，缺失Three/OrbitControls和WebGL构造异常都可立即上报，不再永远滞留pendingErrors。零尺寸时等待ResizeObserver/resize/显示恢复，不再创建1px画布或误报ready；首帧成功才pageReady并请求当前完整航迹快照。桥新增五参数 `reportViewport(width,height,canvasWidth,canvasHeight,details)`，尺寸变化记录UA/WebGL信息；C++捕获控制台warning/error按级别限频5秒，避免新的日志洪泛，诊断中的ANGLE标识改写为Angle以避开既有全局噪音过滤。
- **有限故障恢复**：可见且未就绪的3D页启动10秒监控，隐藏/就绪/失败时停止；顶层窗口最小化导致rAF暂停时只延后检查，不消耗恢复次数。loadFinished(true)不再被等同于初始化成功。致命初始化/运行错误、加载错误或渲染进程终止共用failurePending合并，最多一次重载，再失败显示明确占位；上下文恢复信息不触发致命重载。只清待发送增量，不清C++最新航迹模型，恢复时重发快照。3D仍不接入检测点。
- **验证与边界**：`node tests/track3dweb_test.js` 的10项VM/静态回归全部通过，使用真实Three/Orbit场景逻辑、模拟DOM与WebGLRenderer。`node tests/track3dlayout_browser_test.js` 的真实headless Chrome/WebGL回归通过：模拟不支持inset时旧布局仅300×150，修补后画布填满1024×768，轴网格/标签和2个测试目标有实际像素，resize及隐藏恢复通过。浏览器资源全部由本地内存供给，Qt桥以测试桩替代，不启动显控/UDP、不下载依赖；这不是直接运行Qt Chromium 77或现场ANGLE/GPU。未自动编译Qt工程，完整Qt编译、地图共存和可分离窗口仍需本地验证。

### v5.76 (2026-10-16)

- **经纬高配置界面简化**：PPI左上角仅保留“手动配置经纬高”按钮，移除独立原点来源与坐标说明；弹窗使用同名标题，仅显示经度、纬度、海拔高度输入及“恢复自动 / 确定 / 取消”，输入错误时才显示简短提示。原点优先级、校验、运行期生效和恢复自动逻辑保持原样。

### v5.77 (2026-10-16)

- **分离窗口跨屏最大化**：修复 `CusWindow` 最大化按钮直接使用 `primaryScreen()->availableGeometry()`，导致P/B/H/3D等分离页拖到扩展屏后最大化又跳回主屏的问题。按钮与标题栏双击共用切换逻辑，按窗口当前全局矩形与屏幕的最大重叠面积选屏，重叠相等/窗口已离屏时优先原生窗口所属屏，无屏幕时安全不操作；用目标屏幕可用区域避开任务栏，支持左/上方负坐标及不同逻辑尺寸，不重复换算DPI。还原继续使用最大化前的位置和大小，不改首次居中、拖动、缩放和重附加流程；共用CusWindow的参数/健康窗口同时受益。
- **验证**：新增 `window_geometry_test` QTest源码目标，覆盖右侧扩展屏/过时主屏关联、左/上负坐标、跨屏面积与平局、工作区和逻辑DPI、离屏及无屏回退。MSVC对`cuswindow.cpp`执行`/Zs`，使用本地Qt 5.14.2头文件的语法检查通过（不生成目标文件）；差异和调用路径静态检查通过。未构建Qt工程、未运行新增QTest，实际双屏最大化/还原、双击及分离重附加需本地编译后验收。

### v5.78 (2026-10-20)

- **光电V2独立接入**：新增`PhotoelectricV2/`，按用户提供的《光电设备接口控制文件V2.0》实现小端0x3400引导(47B)、0x3500设备控制(34B)、0x3200目标接收(60B)、0x3000状态接收(43B)、0x3100终端心跳接收(22B)。本软件角色为系统指控，不冒充厂商终端发送心跳。长度为总帧长；累加低8位校验明确隔离旧socket备用XOR实现。设备ID/终端ID、来源IP/可选源端口、固定帧长及字段范围校验。文档“奇校验”文字解释仍需合作方实包确认，详见`docs/光电V2接入说明.md`。
- **配置与UI隔离**：`[photoelectric_v2]`默认关闭，配置独立IP、接收10100/11100、控制发送源10101和目的21001、ID/节拍/超时/容量/日志。启用后PPI显示“光电V2(F6)”及控制窗口、普通航迹右键持续引导/停止、独立自动引导；健康页独立本机IP/ICMP和UDP/协议接收状态。与9009共享F6但不共享协议和缓存；误同时开启记录冲突并V2优先，9009及旧Disp2PhotoManager不创建。不改变总控/电磁行为。
- **坐标与生命周期**：仅订阅traInfoProcess普通航迹，DD05优先于有效配置经纬高备用，复用WGS84换算，不额外旋转姿态，不复用电磁手动原点。经纬高量化按附件无符号范围，速度为现有径向速度绝对值，来源属性0不使用无人机分类；消批/超时仅停止发送，不编造取消指令。收到的光电目标留在独立UI表，不注入其它业务链路。
- **实时边界与验证**：UDP独立线程；发送单批在途、有限控制FIFO，接收有界最新点及100ms合并/GUI确认，超时依据UDP实际接收的单调时刻而非GUI交付时刻；UI可见时500ms刷新，正常/异常日志限频。新增协议/模块QTest及开关互斥用例，含独立字节黄金样本；Qt5.14/MSVC对模块、MOC、主界面/PPI、网络检测及3份测试源码的`/Zs`语法检查通过，附件5种字段总长与TOML校验通过。未构建/运行QTest或完整工程，不将静态检查称为实机验收；实际对端与GUI联调由本机验收。配置/端口/角色/无符号限制见接入说明。

### v5.79 (2026-10-20)

- **舟山仅第6节**：用户将范围收敛为《舟山光电》第6节目标指示及响应，不实现第5节侦察、第3节控制或第4节状态/心跳。新增`ZhoushanOptical/zhoushanmodule.*`及独立编解码；TCP显控服务端监听，光电客户端主动连接。默认`[zhoushan_optical].enabled=false`，独立IP/9010端口、1100→3101设备ID可配置；三种光电共享F6但不共享协议状态，冲突明确告警并按舟山>V2>9009启用一个。健康页本机/对端IP及TCP/响应状态独立，原总控/电磁不接入此链路。
- **字节与坐标**：第6节请求/响应都为48B(18+26+4)，在线类型04 00/04 01，小端IEEE754和附录16位字累加校验，不复用9009逐字节和。RAE与DD05/有效配置备用原点复用WGS84公式，不追加姿态；普通航迹和测试航迹可F6/右键发送，检测/TBD/协同/离线不发送。提示类型1持续最新点，确认类型2每次人工选择只发一次，超时/重连不自动重发确认；响应校验会话、批号、类型、时限及提示坐标回显，无流水号的旧确认歧义明确记录在接入说明。停止仅停止本地，不能撤回已入TCP帧，不伪造取消/心跳。
- **可靠性与验证**：TCP独立线程，有界流缓冲/拆粘包/半帧超时，接收32键合并且一批GUI在途，发送一条待响应并拒绝积压旧坐标，最新航迹缓存/失效有界；日志前缀ZS-OPT，区分本机入队与对端处理。新源码采用独立文件名避免qmake与光电V2的module.obj/moc_module.cpp冲突。新增协议黄金帧、响应/流边界、生命周期/单请求、会话/坐标匹配及互斥QTest源码；完成Qt5.14/MSVC语法和文档字段长度/TOML检查，不自动完整构建，QTest运行与TCP/GUI实机验收仍由本机执行。详细参数与线协议见`docs/舟山光电目指接入说明.md`。

### v5.80 (2026-10-20)

- **光电目指可读性与中性命名**：第6节目指窗口的内容区显式使用不透明深色背景、浅色状态/响应文字和浅灰绿说明，局部限定QSS，避免分离窗口继承系统黑字且不改全局主题。窗口、F6按钮、提示、右键菜单、航迹标注/表格、健康页IP和状态统一去除“舟山”字样，显示为光电/光电目指；UI日志中的协议冲突说明同样中性化。`[zhoushan_optical]`配置段、源码命名和ZS-OPT诊断标识不变，不改变协议、端点、收发时序或互斥规则。
- **验证**：新增`optical_indication_ui_test`，模拟全局黑色QLabel，检查局部深色背景、浅色正文和中性标题，不初始化TCP/UDP；测试默认offscreen。修改源码及测试的Qt5.14/MSVC `/Zs`语法检查、运行时代码地域字样检索和差异检查通过，未完整构建、未执行GUI测试或实机视觉验收。

### v5.81 (2026-10-20)

- **PPI光电入口精简**：移除第6节目指链路在PPI中重复的“光电目指状态”按钮（不创建该控件），状态及详情继续由健康管理入口提供；保留光电F6、右键下发/停止和通信逻辑。`光电V2控制`含实际设备控制功能，仅V2启用时创建并显示，不因精简状态入口而删除其功能。

### v5.82 (2026-10-20)

- **光电目指单目标与回传详情**：第6节链路默认类型1，F6/右键只选择一个普通航迹批次，每次发送时用该批次最新RAE和当前有效雷达原点解算经纬高；切换新批次、消批/过期或手动停止中止旧批次后续发送。右键停止依据选择状态而非TCP在线状态，断线也能取消；共享原子发送令牌使旧Qt队列请求在工作线程写入前失效，不阻塞GUI，已开始写入TCP的帧仍不可撤回。不改变类型2单次确认或旧9009/10100协议。
- **恢复有实际内容的PPI状态入口**：按新需求恢复“光电目指状态”按钮，窗口及健康页共用，UI仍不显示地域名称。显示雷达侧监听端点/光电IP、TCP状态和当前下发批号；两条有界回传记录随人工选择切换为当前批次/上一个不同批次，同批次重复选择不轮换，停止保留结果。展示响应类型、批号、结果枚举及解释、经纬高、对端时标/本机接收时间、双方ID及匹配状态。上批次迟到回复只更新其独立栏，未知批次/旧会话不混入；单调接收顺序号防止合并QHash乱序覆盖。回传只展示，不改雷达目标/原点。
- **验证边界**：扩展模块QTest源码覆盖单目标切换、最新坐标、消批、断线取消、旧发送任务失效、当前/上一批次回传及同毫秒顺序；扩展UI测试检查两栏及TCP端点。Qt5.14 MOC生成及模块/测试/PPI的MSVC `/Zs`语法检查通过（PPI已有bool比较警告未变），差异检查通过。未完整构建、未运行QTest或实机TCP/GUI，运行验收仍需本机编译后执行。

### v5.83 (2026-10-20)

- **光电目指操作员界面**：将第6节链路的详情窗口从纯文本改为状态卡+对照表，正文/表格22px、分区标题24px，深色/青绿风格沿用健康管理。上方显示TCP连接、当前目标/下发状态及雷达/光电端点，使用绿/红/黄区分在线、故障和等待；下方固定10×4表格并列“最近下发/当前批次回传/上一批次回传”，显示批号、类型、响应状态值及解释、经纬高、报文/本机时间、设备ID、匹配结果。移除“仅第6节”等面向开发者的正文，停止按钮统一为“停止下发”，无活动目标时禁用。
- **数据真实性与刷新**：工作线程确认本机TCP写入后才更新最近下发表格，保留对应写入快照，避免将待发送坐标显示成已发送；不将本机入队当作对端执行。回传沿用两批次有界模型和会话/顺序校验，不改变协议、F6/右键选择、消批和单目标规则。表格单元格复用，仅可见/值变化时更新；关闭重开保留记录。窗口在通用CusWindow首次adjustSize后按所在屏幕设置初始尺寸，支持滚动和列宽调整，大字不因缩窗变小。
- **验证**：扩展UI QTest源码检查22px字体、状态卡、四列表格、无开发说明、真实回传字段映射、两批次轮换、单元格复用、停止禁用和关闭重开。使用Qt5.14 MOC及MSVC `/Zs`静态语法检查，不自动完整构建；尚未运行QTest或实机视觉验收。

### v5.84 (2026-10-22)

- **航迹列表显示幅度**：将所有共用航迹列表的第8列从“指控”替换为“幅度”，绑定上报`PointInfo.amp`并沿用1位小数格式，列数/俯仰位置/手动列宽不变。分类结果触发普通航迹复制到无人机表时同步幅度；排序和外部上报状态刷新不再向第8列写入状态文字，保留既有上报行排序、强调及实际通信功能。
- **验证边界**：针对列序、幅度绑定、分类复制和状态刷新路径进行静态检查；不自动完整构建，界面实机验证由本机编译后执行。

### v5.85 (2026-10-22)

- **转储确认退出期访问冲突**：Windows事件及`DispCtrl.exe.20168.dmp`与同批EXE/COFF核对，RVA `0xDFF3E` 为`arrangeParamSettingsButtons`解引用已置空的ui。实际链为总控模块析构→stopNetwork→readyChanged(false)→以主窗口为context的lambda→已销毁覆盖层。改为覆盖层context及捕获，模块析构用`stopNetwork(false)`不通知UI，仍正常封口/flush记录和退出线程。配置总控false原先仍可触发，不能靠关闭配置规避。主窗口成员指针显式初始化，避免初次show引发的延迟回调读未初始化成员。
- **窗口生命周期与阶段日志**：覆盖层析构早期断开总控信号，在成员仍有效时同步删除健康/存储子窗口，避免destroyed回调访问已析构计时器/数组。新增健康/存储开窗阶段、覆盖层析构和应用退出来源日志；正常运行不增加逐点日志。没有修改四阵面协议、显示缓存、BIT过滤或GPU配置。
- **证据边界与验证**：已证实关闭销毁链，不将其直接等同于用户描述的首次开窗触发原因；WebGL警告、非法UDP包及积压日志也不能单独定因。新增`command_control_lifecycle_test`源码覆盖无网络模块析构不通知、接收UI先于模块销毁；Qt5.14 MOC及修改源码/测试的MSVC `/Zs`语法检查、针对性源路径检查通过。未自动完整构建、运行QTest或实机复现。复测与转储对应方法见`docs/window_open_crash_investigation.md`。

### v5.86 (2026-10-22)

- **启动与运行期模式切换**：取消启动模式/登录弹窗，固定用户模式；版本号右侧新增当前模式按钮。升级通过独立深色验证框校验原有账号密码，失败可重试/取消；开发者模式点击直接降级。认证状态只驻留内存，不写配置或记录密码。
- **权限即时生效**：`AuthManager` 新增 QObject 模式变更信号，主参数按钮即时显隐重排，已打开 TAS/TWS 高级项可反复显隐恢复且不丢输入、不重置窗口位置。降级关闭波形控制窗口并禁止其保存/下发，确认对话框使用受保护的实例生命周期；模式切换本身不发送 UDP。同步 CMake/qmake 和新增 `auth_mode_test` 测试源码。
- **验证边界**：针对默认用户、错误/取消、正确认证与重复通知补充 QTest 源码；使用 Qt5.14 MOC、MSVC `/Zs` 和源路径检查验证。未自动完整构建、执行 QTest 或实机 UI 操作，仍需本机编译后验收。

### v5.87 (2026-10-26)

- **AA06 高度处理下界**：信号处理窗口在距离上限右侧增加有符号整数高度下界，单位m、默认0，允许 -32768～32767；下发及保存都校验。复用预留第25～26字节，保留6字节清零和32字节负载，加入编译期长度/偏移断言。仍经原Controller→资源调度下发链路，不调整其余参数、门限量化或通信端点。
- **配置及验证**：`params.sigpro.heightProcessLowerLimitM` 支持启动恢复、保存及缺省/非法回退，同步仓库默认配置和自动配置模板，旧配置无需补字段。新增 `sigpro_param_test` 源码覆盖默认帧、正负高度及short两端边界字节，校验原字段不移位及预留清零；进行UIC/MOC、MSVC `/Zs`及文档/配置静态检查，不自动完整构建或执行QTest，实际信号处理联调需本机验收。

### v5.88 (2026-10-26)

- **统一雷达参考信息**：所有模式提供经纬高/偏航/倾角配置，按字段选择UI、最新有效上报、开机默认值，零值有效、留空自动、运行期覆盖不持久化。Controller合并DD05后分发全部既有位置消费者，统一地图和目标解算，避免GPS覆盖手动配置或电磁专用原点与其他链路分叉；只以阵面1姿态作为平台偏航，不追加RAE姿态补偿。
- **多阵面法线**：手动偏航决定ID0阵面扇区中心，其余物理ID依次+90°，跨度保持原设置，关闭某阵面不重新编号。保留BIT实时扫描线/检测/航迹绝对角，不发送伺服控制或阵面安装命令。
- **验证边界**：新增`radar_reference_test`源码，修改Controller/PPI/ScanLayer的MOC及相关源码用Qt5.14/MSVC `/Zs`检查；未自动完整构建、执行QTest或实机验收。检查位置混合配置、恢复自动及所有上报链路实际发包坐标后再判定端到端通过。

### v5.89 (2026-10-26)

- **雷达位置配置窗口调整**：标题改为“配置雷达经纬高/姿态”，移除说明行、placeholder和全部恢复自动；初值为UI当前有效值。“确定”应用，“保存本地”校验全部五项后原子保存到实际加载的config.toml并应用；高度限制-1000～5000m，其余保持经纬度/姿态边界，空值/非有限值拒绝。
- **电磁隔离恢复**：电磁enabled时打开其原有专用经纬高窗口，独立手动原点不共享全局UI覆盖；Controller额外转发原始DD05给电磁，其他消费者仍使用合并原点。保留多阵面法线及点迹/扫描角语义。
- **验证边界**：新增配置保存QTest源码，覆盖原键更新、缺失键/段补齐、注释/其他键/CRLF保留、精度回读及非法输入/保存失败不改内存；补充高度边界用例。进行MOC及Qt5.14/MSVC `/Zs`检查，未自动完整构建、运行QTest或实机UI联调。

### v5.90 (2026-10-27)

- **TAS随机跳频**：频率下拉框在9.8GHz后追加“随机跳频”，按用户最终确认发送90(0x5A)，与固定选项0/10/.../80连续；保存及重开恢复90→索引9。不影响其他模式频点选项。
- **TAS完整下发日志**：AA04、AA05、AA03的三个下发入口从实际参数结构生成完整日志，同时进入UI栏和滚动文本日志；修正旧日志频率字符串及“方位中心”误标。覆盖隐藏的全部波形参数、物理量及必要raw值，标注SUBMIT/下发请求，不将入队或请求当作回送成功。不增加周期或逐点日志，不改变三条指令顺序。
- **验证**：新增`tas_control_format_test`源码检查固定频点兼容、90字节值/恢复、仅频点字节变化及日志内容；进行UIC/MOC及Qt5.14/MSVC `/Zs`检查，未自动完整构建或运行QTest，实际90指令的对端支持与发包/UI需本机联调。

### v5.91 (2026-10-27)

- **PPI扫描样式精简**：单/多阵面扫描指示改为外圈刻度带的红色粗短线，移除大面积渐变余晖、贯穿中心的线和内部跟随文字。多阵面扇区统一青绿透明色，叠加区域一次填充，红线统一最后绘制。
- **验证边界**：保留绝对BIT扫描角、扫描范围及物理阵面法线顺序。新增`scan_layer_render_test`离屏图像检查源码，覆盖四方位红标、内区无红色贯穿线、统一填充及关闭阵面；Qt5.14/MSVC `/Zs`及静态检查，不自动完整构建或运行QTest，实机缩放/扩展屏效果待本机验收。

### v5.92 (2026-10-27)

- **扫描红标长度/层级**：长度18→36场景像素，宽5设备像素。新增固定一个透明鼠标穿透QWidget覆盖层，使用viewportTransform将最多4根红标投影至PPI控件上方，不遮挡鼠标、不提高半透明扇区层级；16ms合并并仅重画新旧标记区域，窗口尺寸/滚动同步刷新。扩展离屏测试源码检查控件遮挡和长度。
- **TAS缺省9.0GHz及协议核对**：UI及默认TASModeParam频点设0，无效旧编码回退0，有效显式保存频点仍恢复。AA05现有负载为54字节，方位起止int分别占第5～8/9～12字节；3组波形从第19/31/43字节开始，新增长度/偏移断言。用户表末尾50字节的编号不代表正确实际长度；频点沿用0/10/.../80加随机90。当前仍仅波形0～11、场景五项，未因核对问题擅自新增波形12～16或改场景为四项。
- **验证边界**：UIC/MOC、Qt5.14/MSVC `/Zs`及静态检查，保留原PPI已有C4804警告；不自动完整构建、运行QTest或实机验收。实际遮挡、缩放/多屏与协议对端仍需本机验证。

### v5.93 (2026-10-27)

- **健康页布局及状态刷新**：删除扫描角度，FPGA温度/阵面温度/阵面偏航合并一行；保持全页状态100ms可见合并刷新。新增独立100ms主软件状态UI定时器，原始监控接收槽仅缓存和业务分发，不逐帧更新QSS。
- **状态日志降频**：彻底移除BIT UI周期摘要及电磁首帧全零逐事件INFO；保留Controller字段文件限频日志。软件终态UI日志按每秒最新变化、伺服UI日志按每秒最多一条最新变化输出，不重复刷相同值，不改变TAS人工下发日志。子阵电源确认/恢复文件日志按阵面5秒限频，100帧全零防抖判定不变。
- **验证边界**：MOC、Qt5.14/MSVC `/Zs`、接收槽无UI/日志操作和定时器源路径检查，不自动完整构建/实机或长时间压力测试，不将静态检查当作吞吐或无卡顿保证。

### v5.94 (2026-10-27)

- **五秒异常窗口与1000帧防抖**：新增`Basic/healthstatewindow.h`，Controller按固定四阵面独立缓存窗口故障位、最新必要字段和电源全零计数，无逐帧列表/队列。全零确认阈值100→1000，跨窗口累计，部分电源故障沿用原规则；任一路已确认故障锁存至窗口结束。发射/接收关闭不算故障，软件失败状态计入异常。
- **接收与显控解耦**：MainOverLayOut改接五秒健康快照，不接逐帧BIT/监控状态；窗口全部发布后才刷新控件及主健康按钮，移除额外100ms/1s状态定时器。扫描线/姿态/其他必要协议继续用原始BIT；对外健康报文继续按原发送周期发送最新UI快照。
- **仅异常健康日志**：移除正常BIT字段日志（旧开关不再启用它），五秒发布阵面/软件异常摘要；伺服只保留窗口失败摘要，正常/成功/恢复不写健康日志。保留最少五秒限频的非法协议诊断，人工下发日志不变。
- **验证边界**：新增`health_state_window_test`源码覆盖窗口锁存/恢复、999/1000帧、跨窗口计数、四阵面独立、首次未知电源、部分电源故障及软件失败；模型大小编译期断言<1KB。进行MOC、MSVC `/Zs`及数据流静态检查；未自动完整构建/运行QTest或实机压力测试，不据此承诺实际吞吐性能。

### v5.95 (2026-10-27)

- **AA08阵面位置控制**：PPI配置按钮下新增“下发雷达阵面经纬高姿态”，点击采样同一份当前参考值，经Controller→Disp2ResManager→既有资源调度UDP端点/封装发送；不因GPS/UI更新自动下发。负载142B，小端pack(1)，四条31B记录固定ID0～3，预留16B清零。经纬高、倾角、横滚共用ID0值，航向归一化`yaw+ID*90`。UI与文件记录实际量化值及下发请求，不声称对端已收到。
- **位置源与GPS防护**：经纬高改为最新有效UI/GPS事件覆盖；手动姿态继续保持。`GpsPositionFilter`仅保留最近5个有效样本，先排除非有限/越界及经纬高同时全零，再相对近期样本中位数做水平/高程偏离剔除，最后均值平滑（经度跨180°展开）。[radar]的gps_max_horizontal_deviation_m/gps_max_altitude_deviation_m默认100/50，非法阈值回退默认；首帧不拿开机/UI位置限制。剔除样本不改变窗口/原点，5秒限频文件告警。为防误定位，连续远离旧GPS窗口的新位置也会被拒绝，当前不自动重捕获；换场地需重启重建GPS窗口。DD05 memcpy前检查最小长度。
- **界面与兼容**：增加横滚显示/配置/保存；经纬度显示9位、航向/倾角3位、高程/横滚2位，与AA08量化一致。电磁专用窗口/手动原点保持独立，GPS输入改用过滤平滑结果。历史版本中的UI永久压过GPS描述被本次经纬高最新事件规则替代；不改变旧目标协议或追加姿态旋转。
- **验证边界**：补充AA08长度/偏移/符号/量化/四面航向/预留、GPS拒绝/五点滚动/跨日期线及源切换QTest源码；MOC、MSVC语法检查和diff检查，不自动完整构建、执行QTest或实机UDP联调。

### v5.96 (2026-10-27)

- **Linux发布环境复用**：默认复用本机已存在的`dispctrl-builder-1804`及四版本离线依赖镜像，不因应用源码变化重复下载Qt；显式`-RebuildImage`/`--rebuild-image`才重建环境。仍只读挂载当前源码、复制到容器私有/src全新编译，使用挂载的最新container_build.sh兼容旧镜像无ENTRYPOINT；不复用旧二进制。修正aqt5.15.2可选模块列表和CRLF头文件版本号提取。
- **实际验证**：本次当前工作区已用Qt5.15.2/GCC9.4/Ubuntu18.04基线完整编译链接并打包，产物位于deploy/ubuntu1804，VERSION沿用源码V5.27。资源/动态依赖检查、SHA256，以及全新Ubuntu18.04/20.04/22.04/24.04容器的断网依赖安装和主程序/WebEngine的ldd均通过。只做编译与依赖检查，未启动GUI、测试真实GPU/网络或运行QTest；不能据此声称现场全部功能已验收。历史deploy根目录旧包保留。

### v5.97 (2026-10-27)

- **多面阵控件简化**：按default_panel_count=2～4隐藏主界面TWS模式设置/伺服归北、TAS内伺服控制组/积累脉冲数，用户/开发者切换均保持隐藏；不清空隐藏参数，不改变单面阵。仅多面阵取消TAS方位90°限制及相应标题提示，仍按原逻辑发送AA03。
- **全角范围配置**：TAS保存方位及ConfigManager保存/读取方位改为32位int，保持协议不变，避免327.67°以上保存/恢复溢出。补充±36000配置读取测试源码。MOC/MSVC语法及diff静态检查，不自动完整构建/运行测试或重新打包Linux发布包。

### v5.98 (2026-10-27)

- **布局适配**：PPI左上经纬高/姿态区按实际字体计算完整有符号经度最小宽度，增加行列间距、统一输入框高度和两按钮字号/高度，保留9位经纬度、不截断字符。雷达控制按当前可见按钮重排为均分两列，清除旧空行/空列；多面阵三行依次为启动/关闭、TAS/存储、待机/发射，单面阵恢复八按钮四行。权限/模块界面重排时保持紧凑，不改变按钮下发行为。
- **验证边界**：Qt5.14 UIC/MOC、MSVC语法及diff检查；未自动完整构建、实屏截图验证或重新生成Linux包。

### V6.0 (2026-10-28，软件发布版本迭代)

- **版本**：软件运行版本从V5.27升级至V6.0，更新APP_VERSION_STR及主界面UI占位；Qt applicationVersion使用相同常量，发布包VERSION继续从该常量提取，桌面入口显示DispCtrl V6.0/雷达显控 V6.0。此前v5.28～v5.98是本文开发记录编号，不代表旧程序实际显示了这些版本，历史记录完整保留。
- **Linux桌面体验**：对照D:/RadarAPP安装器，保留手动安装/卸载/登录自启动配置，新增首次run.sh自动注册菜单和桌面入口、雷达图标与Qt桌面标识；不需要单独运行安装器，不静默开启自启动，不修改系统服务或真实开发机桌面。应用目录空格/百分号正确转义；重复运行幂等，注册失败继续启动。
- **本版本包含**：延续当前工作区AA08阵面经纬高姿态及GPS过滤、五秒健康快照/电源防抖、多面阵TAS界面简化和经纬高/雷达按钮布局适配等已实现修改；其中实机尚未验收的功能仍保持原验证边界，不能因升级版本号视为已验收。此前电磁目标首点经纬高异常仍未由原始出站报文定位，本次不声称修复该问题。
- **验证**：使用既有Linux镜像、无网络、隔离临时HOME和普通用户执行桌面安装/首次运行脚本测试，应用以无副作用桩替代。覆盖入口/图标/版本、空格与百分号路径、工作目录及参数透传、重复注册、autostart保持/开关、卸载、禁用注册、注册失败继续启动、无独立桌面及非法VERSION拒绝。通过Bash语法、Qt UIC与MSVC语法/diff检查；不自动完整编译、重打包或启动真实显控，现有V5.27压缩包不会因源码改号自动变为V6.0。
- **正式发布验证（用户随后明确要求，2026-10-28）**：沿用已有Qt5.15.2/GCC9.4/Ubuntu18.04镜像，当前源码完整编译链接并生成`deploy/ubuntu1804/DispCtrl-linux-x64.tar.gz`，包大小486972199字节，SHA256=`9c70ad33bb5428b859b3685dce9453eb997f0bf425023ef9330dc6d0216194e4`。压缩包内VERSION和ELF版本字符串均为V6.0，启动脚本与当前源码一致、雷达PNG为512×512；资源/动态依赖、压缩包校验、隔离普通用户桌面脚本测试及Ubuntu18.04/20.04/22.04/24.04断网依赖安装/ldd均通过。包含前述多面阵及布局修改。未启动真实显控、运行QTest或做GPU/雷达网络实机验收，不以依赖检查代替现场验证。
- **后续版本策略**：AGENTS.md和本文开头要求每轮修改先确认保留/小版本/大版本；当前V6.0对应候选V6.0/V6.1/V7.0。已明确版本的同一轮重打包不再次改号或重复询问，不经用户确认自动升级。

### V6.1 (2026-10-28，用户确认小版本升级)

- **版本**：V6.0→V6.1，同步运行期常量、UI占位及从常量提取的发布版本。用户明确本次只更新源码和打包规则，不重编译/重打包，已有V6.0产物不改动。
- **Windows资源核对**：检查E:/DPRelease。black/index/index3d/indexNoL/indexS/maplibre.html及qwebchannel.js与源码内容完全一致；amap/AMap3.js、2.0.1相同，init.js/mapsplugin.js仅LF/CRLF差异，规范化后无逻辑变化，无需用同内容覆盖。Windows resources仅为其Qt运行时文件，不能替换Linux WebEngine资源。
- **配置合并**：仅同步6个现有有效值：electromagnetic_device.enabled/ui_enabled=false、map.default_type=3、displayConfig.max_points=100/max_track_points=300/default_panel_count=4；保留高度处理下界及两项GPS剔除阈值等源码新增键，不用Windows文件乱码注释覆盖源码。其他共同键核对一致。data.txt是现场录制索引，运行期缺省可为空/不存在，不随发布复制。
- **每次发布完整性**：独立资源复制脚本和轻量测试覆盖完整HTML/AMap（包括无扩展名2.0.1）、样式/图标/配置，静态摘要查缺损，配置可现场编辑。取消自动复制本机瓦片目录，并过滤嵌套记录/瓦片/OSM及Windows文件；CMake安装htmls规则同步排除项。check_package校验增加black/maplibre及高德全部文件，避免只检查AMap3而漏其他资源。
- **无瓦片启动**：移除MapProxyWidget构造函数的无瓦片提前返回，保留缺数据告警和正常延迟网页/WebChannel初始化。此改动只修复页面被跳过，不生成底图、不替换在线SDK/授权、不恢复已禁用MapLibre模式；实际地图/GPU渲染需实机验证。
- **验证边界**：Bash资源复制/排除/摘要损坏/缺文件测试及Qt UIC、MSVC语法/diff检查；不完整构建、打包或启动真实显控，不声称已有V6.0包包含本次资源规则。

### V6.1 同版本修复与重新发布（2026-10-28，用户确认保留V6.1）

- **范围**：本轮用户另行授权完整Linux编译和替换ubuntu1804发布压缩包，覆盖前一轮“只更新源码和规则”的验证边界；不自动提交或推送。
- **1920×1200适配**：采用compact布局，局部/全局QSS和显式手绘字号接入ui.ui_scale，固定DPI策略清除继承的额外Qt缩放变量；Linux选择可用中文字体。ScaleHelper抽到独立Basic/uiscale.h，字体工具不依赖协议头，避免把遗留pack(1)引入其他Qt头。启动文件日志[UI_CONFIG]输出实际配置路径与缩放参数；编辑bin/config.toml后重启生效。
- **无地图**：保留black.html实际WebEngine加载；解除瓦片存在性对页面加载的门控，补充页面黑底、Qt黑底兜底及加载/DOM/异常限频诊断。修复不生成瓦片，也不把HTML加载成功当作GPU首帧绘制成功。
- **针对性验证**：Qt5.15.2离屏字体QTest共9项通过；无瓦片black.html加载、JS计算背景色及缺地图文件后恢复QTest共3项通过。离屏DOM测试保持宿主隐藏，不声称验证了目标笔记本GPU合成/真实屏幕布局。
- **发布结果**：复用现有Ubuntu18.04/GCC9/Qt5.15.2 builder完成全量Release编译，替换deploy/ubuntu1804/DispCtrl-linux-x64.tar.gz（486990439字节，VERSION=V6.1，BUILD-INFO UTC=2026-10-28T08:14:56Z）；SHA256=f9eba012570726f04baefc05985b169c4ad2a53fb8a7256c56ab26b18a2858e7。资源复制/排除/损坏检查、桌面入口回归、CMake两组QTest及Qt5.14 MSVC语法检查通过。四个干净断网Ubuntu18.04/20.04/22.04/24.04容器的离线依赖安装和主程序/QtWebEngineProcess动态库闭包检查通过；它们不代替图形桌面、现场GPU或雷达联网验收。包内black.html/config.toml与源码摘要一致，历史deploy根目录旧包未修改。

### V6.2（2026-10-28，用户确认小版本升级与重新发布）

- **版本与范围**：V6.1→V6.2，同步运行常量和UI占位；用户明确授权重新编译并替换deploy/ubuntu1804下的Linux发布包，不授权Git提交/推送。桌面入口和包内VERSION仍从同一常量派生。
- **修复**：按用户要求恢复Linux发布入口默认--no-sandbox，额外设置Qt专用沙箱环境变量，提供显式恢复入口；main.cpp不再用配置flags覆盖启动器已有flags。文件日志新增WEBENGINE_CONFIG，渲染进程退出提示改为中性诊断，不再一概归因GPU。
- **缺瓦片显示**：路网/标准/卫星分别检查mapNoL/map16/map16S，不再以任一目录存在就认为所有地图都有瓦片。缺失/空目录时保留用户下拉选择，实际加载black.html并跳过地图状态刷新；WebEngine/WebChannel仍实际启动。检查仅针对目录缺失/为空，不扫描大瓦片库、不检测当前经纬度/缩放级别是否覆盖。
- **针对性验证**：离屏QTest通过，覆盖无瓦片三种选择、空目录、其他地图有目录而所选目录缺失、有目录/HTML时正常选页及缺HTML后恢复；真实黑页JS背景为黑。隔离普通用户启动器回归通过，覆盖默认沙箱环境/参数、继承参数保留和显式恢复。资源完整性/排除回归、Bash语法、Qt5.14 MOC/MSVC语法及diff检查通过；不代替目标机GPU实测。正式构建/归档校验结果在本节追加，不以此前V6.1的包摘要代替。
- **正式发布**：复用现有Ubuntu18.04/GCC9.4/Qt5.15.2镜像全量Release编译成功，替换deploy/ubuntu1804/DispCtrl-linux-x64.tar.gz（486995447字节，SHA256=19631c2cde278161e2f1e8e4d3e88abf3b3e251403233025e48448a0733db719，BUILD-INFO UTC=2026-10-28T09:18:55Z）。包内VERSION、ELF版本字符串、UI及桌面入口均核对为V6.2；归档内确认默认--no-sandbox/Qt沙箱环境与black.html，启动脚本/config与源码摘要一致，关键C++/头/UI源文件匹配BUILD-INFO。两组Qt测试、资源/桌面脚本（含V6.2升级）和包检查通过；Ubuntu18.04/20.04/22.04/24.04四个干净断网容器的离线依赖安装及主程序/QtWebEngineProcess的ldd闭包通过。未启动真实雷达GUI、未做目标机GPU验收；用户此前已确认临时禁用沙箱不闪白，本包将同一设置默认化。升级前备份现场bin/config.toml，保留ui_scale和网络配置；无Git提交/推送，历史deploy根目录旧包不改动。

### V6.3（2026-10-29，用户确认小版本升级）

- **内部BIT扩展（按用户最终表修订）**：只接内部DE02，不接外部128B BB01。新版BITReportV2为103B小端负载，原39B前缀（含雷达编号后21B预留）完全保留，纬度/经度/高程/航向/倾角/横滚偏移39/47/55/59/63/67，69起34B新预留；完整内部帧122B。default_panel_count=1仅接收39B旧版，2～4仅接收103B新版；UDP在构造时缓存模式，不按长度猜测，不接受跨模式或截断帧，原临时58B方案作废。旧21B预留绝不作为新位置。39B BITReport仍为共同健康/扫描模型。
- **数据流**：接收线程显式小端解码，扩展参考信号先于标准BIT信号排队进入Controller GUI线程。所有既有BIT接收管理器连接新信号；Qt元类型注册新增BITReportV2。没有新增逐帧UI日志、历史缓存或扫描绘制对象；健康五秒窗口、36路电源1000帧全零防抖不变。上游资源管理2秒限流属于其发送要求，显控不修改上游调度或主动丢BIT帧。
- **来源策略**：default_panel_count>1时四个ID的新版BIT位置分别进入GPS有效性/全零/偏离剔除和独立五点平滑，只有ID0分发统一原点及电磁过滤位置入口；多面阵DD05不再混入位置窗口，其他ID不覆盖ID0。没有有效新位置时保留当前UI/开机默认值，不回退为0。新航向0.001°/倾角0.001°/横滚0.01°范围独立校验，有效自动姿态受手动姿态及显式锁定优先规则约束；收到有效扩展姿态后不再用旧0.01°yaw覆盖高精度姿态。单面阵保持DD05及旧BIT yaw。解锁恢复有效缓存并清除该行手动覆盖；不额外旋转RAE或改变绝对扫描角。
- **版本与验证**：V6.2→V6.3，同轮最终表修订不再次改号；不更改config默认值。字节解码QTest覆盖103B字段偏移、负经纬高/姿态、共同BIT状态、两段非零预留，以及单/多面两种模式下所有0～119B长度（含跨模式、作废58B及截断）。连同健康窗口、RadarReference及GPS/AA08共四组QTest全部通过；Qt5.14 MOC/MSVC语法及diff检查通过。未完整工程编译、未重打包Linux，现有归档仍是V6.2。真实UDP与现场目标解算待联调。

### V6.3 补充：四阵面位置编辑与独立锁定（2026-10-29，用户确认保留V6.3）

- **入口/联动补充**：按default_panel_count切换左上按钮：单面阵仅配置入口（电磁专用路由保留），多面阵仅阵面下发入口。多面阵窗口用户编辑任一行经度/纬度/高程时，同字段同步其他未锁定行；已锁定行不变，航向/倾角/横滚独立，程序填值和解锁不触发联动。不改变锁定、校验及AA08封装。
- **窗口可读性修正**：参照健康管理深色卡片/青绿标题风格，显式设置滚动区viewport及内容QWidget背景，避免系统默认浅色底配浅色文字；标签透明底、禁用按钮提高对比度。保留ui_scale及原下发/锁定语义。
- **UI/下发**：新增paramWidget/radarpanelpositiondialog，24个数值输入、每行锁定/解除锁定、确定下发/取消，整表校验后才构造142B AA08。新增构建重载允许四行经纬高、航向、倾角、横滚完全独立；旧默认+90°构建入口保留。记忆实际量化后的上次请求，锁定行优先，非法输入/链路未初始化不记忆为成功请求。
- **状态/生命周期**：Basic/radarpanelstate.h维护固定四行参考与五样本过滤，RadarReference增加Locked来源与独立冻结值。Controller沿用ID0规范参考的稳定别名，不复制两份全局原点；锁定期间报告只更新缓存，主显示不变。解除后恢复有效报告或开机默认，不自动下发AA08、不停止健康/目标通信；清除被解锁行旧手动覆盖。表单草稿不随BIT自动重置，取消仅取消下发，不撤销已点击的锁定/解锁。所有新增状态只存内存，不持久化。
- **保护/性能**：锁定值无法经普通原点配置/保存入口或重新提交AA08覆盖；高程-1000～5000m、其他数值范围及有限值校验复用现有规则。保留103B多面阵/39B单面阵协议、五秒健康/电源防抖、绝对扫描角和旧外部设备功能；不新增热路径场景对象或周期UI日志。人工锁定/解锁及实际下发值写文件日志。
- **验证**：新增独立状态及离屏窗口两组QTest，覆盖默认+90、逐行独立量化/记忆、非零阵面锁不覆盖ID0、锁中BIT更新冻结值不变、解锁恢复过滤均值/无上报回默认、锁定不能被草稿/普通手动覆盖、非法输入/取消/失败无发送、24字段及窄屏尺寸。连同BIT解码、健康、参考状态、GPS/AA08共六组回归全部通过；Qt5.14 MOC/MSVC语法及diff检查通过。姿态单独变化只更新参考通知，不重放经纬高消费者。未完整构建、未现场UDP/GPU验收、未重打包；已有Linux归档仍为V6.2。

### V6.4 实时参考与偏航诊断（2026-10-29，用户确认V6.3→V6.4）

- 本段替代上文五样本平滑、窗口恢复上次下发值及锁定行只读的旧行为。GPS过滤保留非法/全零及100m/50m异常剔除，环形窗口扩大到最近10个有效值，不足10个按实际数量平均。多面阵每ID独立经纬高和姿态窗口；姿态要求该帧位置有效且角度范围合法，航向使用sin/cos圆周平均，倾角/横滚算术平均。单面阵收到的航向也用10次圆周平均；手动姿态、锁定优先级不变，未上报的量保持默认。
- 阵面窗口以200ms定时读取Controller规范快照，未锁定行实时只读，锁定后冻结并允许修改；点击确定下发后，整表先校验，未锁定行强制取当前有效值，锁定行使用编辑值并更新本机冻结参考。取消不应用草稿，解除恢复过滤报告。用户编辑经纬高仅同步其他已锁定可编辑行，不覆盖实时只读行。无新增周期UI日志或逐帧队列。
- 窗口下方自绘偏航曲线：原始BIT.yaw，0.01度量化、0～360°，按配置阵面数合并一轮上报，每轮只取一个样本；有非零取非零（多个则最后收到的），否则仅取一个0°，超范围剔除。DE02无共同采集帧号，采用“配置的ID各收到一次完成一轮；同ID再次到来时结束未齐的上一轮”接收顺序规则，不宣称硬件同步帧、不复用上一轮阵面值，缺阵时可能延迟到下一帧到达才补入。保留200个合并样本，不是均值/冻结航向。固定数组缓存，窗口按200ms绘制；各ID最新DE02接收时间仍逐包独立记录。本规则按2026-10-29用户确认实现，替代之前保留所有范围有效零值的逻辑；状态/离屏窗口两组回归通过。
- PPI左上面板改13px设计字号、紧凑间距，六项显示5位小数，发送/存储的量化精度不变。运行版本与UI占位同步V6.4，发布/桌面版本继续从运行常量派生。六组QTest（含10样本、跨零、200帧上限、实时只读/锁定可编辑）以及Qt5.14 MSVC语法检查通过；未完整构建或重打包，现有Linux归档未替换。

- **V6.4曲线/窗口补充（2026-10-29，沿用本轮版本）**：图表常规最小高度由180增加到260设计像素，布局优先分配新增高度，小屏允许表单滚动并缩减最小高度。纵轴覆盖200个在窗原始/平均样本的最小最大值并各加5°，不固定0～360。青色原始BIT偏航、黄色最近10个合并轮次偏航圆周平均，平均随样本入环时计算并保留，窗口滑动不重启平均。仅绘图做跨零展开（例如359→361），不改实际协议/控制角度；该黄色曲线是诊断平均，不等同于每ID扩展heading字段的参考平均。阵面窗口由exec改为show非模态，允许并行操作主显控/健康等窗口；QPointer持有单实例，关闭自动释放，重复打开只唤起已有窗口，不丢草稿。窗口状态/纵轴范围、跨零与均值回归及Qt5.14语法检查通过；未完整构建或重打包。

- **V6.4故障日志/30次均值补充（2026-10-29，延续本轮版本）**：本条覆盖以上10样本描述，经纬高、各ID姿态、单面阵航向及诊断偏航均扩大至最近30个有效样本，不足30按实际数量；圆周平均/异常剔除/锁定优先级及200点曲线容量不变。健康5秒窗口与界面快照照常；仅故障摘要到文件及UI路径限频，四阵面和软件各一个固定槽，首次异常立即记录，相同故障持续时5分钟提醒一次，变化故障每槽最短30秒一条。正常快照清除故障身份但不重置冷却，避免异常/正常抖动刷屏；没收到报文不视为恢复。门限只影响日志，不延迟健康状态、设备状态上报及故障检测；没有新增正常周期日志。

- 伺服失败摘要复用相同30秒变化/5分钟持续故障限频，按控制指令分类，不把不断变化的反馈角度当新故障；不修改伺服控制/状态接收。30样本及日志门限六组QTest通过，Controller、窗口与主界面Qt5.14语法检查通过；本轮仍未完整构建或打包。

- **V6.4偏航按ID独立修正（2026-10-29，用户现场口径优先）**：撤销以上“凑齐四阵面一轮/非零优先/全零只取一个”的推断。每条DE02按本帧radarId即时更新，不等待其他ID；0°作为该ID合法值保留，超范围拒绝。四个阵面分别维护原始BIT.yaw的30次圆周均值和200帧历史（固定800个历史样本），帧计数也分别递增。窗口显示最近收到有效偏航的ID及其两条曲线；ID0→ID1切换显示ID1自己的历史，再回ID0继续ID0历史，不混合均值、不把两阵面连接成航向跳变。各ID经纬高/姿态与接收时间依旧独立，ID0统一参考及锁定/下发语义不变。版本保持V6.4。

- **V6.4默认锁定与选中接入（2026-10-29，最新用户口径）**：本条覆盖以上“默认实时/各ID独立诊断曲线”等旧描述。多面阵初始化四行全部冻结为配置值（航向按ID×90°），单面阵不受此默认锁定影响。仅手动解锁的一行接受后续DE02参考平均/诊断偏航；切换解锁ID时自动冻结其他行，原有效值不变。锁定期间仅接收时间/健康和扫描路径照常，参考滤波与诊断曲线不喂入锁定报文，也不缓存锁定期间的参考报告供回放。解锁从冻结值继续，等待新BIT，而不跳回旧缓存。窗口200ms同步模型的锁状态，自动锁回行按钮/只读状态同步，既有锁定编辑、整表校验/AA08下发仍保留。
- 诊断改为一条固定200点历史/一个30样本圆周均值窗口，解锁ID切换不清空、不重置帧号和均值，样本继续标注真实ID；不再对BIT.yaw作0～360范围筛选（原始uint16×0.01保留），图上跨零展开及圆周平均保持。经纬高及扩展姿态的物理输入校验、人工下发合法性校验不撤销。全锁时曲线/均值暂停，最新BIT接收时间继续更新。当前版本保持V6.4，未完整构建或重打包。

- **V6.4阵面锁定持久化（2026-10-29，用户确认保留V6.4）**：新增独立radar_panel_reference.toml，路径为当前实际加载config.toml的同一目录，不读写原config.toml。文件format_version=1，panel0～panel3各六项longitude/latitude/altitude/heading/pitch/roll，角度度、高程米，17位有效数字保留double精度。多面阵启动优先整表读取并恢复为四行锁定；不存在则用config默认+90°初始化并创建；文件损坏/字段不齐/范围错误则整表拒绝、使用启动默认且保留损坏文件，文件日志记录路径和原因，单面阵不加载或创建该文件。
- 每次人工锁定、解锁其他行导致自动锁回、确定下发应用锁定编辑值时，将四行最近锁定快照整体写入；当前解锁行保持其上次落盘值，不把实时BIT作为锁定值保存。QSaveFile原子替换、不退化直接覆盖；写失败时拒绝当前人工操作/下发且显示写权限提示，原文件及本机锁状态不变。只在启动/人工操作写盘，BIT热路径不落盘；不保存曲线/均值历史或“解锁”状态，重启仍默认全锁。此条替代上文“锁定仅本次运行有效”描述；原电磁独立原点配置不受影响。

- **V6.4百样本滑窗与实际角度绘图（2026-10-29，延续本轮版本）**：GPS、各ID姿态、单面阵航向及诊断偏航的窗口统一为最近100个接纳样本，满窗每次替换最旧值，不足100按实际数量；角度仍圆周平均、倾角横滚算术平均。经纬高离群规则仍为有限值/物理范围/三项全零剔除，以及相对最近有效样本逐字段中位数水平距离>配置100m或高程差>配置50m时剔除，首个有效GPS不受开机默认位置限制。诊断偏航仍仅接入解锁阵面，不做范围/跳变剔除；跨ID切换沿同一100样本诊断窗口继续。拒绝或锁定报文不占参考/曲线窗口。
- 撤销绘图跨零展开：此前可将真实40°绘为等效400°，不代表协议接收了400°。原始曲线直接使用解码raw×0.01，均值线使用计算出的实际角度，纵轴继续按两条曲线±5°自适应，不再人为加减360。右上角最新原始值此前就未做展开；若该文本本身超范围需独立核对原报文/解析，不能归因于绘图。该改动不修改协议、锁定文件及实际下发值。版本仍V6.4，未完整构建或打包。

- **V6.4参考值差异调试（2026-10-29，沿用本轮版本）**：表格航向来自DE02负载offset59的headingMilliDeg×0.001，按该ID自己的有效位置/姿态报告滤波；曲线来自offset8的yaw×0.01，使用解锁选择跨ID连续窗口且不做GPS拒绝联动。这是不同字段、不同接纳条件与窗口，表格约97°而曲线约101°不能直接判为同一算法出错。当前未擅自改动表格/曲线或AA08字段用途，表头tooltip明确区别。
- [radar]新增reference_debug_log_enabled=true、reference_debug_log_interval_ms=5000（下限5秒/每ID）、reference_debug_log_samples=true。Controller缓存选项，在合并前不做日志构造，只有限频放行时输出文件标签[RADAR_REFERENCE][DEBUG]紧凑JSON，不发healthFaultSummary/UI日志。内容为本包raw整数及换算值、锁定/位置拒绝原因、姿态接纳结果、表格有效来源/均值、曲线均值/最后ID/帧号、各窗口计数、跳过报文计数；samples启用时附最近100个实际入窗样本（最旧到最新，GPS纬经高/本ID航向倾角横滚/共享诊断偏航及对应ID），可离线复算。此为限频原始包抽样+完整均值窗口，不声称逐包原始录制；禁用时不复制窗口/格式化，定位后建议关闭。源配置默认开启，旧配置缺键亦按此安全限频默认；不改变已有健康故障日志限频。

- **V6.4扫描环形块（2026-10-29，沿用本轮版本）**：PPI扫描标记由红色径向短线改为实心红色环形扇块，以BIT绝对扫描方位为中心左右各2.5°，总角宽5°，外缘仍为刻度环、径向厚度沿用36场景像素。不恢复全扇面余晖、不改变扫描角数据或统一青绿扫描面的颜色。ScanLayer与置顶ScanMarkerOverlay共用markerPaths几何，主视图保持控件之上且鼠标穿透，16ms局部旧/新路径包围框刷新；备用场景视图也使用相同填充样式。

- **V6.4控制按钮可读性（2026-10-29，延续本轮版本）**：仅“雷达控制/参数设置”两页QPushButton使用16px设计字号（原全局14px），通过UiFontScale::style缩放一次；动态测试点迹/航迹和可选总控按钮继承同样字号。保留布局、颜色、状态切换及其他区域字号不变。

### V6.5 CC01数据存储选项（2026-10-29，用户确认V6.4→V6.5）

- DataSave由4字节扩展为16字节，原ID/开关/数据编号位置不变，offset4确认波束、5跟踪波束、6搜索波束、7检测点迹（均uchar 0/1，默认0/1/0/1），offset8～15预留清零，新增sizeof/offsetof静态断言。Disp2SigManager沿用sizeof(param.save)，因此自动发送新16字节负载，原外层14字节帧头/5字节帧尾和UDP端点不变，完整35字节。旧信处4字节解析须配套升级，不能宣称透明兼容；其他CC/DD存储相关协议不变。
- DataSaveUI的编号工具栏下增加四个复选框，开始/停止从UI填字段；编号校验0～255，停止无选中项不发默认ID0。四项选项写入人工命令UI和文件日志，不新增周期日志或BIT热路径。窗口持久化/健康回执/删除与离线功能不变。对新增Qt控件头/生成UI头做pack(8)隔离，避免Protocol.h遗留pack(1)污染新控件ABI。
- 运行常量/UI占位同步V6.5，发布/桌面版本继续从该常量派生。新增定向QTest通过，覆盖16字节布局/默认值/35字节封装校验及复选框开始停止信号；Qt5.14 UIC/MOC及存储窗口、发送入口、主界面、Protocol源码MSVC语法检查通过。未完整构建、未实机UDP验收或重打包。

- **V6.5阵面窗口统一外壳（2026-10-29，延续本轮版本）**：PPI阵面经纬高/姿态下发改为CusWindow统一标题栏/图标/最小化/最大化/关闭/拖动缩放外壳，RadarPanelPositionDialog以Qt::Widget嵌入，隐藏内容重复标题。保持非模态，重复入口唤起现有宿主而不重建；QPointer分别跟踪宿主/内容，宿主WA_DeleteOnClose持有内容所有权，不对内容单独启用自动删除。确定下发/取消仍沿原QDialog完成语义，finished排队关闭宿主，避免在accept/reject调用栈内同步销毁。标题栏关闭也正常释放内容计时器；锁定/持久化/均值/协议逻辑不变。

- **V6.5扫描短弧试样（2026-10-29，延续本轮版本）**：替代此前36px厚实心环形块，标记使用5°圆弧中心线、5逻辑像素cosmetic圆头画笔，中心亮红/两端透明，径向不再形成大方块。ScanLayer备用绘制与主视图置顶Overlay共用paintMarker。主Overlay每阵面最多保留一个旧位置，真实角度更新后旧/新位置做120ms透明度交叠，不插值角度、不声称扫描经过中间方位；同角度不重启，禁用阵面立即清除。16ms局部刷新仅在淡出期间续调，结束后停止；视图缩放、平移、尺寸/半径变化清除过渡以免留下屏幕位置假影。鼠标穿透/置顶/绝对BIT角度及统一青绿范围面不变。

- 本次扫描渲染QTest通过，覆盖单/多阵面、外圈薄弧/端点渐隐、控件之上鼠标穿透、过渡期新旧位置共存及120ms后旧位置清除；Qt5.14 MSVC语法和diff检查通过。仅完成定向验证，未完整构建/打包或现场显示效果验收，版本保持V6.5。

- **V6.5多面阵覆盖/TAS边界分离（2026-10-29，用户确认保持V6.5）**：default_panel_count>1时，绿色透明覆盖区域只取已开启的物理ID，每阵面中心为当前ID0统一参考偏航+ID×90°，覆盖左右各45°，不因关闭中间阵面压缩ID，不以TAS起止角改变覆盖宽度/方位。参考偏航无论开机默认、锁定/UI或已接纳上报均使用，与左上显示一致；其他阵面独立编辑航向不改变本覆盖定义。多面阵的TAS“确定下发”既有角度只按0.01°协议量化直接发送，不增加偏航、不按ID旋转、不裁剪；此轮未改报文生成/发送、AA03或量化逻辑。
- 原scanRangeChanged通知只更新两条红色虚线径向边界，分别从雷达中心指向实际下发起止角，不给每个阵面重复旋转；负角/跨零只作绘图周期表示。多面阵启动不再伪造-30～30° TAS提交，没有实际范围通知前不画红边界。阴影仍一次填充保持统一青绿透明度，扫描短弧及120ms交叠不变。单面阵保留原TAS范围阴影，不新增红色边界。新增覆盖/边界几何定向测试，未完整构建或重打包。

- 本轮验证：scan_layer_render_test、tas_control_format_test 两项定向QTest通过；Qt5.14/MSVC对scanlayer、ppisscene及MOC产物的语法检查通过，相关diff检查通过。未执行完整工程构建、发布打包或现场效果验收。

- **健康故障日志再抑制（2026-10-30，版本暂保持V6.5，待用户确认）**：覆盖此前“相同故障每5分钟提醒、变化故障30秒”的日志规则。HealthFaultLogGate对阵面/软件独立固定槽，相同持续故障不再周期写入，变化故障每槽至少间隔5分钟；连续收到正常快照满60秒才允许重新记录同类故障，相邻正常快照间隔超过10秒重新计时，断流不算恢复。文件与UI摘要共用同一门控，伺服失败门控同样不重复提醒。不改变5秒健康快照、异常锁存、1000帧全零防抖、实时扫描或协议数据更新。不新增逐帧日志/历史缓存；定向回归覆盖持续故障、变化位图、短暂正常抖动、稳定恢复和断流。

- 本轮定向验证：health_state_window_test通过（持续故障不重写、变化故障5分钟冷却、正常60秒恢复及断流不恢复），git diff --check通过；未完整工程构建、实机运行、打包或提交。

- **航向字段按安装模式统一及下发保存（2026-10-30，暂保持V6.5待确认）**：单面阵旧BIT偏航继续yaw×0.01；多面阵诊断曲线改取扩展headingMilliDeg×0.001，与表格/参考姿态字段统一，不再取旧yaw。100次圆周滑窗、跨解锁ID连续诊断均值/200点历史保持；表格仍按本ID接纳的位置/姿态样本独立平均，诊断仍保留未经GPS拒绝联动的原始字段，所以窗口切换/拒绝条件仍可造成均值不同。解除锁定保持冻结值，其他ID不更新该行，无新报文不改变航向。调试日志保留两个原始字段，新增curve_field指明曲线实际来源，表头提示同步。
- AA08“确定下发”原先仅保存锁定行，本轮改为保存本次完整command的四阵面六项量化值（包括解锁行）到实际config.toml同目录radar_panel_reference.toml；原子保存成功后才发起发送请求，失败保留原文件并拒绝本次下发。人工锁定/解锁仍沿用锁定快照语义，BIT热路径不写文件；重启整表恢复且全部锁定，不将发送请求等同设备ACK。

- 本轮验证：radar_panel_state_test、radar_panel_dialog_test、health_state_window_test均通过，覆盖模式字段选择、解锁无对应报告保持、解锁行下发快照四阵面落盘/重载；Qt5.14/MSVC对Controller及阵面窗口语法检查通过，差异检查通过。未完整构建、现场UDP验收、打包或提交。

- **TAS选项措辞（2026-10-30，沿用V6.5）**：下拉选项“随机跳频”改为“跳频”，校验提示与UI/文件下发日志同步，freqID=90/0x5A及保存恢复映射不变。

- **CC03多面阵回放选择（2026-10-30，暂保持V6.5待用户确认）**：数据存储表右键“离线处理”在default_panel_count>1时弹出四个物理ID0～3复选框，默认全不选，至少选一面才能提交，取消/关闭不发送。OfflineDel追加replayPanel[4]并断言8B，小端偏移4～7为各ID回放0/1标志；DataSet.extendedOffline为本机元数据，发送管理器据其选择多面阵8B或单面阵旧4B负载，不将整个DataSet序列化。恢复正常处理不弹窗，多面阵发送8B开关0/标志全0；单面阵仍4B。未选数据行不发送离线/正常命令，UI/文件人工日志记录ID/模式/回放ID和负载长度，DD04回执逻辑不变。不改CC01四项存储选项及CC02。
- 定向data_save_test通过，覆盖CC03默认全零、黄金字节/27B封装校验、多面阵勾选/空选拒绝/取消不发送、恢复正常与单面阵旧4B兼容；未完整构建、打包或实机回放验收。

- **AA09跟踪数据率调整（2026-10-30，暂保持V6.5待确认）**：新增TrackDataRateControl，负载5B/完整UDP帧24B，小端offset0信息编号AA09、offset2 uint16批号、offset4 uint8 Hz；默认2Hz，上限100，未定义0语义故UI/发送双重校验1～100，批号0～65535，拒绝截断。普通航迹PPI右键菜单和普通/无人机航迹表右键增加“调整跟踪数据率”，复用CustomMessageBox整数输入，取消不发送，不用于TBD/协同批次。Controller GUI入口记录人工提交UI/文件日志，信号传递quint16/quint8到Disp2ResManager沿原资源调度UDP封装下发，无新增周期任务/热路径日志；不把下发请求视为设备ACK，不修改本机显示刷新率。新增track_data_rate_test覆盖5B布局、默认/边界/拒绝及封装黄金字节。

- AA09本轮验证：track_data_rate_test通过；Qt5.14/MSVC对Controller、资源发送管理器、PPI、主界面及新增信号MOC语法检查通过（PPI原有bool比较警告仍在，非本轮修改）。未完整构建、实机UDP验收、打包或提交。

- **调度掉线复位发射显示（2026-10-30，暂保持V6.5待确认）**：MainOverLayOut订阅原始CF01而非五秒故障锁存快照，beamConSta仅0运行正常/2启动成功视为可运行，1/3/4/5及未知状态将本机发射状态复位关闭；显控确认手动关闭处理软件时立即复位。CF01连续15秒不更新也复位，displayConfig.resource_status_timeout_ms可调（5～300秒），GUI父子定时器每秒检查单调时钟。无状态变化不重复日志，恢复状态不自动开启。复位同步m_isTransmitting、TranRecControl.tran及指控radiation状态，复用原红色“开启发射”按钮，不改变接收位、不额外发送AA02，也不伪造BIT/硬件关闭ACK。该失联判断是监控状态不可用，不声称能独立证明资源进程已退出；健康五秒刷新/故障日志门控不变。

- 本轮发射状态复位完成Qt5.14/MSVC主界面语法检查及diff检查；核对手动关闭、CF01原始状态及超时入口共用幂等复位函数，未完整构建、运行GUI/设备掉线测试或提交。

- **36路电源按5秒故障占比统计（2026-10-30，暂保持V6.5待确认）**：本条覆盖所有旧“窗口有一次故障即锁红/全零连续1000帧”描述。每个阵面独立一个5秒非重叠窗口、36路独立故障计数，分母为该阵面窗口内有效DE02帧数，bit0=故障/bit1=正常；每路故障帧数×10严格大于总帧数×9才红，否则绿，正好90%仍绿。全零无特殊防抖，全部按各路比例统计。空窗口不发布新状态（首次未知仍黄、已有显示保持），关闭窗口清零计数、不跨窗滑动。32位固定计数器饱和保护，比较乘法先扩64位，无逐帧缓存/分配。显示摘要同步样本数量和90%规则；聚合位图仍供健康日志及电磁状态快照使用，其他硬件/软件报警窗口语义、日志限频、扫描及位置姿态热路径不变。

- 本轮90%规则验证：health_state_window_test通过，覆盖90%/91%边界、全零单帧100%、各路/各阵面独立、保留位、跨窗口清零和无数据不发布；窗口模型仍小于1KB。Qt5.14/MSVC对Controller、主界面及双方MOC语法检查通过，diff检查通过；未完整构建、实机验收、打包或提交。

**文档结束**

*本文档整合了 DispCtrl 项目全部 34 个 MD 文档的核心知识与最新实现状态，为 AI 开发助手提供准确的项目全局上下文。重大修改后请同步更新版本历史与注意事项章节。*
