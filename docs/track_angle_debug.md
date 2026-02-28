# 航迹角度显示调试记录

## 问题描述

**日期**: 2026-02-04
**版本**: x576分支

### 现象
- trackTable中显示的航迹角度是 2.x°（如 2.4°, 2.7°）
- PPI显示上的航迹位置全部堆叠在0°附近（接近正北）
- **检测点（绿色）显示正确** - 角度分布正常
- 航迹点（红色）显示错误 - 全部在中心0°附近

## 根本原因 ✅ 已确认

**协议单位差异**：
- **检测点 (`detInfo.azi`)**: 单位是**度** (如 270.0°)
- **航迹点 (`trackInfo.azi`)**: 单位是**弧度** (如 2.7 弧度 ≈ 154.7°)

### 证据
1. 检测点显示分布正确 → 检测点角度单位是度
2. 航迹表格显示 2.4°-2.7° → 如果是弧度：
   - 2.7 弧度 = 2.7 × 180° / π ≈ 154.7°
   - 2.4 弧度 = 2.4 × 180° / π ≈ 137.5°
3. 这些转换后的角度与检测点位置一致

## 修复内容

### 1. DBT航迹角度转换
**文件**: `Controller/data2dispmanager.cpp`
```cpp
// 修复前
info.azimuth = traPointInfo->azi;  // ❌ 弧度值被当作度使用

// 修复后
info.azimuth = traPointInfo->azi * 180.0f / 3.14159265358979f;  // ✅ 弧度转度
info.elevation = traPointInfo->ele * 180.0f / 3.14159265358979f;
```

### 2. TBD航迹角度转换
**文件**: `Controller/tbd2dispmanager.cpp`
```cpp
// 修复前
info.azimuth = pt->azi;  // ❌ 弧度值被当作度使用

// 修复后
info.azimuth = pt->azi * 180.0f / 3.14159265358979f;  // ✅ 弧度转度
info.elevation = pt->ele * 180.0f / 3.14159265358979f;
```

### 3. 移除重复信号连接（之前修复）
**文件**: `PointManager/trackmanager.cpp`
- 移除了 `TrackManager` 与 `RadarDataManager::trackReceived` 的重复连接
- 数据流统一由 `PPIScene` 通过 `Controller` 管理

## 协议单位总结

| 数据类型 | 字段 | 协议单位 | 显控内部单位 | 转换 |
|---------|------|---------|-------------|-----|
| 检测点 detInfo | azi/ele | 度 | 度 | 无需转换 |
| DBT航迹 trackInfo | azi/ele | **弧度** | 度 | × 180/π |
| TBD航迹 TBDPoint | azi/ele | **弧度** | 度 | × 180/π |
| 距离 | dis | 米 | 米 | 无需转换 |

## 验证方法

1. 重新编译运行程序
2. 观察航迹表格的方位角列：
   - 修复前：2.4°, 2.7° 等弧度值
   - 修复后：137.5°, 154.7° 等正常度数值
3. 观察PPI显示：
   - 修复前：航迹点全部在0°附近堆叠
   - 修复后：航迹点分布在各个方向，与检测点位置一致

## 相关文件
- `Controller/data2dispmanager.cpp` - DBT航迹数据处理 ✅ 已修复
- `Controller/tbd2dispmanager.cpp` - TBD航迹数据处理 ✅ 已修复
- `Controller/sig2dispmanager.cpp` - 检测点数据处理（无需修改）
- `PointManager/trackmanager.cpp` - 航迹显示管理 ✅ 已修复重复连接
- `Basic/Protocol.h` - 协议结构体定义
