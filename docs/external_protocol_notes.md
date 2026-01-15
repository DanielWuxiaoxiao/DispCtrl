# 外部通信协议实现记录（显控 ↔ 外部雷控）

## 本次实现范围

- 系统控制 512B 帧：头 0xFA55FA55 + 内容 504B + 尾 0x55FA55FA。
- AD 数据帧：头 0x7FFFBC1C，尾 0x7FFF5A5A，带可变复数采样（I/Q，各 2 字节）。
- 伺服控制/回执 32B，系统控制回执 64B 继续按长度透传。
- 新增 AD 帧解析/编码、系统控制帧校验、可拼接同报文发送（系统控制 + AD）。

## 代码变更摘要

- `Basic/Protocol.h`
  - 补充外部协议常量与结构：系统控制帧、AD 头/尾、采样、尾部预留等。
- `Controller/ExternalCtrlManager.h/.cpp`
  - 新增发送接口：
    - `sendSystemControl` 支持 512B 带头尾或 504B 纯内容。
    - `sendSystemControlPayload` 自动加头尾。
    - `sendAdFrame` 编码 AD 数据帧。
    - `sendSystemControlWithAd` 同报文发送 504B 系统控制 + AD 帧。
  - 新增接收信号：`systemControlFrameReceived(ExternalSystemControl512)`、`adFrameReceived(ExternalAdFrame)`，保留 ACK 信号。
  - 接收端按长度区分 64B/32B 回执；其余报文尝试解析系统控制 + AD 帧，校验头尾与长度。
  - AD 解析：`beamCount * samplesPerPulse` 复采样，每个采样 I/Q 为 2 字节。

## 项目覆盖情况

- 控制发送：已覆盖系统控制、伺服控制、AD 帧以及组合发送接口。
- 控制接收：新增对系统控制帧、AD 数据帧的解析与信号发出；回执仍按长度透传。
- 尚未落地的消费逻辑：当前未在其他模块订阅 `systemControlFrameReceived` / `adFrameReceived`，后续可根据需求在 UI/日志/数据链路中接入。

## 使用要点

- 系统控制：传 504B 内容时用 `sendSystemControlPayload` 或 `sendSystemControlWithAd`；传 512B 完整帧时用 `sendSystemControl`（需自带头尾且校验通过）。
- AD 帧：构造 `ExternalAdFrame`，设置 `header.beamCount` 与 `samplesPerPulse`，填充等量 `samples`（顺序：先波束后距离，I/Q 各 2 字节），调用 `sendAdFrame` 或组合发送接口。
- 接收：监听 `systemControlFrameReceived` / `adFrameReceived` 获取解析后的结构；64B/32B 回执保持不变。

## 后续建议

- 在 UI 或日志中展示收到的系统控制 / AD 帧概要（头/尾校验、beamCount、样本数）。
- 根据业务需要添加 AD 数据落盘或转发。
- 若需要校验 AD 数据长度与波束/距离对应关系，可在解析后增加更细的业务校验。
