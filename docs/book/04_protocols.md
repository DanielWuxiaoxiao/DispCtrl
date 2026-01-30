# 第4章 协议与数据格式（详细）

本章详细说明项目使用的协议与重要消息格式，参考 `Basic/Protocol.h` 与 `docs/internal_protocol.md`。

## 通用说明
- 字节序：小端（Little Endian）。
- 对齐：1 字节对齐。
- 报文结构：通常包含帧头（固定值）、消息 ID（2 字节）、长度、有效载荷、校验与帧尾（可选）。

## 重要消息（示例）
### BIT 报告（0xDE02）
结构体字段示例：
- `mesID`：0xDE02
- `bitGroup`：8 位状态字，每位代表一项子系统状态（bit7 阵面发射、bit6 占空比报警、...）
- `powerState`：波控板电源状态

GUI 处理：`MainOverLayOut::onBITReport()` 将数据缓存到 `m_lastBITReport`，并在“雷达系统健康管理”对话中以绿/红按钮显示八位状态。

### BeamControl（0xAA05）
- 移除 `tranStart` 字段（协议更新）。
- `sampleEnd` 替代 `sampleLen`。
- `freqID` 的协议语义：0~80 对应 9.0~9.8 GHz；UI 的 ComboBox 使用 index 0~8，发送时需 `index * 10`。

## 扩展链路
外部雷控/伺服链路包含 512B 与 32B 两类占位消息，含 ACK 机制；详见 `ExternalCtrlManager` 接口注释。

## 兼容性与版本管理
- 建议为协议变更维护版本号与变更日志（docs/changes_*.md），并在 `Basic/Protocol.h` 顶部注释变更说明。
