# 第5章 核心模块详解

本章对关键模块进行逐个解剖，包含接口、内部实现细节与使用示例。

## Controller 层
- 入口：`Controller/controller.h` 与 `Controller/controller.cpp`。
- 功能：协调 Managers、桥接 UI 信号、提供系统级控制（最小化、退出、外部链路控制等）。

## UDP 与 ThreadedUdpSocket
- 负责多线程接收与数据队列管理。  
- 核心函数：`handleDatagram()`，在接收端识别消息 ID 并通过 `memcpy` 解析到结构体，随后发射信号（如 `bitReport()`）。

## RadarDataManager 与 Manager 系列
- 作用：统一缓存、合并重复帧、按订阅分发。  
- API 示例：`addDetPoint()`, `addTrackPoint()` 等；上层通过信号连接到 PPI 或表格视图。

## PolarDisp（PPI/扇区显示）
- 主要类：`PPIView`, `PPIScene`, `ScanLayer`, `SectorWidget`。
- 优化点：使用 QGraphicsView/Scene 渲染分层对象，重绘时仅更新受影响区域以减少开销。

## MapProxy 与地图集成
- 使用 Qt WebEngine 承载 `htmls/` 内的地图页面，通过 JS -> Qt 通信同步雷达中心和范围。

## MainOverLayOut
- 作为顶层控制 UI 的入口，管理工具栏、状态标签与参数面板；负责显示“雷达系统健康管理”对话。
- 注意跨平台兼容性：overlay 要作为子组件创建，并延迟设置 geometry（见第 8 章）。

## 自定义控件（cusWidgets）
- `CusWindow`, `CustomComboBox`, `CustomSpinBox` 等，目的是提供一致的风格与可复用组件。

## 参数对话（paramWidget）
- 每个对话负责参数校验、恢复与下发（`onAccept()` 实现下发逻辑，`restoreParam()` 从结构恢复 UI）。

## 日志与错误处理
- `Basic/log.*` 提供格式化日志接口；`Controller/ErrorHandler` 负责策略性错误处理与提示。