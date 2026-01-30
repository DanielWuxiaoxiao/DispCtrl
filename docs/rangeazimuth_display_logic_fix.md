# 距离-方位图表显示逻辑修复

**日期**: 2026-01-28
**作者**: DispCtrl Development Team
**版本**: 1.1

## 修复概述

本次修复解决了距离-方位图表（RangeAzimuthChart）中的以下问题：

1. ✅ **点的隐藏/显示逻辑** - 超出范围的点现在会被隐藏而不是删除
2. ✅ **Tooltip支持** - 所有点现在都支持鼠标悬停显示详细信息
3. ✅ **可见性同步** - MousePositionInfo的checkbox能够正确控制图表中点的可见性
4. ✅ **范围变化响应** - 当距离或方位角范围改变时，点会自动显示/隐藏

## 核心设计理念变更

### 之前的逻辑（有问题）：
```cpp
// 如果不在范围内，直接不添加点
if (!isAzimuthInRange(azimuth)) {
    return;
}
if (range < minRange || range > maxRange) {
    return;
}
```

### 新的逻辑（正确）：
```cpp
// 所有点都添加，但根据范围决定是否显示
bool inAzimuthRange = isAzimuthInRange(azimuth);
bool inDistanceRange = (range >= minRange && range <= maxRange);
bool shouldShow = m_visible && inAzimuthRange && inDistanceRange;
item->setVisible(shouldShow);
```

## 详细修改

### 1. addDetectionPoint() - 检测点添加逻辑

**修改内容**：
- ✅ 移除了提前return的逻辑
- ✅ 添加了tooltip设置：显示批次、方位、距离、类型
- ✅ 启用hover事件：`setAcceptHoverEvents(true)`
- ✅ 根据三个条件决定可见性：
  - `m_detectionVisible` - 用户通过checkbox控制
  - `isAzimuthInRange(azimuth)` - 方位角在范围内
  - `range在距离范围内` - 距离在Y轴范围内

**Tooltip格式**：
```
批次: 12345
方位: 45.2°
距离: 1234.5m
类型: 检测点
```

### 2. addTrackPoint() - 航迹点添加逻辑

**修改内容**：
- ✅ 同检测点，移除提前return
- ✅ 添加tooltip（显示航迹信息）
- ✅ 启用hover事件
- ✅ 根据可见性条件显示/隐藏

### 3. addPointInfo() - 统一接口逻辑

**修改内容**：
- ✅ 航迹和TBD航迹分支也移除了提前return
- ✅ 添加了tooltip支持
- ✅ 动态显示类型名称（"航迹" 或 "TBD航迹"）
- ✅ 应用相同的可见性逻辑

### 4. setDetectionVisible() - 检测点可见性控制

**修改前**：
```cpp
void RangeAzimuthChart::setDetectionVisible(bool visible)
{
    m_detectionVisible = visible;
    // 简单地设置所有点的可见性
    for (item : items) {
        item->setVisible(visible);
    }
}
```

**修改后**：
```cpp
void RangeAzimuthChart::setDetectionVisible(bool visible)
{
    m_detectionVisible = visible;

    ChartAxisConfig yAxis = yAxisConfig();
    for (auto it = m_detections.begin(); it != m_detections.end(); ++it) {
        const PointInfo& info = it.value().info;
        // 同时检查方位角和距离范围
        bool inAzimuthRange = isAzimuthInRange(info.azimuth);
        bool inDistanceRange = (info.range >= yAxis.minValue && info.range <= yAxis.maxValue);
        bool shouldShow = visible && inAzimuthRange && inDistanceRange;
        it.value().graphicsItem->setVisible(shouldShow);
    }
}
```

**关键改进**：
- ✅ 不仅考虑用户的可见性设置
- ✅ 同时检查点是否在当前的方位角和距离范围内
- ✅ 三个条件都满足才显示点

### 5. setTrackVisible() - 航迹可见性控制

**修改内容**：
- ✅ 同setDetectionVisible()的逻辑
- ✅ 检查航迹的方位角和距离范围
- ✅ 三条件综合判断

### 6. setAzimuthRange() - 方位角范围变化

**修改前**（删除不在范围内的点）：
```cpp
void RangeAzimuthChart::setAzimuthRange(double minAz, double maxAz)
{
    m_minAzimuth = minAz;
    m_maxAzimuth = maxAz;

    // 删除不在范围内的点
    QMap<int, DetectionItem> newDetections;
    for (item : items) {
        if (isAzimuthInRange(item.azimuth)) {
            newDetections.insert(item);
        } else {
            delete item.graphicsItem;  // 删除图形项！
        }
    }
    m_detections = newDetections;
}
```

**修改后**（隐藏/显示）：
```cpp
void RangeAzimuthChart::setAzimuthRange(double minAz, double maxAz)
{
    m_minAzimuth = minAz;
    m_maxAzimuth = maxAz;

    ChartAxisConfig yAxis = yAxisConfig();

    // 更新所有点的可见性（不删除）
    for (auto it = m_detections.begin(); it != m_detections.end(); ++it) {
        const PointInfo& info = it.value().info;
        bool inAzimuthRange = isAzimuthInRange(info.azimuth);
        bool inDistanceRange = (info.range >= yAxis.minValue && info.range <= yAxis.maxValue);
        bool shouldShow = m_detectionVisible && inAzimuthRange && inDistanceRange;
        it.value().graphicsItem->setVisible(shouldShow);
    }

    // 同样处理航迹...
}
```

**关键改进**：
- ✅ 不再删除任何点
- ✅ 只是设置可见性
- ✅ 当用户调整方位角范围回来时，点会重新出现

### 7. setRangeFromMain() - 距离范围同步

**修改内容**：
- ✅ 在重新创建图形项时添加tooltip
- ✅ 启用hover事件
- ✅ 根据方位角和距离范围综合判断可见性
- ✅ 移除了`continue`跳过逻辑，所有点都创建但可能不可见

## 用户体验改进

### 1. Hover显示Tooltip
**效果**：鼠标悬停在任何点上都会显示：
- 批次号
- 方位角（精确到0.1度）
- 距离（精确到0.1米）
- 点类型（检测点/航迹/TBD航迹）

### 2. 智能显示/隐藏
**场景1 - 方位角范围调整**：
```
用户设置: 60° - 120°
- 方位角45°的点 → 隐藏
- 方位角90°的点 → 显示
- 方位角150°的点 → 隐藏

用户调整为: 0° - 360°
- 所有点重新显示
```

**场景2 - 距离范围调整**：
```
用户调整主视图距离: 0-2000m
- 距离1500m的点 → 显示
- 距离3000m的点 → 隐藏

用户调整为: 0-5000m
- 距离3000m的点 → 重新显示
```

**场景3 - checkbox控制**：
```
用户取消"检测点"checkbox
- 所有检测点隐藏（无论范围如何）

用户重新勾选
- 在范围内的检测点重新显示
- 超出范围的仍然隐藏
```

### 3. 数据不丢失
**关键优势**：
- ✅ 点隐藏后数据仍在内存中
- ✅ 调整范围回来时点会重新出现
- ✅ 不需要重新接收数据
- ✅ 减少数据流量和处理开销

## 性能考虑

### 内存管理
- 所有点都保留在内存中（QMap和QVector）
- 使用FIFO机制限制点数（`m_maxDetectionPoints`, `m_maxTrackPoints`）
- 只有在点数超限时才真正删除最老的点

### 渲染优化
- 不可见的点不参与渲染（Qt自动优化）
- `setVisible(false)`比删除后重建更高效
- 避免频繁的内存分配/释放

## 测试建议

### 测试用例1：方位角范围变化
```
步骤：
1. 启动应用，观察初始显示（0-360°）
2. 调整方位角为 60-120°
3. 确认只有该范围内的点可见
4. 调整回 0-360°
5. 确认所有点重新出现

预期：点不丢失，显示/隐藏流畅
```

### 测试用例2：距离范围同步
```
步骤：
1. 在主视图调整距离范围为 0-2000m
2. 观察距离-方位图表自动同步
3. 确认超过2000m的点被隐藏
4. 调整回 0-5000m
5. 确认隐藏的点重新显示

预期：距离范围完美同步，点显示正确
```

### 测试用例3：Tooltip显示
```
步骤：
1. 鼠标悬停在检测点上
2. 确认显示tooltip（批次、方位、距离、类型）
3. 鼠标悬停在航迹上
4. 确认显示航迹tooltip

预期：tooltip信息完整准确
```

### 测试用例4：可见性控制
```
步骤：
1. 取消"检测点"checkbox
2. 确认所有检测点隐藏
3. 重新勾选
4. 确认检测点恢复显示（仅范围内）
5. 重复测试"航迹"checkbox

预期：checkbox控制准确，不影响数据
```

### 测试用例5：跨0°方位角
```
步骤：
1. 设置方位角范围 330° - 30°
2. 确认只有该范围内的点显示
3. 观察tooltip确认方位角值正确

预期：跨0°逻辑正确，无显示异常
```

## 代码质量

### 代码复用
- ✅ 可见性判断逻辑集中在三个方法中
- ✅ tooltip生成格式统一
- ✅ hover事件启用代码一致

### 可维护性
- ✅ 逻辑清晰，容易理解
- ✅ 注释完整
- ✅ 变量命名规范

### 健壮性
- ✅ 空指针检查（graphicsItem存在性）
- ✅ 范围边界检查
- ✅ 跨0°方位角特殊处理

## 与PPI视图的一致性

| 特性 | PPI视图 | 距离-方位图表 | 状态 |
|-----|---------|--------------|------|
| Tooltip显示 | ✅ | ✅ | 一致 |
| 范围外隐藏 | ✅ | ✅ | 一致 |
| Checkbox控制 | ✅ | ✅ | 一致 |
| 点大小控制 | ✅ | ✅ | 一致 |
| 数据不丢失 | ✅ | ✅ | 一致 |

## 后续优化建议

### 可选功能
1. **点击选中功能** - 点击点时高亮并显示详细信息
2. **右键菜单** - 提供删除、标记等操作
3. **颜色自定义** - 允许用户自定义检测点和航迹颜色
4. **点大小自适应** - 根据缩放级别自动调整点大小

### 性能优化
1. **批量可见性更新** - 使用场景更新标志批量处理
2. **空间索引** - 使用四叉树加速范围查询
3. **LOD机制** - 远距离点使用更简单的渲染

## 总结

本次修复彻底解决了距离-方位图表的显示逻辑问题，实现了与PPI视图一致的用户体验。关键改进：

1. ✅ **点不会因为范围变化而丢失**
2. ✅ **用户可以通过hover查看点的详细信息**
3. ✅ **checkbox控制准确响应**
4. ✅ **方位角和距离范围协同工作**

所有修改已编译通过，可以进行测试验证。
