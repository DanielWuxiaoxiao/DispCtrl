# 航迹显示问题修复说明

## 问题描述

用户报告了两个航迹显示问题：

### 1. 航迹位置显示错误
**现象**: 左下角表格显示的航迹角度为 270°、200多度等（应该在左侧和下方），但 PPI 显示时所有航迹都汇聚到右边。

**原因**: 协议中航迹的 `float azi` 和 `float ele` 字段虽然是 float 类型，但实际数据仍以 **0.01° 为单位**（与旧版 ushort 协议保持一致）。例如：
- 协议值：27000.0 (float)
- 实际角度：270.00°
- 转换公式：`角度 = 协议值 / 100.0`

检测点显示正常是因为检测点协议已经更新为直接使用度为单位，而航迹协议仍使用 0.01° 单位。

### 2. 航迹样式不美观
**现象**: 红色细线显示的航迹不够醒目，视觉效果较差。

**改进**:
- 颜色：从红色改为**橙色** (RGB: 255,128,0)，更醒目且与检测点绿色、TBD黄色有明显区分
- 线宽：从 1 像素增加到 2 像素
- 样式：添加圆形端点和连接
- 透明度：设置为 80% 不透明度，避免过于刺眼

**颜色方案**:
- 检测点：绿色 (0, 255, 0)
- DBT航迹：橙色 (255, 128, 0) ⭐ 新
- TBD航迹：黄色 (255, 255, 0)

## 修复方案

### 修复1：添加角度单位转换

#### 文件1: `Controller/data2dispmanager.cpp`
**修改位置**: 航迹数据解析部分（第51-52行）

**修改前**:
```cpp
info.azimuth = traPointInfo->azi;
info.elevation = traPointInfo->ele;
```

**修改后**:
```cpp
info.azimuth = traPointInfo->azi / 100.0f;  // 转换：0.01° -> 度
info.elevation = traPointInfo->ele / 100.0f;  // 转换：0.01° -> 度
```

#### 文件2: `Controller/tbd2dispmanager.cpp`
**修改位置**: TBD 航迹数据解析部分（第70-71行）

**修改前**:
```cpp
info.azimuth = pt->azi;
info.elevation = pt->ele;
```

**修改后**:
```cpp
info.azimuth = pt->azi / 100.0f;  // 转换：0.01° -> 度
info.elevation = pt->ele / 100.0f;  // 转换：0.01° -> 度
```

### 修复2：优化航迹颜色

#### 文件: `Basic/DispBasci.h`
**修改位置**: 颜色常量定义（第18行）

**修改前**:
```cpp
const QColor TRA_COLOR = Qt::red;
```

**修改后**:
```cpp
const QColor TRA_COLOR = QColor(255, 128, 0);  // 橙色，比红色更醒目且与绿色/黄色有明显区分
```

### 修复3：增强航迹连线样式

#### 文件: `PointManager/trackmanager.cpp`
**修改位置**: 航迹连线创建部分（第284-288行）

**修改前**:
```cpp
QPen pen(s.color);
pen.setWidth(1);
line->setPen(pen);
```

**修改后**:
```cpp
QPen pen(s.color);
pen.setWidth(2);  // 增加线宽从1到2，使航迹更明显
pen.setStyle(Qt::SolidLine);  // 实线
pen.setCapStyle(Qt::RoundCap);  // 圆形端点
pen.setJoinStyle(Qt::RoundJoin);  // 圆形连接
line->setPen(pen);
line->setOpacity(0.8);  // 设置80%不透明度，避免过于刺眼
```

### 调试输出（临时）

在 `trackmanager.cpp` 的 `addTrackPoint()` 函数中添加了调试输出（第257-262行）：

```cpp
// 调试输出：查看原始角度值
qDebug() << "[TrackManager] Batch:" << info.batch
         << "Range:" << copy.range
         << "Azimuth:" << copy.azimuth
         << "Elevation:" << copy.elevation;

const QPointF pos = polarToPixel(copy.range, copy.azimuth);
qDebug() << "    -> Screen pos:" << pos.x() << pos.y();
```

**用途**: 验证角度转换是否正确。修复完成后可以删除这些调试代码。

## 验证方法

1. **编译并运行程序**
2. **观察航迹显示位置**：
   - 270° 应该显示在左侧（西方）
   - 0° 应该显示在上方（北方）
   - 90° 应该显示在右侧（东方）
   - 180° 应该显示在下方（南方）
3. **检查调试输出**：
   - 查看控制台输出，确认 Azimuth 值在 0-360 范围内
   - 验证屏幕坐标 (x, y) 与角度对应关系正确
4. **观察航迹样式**：
   - 航迹应该是青色
   - 连线宽度适中
   - 视觉效果舒适

## 相关协议说明

### DBT 航迹上报 (0xEE01)
- 帧头：2B (0xEE01)
- 航迹数量：2B (ushort)
- 航迹列表：每条 58B
  - 批号：2B (ushort)
  - CPI：2B (ushort)
  - UTC时间：4B (uint)
  - 纳秒/微秒：4B (uint)
  - 状态估计方式：1B (uchar，0=滤波/1=预测)
  - 幅度：4B (float)
  - 信噪比：4B (float)
  - **径向距离：4B (float，单位：米)**
  - **方位角：4B (float，单位：0.01°)** ⚠️
  - **俯仰角：4B (float，单位：0.01°)** ⚠️
  - 高度：4B (float，单位：米)
  - 多普勒速度：4B (float，单位：m/s)
  - 空间速度：4B (float，单位：m/s)
  - 加速度：4B (float，单位：m/s²)
  - 预留：12B (float×3)

### TBD 航迹上报 (0xEE02)
- 结构类似，每个点的方位角、俯仰角也是 **0.01° 为单位**

## 后续优化建议

1. **统一协议单位**：建议协议层面统一使用度为单位（与检测点保持一致）
2. **航迹样式可配置**：考虑在配置文件中添加航迹颜色、线宽等参数
3. **渐变效果**：可以考虑航迹尾部逐渐淡化，更清晰地显示运动方向
4. **箭头指示**：在航迹头部添加方向箭头，显示目标运动方向

## 修复时间
- 2026-02-03

## 修复人员
- AI Agent (GitHub Copilot)
