# 距离-方位显示组件集成指南

## 概述
本文档说明如何将新的距离-方位显示组件（RangeAzimuthWidget）集成到MainOverLayOut中，并实现与主视图的完全同步。

## 已完成的工作

### 1. 新建文件
- `PolarDisp/rangeazimuthwidget.h` - 头文件
- `PolarDisp/rangeazimuthwidget.cpp` - 实现文件
- 已添加到 `CMakeLists.txt`

### 2. 修改的文件
- `PolarDisp/mousepositioninfo.ui` - 添加了点迹大小控制spinbox
- `PolarDisp/mousepositioninfo.h` - 添加了detectionSizeChanged和trackSizeChanged信号
- `PolarDisp/mousepositioninfo.cpp` - 连接了spinbox信号
- `Basic/ConfigManager.h` - 添加了rangeAzimuthAngle配置支持

### 3. 组件特性
- 方位角范围：0~360°（可通过工具栏设置）
- 距离范围：自动与主PPI视图同步
- 点迹可见性：与主视图同步
- 点迹数量限制：与主视图同步
- 点迹大小：支持全局统一控制

## 集成步骤

### 步骤1: 修改 mainoverlayout.h

在MainOverLayOut类声明中添加成员变量：

```cpp
// 在private成员区域添加
private:
    // ... 现有成员变量 ...
    SectorWidget* m_sectorWidget;  // 现有的扇区显示
    RangeAzimuthWidget* m_rangeAzimuthWidget;  // 新增：距离-方位显示
```

### 步骤2: 修改 mainoverlayout.cpp - 添加头文件

在文件顶部添加include：

```cpp
#include "PolarDisp/rangeazimuthwidget.h"
```

### 步骤3: 在 mainPView() 函数中集成组件

在mainPView()函数中，在扇区显示代码之后添加：

```cpp
void MainOverLayOut::mainPView() {
    // ... 现有的 pviewBIG 和 pviewZoomW 代码 ...

    // ========== 现有的扇区显示代码 ==========
    m_sectorWidget = new SectorWidget(this);
    QVBoxLayout* layout2 = new QVBoxLayout(ui->pviewSectorW);
    layout2->setContentsMargins(0, 0, 0, 0);
    layout2->addWidget(
        new DetachableWidget("扇区显示", m_sectorWidget, QIcon(":/resources/icon/scan.png"), this));

    // 连接扇区显示数据流
    connect(CON_INS, &Controller::detInfoProcess, m_sectorWidget->scene()->detManager(),
            &SectorDetManager::addDetPoint);
    connect(CON_INS, &Controller::traInfoProcess, m_sectorWidget->scene()->trackManager(),
            &SectorTrackManager::addTrackPoint);
    connect(CON_INS, &Controller::tbdInfoProcess, m_sectorWidget->scene()->trackManager(),
            &SectorTrackManager::addTrackPoint);

    // ========== 新增：距离-方位显示 ==========
    m_rangeAzimuthWidget = new RangeAzimuthWidget(this);

    // 创建Tab Widget来容纳扇区显示和距离-方位显示
    QTabWidget* displayTabWidget = new QTabWidget(this);
    displayTabWidget->setObjectName("DisplayTabWidget");

    // 将扇区显示包装为可分离的widget
    DetachableWidget* sectorDetachable = new DetachableWidget(
        "扇区显示", m_sectorWidget, QIcon(":/resources/icon/scan.png"), this);

    // 将距离-方位显示包装为可分离的widget
    DetachableWidget* rangeAzDetachable = new DetachableWidget(
        "距离-方位显示", m_rangeAzimuthWidget, QIcon(":/resources/icon/radar.png"), this);

    // 添加到TabWidget
    displayTabWidget->addTab(sectorDetachable, "扇区显示");
    displayTabWidget->addTab(rangeAzDetachable, "距离-方位");

    // 将TabWidget添加到布局
    QVBoxLayout* layout2 = new QVBoxLayout(ui->pviewSectorW);
    layout2->setContentsMargins(0, 0, 0, 0);
    layout2->addWidget(displayTabWidget);

    // ========== 连接距离-方位显示的数据流 ==========
    // 注意：RangeAzimuthWidget内部已经通过RadarDataManager自动连接数据
    // 这里不需要手动连接detInfoProcess等信号

    // ========== 同步主视图的距离范围到距离-方位显示 ==========
    if (m_ppi && m_ppi->scene() && m_ppi->scene()->axis()) {
        connect(m_ppi->scene()->axis(), &PolarAxis::rangeChanged,
                m_rangeAzimuthWidget, [this](double min, double max) {
            m_rangeAzimuthWidget->setRangeFromMain(min, max);
        });

        // 初始化时同步一次
        double minR = m_ppi->scene()->axis()->minRange();
        double maxR = m_ppi->scene()->axis()->maxRange();
        m_rangeAzimuthWidget->setRangeFromMain(minR, maxR);
    }

    // ========== 同步扇区显示的距离范围 ==========
    if (m_ppi && m_ppi->scene() && m_ppi->scene()->axis()) {
        connect(m_ppi->scene()->axis(), &PolarAxis::rangeChanged,
                this, [this](double min, double max) {
            if (m_sectorWidget && m_sectorWidget->scene()) {
                m_sectorWidget->scene()->axis()->setRange(min, max);
                m_sectorWidget->scene()->detManager()->refreshAll();
                m_sectorWidget->scene()->trackManager()->refreshAll();
            }
        });
    }
}
```

### 步骤4: 连接点迹可见性控制

在构造函数中连接mousepositioninfo的可见性信号：

```cpp
MainOverLayOut::MainOverLayOut(QWidget* parent) : QWidget(parent), ui(new Ui::MainOverLayOut) {
    // ... 现有代码 ...

    // ========== 连接点迹可见性控制 ==========
    // 假设 m_ppi 的底部有 mousePositionInfo 组件
    if (m_ppi && m_ppi->mousePositionInfo()) {
        MousePositionInfo* mousePosInfo = m_ppi->mousePositionInfo();

        // 连接检测点可见性
        connect(mousePosInfo, &MousePositionInfo::detectionVisibilityChanged,
                this, [this](bool visible) {
            // 主视图
            if (m_ppi && m_ppi->scene() && m_ppi->scene()->detManager()) {
                m_ppi->scene()->detManager()->setAllVisible(visible);
            }
            // 扇区显示
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->detManager()) {
                m_sectorWidget->scene()->detManager()->setAllVisible(visible);
            }
            // 距离-方位显示
            if (m_rangeAzimuthWidget) {
                m_rangeAzimuthWidget->setDetectionVisible(visible);
            }
        });

        // 连接航迹可见性
        connect(mousePosInfo, &MousePositionInfo::trackVisibilityChanged,
                this, [this](bool visible) {
            // 主视图
            if (m_ppi && m_ppi->scene() && m_ppi->scene()->trackManager()) {
                m_ppi->scene()->trackManager()->setAllVisible(visible);
            }
            // 扇区显示
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->trackManager()) {
                m_sectorWidget->scene()->trackManager()->setAllVisible(visible);
            }
            // 距离-方位显示
            if (m_rangeAzimuthWidget) {
                m_rangeAzimuthWidget->setTrackVisible(visible);
            }
        });

        // 连接检测点大小控制
        connect(mousePosInfo, &MousePositionInfo::detectionSizeChanged,
                this, [this](double ratio) {
            // 主视图
            if (m_ppi && m_ppi->scene() && m_ppi->scene()->detManager()) {
                m_ppi->scene()->detManager()->setPointSizeRatio(ratio);
            }
            // 扇区显示
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->detManager()) {
                m_sectorWidget->scene()->detManager()->setPointSizeRatio(ratio);
            }
            // 距离-方位显示
            if (m_rangeAzimuthWidget) {
                m_rangeAzimuthWidget->setDetectionSizeRatio(ratio);
            }
        });

        // 连接航迹点大小控制
        connect(mousePosInfo, &MousePositionInfo::trackSizeChanged,
                this, [this](double ratio) {
            // 主视图
            if (m_ppi && m_ppi->scene() && m_ppi->scene()->trackManager()) {
                m_ppi->scene()->trackManager()->setPointSizeRatio(ratio);
            }
            // 扇区显示
            if (m_sectorWidget && m_sectorWidget->scene() && m_sectorWidget->scene()->trackManager()) {
                m_sectorWidget->scene()->trackManager()->setPointSizeRatio(ratio);
            }
            // 距离-方位显示
            if (m_rangeAzimuthWidget) {
                m_rangeAzimuthWidget->setTrackSizeRatio(ratio);
            }
        });
    }
}
```

### 步骤5: 清理资源

在析构函数中确保清理：

```cpp
MainOverLayOut::~MainOverLayOut() {
    // ... 现有的清理代码 ...

    // 清理距离-方位显示的连接
    if (m_rangeAzimuthWidget) {
        disconnect(CON_INS, nullptr, m_rangeAzimuthWidget->getDetManager(), nullptr);
        disconnect(CON_INS, nullptr, m_rangeAzimuthWidget->getTrackManager(), nullptr);
    }

    // ... 继续现有的清理代码 ...
}
```

## 配置文件支持

在 `config.toml` 中添加：

```toml
[rangeAzimuthDisp.angle]
min = 0
max = 360
```

## 功能验证清单

- [ ] 距离-方位显示正确创建并显示在Tab中
- [ ] 方位角范围可以通过工具栏设置
- [ ] 距离范围自动与主视图同步
- [ ] 检测点checkbox控制所有视图的可见性
- [ ] 航迹点checkbox控制所有视图的可见性
- [ ] 检测点大小spinbox控制所有视图的点迹大小
- [ ] 航迹点大小spinbox控制所有视图的航迹大小
- [ ] 点迹数量限制与主视图同步
- [ ] 配置保存和加载正常工作

## 注意事项

1. **数据连接方式**：RangeAzimuthWidget内部已经通过RadarDataManager自动注册并连接数据，不需要像SectorWidget那样手动连接Controller信号

2. **同步机制**：
   - 距离范围：监听PolarAxis::rangeChanged信号
   - 可见性：通过MousePositionInfo的信号同步
   - 点迹大小：通过MousePositionInfo的新信号同步

3. **UI布局**：使用QTabWidget将扇区显示和距离-方位显示组织在一起，用户可以通过标签页切换

4. **资源管理**：RangeAzimuthWidget会在析构时自动从RadarDataManager注销，但建议在MainOverLayOut析构函数中显式断开连接

## 可选改进

1. 添加快捷键切换Tab页
2. 添加右键菜单快速切换显示模式
3. 保存用户选择的Tab页到配置文件
4. 添加工具栏按钮快速切换全屏显示

## 故障排查

如果遇到问题：

1. **点迹不显示**：检查RadarDataManager注册是否成功
2. **距离不同步**：检查PolarAxis::rangeChanged信号连接
3. **大小不同步**：检查MousePositionInfo信号连接
4. **配置不保存**：确保调用CF_INS.save()

---
文档创建时间：2026-01-28
