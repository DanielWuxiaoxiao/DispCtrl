# 航迹角度显示调试记录

> 2026-05-15 更新：本记录中的“航迹协议单位为弧度”结论已不再采用。当前项目按用户最新联调约定，检测点、常规航迹、TBD 航迹、协同航迹的方位/俯仰字段统一按“度”处理。

## 问题描述

**日期**: 2026-02-04
**版本**: x576分支

### 现象
- trackTable中显示的航迹角度是 2.x°（如 2.4°, 2.7°）
- PPI显示上的航迹位置全部堆叠在0°附近（接近正北）
- **检测点（绿色）显示正确** - 角度分布正常
- 航迹点（红色）显示错误 - 全部在中心0°附近

## 根本原因

旧版本曾将航迹协议解释为弧度，并在显控端执行弧度到度的转换。根据当前联调结论，航迹协议字段本身就是度，继续做该转换会把正常角度放大为数千度，进而触发无效数据校验。

### 证据
1. 检测点显示分布正确 → 检测点角度单位是度
2. 航迹表格显示 2.4°-2.7° → 如果是弧度：
   - 2.7 弧度 = 2.7 × 180° / π ≈ 154.7°
   - 2.4 弧度 = 2.4 × 180° / π ≈ 137.5°
3. 这些转换后的角度与检测点位置一致

## 当前修复内容

### 1. DBT航迹角度处理
**文件**: `Controller/data2dispmanager.cpp`
```cpp
// 当前
info.azimuth = traPointInfo->azi;  // 协议值直接按度使用
info.elevation = traPointInfo->ele;
```

### 2. TBD航迹角度处理
**文件**: `Controller/tbd2dispmanager.cpp`
```cpp
info.azimuth = pt->azi;  // 协议值直接按度使用
info.elevation = pt->ele;
```

### 3. 协同航迹角度处理
**文件**: `Controller/collabtrack2dispmanager.cpp`
```cpp
info.azimuth = pt->azi;  // 协议值直接按度使用
info.elevation = pt->ele;
```

### 3. 移除重复信号连接（之前修复）
**文件**: `PointManager/trackmanager.cpp`
- 移除了 `TrackManager` 与 `RadarDataManager::trackReceived` 的重复连接
- 数据流统一由 `PPIScene` 通过 `Controller` 管理

## 协议单位总结

| 数据类型 | 字段 | 协议单位 | 显控内部单位 | 转换 |
|---------|------|---------|-------------|-----|
| 检测点 detInfo | azi/ele | 度 | 度 | 无需转换 |
| DBT航迹 trackInfo | azi/ele | 度 | 度 | 无需转换 |
| TBD航迹 trackInfo | azi/ele | 度 | 度 | 无需转换 |
| 协同航迹 trackInfo | azi/ele | 度 | 度 | 无需转换 |
| 距离 | dis | 米 | 米 | 无需转换 |

## 验证方法

1. 重新编译运行程序
2. 观察航迹表格的方位角列：
   - 应与发送端原始角度值一致
   - 不应再出现乘以 `180/π` 后放大的数值
3. 观察PPI显示：
   - 航迹点应与发送端角度方向一致
   - 不应再因角度超出 360° 被大量判定为无效

## 相关文件
- `Controller/data2dispmanager.cpp` - DBT航迹数据处理 ✅ 已修复
- `Controller/tbd2dispmanager.cpp` - TBD航迹数据处理 ✅ 已修复
- `Controller/sig2dispmanager.cpp` - 检测点数据处理（无需修改）
- `PointManager/trackmanager.cpp` - 航迹显示管理 ✅ 已修复重复连接
- `Basic/Protocol.h` - 协议结构体定义
