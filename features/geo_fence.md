# 电子围栏（区域告警）

## 功能描述

用户在PPI显示界面上通过鼠标点击绘制多边形区域（电子围栏）。当任何检测点或航迹点进入该区域时，触发视觉高亮和声音告警。

## 实现文件

- `PolarDisp/geofenceitem.h / .cpp`  — PPI图形项（多边形绘制）
- `Controller/geofencemanager.h / .cpp` — 围栏管理与碰撞检测

## 操作流程

1. 用户点击"电子围栏"按钮进入绘制模式
2. 在PPI上依次左键点击添加顶点
3. 右键/双击完成多边形，自动闭合
4. 系统开始检测进入该区域的目标
5. 目标进入时：闪烁高亮 + 颜色变红 + 可选声音提示

## 接口

```cpp
// 添加一个围栏
GeoFenceManager::addFence(QPolygonF polygonInSceneCoords);

// 删除所有围栏
GeoFenceManager::clearFences();

// 信号：目标进入围栏
signal: targetEnteredFence(unsigned int batchID, float range, float azimuth);
```

## 坐标转换

PPI使用极坐标(range, azimuth)，多边形顶点存储为PPI场景像素坐标。
碰撞检测时将 `PointInfo.range/azimuth` 转换为 PPI 像素坐标后，使用 `QPolygonF::containsPoint()` 判断。
