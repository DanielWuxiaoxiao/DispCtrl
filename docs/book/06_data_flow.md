# 第6章 数据流与处理流程

本章描述典型的数据流路径、处理时序及如何在代码中跟踪数据。

## 常见数据流：检测点 -> 显示
1. UDP 接收线程收到 datagram（`UDP/threadudpsocket.cpp`）。
2. 解析报文头并转换为 `PointInfo` 或其它结构。
3. 将数据发射到 `Controller`（信号如 `traInfoProcess`、`tbdInfoProcess`）。
4. `MainOverLayOut` 和 `PPIView` 等订阅信号并更新视图。

## BIT 上报流
- UDP -> `onBITReport(BITReport res)` -> `m_lastBITReport=res` -> 用户点击“雷达系统”打开对话查看详细位信息。

## 下发控制命令（如 BeamControl）
1. 用户在 UI 修改参数并点击“下发”。
2. 对话构建 `BeamControl` 结构并填充协议信息（频点映射、采样时间等）。
3. 使用 `Controller` 或 `ExternalCtrlManager` 包装并发送到相应网络端口。

## 性能考虑
- 后台线程负责网络 I/O，避免阻塞主线程。
- 大量更新通过合并或节流（debounce）策略减少 UI 刷新频率。
- 对于高频数据，可采用降采样或只更新视图中可见区域。
