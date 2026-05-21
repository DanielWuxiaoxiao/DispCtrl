# DispCtrl 项目（Ship-Radar 分支）- AI Agent 专用说明文档

> **文档用途**: 本文档专为 AI 开发助手（Claude、GitHub Copilot、Cursor 等）提供项目全局上下文，便于快速理解项目架构、代码规范、常见任务与注意事项。
>
> **当前分支**: `ship-radar`（船用导航雷达显示系统）
>
> **最后更新**: 2026-04-01
>
> **📌 重要提示**:
> - **每次重大修改后**，AI Agent 应主动更新本文档的相关章节
> - 本文档仅反映 `ship-radar` 分支内容，不包含 `x576` 分支的功能

---

## 项目概览

**DispCtrl（Ship-Radar）** 是一个基于 **Qt 5.14 + C++17** 的**船用导航雷达显示与控制系统**，采用 SIMRAD 风格界面，实现海上雷达回波实时渲染、航迹显示、地图叠加与伺服控制。SIMRAD风格界面简洁易操作。

### 核心特征
- **PPI 回波渲染**: 4096方位×2048距离单元的扫描缓冲区，支持 SIMRAD/Rainbow/Green 三种色图
- **SIMRAD 风格 UI**: 暖橙色主题、导航信息面板（航速/航向/位置/水深/时间）、折叠式雷达控制面板
- **船用协议**: 自定义二进制协议（显控↔伺服），16B控制帧、变长回波帧、24档量程
- **回波仿真**: 内置 RadarSimulator，可生成逼真的船只、海岸线、岛屿回波用于开发测试
- **基础设施复用**: 基于 x576 分支的 PPI/检测点/航迹/地图/配置系统

### 核心技术栈
| 项目 | 技术 |
|------|------|
| 语言 | C++17 |
| 框架 | Qt 5.14 (Widgets, WebEngine, WebChannel, Network) |
| 构建 | CMake >= 3.16（主要）+ qmake（备用） |
| 编译器 | MSVC 2019+（Windows），GCC 9+（Linux） |
| 网络 | UDP 多线程收发（ThreadedUdpSocket） |
| PPI渲染 | QGraphicsView/Scene + EchoRenderer（4096×2048扫描缓冲） |
| 地图 | Qt WebEngine + 高德地图（2D/3D/卫星/无图） |
| 样式 | SIMRAD 风格暗色主题 QSS（resources/style/darkstyle.qss） |
| 配置 | TOML 格式（config.toml） |

---

## 项目结构

```
DispCtrl/
├── Basic/              # 基础设施
│   ├── Protocol.h/cpp      # 内部协议结构体定义（小端、1B对齐）
│   ├── MarineProtocol.h     # ★ 船用协议结构体（控制帧/回波帧/量程表）
│   ├── ConfigManager.h     # TOML 配置单例（CF_INS宏访问）
│   ├── authmanager.h       # 认证管理
│   ├── bindThread.h        # 线程绑定工具
│   ├── log.h/cpp           # 统一日志系统
│   ├── mathUtil.h          # 数学工具（极坐标转换等）
│   └── DispBasci.h         # 全局常量（颜色暖橙、ScaleHelper等）
│
├── Controller/         # 控制层：业务逻辑与数据分发
│   ├── controller.h/cpp              # 总控制器（CON_INS单例）
│   ├── MarineRadarManager.h/cpp      # ★ 船用雷达UDP管理器（控制帧发送/回波接收）
│   ├── RadarSimulator.h/cpp          # ★ 回波仿真器（船只/海岸线/岛屿模拟）
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
│   ├── echorenderer.h/cpp          # ★ 回波渲染引擎（4096×2048扫描缓冲，三色图）
│   ├── colorbarwidget.h/cpp        # ★ 色阶图例控件
│   ├── ppiview.h/cpp               # PPI视图主容器
│   ├── ppisscene.h/cpp             # PPI场景（含Det/Track/EchoRenderer）
│   ├── polaraxis.h/cpp             # 极坐标轴与刻度绘制
│   ├── polargrid.h/cpp             # 极坐标网格
│   ├── ppivisualsettings.h/cpp/ui  # 右下角设置
│   ├── mousepositioninfo.h/cpp/ui  # 鼠标位置与点迹大小控制
│   ├── scanlayer.h/cpp             # 扫描动画层
│   ├── sectorwidget.h/cpp          # 扇区显示容器
│   ├── sectorscene.h/cpp           # 扇区场景
│   ├── sectorpolargrid.h/cpp       # 扇区极坐标网格
│   ├── rangeazimuthchart.h/cpp     # 距离-方位直角坐标图表
│   ├── rangeazimuthwidget.h/cpp    # 距离-方位显示组件
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
│   ├── mainoverlayout.h/cpp/ui     # 顶层UI（★含船用控制面板/SIMRAD导航面板/PPI覆盖层）
│   ├── azelrangewidget.h/cpp       # 方位/仰角/距离实时显示控件
│   ├── screenrecorderwidget.h/cpp  # 屏幕录制功能控件
│   └── simpleavi.h/cpp             # AVI文件录制实现
│
├── paramWidget/        # 参数配置对话框
│   ├── servocontrol.h/cpp/ui       # 伺服控制
│   ├── batterycontrol.h/cpp/ui     # 电池/波束控制
│   ├── waveandsample.h/cpp/ui      # 波形与采样参数
│   ├── freqcontrolui.h/cpp/ui      # 频率控制
│   ├── sigparamui.h/cpp/ui         # 信号处理参数
│   ├── dataprocessui.h/cpp/ui      # 数据处理参数
│   ├── datasaveui.h/cpp/ui         # 数据存储控制
│   ├── scanrangeui.h/cpp/ui        # 扫描范围配置
│   ├── photoelectricparam.h/cpp/ui # 光电设备参数
│   ├── tranrecvui.h/cpp/ui         # 收发参数
│   ├── tasmodedialog.h/cpp/ui      # TAS工作模式
│   └── twsmodedialog.h/cpp/ui      # TWS工作模式
│
├── cusWidgets/         # 自定义基础控件库
│   ├── cuswindow.h/cpp             # 可拖拽浮动窗口基类
│   ├── detachablewidget.h/cpp      # 可分离/重附加窗口容器
│   ├── customcombobox.h/cpp        # 自定义下拉框
│   ├── custommessagebox.h/cpp      # 自定义消息框
│   ├── customspinbox.h/cpp         # 自定义数值输入框
│   ├── customspinboxstyle.h/cpp    # 数值框样式代理
│   ├── customlinechart.h/cpp       # 通用直角坐标图表基类
│   └── frozentablewidget.h/cpp     # 冻结列表格
│
├── htmls/              # Web地图前端页面（高德地图）
│   ├── index.html / index3d.html / indexNoL.html / indexS.html / black.html
│
├── resources/
│   ├── style/darkstyle.qss  # SIMRAD 风格暗色主题样式表
│   ├── icon/                # 图标资源
│   └── images/
│
├── docs/               # 项目文档
├── config.toml         # 主配置文件（含 [marine.*] 船用段落）
├── CMakeLists.txt      # CMake 构建配置
├── DispCtrl.pro        # Qt Creator qmake 项目文件
└── Claude.md           # 本AI Agent纲领文档
```

---

## 架构设计

### 1. 分层架构
```
[Web/HTML地图层]   Qt WebEngine + 高德地图（htmls/*.html）
      ↑↓
[UI 视图层]        PPIView / SectorWidget / RangeAzimuthWidget / 参数对话框
                   MainOverLayOut（SIMRAD导航面板 + 船用控制面板 + PPI覆盖层）
      ↑↓（Qt信号槽）
[Controller层]     Controller（CON_INS单例）
                   MarineRadarManager（船用协议收发）
                   RadarSimulator（回波仿真）
      ↑↓
[渲染层]           EchoRenderer（4096×2048扫描缓冲 → QGraphicsPixmapItem）
                   DetManager / TrackManager（检测点/航迹图形对象）
      ↑↓
[UDP 网络层]       ThreadedUdpSocket（每路独立接收线程）
                   MarineRadarManager（船用伺服UDP通信）
```

### 2. 核心设计模式
| 模式 | 应用 |
|------|------|
| 单例 | ConfigManager（CF_INS）、RadarDataManager（RADAR_DATA_MGR）、Controller（CON_INS）|
| 观察者 | Qt 信号槽：Manager发射信号 → UI订阅更新 |
| 工厂 | 点对象创建（DetPoint, TrackPoint, TBDPoint）|
| 策略 | 错误处理（ErrorHandler），EchoRenderer色图策略（SIMRAD/Rainbow/Green）|
| 桥接 | ExternalCtrlManager 封装外部链路透传 |

### 3. 船用回波数据流（核心链路）
```
伺服设备 —UDP→ MarineRadarManager::handleEchoDatagram()
    → Controller::marineEchoLine(MarineEchoLine) 信号
    → PPIScene::EchoRenderer::updateEchoLine()     # 扫描缓冲写入+渲染
    → QGraphicsPixmapItem 更新到 PPIScene           # PPI显示回波图像

仿真模式（无真实硬件时）:
RadarSimulator::tick()
    → echoLineGenerated(MarineEchoLine) 信号
    → EchoRenderer::updateEchoLine()                # 同上渲染链路
    → simulatedDetection(PointInfo) 信号
    → DetManager::addDetPoint()                     # 也推送检测点到PPI/扇区/距离-方位图
```

### 4. 控制帧数据流（下行）
```
MainOverLayOut UI操作（量程/增益/海杂/雨杂/干扰/发射）
    → MarineRadarManager::setRange()/setGain()/setTxOn()...
    → MarineRadarManager::sendControl()（周期性或即时发送）
    → UDP 16B 控制帧 → 伺服设备
```

### 5. 线程模型
- **主线程**: 全部 UI 操作与 Qt 事件循环
- **UDP线程**: 每个 ThreadedUdpSocket 独立线程（接收+初步解析）
- **MarineRadarManager**: 内部UDP线程接收回波 + 定时器线程周期发送控制帧
- **RadarSimulator**: 50ms tick 定时器在主线程中驱动仿真
- **原则**: 数据处理在后台，UI更新通过信号槽自动 marshal 到主线程

---

## 船用雷达协议（MarineProtocol.h）

### 控制帧（显控→伺服，16字节）
```cpp
#pragma pack(push, 1)
struct MarineControlFrame {
    uint8_t  head      = 0xA5;        // 帧头
    uint8_t  rangeCode = 8;           // 量程代号 0~23（详见量程表）
    uint8_t  gain      = 0;           // 增益 0=自动, 1~255手动
    uint8_t  interference = 0;        // 干扰抑制 0~3, 0=关
    uint8_t  level     = 0;           // 电平提升 0~3
    uint8_t  seaClutter = 0;          // 海杂波抑制 0=自动, 1~255手动
    uint8_t  rainClutter = 0;         // 雨杂波抑制 0=自动, 1~255手动
    uint8_t  txOn      = 0;           // 发射控制 0=待机, 1=发射
    uint16_t servoSpeed = 24;         // 伺服转速 rpm（小端）
    uint8_t  reserved[4] = {};        // 保留
    uint8_t  checksum  = 0;           // XOR校验（head到reserved）
    uint8_t  tail      = 0x5A;        // 帧尾
};
#pragma pack(pop)
```

### 回波帧（伺服→显控，变长）
```cpp
#pragma pack(push, 1)
struct MarineEchoHeader {
    uint32_t syncWord   = 0x5A5A5A5A; // 同步字
    uint16_t azimuth;                  // 方位步进 0~4095（4096步 = 360°）
    uint16_t rangeCells;               // 距离单元数
    uint8_t  rangeCode;                // 量程代号 0~23
    uint8_t  status;                   // 伺服状态
    uint16_t rpm;                      // 实际转速
    uint8_t  gain;                     // 当前增益
    uint8_t  seaClutter;               // 海杂波设置
    uint8_t  rainClutter;              // 雨杂波设置
    uint8_t  txActive;                 // 发射状态
    uint32_t timestamp;                // 时间戳
    uint16_t checksum;                 // 头校验
    // 之后跟 rangeCells 个 uint8_t 振幅数据
};
#pragma pack(pop)
```

### 量程表（24档）
| 代号 | 显示标签 | 实际距离(m) | 代号 | 显示标签 | 实际距离(m) |
|------|---------|------------|------|---------|------------|
| 0 | 1/16 nm | 115.7 | 12 | 3 nm | 5556 |
| 1 | 1/8 nm | 231.5 | 13 | 4 nm | 7408 |
| 2 | 1/4 nm | 463 | 14 | 6 nm | 11112 |
| 3 | 3/8 nm | 694.5 | 15 | 8 nm | 14816 |
| 4 | 1/2 nm | 926 | 16 | 12 nm | 22224 |
| 5 | 3/4 nm | 1389 | 17 | 16 nm | 29632 |
| 6 | 1 nm | 1852 | 18 | 24 nm | 44448 |
| 7 | 1.5 nm | 2778 | 19 | 36 nm | 66672 |
| 8 | 2 nm | 3704 | 20 | 48 nm | 88896 |
| 9 | 2.5 nm | 4630 | 21 | 64 nm | 118528 |
| 10 | 3 nm | 5556 | 22 | 70 nm | 129640 |
| 11 | 3 nm | 5556 | 23 | 70 nm | 129640 |

**辅助函数**: `marineRangeLabel(code)` → 显示标签, `marineRangeMeters(code)` → 米

### 方位系统
- **4096 步 = 360°**，每步 ≈ 0.088°
- `azimuthToDegrees(azi)`: 4096步 → 度
- `degreesToAzimuth(deg)`: 度 → 4096步

### 数据类型注册
```cpp
Q_DECLARE_METATYPE(MarineEchoLine)
Q_DECLARE_METATYPE(MarineRadarStatus)
// 在 Controller 构造函数中 qRegisterMetaType 注册
```

---

## 回波渲染引擎（EchoRenderer）

### 核心参数
| 参数 | 值 | 说明 |
|------|---|------|
| 方位分辨率 | 4096 步 | 360°全覆盖 |
| 距离分辨率 | 2048 单元 | 可配置（构造函数参数） |
| 缓冲尺寸 | 4096×2048 uint8_t | 存储振幅值 0~255 |
| 渲染目标 | QGraphicsPixmapItem | 直径 = 2×maxRangeCells 像素 |
| 衰减时间 | 10000ms（默认） | 旧数据逐渐淡出 |

### 色图方案（ColorMap 枚举）
| 枚举值 | 风格 | 特点 |
|--------|------|------|
| SIMRAD | SIMRAD Halo 高保真 | 深蓝→青→绿→黄→橙→红→白 8段色阶 |
| Rainbow | 彩虹 | 标准HSV映射 |
| Green | 绿色 | 经典雷达绿色单色 |

### 关键接口
```cpp
void updateEchoLine(const MarineEchoLine& line);  // 写入扫描缓冲并渲染
void setRange(double rangeMeters);                 // 设置当前量程（米）
void setColorMap(ColorMap map);                    // 切换色图
void setDecayTime(int ms);                         // 衰减时间
void clear();                                       // 清除缓冲
QGraphicsPixmapItem* pixmapItem() const;           // 获取渲染结果图形项
```

### 渲染原理
1. `updateEchoLine()` 收到一条方位的振幅数据
2. 写入 `m_sweepBuffer[azimuth][0..rangeCells-1]`
3. 记录 `m_sweepTimestamp[azimuth]` 用于衰减计算
4. 预计算的 `sin/cos` 查找表将极坐标映射到正交像素坐标
5. 应用色图 LUT 将振幅 → ARGB 颜色
6. 超过衰减时间的旧数据自动淡出

---

## 回波仿真器（RadarSimulator）

### 用途
无真实伺服硬件时，生成逼真的合成回波数据用于开发和演示。

### 仿真内容
| 目标类型 | 描述 | 参数 |
|---------|------|------|
| 移动船只 | 匀速运动的点目标 | azimuth, range, speed, heading, arcSpan |
| 海岸线 | 固定弧状大面积回波 | isStatic=true, arcSpan较大 |
| 岛屿 | 固定小面积回波 | isStatic=true, arcSpan较小 |

### 关键接口
```cpp
void start();                                       // 启动仿真
void stop();                                        // 停止仿真
void setRangeMeter(double meters);                  // 设置量程

signals:
    void simulatedDetection(const PointInfo& info);     // 检测点（传统管线）
    void echoLineGenerated(const MarineEchoLine& line); // 回波线（EchoRenderer管线）
```

### 工作方式
- 50ms 定时器驱动 `tick()`
- 模拟 ~24°/s（约4rpm）扫描速度
- 512 距离单元/线
- 每 tick 生成当前扫描角度区间内的所有方位线

---

## MarineRadarManager（船用UDP管理器）

### 职责
- 向伺服设备周期性发送 16B 控制帧（可配置周期，默认200ms）
- 接收并解析伺服回波数据（变长帧）
- 提取伺服状态信息（转速、发射状态、增益、杂波设置等）

### 初始化
```cpp
void init(const QString& localIp, int localPort,
          const QString& servoIp, int servoPort);
```

### 控制接口
```cpp
void sendControl();                           // 立即发送一次控制帧
void setRange(uint8_t code);                  // 量程代号 0~23
void setGain(uint8_t val);                    // 增益 0=自动, 1~255
void setSeaClutter(uint8_t val);              // 海杂波
void setRainClutter(uint8_t val);             // 雨杂波
void setInterference(uint8_t val);            // 干扰抑制
void setLevel(uint8_t val);                   // 电平提升
void setTxOn(bool on);                        // 发射开关
void setServoSpeed(uint16_t rpm);             // 转速
```

### 信号
```cpp
signals:
    void echoLineReceived(const MarineEchoLine& line);       // 一条回波线
    void radarStatusUpdated(const MarineRadarStatus& status); // 状态更新
    void logMessage(const QString& msg);                      // 调试日志
```

### Controller 集成
```cpp
// controller.cpp 初始化
m_marineMgr = new MarineRadarManager(this);
m_marineMgr->init(localIp, echoPort, servoIp, servoPort);
// 信号转发
connect(m_marineMgr, &MarineRadarManager::echoLineReceived,
        this, &Controller::marineEchoLine);
connect(m_marineMgr, &MarineRadarManager::radarStatusUpdated,
        this, &Controller::marineStatusUpdated);
```

---

## 网络配置

### 船用网络（config.toml [marine.network]）
| 参数 | 键名 | 默认值 | 说明 |
|------|------|--------|------|
| 本机IP | local_ip | 192.168.1.100 | 显控接收回波地址 |
| 回波端口 | echo_port | 9000 | 接收回波UDP端口 |
| 伺服IP | servo_ip | 192.168.1.30 | 伺服设备地址 |
| 伺服端口 | servo_port | 9000 | 伺服控制UDP端口 |
| 自动发送周期 | auto_send_ms | 0 | 控制帧发送间隔(ms) |

### 内部网络（保留自x576基础设施）
```
192.168.64.x（内部主系统网段）
├── 192.168.64.3  数据处理/信号处理/资源调度/监控/目标识别
└── 192.168.64.4  显示控制（本系统）

192.168.101.x（光电设备专用网段）
```

### 关键端口（内部 192.168.64 网段，沿用 x576）
| 端口 | 号 | 方向 | 内容 |
|------|---|------|------|
| SIG_2_DISP_PORT1 | 8003 | 信处→显控 | 检测点 |
| DATA_PRO_2_DISP | 8006 | 数处→显控 | DBT航迹 |
| DATA_PRO_2_DISP2 | 8010 | 数处→显控 | TBD航迹 |

---

## 主界面（SIMRAD 风格）

### 布局结构
```
┌─────────────────────────────────────────────────────┐
│ 顶栏 [西电船用] [HU RM]      [模式/亮度/按钮...]    │ ← topBarWidget（橙色底线）
├────────────────────┬────────┬────────────────────────┤
│                    │        │  SIMRAD 导航面板       │
│  PPI 回波显示区    │        │  ├ 航速 SOG (kn)      │
│  ┌ 覆盖层(左上)    │        │  ├ 航向 HDG (°T)      │
│  │ 量程 / 船首向上  │        │  ├ 对地航向 COG       │
│  │ 相对运动 / 发射  │        │  ├ 转头率 TURN        │
│  │ 距标圈 / 西电船用│        │  ├ 位置 POS (经纬度)  │
│  └                 │ 色阶   │  ├ 水深 (m)           │
│                    │ 图例   │  ├ 日期/时间           │
│  ┌ 导航信息(左下)   │        │  └                    │
│  │ 位置经纬度       │        ├────────────────────────┤
│  │ 光标距离/方位     │        │  船用控制面板（折叠）   │
│  └                 │        │  ├ 量程 ComboBox       │
│                    │        │  ├ 增益 Slider         │
│                    │        │  ├ 海杂波 Slider       │
│                    │        │  ├ 雨杂波 Slider       │
│                    │        │  └ 干扰抑制 Slider     │
├────────────────────┴────────┴────────────────────────┤
│ 底栏 [光标信息] [亮度调节]                            │ ← bottomBarWidget（橙色顶线）
└─────────────────────────────────────────────────────┘
```

### PPI 覆盖层（左上角 m_ppiOverlay）
| 元素 | 内容 | 颜色 |
|------|------|------|
| 量程 | `量程 X nm/km` | 绿色 #44ff44 |
| 模式 | `船首向上` | 绿色 |
| 运动 | `相对运动` | 绿色 |
| 发射 | `● 发射开/关` | 绿/红 |
| 距标圈 | `● 距标圈` | 绿色 |
| 型号 | `西电船用` | 灰色 |

### PPI 导航覆盖层（左下角 m_ppiNavOverlay）
- 位置: 经纬度（度°分'格式）
- 光标: 距离(km) + 方位(°T)

### SIMRAD 导航面板（右侧 m_navPanel）
| 数据项 | 字号 | 颜色 | 单位 |
|--------|-----|------|------|
| 航速 SOG | 42px | 橙色 #ff8800 | kn |
| 航向 HDG | 42px | 白色 | °T |
| 对地航向 COG | 38px | 白色 | °T |
| 转头率 TURN | 34px | 白色 | °/s |
| 位置 POS | 20px | 白色 | 度分格式 |
| 水深 | 46px | 绿色 #44ff44 | m |
| 时间 | 28px | 橙色 | HH:mm:ss |
| 日期 | 18px | 橙色 | dd/MM/yyyy |

### 船用控制面板（右下折叠 m_marineCtrlPanel）
| 控件 | 类型 | 范围 | 连接目标 |
|------|------|------|---------|
| 量程 | QComboBox | 24档 | MarineRadarManager::setRange + syncMarineRange |
| 增益 | QSlider | 0~255 | MarineRadarManager::setGain |
| 海杂波 | QSlider | 0~255 | MarineRadarManager::setSeaClutter |
| 雨杂波 | QSlider | 0~255 | MarineRadarManager::setRainClutter |
| 干扰抑制 | QSlider | 0~3 | MarineRadarManager::setInterference |

### 量程同步（syncMarineRange）
量程切换时同步以下组件:
1. `EchoRenderer::setRange(rangeM)` — 回波渲染范围
2. `PolarAxis::setRange(0, rangeM)` — PPI极坐标轴
3. `m_lblOverlayRange` — PPI覆盖层量程标签
4. `MarineRadarManager::setRange(code)` — 控制帧量程代号
5. `RadarSimulator::setRangeMeter(rangeM)` — 仿真器量程

### X576 控件隐藏
`setupMarineControls()` 启动时隐藏以下 X576 按钮:
- btnStartSoftware, btnStopSoftware, btnServoControl
- btnDataStorage, btnTWSMode, btnTASMode
- btnScanRange, btnDataProcess, btnSignalProcess
- btnFreqControl, btnBatteryControl
- rightTabWidget（功能Tab面板）

---

## 样式系统（SIMRAD 主题）

### 主题特征
- **主色调**: 暖橙 `#ff8800`（边框、高亮、标题）
- **背景**: 纯黑/深灰 `#000` / `#0a0a0a` / `#111`
- **文字**: 白色/浅灰（数据值）、橙色（标题/重点）、绿色（状态指示）
- **字体**: 数据值用 `Consolas` 等宽字体，标题用 `Microsoft YaHei`

### setupSimradTheme() 应用规则
```cpp
// PPI 外围
ui->viewWidget: border 2px solid #ff8800, background #000
// 顶栏
ui->topBarWidget: background #111, border-bottom 2px solid #ff8800
ui->TitleLabel: color #ff8800, font-size 18px
// 底栏
ui->bottomBarWidget: background #111, border-top 2px solid #ff8800
// 状态显示
ui->lblDisplayMode: "HU  RM", color #44ff44（绿色）
```

### 全局颜色常量（DispBasci.h）
```cpp
const QColor DET_COLOR = QColor(255, 180, 50);  // 暖橙（检测点/回波主色）
const QColor TRA_COLOR = QColor(255, 128, 0);   // 橙色（航迹）
```

### darkstyle.qss 关键样式
```css
QWidget#topBarWidget { background: #111; border-bottom: 2px solid #ff8800; }
QWidget#bottomBarWidget { background: #111; border-top: 2px solid #ff8800; }
QWidget#rightPanelWidget { background: #0a0a0a; }
#simradNavPanel { background: #0a0a0a; border-left: 2px solid #ff8800; }
```

---

## 配置系统（config.toml）

### 配置访问宏
```cpp
#define CF_INS ConfigManager::getInstance()

// 船用配置
CF_INS.marineNetworkStr("local_ip", "192.168.1.100");
CF_INS.marineNetworkInt("echo_port", 9000);
CF_INS.marineControl("gain", 0);
CF_INS.marineControlBool("tx_on", false);
CF_INS.marineDisplay("colormap", "simrad");
CF_INS.marineDisplayInt("echo_decay_ms", 10000);

// 通用配置
CF_INS.ip("KEY", "default");
CF_INS.port("KEY", default);
CF_INS.polarDispRange("max", 5.0);
CF_INS.displayConfig("max_points", 1000);
```

### 完整配置结构
```toml
[network.ips]
DISP_CTRL_IP = "192.168.64.4"
DATA_PRO_IP  = "192.168.64.3"

[network.ports]
SIG_2_DISP_PORT1 = 8003
DATA_PRO_2_DISP  = 8006
DATA_PRO_2_DISP2 = 8010

[polarDisp.range]
min = 1
max = 5

[map]
mode = "standard"

[displayConfig]
max_points = 1000

# ===== 船用雷达专用 =====

[marine.network]
local_ip     = "192.168.1.100"
echo_port    = 9000
servo_ip     = "192.168.1.30"
servo_port   = 9000
auto_send_ms = 0

[marine.control]
range        = 8        # 量程代号（2nm）
gain         = 0        # 增益 0=自动
interference = 0        # 干扰抑制
level        = 0        # 电平提升
sea_clutter  = 0        # 海杂波 0=自动
rain_clutter = 0        # 雨杂波 0=自动
tx_on        = false    # 发射控制
servo_speed  = 8        # 伺服转速 rpm

[marine.display]
echo_decay_ms = 10000   # 回波衰减时间
colormap      = "simrad" # 色图: simrad/rainbow/green

[params.servo]
# ... 运行时写入

[params.beamcontrol]
# ... 运行时写入
```

---

## 内部协议体系（Protocol.h，沿用 x576）

### 基本约定
- **字节序**: 小端（Little-Endian）
- **对齐**: 1字节对齐（`#pragma pack(1)`）
- **帧头**: 0xFA55FA55 / **帧尾**: 0x55FA55FA

### 分机代号
| 软件 | 代号 |
|------|------|
| 资源调度 | 0xBB01 |
| 信号处理 | 0xBB02 |
| 数据处理 | 0xBB03 |
| **显控（本系统）** | **0xBB04** |
| 目标识别 | 0xBB05 |
| 自启动监控 | 0xBB06 |
| 外部雷控 | 0xBB07 |

### 协议单位注意（已有Bug修复记录）
| 数据类型 | 字段 | 协议单位 | 显控内部单位 | 转换 |
|---------|------|---------|-------------|------|
| 检测点 detInfo | azi/ele | **度** | 度 | 无需转换 |
| DBT航迹 trackInfo | azi/ele | **弧度** | 度 | ×(180/π) |
| TBD航迹 TBDPoint | azi/ele | **弧度** | 度 | ×(180/π) |
| 距离所有类型 | dis | 米 | 米 | 无需转换 |

---

## 代码规范与约定

### 命名规范
| 类别 | 规范 | 示例 |
|------|------|------|
| 类名 | PascalCase | `PPIView`, `EchoRenderer`, `MarineRadarManager` |
| 成员变量 | `m_` + camelCase | `m_sweepBuffer`, `m_marineMgr`, `m_navPanel` |
| 函数名 | camelCase | `updateEchoLine()`, `syncMarineRange()` |
| 常量/宏 | UPPER_SNAKE | `DET_COLOR`, `CF_INS`, `CON_INS` |
| 信号 | camelCase | `marineEchoLine`, `echoLineReceived` |
| 槽函数 | `on` + 描述 | `onAccept()`, `onMaxPointsChanged()` |

### 信号槽（必须使用新式语法）
```cpp
connect(sender, &SenderClass::signal, receiver, &ReceiverClass::slot);
```

### 内存管理原则
- Qt对象优先指定 parent（自动随父对象销毁）
- 非Qt对象使用 `std::unique_ptr`/`std::shared_ptr`
- 禁止在非主线程操作任何 QWidget/QGraphicsItem

---

## 常见开发任务

### 任务1: 修改回波渲染效果
1. `PolarDisp/echorenderer.cpp` — 修改色图 LUT（`buildColorTableXxx`）
2. `PolarDisp/echorenderer.h` — 添加新 ColorMap 枚举值
3. `mainPanel/mainoverlayout.cpp` — `setupColorBar()` 同步色阶图例
4. `config.toml` — `[marine.display] colormap` 添加新选项

### 任务2: 添加新的仿真目标
1. `Controller/RadarSimulator.cpp` — 在构造函数中添加 SimTarget
2. 设置 azimuth/range/speed/heading/arcSpan/isStatic/snr 参数

### 任务3: 修改船用控制UI
1. `mainPanel/mainoverlayout.cpp` — `setupMarineControls()` 中修改
2. 新增 QSlider/QComboBox/按钮 并连接到 `MarineRadarManager` 对应 setter
3. 确保 `syncMarineRange()` 或类似同步函数保持一致性

### 任务4: 添加新协议字段
1. `Basic/MarineProtocol.h` — 修改 MarineControlFrame/MarineEchoHeader 结构体
2. `Controller/MarineRadarManager.cpp` — 修改 `sendControl()`/`handleEchoDatagram()`
3. 更新 UI slider/按钮连接

### 任务5: 修改参数对话框
1. 断开 `QDialogButtonBox` 默认连接，连接 `onAccept()/onCancel()`
2. `onAccept()`: 读取UI → 填充结构体 → 发射信号 → **不关闭窗口**
3. `onCancel()`: 查找 `CusWindow` 父指针 → 关闭

### 任务6: 调试回波显示异常
1. 确认 `config.toml` `[marine.network]` 端口/IP 正确
2. 检查 MarineRadarManager 是否收到回波数据（logMessage信号）
3. 确认 EchoRenderer 的 range 与当前量程一致
4. 检查 PPIScene 中 EchoRenderer 的层级（应在grid之上、DetManager之下）
5. `qDebug()` 打印 MarineEchoLine 的 azimuth/rangeCells 值

---

## 注意事项与已知陷阱

### 0. Git 命令限制（最高优先级）
**严格禁止**: 未经用户明文指定，AI 绝不执行任何 git 命令。

### 1. 不要执行自动编译
AI 只提供代码修改，不执行编译任务，用户自行编译验证。

### 2. 析构函数信号断开
```cpp
~MyWidget() {
    disconnect(CON_INS, nullptr, this, nullptr);
}
```

### 3. 单例→子对象连接的析构
```cpp
// MainOverLayOut::~MainOverLayOut()
if (CON_INS) {
    disconnect(CON_INS, nullptr, this, nullptr);
    if (m_sectorWidget && m_sectorWidget->scene()) {
        disconnect(CON_INS, nullptr, m_sectorWidget->scene()->detManager(), nullptr);
        disconnect(CON_INS, nullptr, m_sectorWidget->scene()->trackManager(), nullptr);
    }
}
```

### 4. DetachableWidget 安全析构
```cpp
~DetachableWidget() {
    m_isDestroying = true;
    if (m_floatingWindow) { m_floatingWindow->close(); delete m_floatingWindow; }
}
```

### 5. 量程同步必须全链路
修改量程时必须同步: EchoRenderer + PolarAxis + 覆盖层标签 + MarineRadarManager + RadarSimulator。遗漏任一环节会导致回波与坐标不匹配。

### 6. MarineEchoLine 方位范围
方位值 0~4095，如果收到 >= 4096 的值应丢弃或取模。EchoRenderer 内部对此做了安全检查。

### 7. 控制帧校验
MarineControlFrame 的 checksum 为 head 到 reserved 所有字节的 XOR。MarineRadarManager::sendControl() 自动计算，外部调用者无需手动填充。

### 8. .ui 文件修改后需重新构建
修改 `.ui` 后需 cmake 重新构建才能生成 `ui_xxx.h`。

### 9. CMakeLists.txt 同步
每新增 `.cpp` 文件，必须同步添加到 `CMakeLists.txt`。

### 10. 色图一致性
修改 EchoRenderer 的色图 LUT 后，必须同步更新 `setupColorBar()` 中的色阶图例，否则图例与回波颜色不一致。

### 11. 仿真器与真实数据
RadarSimulator 的信号同时推送到 DetManager（检测点）和 EchoRenderer（回波），接入真实硬件时应停止仿真器以避免数据混叠。

### 12. ship-radar 当前运行链路
当前 `ship-radar` 默认只启用船用协议链路：`MarineRadarManager -> Controller::marineEchoLine -> EchoRenderer`。
旧 x576 runtime manager、PPI det/track/TBD 信号、扇区/B显/缩放辅助面板、RadarSimulator 测试点迹的运行接线已从主流程删除。后续需要恢复仿真或 x576 辅助显示时，必须显式确认测试目标，避免真实协议调试时混入测试点迹或隐藏数据流。
PPIScene 当前不创建旧 `DetManager` / `TrackManager` 图元管理器。船用回波显示采用增量写入 `EchoRenderer` 图像并按定时器刷新 pixmap，避免每帧全量重绘 4096 条方位线。

---

## 快速定位代码

### 功能 → 关键文件
| 功能 | 关键文件 |
|------|---------|
| 回波渲染 | `PolarDisp/echorenderer.cpp` |
| 色阶图例 | `PolarDisp/colorbarwidget.cpp` |
| 回波仿真 | `Controller/RadarSimulator.cpp` |
| 船用协议 | `Basic/MarineProtocol.h` |
| 船用UDP管理 | `Controller/MarineRadarManager.cpp` |
| 船用UI控制面板 | `mainPanel/mainoverlayout.cpp`（`setupMarineControls()`）|
| SIMRAD导航面板 | `mainPanel/mainoverlayout.cpp`（`setupSimradNavPanel()`）|
| PPI覆盖层 | `mainPanel/mainoverlayout.cpp`（`setupPPIOverlay()`）|
| SIMRAD主题 | `mainPanel/mainoverlayout.cpp`（`setupSimradTheme()`）|
| 量程同步 | `mainPanel/mainoverlayout.cpp`（`syncMarineRange()`）|
| PPI主视图 | `PolarDisp/ppiview.cpp`, `ppisscene.cpp` |
| 扇区显示 | `PolarDisp/sectorwidget.cpp`, `sectorscene.cpp` |
| 距离-方位图 | `PolarDisp/rangeazimuthchart.cpp` |
| 检测点管理 | `PointManager/detmanager.cpp` |
| 航迹管理 | `PointManager/trackmanager.cpp` |
| 全局颜色常量 | `Basic/DispBasci.h` |
| 协议结构体 | `Basic/Protocol.h`（内部）/ `Basic/MarineProtocol.h`（船用）|
| 配置管理 | `Basic/ConfigManager.h` |
| 暗色主题样式 | `resources/style/darkstyle.qss` |
| 主面板布局 | `mainPanel/mainoverlayout.cpp` |
| UDP收发 | `UDP/threadudpsocket.cpp` |

### 常用搜索关键词
```
MarineRadarManager       # 船用UDP管理入口
EchoRenderer            # 回波渲染
RadarSimulator          # 仿真器
MarineControlFrame      # 控制帧结构体
MarineEchoLine          # 回波数据结构
marineEchoLine          # Controller 回波信号
syncMarineRange         # 量程同步
setupMarineControls     # 船用UI初始化
setupSimradNavPanel     # SIMRAD导航面板
setupSimradTheme        # SIMRAD主题
setupPPIOverlay         # PPI覆盖层
setupColorBar           # 色阶图例
updateEchoLine          # 回波写入渲染器
buildColorTable         # 色图LUT构建
marineRangeMeters       # 量程代号→米
addDetPoint             # 检测点添加
addTrackPoint           # 航迹添加
CF_INS.marine           # 船用配置读取
CON_INS                 # Controller单例
m_marineMgr             # MarineRadarManager实例
```

---

## AI Agent 工作建议

### 修改原则
1. **读先于写**: 先读相关文件，理解上下文再修改
2. **保持风格**: `m_`前缀、新式信号槽、Doxygen注释
3. **最小范围**: 只改必要部分，不引入无关重构
4. **量程同步**: 涉及量程变更时检查 `syncMarineRange` 全链路
5. **色图一致**: 修改渲染色图必须同步色阶图例
6. **检查包含**: 新增调用后确认 `#include` 完整
7. **同步构建**: 新增 `.cpp` 后提醒用户更新 `CMakeLists.txt`

### 回答策略
1. 用 `grep_search`/`semantic_search` 精准定位代码
2. 说明"为什么"，不只是"怎么做"
3. 给出含上下文的完整代码片段
4. 主动提醒本文档中相关的已知陷阱

---

## 版本历史（ship-radar 分支）

### 初始版本（基于 x576 分支 fork）
- 保留 x576 基础设施：PPI/检测点/航迹/地图/配置/UDP/参数对话框
- 保留内部协议体系（Protocol.h）

### 船用雷达核心（2026-03~04）
- **MarineProtocol.h**: 定义船用协议（16B控制帧/22B回波头/24档量程表/4096方位步进）
- **MarineRadarManager**: UDP管理器（控制帧周期发送、回波帧接收解析）
- **EchoRenderer**: PPI回波渲染引擎（4096×2048扫描缓冲、SIMRAD/Rainbow/Green三色图、衰减/余辉）
- **RadarSimulator**: 合成回波仿真器（移动船只/海岸线/岛屿，~4rpm扫描）
- **ColorBarWidget**: 色阶图例控件
- **SIMRAD 风格 UI**:
  - 导航面板（航速/航向/对地航向/转头率/位置/水深/日期时间）
  - 折叠式控制面板（量程/增益/海杂波/雨杂波/干扰抑制滑块）
  - PPI覆盖层（量程/船首向上/相对运动/发射状态/距标圈/西电船用）
  - PPI导航覆盖层（位置经纬度/光标信息）
- **颜色方案**: DET_COLOR 改为暖橙 QColor(255,180,50)
- **darkstyle.qss**: 重写为 SIMRAD 主题（橙色边框/深黑背景）
- **config.toml**: 新增 `[marine.network]`/`[marine.control]`/`[marine.display]` 段落
- **X576 按钮隐藏**: setupMarineControls() 隐藏所有不适用的 X576 功能按钮
- **CMakeLists.txt**: 新增 echorenderer/colorbarwidget/RadarSimulator/MarineRadarManager

---

**文档结束**

*本文档反映 DispCtrl 项目 ship-radar 分支的完整架构与实现状态。重大修改后请同步更新版本历史与注意事项章节。*
