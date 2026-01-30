# 距离-方位显示组件架构重构文档

**日期**: 2026-01-28
**版本**: 2.0
**作者**: AI Agent & Development Team

## 1. 架构变更概述

### 1.1 变更原因
原有的 `RangeAzimuthWidget` 使用极坐标系统（`SectorScene`），与用户需求不符。用户需要的是：
> "类似 qlinechart 一样的图表，是一个二维的有横纵坐标的图。坐标有刻度显示。有横坐标即方位角，纵坐标即距离。而不是现在的图中的极坐标。"

### 1.2 新架构设计
采用**直角坐标系（Cartesian Coordinate System）**：
- **横轴（X轴）**: 方位角 0-360°
- **纵轴（Y轴）**: 距离（米）
- **网格系统**: 主刻度线 + 副刻度线
- **样式**: 深色主题，匹配 darkstyle.qss

## 2. 新组件结构

### 2.1 基础类：CustomLineChart
**文件位置**: `cusWidgets/customlinechart.h` / `.cpp`

**功能特性**:
```cpp
// 轴配置
struct ChartAxisConfig {
    double minValue;              // 最小值
    double maxValue;              // 最大值
    double majorTickInterval;     // 主刻度间隔
    double minorTickInterval;     // 副刻度间隔
    QString label;                // 轴标签
    QString unit;                 // 单位
};

// 核心方法
void setXAxisConfig(const ChartAxisConfig& config);  // 配置X轴
void setYAxisConfig(const ChartAxisConfig& config);  // 配置Y轴
void addPoint(double x, double y, const QColor& color, double size);  // 添加数据点
QPointF dataToScene(double x, double y);  // 数据坐标 → 场景坐标
```

**默认配置**:
- X轴: 0-360°, 主刻度45°, 副刻度15°
- Y轴: 0-5000m, 主刻度1000m, 副刻度500m

### 2.2 雷达图表类：RangeAzimuthChart
**文件位置**: `PolarDisp/rangeazimuthchart.h` / `.cpp`

**继承关系**: `RangeAzimuthChart` → `CustomLineChart`

**核心功能**:
```cpp
// 数据添加
void addDetectionPoint(const PointInfo& info);   // 添加检测点（绿色）
void addTrackPoint(const trackInfo& info);       // 添加航迹（黄色）

// 显示控制
void setDetectionVisible(bool visible);          // 检测点可见性
void setTrackVisible(bool visible);              // 航迹可见性
void setDetectionSizeRatio(double ratio);        // 检测点大小（0.5-3.0x）
void setTrackSizeRatio(double ratio);            // 航迹大小（0.5-3.0x）

// 范围同步
void setRangeFromMain(double minRange, double maxRange);  // 与主PPI同步

// 数据管理
void clearRadarData();                           // 清除所有数据
void setMaxDetectionPoints(int maxPoints);       // 设置最大检测点数
void setMaxTrackPoints(int maxTracks);           // 设置最大航迹数
```

**数据结构**:
```cpp
struct DetectionItem {
    QGraphicsEllipseItem* graphicsItem;  // 图形对象
    PointInfo info;                      // 检测点信息
    qint64 timestamp;                    // 时间戳（FIFO用）
};

struct TrackItem {
    QGraphicsEllipseItem* graphicsItem;  // 图形对象
    trackInfo trackData;                 // 航迹信息
    qint64 timestamp;                    // 时间戳（FIFO用）
};
```

### 2.3 完整控件类：RangeAzimuthChartWidget
**文件位置**: `PolarDisp/rangeazimuthchart.h` / `.cpp`

**组成**:
- `RangeAzimuthChartToolBar`: 工具栏（清除/重置按钮）
- `RangeAzimuthChart`: 核心图表

**使用接口**:
```cpp
RangeAzimuthChart* chart() const;  // 获取图表实例
```

## 3. 数据流集成

### 3.1 数据源连接
**位置**: `mainPanel/mainoverlayout.cpp` 第463-483行

```cpp
// 检测点数据流
connect(CON_INS, &Controller::detInfoProcess,
        this, [this](const PointInfo& info) {
    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        m_rangeAzimuthWidget->chart()->addDetectionPoint(info);
    }
});

// 航迹数据流
connect(CON_INS, &Controller::traInfoProcess,
        this, [this](const trackInfo& info) {
    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        m_rangeAzimuthWidget->chart()->addTrackPoint(info);
    }
});
```

### 3.2 距离范围同步
**位置**: `mainPanel/mainoverlayout.cpp` 第488-495行

```cpp
// 从主PPI轴同步距离范围
connect(mScene->axis(), &PolarAxis::rangeChanged,
        this, [this](double min, double max) {
    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        m_rangeAzimuthWidget->chart()->setRangeFromMain(min, max);
    }
});
```

### 3.3 可见性和大小控制同步
**位置**: `mainPanel/mainoverlayout.cpp` 第284-360行

```cpp
// 检测点可见性
connect(posInfo, &MousePositionInfo::detectionVisibilityChanged,
        this, [this](bool visible) {
    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        m_rangeAzimuthWidget->chart()->setDetectionVisible(visible);
    }
});

// 航迹可见性
connect(posInfo, &MousePositionInfo::trackVisibilityChanged,
        this, [this](bool visible) {
    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        m_rangeAzimuthWidget->chart()->setTrackVisible(visible);
    }
});

// 检测点大小
connect(posInfo, &MousePositionInfo::detectionSizeChanged,
        this, [this](double ratio) {
    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        m_rangeAzimuthWidget->chart()->setDetectionSizeRatio(ratio);
    }
});

// 航迹大小
connect(posInfo, &MousePositionInfo::trackSizeChanged,
        this, [this](double ratio) {
    if (m_rangeAzimuthWidget && m_rangeAzimuthWidget->chart()) {
        m_rangeAzimuthWidget->chart()->setTrackSizeRatio(ratio);
    }
});
```

## 4. 数据坐标映射

### 4.1 Protocol.h 数据结构适配
```cpp
// 检测点：PointInfo
info.azimuth  → X轴（方位角）
info.range    → Y轴（距离）
info.batch    → 批次号（用于去重）

// 航迹：trackInfo
info.azi      → X轴（方位角）
info.dis      → Y轴（距离）
info.batch    → 批次号（用于去重）
```

### 4.2 坐标转换
```cpp
// 数据坐标 → 像素坐标
QPointF scenePos = dataToScene(azimuth, distance);

// 像素坐标 → 数据坐标
QPointF dataPos = sceneToData(sceneX, sceneY);
```

## 5. 样式配置

### 5.1 颜色方案
```cpp
// 背景
QColor bgColor(30, 30, 30);

// 网格
QColor gridMajorColor(60, 60, 60);    // 主网格线
QColor gridMinorColor(45, 45, 45);    // 副网格线

// 坐标轴
QColor axisColor(200, 200, 200);      // 坐标轴线
QColor textColor(220, 220, 220);      // 文字颜色

// 数据点
QColor detectionColor(0, 255, 0);     // 检测点（绿色）
QColor trackColor(255, 255, 0);       // 航迹（黄色）
```

### 5.2 布局参数
```cpp
// 边距（为坐标轴标签留空间）
int m_leftMargin = 60;    // 左边距（Y轴标签）
int m_rightMargin = 20;   // 右边距
int m_topMargin = 20;     // 上边距
int m_bottomMargin = 40;  // 下边距（X轴标签）

// 点大小
double m_baseDetectionSize = 3.0;  // 基础检测点大小
double m_baseTrackSize = 5.0;      // 基础航迹大小
```

## 6. FIFO 队列机制

### 6.1 检测点管理
```cpp
// 使用 QMap 存储，批次号作为键
QMap<int, DetectionItem> m_detections;

// 超出限制时删除最旧的点
void limitDetectionPoints() {
    while (m_detections.size() > m_maxDetectionPoints) {
        // 根据 timestamp 找到最旧的点并删除
        // ...
    }
}
```

### 6.2 航迹管理
```cpp
// 使用 QVector 存储
QVector<TrackItem> m_tracks;

// 超出限制时删除头部（最旧）
void limitTrackPoints() {
    while (m_tracks.size() > m_maxTrackPoints) {
        m_tracks.removeFirst();
    }
}
```

## 7. 构建系统更新

### 7.1 CMakeLists.txt
```cmake
# 自定义组件模块
cusWidgets/customlinechart.cpp

# 极坐标显示模块
PolarDisp/rangeazimuthchart.cpp

# 头文件
cusWidgets/customlinechart.h
PolarDisp/rangeazimuthchart.h
```

### 7.2 DispCtrl.pro
```qmake
SOURCES += \
    cusWidgets/customlinechart.cpp \
    PolarDisp/rangeazimuthchart.cpp \

HEADERS += \
    cusWidgets/customlinechart.h \
    PolarDisp/rangeazimuthchart.h \
```

## 8. 编译错误修复记录

### 8.1 类型声明错误
**错误**: `TrackInfo` 类型未定义
**原因**: Protocol.h 中定义的是 `trackInfo`（小写t）
**修复**: 将所有 `TrackInfo` 替换为 `trackInfo`

### 8.2 成员变量错误
**错误**: `info.batchNum` 不存在
**原因**: `PointInfo` 的批次号成员是 `batch` 不是 `batchNum`
**修复**: 使用 `info.batch`

### 8.3 航迹数据成员错误
**错误**: `info.azimuth` 和 `info.range` 不存在
**原因**: `trackInfo` 使用 `azi` 和 `dis` 作为成员名
**修复**: 使用 `info.azi` 和 `info.dis`

### 8.4 结构体成员命名冲突
**错误**: `TrackItem::info` 与函数参数冲突
**原因**: 成员名 `info` 与参数名相同
**修复**: 将成员重命名为 `trackData`

## 9. 测试要点

### 9.1 功能测试
- [ ] 检测点显示正确（绿色圆点）
- [ ] 航迹显示正确（黄色圆点）
- [ ] 坐标轴刻度显示清晰
- [ ] 网格线正确显示

### 9.2 同步测试
- [ ] 主PPI距离范围变化时图表同步更新
- [ ] 检测点可见性开关有效
- [ ] 航迹可见性开关有效
- [ ] 检测点大小调节有效（0.5-3.0x）
- [ ] 航迹大小调节有效（0.5-3.0x）

### 9.3 性能测试
- [ ] 大量数据点时响应流畅
- [ ] FIFO队列正确限制点数
- [ ] 内存占用合理

### 9.4 UI测试
- [ ] Tab切换流畅
- [ ] 窗口分离功能正常
- [ ] 清除/重置按钮有效
- [ ] 样式与主题一致

## 10. 优势总结

### 10.1 架构优势
✅ **清晰的直角坐标系**: 符合用户直觉，横轴方位角，纵轴距离
✅ **可复用的基类**: CustomLineChart 可用于其他图表需求
✅ **解耦的设计**: 图表渲染与数据管理分离

### 10.2 用户体验优势
✅ **直观的数据展示**: 一眼看清目标的方位角和距离关系
✅ **精确的刻度显示**: 主副刻度线帮助快速读数
✅ **统一的控制接口**: 与PPI和扇区显示保持一致的操作体验

### 10.3 维护优势
✅ **类型安全**: 使用 Protocol.h 中的标准数据结构
✅ **易于扩展**: 基类提供完整的坐标转换和渲染基础
✅ **代码复用**: 避免重复实现坐标系统

## 11. 迁移指南（未来参考）

如果需要将其他极坐标显示迁移到直角坐标系：

### 11.1 步骤
1. 创建继承自 `CustomLineChart` 的新类
2. 配置合适的X/Y轴参数
3. 实现数据添加方法（从极坐标转换到笛卡尔坐标）
4. 连接数据源信号
5. 更新UI集成代码

### 11.2 模板代码
```cpp
class MyChart : public CustomLineChart {
    Q_OBJECT
public:
    MyChart(QWidget* parent = nullptr) : CustomLineChart(parent) {
        // 配置轴
        ChartAxisConfig xAxis;
        xAxis.minValue = 0;
        xAxis.maxValue = 360;
        setXAxisConfig(xAxis);

        ChartAxisConfig yAxis;
        yAxis.minValue = 0;
        yAxis.maxValue = 1000;
        setYAxisConfig(yAxis);
    }

    void addData(double angle, double radius) {
        // 添加数据点
        addPoint(angle, radius, Qt::green, 3.0);
    }
};
```

## 12. 相关文档
- [CustomLineChart API文档](./customlinechart_api.md)
- [坐标系统设计文档](./coordinate_system_design.md)
- [距离-方位集成指南](./rangeazimuth_integration_guide.md)

---

**文档版本**: 1.0
**最后更新**: 2026-01-28
**维护者**: DispCtrl Development Team
