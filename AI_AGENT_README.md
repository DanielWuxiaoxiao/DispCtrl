# DispCtrl 项目 - AI Agent 专用说明文档

> **文档用途**: 本文档专为 AI 开发助手（Claude、GitHub Copilot、Cursor 等）提供项目全局上下文，便于快速理解项目架构、代码规范、常见任务与注意事项。
>
> **最后更新**: 2026-01-19
>
> **📌 重要提示**:
> - **每次重大修改后**，AI Agent 应主动更新本文档的相关章节（版本历史、注意事项等）
> - **修改位置**: 在对应章节直接更新，保持文档与代码同步
> - **更新内容**: 新功能、Bug修复、编译错误解决方案、新的注意事项等

---

## 项目概览

**DispCtrl** 是一个基于 **Qt 5.14 + C++17** 的雷达显示与控制系统，用于实时展示雷达数据（检测点、航迹、目标）、地图叠加、参数配置与外部链路控制。

### 核心技术栈
- **语言**: C++17
- **框架**: Qt 5.14 (Widgets, WebEngine, WebChannel, Network)
- **构建系统**: CMake (主要) + qmake (备用)
- **编译器**: MSVC 2019+ (Windows), GCC 9+ (Linux)
- **网络**: UDP 多线程收发
- **渲染**: QGraphicsView/Scene (PPI极坐标)、Qt WebEngine (地图)
- **样式**: 自定义暗色主题 QSS (`resources/style/darkstyle.qss`)
- **配置**: TOML 格式 (`config.toml`)

---

## 项目结构（关键目录）

```
DispCtrl/
├── Basic/              # 基础设施：协议定义、配置管理、日志、数学工具
│   ├── Protocol.h/cpp  # 协议结构体与解析（小端、1B对齐）
│   ├── ConfigManager.h # TOML 配置单例
│   ├── log.h/cpp       # 日志系统
│   └── DispBasci.h     # 常量定义（颜色、范围等）
│
├── Controller/         # 控制层：数据分发与业务逻辑
│   ├── controller.h/cpp              # 总控制器，协调所有Manager
│   ├── RadarDataManager.h/cpp        # 统一数据缓存与分发中心
│   ├── data2dispmanager.h/cpp        # 数据处理 -> 显示
│   ├── sig2dispmanager.h/cpp         # 信号处理 -> 显示
│   ├── mon2dispmanager.h/cpp         # 监控 -> 显示
│   ├── targetdispmanager.h/cpp       # 目标分类 -> 显示
│   ├── ExternalCtrlManager.h/cpp     # 外部雷控/伺服链路（512B/32B）
│   └── ErrorHandler.h/cpp            # 错误处理策略
│
├── PolarDisp/          # PPI极坐标显示模块
│   ├── ppiview.h/cpp           # PPI视图（主显示容器）
│   ├── ppisscene.h/cpp         # PPI场景管理
│   ├── polaraxis.h/cpp         # 极坐标轴与刻度
│   ├── polargrid.h/cpp         # 极坐标网格
│   ├── ppivisualsettings.h/cpp # 右下角设置控件（距离、地图、监测点数）
│   ├── scanlayer.h/cpp         # 扫描动画层
│   ├── sectorwidget.h/cpp      # 扇区显示
│   └── zoomview.h/cpp          # 缩略图
│
├── PointManager/       # 检测点与航迹管理
│   ├── detmanager.h/cpp        # 检测点管理（含数量限制）
│   ├── trackmanager.h/cpp      # 航迹管理
│   ├── point.h/cpp             # 点对象基类
│   └── sectorXXX.h/cpp         # 扇区版本的点管理器
│
├── UDP/                # UDP 网络通信
│   └── threadudpsocket.h/cpp   # 多线程UDP Socket封装
│
├── mapDisp/            # WebEngine 地图集成
│   └── mapprox.h/cpp           # 地图代理，Qt<->JS通信
│
├── mainPanel/          # 主界面布局
│   └── mainoverlayout.h/cpp/ui # 顶层UI组织
│
├── paramWidget/        # 参数配置对话框
│   ├── servocontrol.h/cpp/ui       # 伺服控制
│   ├── batterycontrol.h/cpp/ui     # 电池控制
│   ├── sigparamui.h/cpp/ui         # 信号参数
│   ├── dataprocessui.h/cpp/ui      # 数据处理参数
│   └── ... (其他参数对话框)
│
├── cusWidgets/         # 自定义控件
│   ├── cuswindow.h/cpp             # 可拖拽窗口基类
│   ├── detachablewidget.h/cpp      # 可分离窗口
│   ├── customcombobox.h/cpp        # 自定义下拉框
│   └── custommessagebox.h/cpp      # 自定义消息框
│
├── htmls/              # Web地图前端页面
│   ├── index.html      # 标准2D地图
│   ├── index3d.html    # 3D地图
│   ├── indexNoL.html   # 路网图
│   ├── indexS.html     # 卫星图
│   └── black.html      # 无地图（纯黑）
│
├── resources/          # 资源文件
│   ├── style/darkstyle.qss  # 暗色主题样式表
│   ├── icon/                # 图标资源
│   └── images/              # 图片资源
│
├── docs/               # 项目文档
│   ├── book/           # 完整技术专著（15章）
│   ├── internal_protocol.md      # 内部协议详细说明
│   ├── external_protocol_notes.md # 外部协议笔记
│   └── changes_2025-12-25.md     # 重要变更记录
│
├── config.toml         # 主配置文件
├── CMakeLists.txt      # CMake 构建配置
└── DispCtrl.pro        # Qt Creator 项目文件
```

---

## 架构设计原则

### 1. 分层架构
```
[UI层] ← [Controller层] ← [Manager层] ← [网络层]
  ↓          ↓              ↓            ↓
视图组件   总控/桥接     数据缓存/分发   UDP收发
```

### 2. 核心设计模式
- **单例模式**: `ConfigManager`, `RadarDataManager`
- **观察者模式**: Qt 信号槽通信，实现模块解耦
- **工厂模式**: 点对象创建（DetPoint, TrackPoint）
- **策略模式**: 错误处理（ErrorHandler）

### 3. 数据流向
```
UDP接收 → 协议解析 → RadarDataManager缓存
  → 各Manager订阅 → 信号发射 → UI更新
```

### 4. 线程模型
- **主线程**: 所有 UI 操作与事件处理
- **UDP线程**: 每个 ThreadedUdpSocket 独立线程接收数据
- **原则**: 数据处理在后台，UI更新在主线程（通过信号槽跨线程）

---

## 关键特性与实现

### 1. 检测点数量限制与显示清除（2026-01-19）
**问题**: 长时间运行时，检测点无限增长导致内存溢出和卡死。

**解决方案**:

- 在 `PPIVisualSettings` 右下角添加"检测点"输入框（默认1000）
- `DetManager` 实现 FIFO 队列，超出限制自动删除最旧的点
- 用户可通过界面动态调整限制（100-1000000范围）
- 添加"显清"按钮，一键清除P显所有检测点和航迹点数据

**相关文件**:
- `PolarDisp/ppivisualsettings.h/cpp/ui` - UI控件与信号定义
- `PointManager/detmanager.h/cpp` - 检测点FIFO管理
- `PointManager/trackmanager.h/cpp` - 航迹点管理
- `PolarDisp/ppiview.h/cpp` - 信号响应与数据清除
- `cusWidgets/custommessagebox.h/cpp` - 确认对话框

**关键代码**:
```cpp
// DetManager::addDetPoint() 中
while (mNodes.size() > m_maxPoints) {
    DetNode& oldNode = mNodes.first();
    if (oldNode.point) {
        mScene->removeItem(oldNode.point);
        delete oldNode.point;
    }
    mNodes.removeFirst();
}

// PPIView::onClearDisplayRequested() 中
if (m_scene->det()) {
    m_scene->det()->clear();  // 清除检测点
}
if (m_scene->tra()) {
    m_scene->tra()->clear();  // 清除航迹点
}
```

**使用方式**:
1. 调整检测点数量限制：在"检测点"输入框输入数值，按回车应用
2. 清除显示数据：点击"显清"按钮 → 确认对话框 → 清除所有检测点和航迹点

### 2. 外部雷控链路（2025-12-25）
- 支持 512B 系统控制表、64B 控制回执
- 支持 32B 伺服控制、32B 伺服回执
- 通过 `ExternalCtrlManager` 封装发送/接收逻辑
- 配置项: `EXT_SYSCTRL_SRC`, `EXT_SYSCTRL_DST`, `EXT_ACK_DST` 等

### 3. 地图集成
- 使用 Qt WebEngine 加载 `htmls/` 中的高德地图页面
- 通过 `QWebChannel` 实现 Qt ↔ JavaScript 双向通信
- 支持地图类型动态切换（无图/路网/标准/卫星/3D）
- 雷达中心、范围、姿态参数实时同步

### 4. PPI 显示优化
- 使用 `QGraphicsView/Scene` 实现分层渲染
- 极坐标网格、扫描层、检测点、航迹分层管理
- 橡皮筋缩放、测距功能、鼠标位置实时显示
- 支持窗口分离（DetachableWidget）

### 5. 参数对话框统一规范
**按钮行为标准** (2026-01-15修复):
- 所有参数对话框的"确定下发/取消"按钮必须与 `ServoControl` 保持一致
- 断开默认 accept/reject 连接，使用自定义槽函数
- "确定下发"不关闭窗口（便于连续下发）
- "取消"向上查找 `CusWindow` 父窗口并关闭

**相关文件**:
- `paramWidget/servocontrol.cpp` (参考实现)
- `paramWidget/batterycontrol.cpp`
- `paramWidget/tranrecvui.cpp`
- 等所有参数对话框

### 6. 参数保存功能（2026-01-21）
**功能**: 允许用户将参数窗口的当前配置保存到 `config.toml`，下次启动自动恢复。

**实现要点**:
1. **保存按钮**: 所有参数窗口都添加"保存参数"按钮（位于对话框按钮之前）
2. **确认对话框**: 点击后弹出 `CustomMessageBox::showConfirm()` 确认
3. **保存流程**:
   - 用户确认 → 调用 `CF_INS.saveXxxParam()` → 调用 `CF_INS.save()` 写入文件
   - 成功后显示 `CustomMessageBox::showInfo()` 提示
4. **加载默认值**: 构造函数中从 `CF_INS.xxxParam()` 读取，如无则使用硬编码默认值
5. **配置存储**: 所有参数保存在 `config.toml` 的 `[params.*]` 段落

**已支持的参数类型**:
- `params.servo` - 伺服控制（cmd, speed, az）
- `params.scanrange` - 扫描范围（workMode）
- `params.beamcontrol` - 波形控制（freqID, type, azi/ele参数，3个波形配置）
- `params.sigpro` - 信号处理（噪声、门限、CFAR等16个参数）
- `params.datapro` - 数据处理（航迹起始窗口、门限、凝聚门等15个参数）
- `params.datasave` - 数据保存（saveSwitch, dataID）

**实现模式**（以 servocontrol 为例）:
```cpp
// .h 文件
private slots:
    void onSaveToConfig();

// .cpp 构造函数
ui->field->setValue(CF_INS.xxxParam(默认值));
connect(ui->saveButton, &QPushButton::clicked, this, &类名::onSaveToConfig);

// .cpp 实现
void 类名::onSaveToConfig() {
    if (!CustomMessageBox::showConfirm(this, tr("确认保存"),
                                       tr("是否将当前参数保存到配置文件？"))) {
        return;
    }

    // 获取UI参数
    类型 value = ui->field->获取值();

    // 保存到ConfigManager
    CF_INS.saveXxxParam(value, ...);

    // 写入文件
    if (CF_INS.save()) {
        CustomMessageBox::showInfo(this, tr("保存成功"),
                                  tr("参数已保存！\n下次启动将自动加载。"));
    } else {
        CustomMessageBox::showWarning(this, tr("保存失败"),
                                     tr("无法保存配置文件，请检查文件权限。"));
    }
}

// .ui 文件（在 QDialogButtonBox 之前）
<item>
 <widget class="QPushButton" name="saveButton">
  <property name="text">
   <string>保存参数</string>
  </property>
 </widget>
</item>
```

**相关文件**:
- `Basic/ConfigManager.h` - 配置管理器（保存/读取方法）
- `paramWidget/servocontrol.*` - 完整实现示例
- `docs/parameter_save_feature.md` - 详细实现指南
- `config.toml` - 参数存储文件

**注意事项**:
- 保存前必须弹出确认对话框，避免误操作
- 按钮文本统一为"保存参数"（不是"保存参数到配置文件"）
- 使用 `CustomMessageBox` 系列方法，保持UI风格一致
- 参数单位转换：保存的是协议值（如角度*100），不是UI显示值

---

## 代码规范与约定

### 1. 命名规范
- **类名**: PascalCase (如 `PPIView`, `DetManager`)
- **成员变量**: m_ 前缀 + camelCase (如 `m_scene`, `m_maxPoints`)
- **函数名**: camelCase (如 `addDetPoint()`, `onMaxPointsChanged()`)
- **常量**: UPPER_SNAKE_CASE (如 `MAX_RANGE`, `DET_COLOR`)
- **信号**: camelCase, 描述性名称 (如 `maxPointsChanged`, `radarCenterChanged`)
- **槽函数**: on + 信号源 + 动作 (如 `onMaxPointsChanged`, `onAccept`)

### 2. 注释规范
使用 Doxygen 风格注释：
```cpp
/**
 * @brief 简短描述
 * @param paramName 参数说明
 * @return 返回值说明
 * @details 详细说明
 *          - 要点1
 *          - 要点2
 */
void functionName(int paramName);
```

### 3. 内存管理
- **Qt 对象树**: 优先使用 Qt 父子关系管理内存
- **原则**: `new` 出来的对象必须指定 parent 或在析构函数中 `delete`
- **智能指针**: 非 Qt 对象可使用 `std::unique_ptr`, `std::shared_ptr`
- **容器**: 优先使用 Qt 容器 (`QVector`, `QList`, `QMap`)

### 4. 信号槽连接
```cpp
// 推荐：新式语法（类型安全）
connect(sender, &SenderClass::signalName,
        receiver, &ReceiverClass::slotName);

// 避免：旧式语法（字符串，无类型检查）
connect(sender, SIGNAL(signalName(int)),
        receiver, SLOT(slotName(int)));
```

### 5. 跨线程通信
- 信号槽默认是 `Qt::AutoConnection`（跨线程自动排队）
- UI 更新必须在主线程，数据处理可在后台线程
- 避免在非主线程直接操作 UI 组件

---

## 配置系统 (config.toml)

### 配置文件结构
```toml
[network.ips]
RADAR_CTRL_IP = "192.168.1.16"
SCHED_IP = "192.168.1.5"
# ... 更多IP配置

[network.ports]
SIG_2_DISP_PORT1 = 5001  # 检测点
DATA_PRO_2_DISP = 5003   # 航迹
EXT_SYSCTRL_SRC = 6001   # 外部系统控制发送
# ... 更多端口配置

[range]
max = 5.0    # 最大显示距离（公里）
min = 0.0    # 最小显示距离

[mapType]
default_type = 1  # 0=无图, 1=路网, 2=标准, 3=卫星

[displayConfig]
max_points = 10000  # 最大监测点数量
```

### 配置读取
```cpp
// 单例访问
#define CF_INS ConfigManager::getInstance()

// 读取配置
QString ip = CF_INS.radarIP("RADAR_CTRL_IP", "192.168.1.16");
int port = CF_INS.port("SIG_2_DISP_PORT1", 5001);
double maxRange = CF_INS.range("max", 5.0);
int maxPoints = CF_INS.displayConfig("max_points", 10000);
```

---

## 协议定义 (Basic/Protocol.h)

### 协议特点
- **字节序**: 小端 (Little-Endian)
- **对齐**: 1字节对齐 (`#pragma pack(1)`)
- **帧结构**: 帧头(4B) + 消息ID(2B) + 数据长度(2B) + 数据体 + 帧尾(4B)

### 常见消息类型
```cpp
// 检测点上报 (168B)
struct DetReport {
    unsigned int batch;      // 批号
    PointInfo points[10];    // 10个点信息
};

// 航迹上报 (128B)
struct TrackReport {
    TrackInfo trackList[8];  // 8条航迹
};

// 外部系统控制 (512B)
struct ExternalSystemControl512 {
    char data[512];          // 控制表原始数据
};

// 外部伺服控制 (32B)
struct ExternalServoControl32 {
    char cmd;                // 命令码
    char speed;              // 速度
    short azimuth;           // 方位角
    // ...
};
```

### 解析流程
```cpp
// UDP接收 → 帧头/尾校验 → 消息ID识别 → memcpy到结构体 → 发射信号
void handleDatagram(const QByteArray& data) {
    // 1. 校验帧头/尾
    // 2. 解析消息ID
    // 3. 根据ID创建对应结构体
    DetReport report;
    memcpy(&report, data.data() + 8, sizeof(DetReport));
    // 4. 发射信号
    emit detectionReceived(report);
}
```

---

## 常见开发任务

### 任务1: 添加新的数据类型显示
**步骤**:
1. 在 `Basic/Protocol.h` 中定义新的消息结构体
2. 在 `config.toml` 中添加对应的端口配置
3. 创建新的 Manager (如 `NewDataManager`)，继承自 `QObject`
4. 在 Manager 中监听 UDP 信号，解析数据
5. 在 `RadarDataManager` 中注册新数据类型
6. 在 UI 层（View/Widget）订阅 Manager 的信号并更新显示
7. 在 `Controller` 中初始化 Manager 并连接信号

### 任务2: 修改参数对话框
**步骤**:
1. 找到对应的 `.ui` 文件，在 Qt Designer 中修改界面
2. 在 `.h` 文件中声明槽函数
3. 在 `.cpp` 构造函数中断开默认 accept/reject:
   ```cpp
   disconnect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
   disconnect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
   ```
4. 连接自定义槽函数:
   ```cpp
   connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
           this, &YourDialog::onAccept);
   connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
           this, &YourDialog::onCancel);
   ```
5. 实现 `onAccept()` (验证、打包、发射信号) 和 `onCancel()` (查找并关闭父窗口)

### 任务3: 添加新的配置项
**步骤**:
1. 在 `config.toml` 中添加配置项
2. 在 `Basic/ConfigManager.h` 中添加读取方法（如果需要新类型）
3. 在代码中使用 `CF_INS.xxx("key", defaultValue)` 读取
4. 更新 `config_documentation.md` 说明新配置项

### 任务4: 优化性能/修复卡顿
**常见原因**:
1. **数据量过大**: 检测点/航迹无限增长 → 添加数量限制
2. **频繁重绘**: Scene 刷新过于频繁 → 使用批量更新 `setUpdatesEnabled(false)`
3. **阻塞主线程**: 耗时操作在主线程 → 移到后台线程
4. **内存泄漏**: 对象未释放 → 检查 parent 设置和析构函数

**调试方法**:
- 使用 `qDebug()` 打印关键路径耗时
- 使用 Qt Creator 的 Profiler 分析性能
- 检查 `get_errors` 工具查看警告/错误
- 查看日志文件 (`Basic/log.cpp` 输出)

---

## 注意事项与陷阱

### 1. 不要执行自动编译
**重要**: 用户明确要求 AI 不要执行编译任务，只提供代码修改。
- ❌ 不要调用 `run_task` 或 `cmake --build`
- ✅ 只修改代码文件，让用户自己编译

### 2. QSS 样式优先级问题
**现象**: 参数对话框按钮样式不一致。
**原因**: `darkstyle.qss` 中有特定对话框的 `QDialogButtonBox QPushButton` 规则覆盖全局样式。
**解决**: 删除特定对话框的按钮样式规则，让所有对话框继承全局 `QPushButton` 样式。

### 3. 析构函数中的信号断开
**问题**: 对象析构时，如果有槽函数连接到即将销毁的对象，会导致崩溃。
**解决**: 在析构函数中断开所有相关信号:
```cpp
~MyClass() {
    disconnect(&SomeManager, nullptr, this, nullptr);
}
```

### 3.1 子对象信号连接的析构处理（2026-01-22 修复）
**问题**: 在 `MainOverLayOut` 中，`CON_INS` 的信号连接到了子对象 `m_sectorWidget->scene()->detManager()` 和 `trackManager()`。程序退出时，子对象随父对象被自动删除，但 `CON_INS`（单例）仍持有对已删除对象的连接，导致 `_CrtIsValidHeapPointer` 断言失败。

**解决**: 在析构函数中显式断开连接到子对象的信号:
```cpp
MainOverLayOut::~MainOverLayOut() {
    if (CON_INS) {
        disconnect(CON_INS, nullptr, this, nullptr);
        // 关键：断开连接到子对象的信号
        if (m_sectorWidget && m_sectorWidget->scene()) {
            disconnect(CON_INS, nullptr, m_sectorWidget->scene()->detManager(), nullptr);
            disconnect(CON_INS, nullptr, m_sectorWidget->scene()->trackManager(), nullptr);
        }
    }
    disconnect(&RADAR_DATA_MGR, nullptr, this, nullptr);
    delete ui;
}
```

**规则**: 当将单例/全局对象的信号连接到子对象时，必须在父对象析构函数中显式断开这些连接。

### 4. DetachableWidget 的安全析构
**问题**: 可分离窗口析构时，浮动窗口可能触发 reattach 导致崩溃。
**解决**: 添加析构标志，阻止析构期间的 reattach:
```cpp
~DetachableWidget() {
    m_isDestroying = true;
    if (m_floatingWindow) {
        m_floatingWindow->close();;
        delete m_floatingWindow;
    }
}
```

### 5. UI 文件修改后需要重新 qmake/cmake
**问题**: 修改 `.ui` 文件后，编译器报错找不到 `ui_xxx.h`。
**解决**:
- CMake: 重新运行 `cmake --build`（用户自己做）
- qmake: 运行 `qmake` 然后 `make`

### 6. 跨平台路径分隔符
**问题**: Windows 使用 `\`，Linux 使用 `/`。
**解决**: 使用 `QDir::separator()` 或 Qt 的路径处理函数 (`QDir`, `QFileInfo`)。

### 7. 配置方法缺失导致编译错误
**问题**: 使用了未定义的配置读取方法（如 `CF_INS.displayConfig()`）。
**解决**: 在 `Basic/ConfigManager.h` 中添加对应的配置访问方法：
```cpp
int displayConfig(const QString& key, int def = 10000) const {
    return getValue("displayConfig." + key, def).toInt();
}
```

### 8. 头文件缺失导致类型未定义
**问题**: 使用了前向声明的类的成员方法，但未包含完整头文件。
**现象**: 编译器报错"使用了未定义类型"（如 `DetManager`）。
**解决**: 在 `.cpp` 文件中包含完整的头文件：
```cpp
#include "../PointManager/detmanager.h"
```
**注意**: 头文件中使用前向声明，`.cpp` 中使用完整 `#include`。

---

## 文档资源

### 项目内文档
- **完整技术专著**: `docs/book/` (15章，涵盖需求、架构、协议、模块、部署等)
- **协议详细说明**: `docs/internal_protocol.md`
- **配置说明**: `config_documentation.md`
- **重要变更**: `docs/changes_2025-12-25.md`

### 关键章节速查
- **第3章**: 架构设计原则与分层结构
- **第4章**: 协议详解（帧格式、消息ID、结构体定义）
- **第5章**: 核心模块详解（Controller, Manager, PolarDisp 等）
- **第6章**: 数据流与处理流程
- **第7章**: 配置系统说明
- **第13章**: 代码规范与贡献流程

---

## 快速定位代码

### 功能模块 → 文件映射
| 功能 | 关键文件 |
|------|---------|
| **PPI 显示** | `PolarDisp/ppiview.cpp`, `ppisscene.cpp` |
| **检测点管理** | `PointManager/detmanager.cpp` |
| **航迹管理** | `PointManager/trackmanager.cpp` |
| **数据分发** | `Controller/RadarDataManager.cpp` |
| **外部链路** | `Controller/ExternalCtrlManager.cpp` |
| **UDP 通信** | `UDP/threadudpsocket.cpp` |
| **地图集成** | `mapDisp/mapprox.cpp` |
| **参数对话框** | `paramWidget/*.cpp` |
| **配置管理** | `Basic/ConfigManager.cpp` |
| **协议定义** | `Basic/Protocol.h` |
| **日志系统** | `Basic/log.cpp` |
| **主界面** | `mainPanel/mainoverlayout.cpp` |

### 常见搜索关键词
- 信号定义: `signals:`
- 槽函数: `slots:` 或 `void onXxx(`
- 配置读取: `CF_INS.`
- 日志输出: `LOG_INFO`, `LOG_ERROR`
- UDP 接收: `handleDatagram`, `readPendingDatagrams`
- 坐标转换: `polarToPixel`, `sceneToPolar`

---

## AI Agent 工作建议

### 代码修改原则
1. **理解上下文**: 优先阅读相关头文件和注释
2. **保持一致**: 遵循现有代码风格和命名规范
3. **最小修改**: 只改必要的部分，避免大范围重构
4. **注释完整**: 添加 Doxygen 风格注释说明修改意图
5. **考虑影响**: 修改前搜索相关调用点，评估影响范围
6. **验证编译**: 修改后检查是否缺少头文件或配置方法

### 回答问题策略
1. **定位精准**: 使用 `grep_search`, `semantic_search` 快速定位相关代码
2. **引用代码**: 回答时引用具体文件和行号
3. **解释原理**: 说明"为什么"这样设计，不只是"怎么做"
4. **提供示例**: 如果涉及修改，给出完整的代码片段
5. **注意陷阱**: 主动提醒已知的坑点和注意事项

### 任务执行建议
1. **分步实施**: 复杂任务拆分为多个小步骤
2. **验证逻辑**: 修改后检查编译错误（使用 `get_errors`）
3. **测试建议**: 提示用户需要测试的场景
4. **文档同步**: 重大修改时建议更新相关文档

---

## 版本历史摘要

### v5.0 (2025-12-25)
- 新增外部雷控链路（512B 系统控制、32B 伺服控制）
- 添加 `ExternalCtrlManager` 封装外部链路逻辑
- 更新协议文档 `docs/internal_protocol.md`

### v5.1 (2026-01-15)
- 修复参数对话框按钮样式不一致问题
- 统一所有对话框按钮行为（参考 `ServoControl`）
- 移除 QSS 中特定对话框的按钮样式覆盖

### v5.2 (2026-01-19) - 当前版本
- **新增检测点数量限制功能**，解决长时间运行卡死问题
- `PPIVisualSettings` 添加"检测点"输入框（默认1000）
- `DetManager` 实现 FIFO 队列，自动删除超出限制的旧数据
- 用户可通过界面动态调整限制（100-1000000）
- **新增P显清除功能**，一键清除所有检测点和航迹点
- 在"测距"按钮旁添加"显清"按钮，点击后弹出确认对话框
- 清除操作不影响后续新数据的添加
- **修复编译错误**：
  - 添加 `ConfigManager::displayConfig()` 方法支持显示配置读取
  - 添加 `ppiview.cpp` 中缺失的 `detmanager.h` 头文件包含
- **文档改进**：创建 AI_AGENT_README.md 为 AI 助手提供项目全局上下文

  - 在 `ppiview.cpp` 中添加 `detmanager.h` 头文件引用
  - 确保 `DetManager::setMaxPoints()` 方法可正常调用

---

## 联系与贡献

- **项目仓库**: GitHub (详见 `README.md`)
- **问题反馈**: GitHub Issues
- **代码审查**: 遵循第13章的贡献流程

---

**文档结束**

*本文档旨在帮助 AI 开发助手快速理解 DispCtrl 项目，提供高质量的代码补全、问题解答与开发建议。如有更新需求，请同步修改本文档。*
