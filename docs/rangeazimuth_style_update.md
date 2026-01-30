# 距离-方位图表样式和功能升级

**日期**: 2026-01-28
**版本**: 2.1

## 更新内容

### 1. ✅ 新增方位角范围控制
在工具栏添加了方位角范围输入框，可以过滤显示特定方位角范围内的目标：

```cpp
// 工具栏布局（从左到右）
[标题] [方位范围: <最小>° - <最大>°] [状态统计] [清除] [重置]
```

**功能特性**：
- 支持 0-360° 任意范围设置
- 支持跨越0°的范围（如 330° - 30°）
- 实时过滤检测点和航迹
- 输入验证和自动纠正

### 2. ✅ 添加深色主题样式
在 `darkstyle.qss` 中新增专用样式规则：

#### 工具栏样式
```css
#RangeAzimuthChartToolBar {
    background-color: #2b2b2b;         /* 深色背景 */
    border-bottom: 1px solid #3c3c3c;  /* 底部分隔线 */
}
```

#### 按钮样式
- 普通状态：深灰色背景 `#3c3c3c`
- 悬停状态：变亮并显示青色边框 `#00ff88`
- 按下状态：变暗 `#2a2a2a`

#### 输入框样式
- 背景：深色 `#2b2b2b`
- 边框：灰色 `#555555`
- 聚焦：青色边框 `#00ff88`
- 选中文字：青色背景

### 3. ✅ 图表对象命名
为支持 QSS 样式，添加了 `setObjectName()`：

```cpp
// 工具栏
setObjectName("RangeAzimuthChartToolBar");

// 图表主体
setObjectName("RangeAzimuthChart");
```

### 4. ✅ 实时统计显示
工具栏状态标签实时显示：
```
检测点: 1234 | 航迹: 56
```

通过信号连接自动更新：
```cpp
connect(m_chart, &RangeAzimuthChart::pointCountChanged,
        m_toolbar, &RangeAzimuthChartToolBar::updateStatus);
```

## 新增 API

### RangeAzimuthChart 类

#### 方位角范围设置
```cpp
void setAzimuthRange(double minAz, double maxAz);
```
设置方位角显示范围，范围外的点将被过滤。

**参数**：
- `minAz`: 最小方位角（0-360°）
- `maxAz`: 最大方位角（0-360°）

**特性**：
- 支持跨越0°的范围
- 自动清除范围外的现有点
- 新增点自动应用过滤

#### 内部辅助方法
```cpp
bool isAzimuthInRange(double azimuth) const;
```
判断方位角是否在设置的范围内，处理跨0°的特殊情况。

### RangeAzimuthChartToolBar 类

#### 新增信号
```cpp
signals:
    void azimuthRangeChanged(double minAz, double maxAz);
```
当用户修改方位角范围时发出。

#### 新增方法
```cpp
double getMinAzimuth() const;  // 获取最小方位角
double getMaxAzimuth() const;  // 获取最大方位角
void updateStatus(int detectionCount, int trackCount);  // 更新统计信息
```

## 使用示例

### 设置方位角范围
```cpp
// 只显示东向目标（60° - 120°）
chart->setAzimuthRange(60, 120);

// 显示北向目标（跨0°：330° - 30°）
chart->setAzimuthRange(330, 30);

// 显示所有方位
chart->setAzimuthRange(0, 360);
```

### 获取用户输入的范围
```cpp
double minAz = toolbar->getMinAzimuth();
double maxAz = toolbar->getMaxAzimuth();
```

## 样式定制

如需修改颜色主题，编辑 `resources/style/darkstyle.qss`：

```css
/* 修改工具栏背景色 */
#RangeAzimuthChartToolBar {
    background-color: #你的颜色;
}

/* 修改按钮悬停颜色 */
#RangeAzimuthChartToolBar QPushButton:hover {
    border-color: #你的高亮色;
}

/* 修改输入框聚焦边框 */
#RangeAzimuthChartToolBar QLineEdit:focus {
    border-color: #你的高亮色;
}
```

## 数据流更新

方位角过滤在数据添加时生效：

```
数据源 → addPointInfo() → isAzimuthInRange() → 绘制（或丢弃）
         addDetectionPoint() ↗
         addTrackPoint() ↗
```

所有三个添加方法都会调用 `isAzimuthInRange()` 进行过滤。

## 编译状态

✅ **Debug配置编译成功**
✅ **无警告，无错误**
✅ **Qt MOC 正常处理信号/槽**

## 测试要点

- [ ] 方位角范围输入验证（0-360°）
- [ ] 跨0°范围正确过滤（如 330°-30°）
- [ ] 实时统计数字准确
- [ ] 样式与整体主题一致
- [ ] 清除/重置按钮功能正常
- [ ] 方位角变化后现有点正确过滤

---

**维护者**: DispCtrl Development Team
**相关文档**:
- [rangeazimuth_chart_migration.md](./rangeazimuth_chart_migration.md)
- [darkstyle.qss](../resources/style/darkstyle.qss)
