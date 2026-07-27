# Ship-Radar 对外 UDP 接口协议（以现行代码为准）

**适用工程**：`DispCtrl` 的 `ship-radar` 分支  
**协议依据**：`Basic/MarineProtocol.h`、`Controller/MarineRadarManager.cpp`、`Controller/controller.cpp`、`mainPanel/mainoverlayout.cpp`。  
**当前联调约定**：`PS端与显控通信协议_当前实现.docx`。本文已按该文档的分片变更更新；当文字说明与代码不一致时，**以当前代码实现为准**。

## 1. 范围和接口

当前显控实际实现并使用以下两路 UDP 接口：

| 接口 | 方向 | 用途 | 当前代码入口 |
| --- | --- | --- | --- |
| 接口 1 | 显控 -> 雷达/伺服 | 下发雷达参数和转台命令 | `MarineRadarManager::sendControl()` |
| 接口 4 | 雷达/伺服 -> 显控 | 上报雷达状态和一条方位线的回波幅度 | `MarineRadarManager::parseEchoDatagram()` |

Word 中描述的雷达阵面内部接口、伺服与阵面之间的内部接口，不是当前 `DispCtrl` 进程收发的协议，本文不定义其报文。

## 2. 网络配置

配置文件使用以下字段：

```toml
[marine.network]
local_ip = "192.168.1.100"
echo_port = 9000
servo_ip = "192.168.1.30"
servo_port = 9000
auto_send_ms = 0
fragment_timeout_ms = 500
```

| 字段 | 含义 | 现行代码行为 |
| --- | --- | --- |
| `local_ip` | 显控本机 IPv4 地址 | 在此地址绑定回波接收 Socket；空字符串时绑定 `0.0.0.0`。 |
| `echo_port` | 回波接收端口 | 显控绑定此 UDP 端口接收设备上报。 |
| `servo_ip` | 雷达/伺服目的 IPv4 地址 | 控制帧的目标地址。 |
| `servo_port` | 雷达/伺服目的端口 | 控制帧的目标端口。 |
| `auto_send_ms` | 自动下发周期，单位 ms | `<= 0` 时关闭周期发送；大于 0 时按该周期重复发送当前控制帧。 |
| `fragment_timeout_ms` | 回波分片重组超时，单位 ms | 默认 `500`，可配置范围 `100~5000`；未完成分片在最后一个分片到达后超过该时间才丢弃。 |

正常情况下，控制帧也通过已经绑定 `local_ip:echo_port` 的接收 Socket 发出，因此其**源端口通常是 `echo_port`**。这样可兼容“设备将回波回复到控制帧源端口”的实现。只有接收 Socket 未成功绑定时，才退回到未绑定发送 Socket，此时操作系统可能分配临时源端口。

当前接收端按本地绑定地址和端口接收 UDP 数据报，**不额外校验发送方 IP 或端口**。

## 3. 接口 1：显控 -> 雷达/伺服控制帧

### 3.1 基本约定

- 传输层：UDP。
- 帧长：固定 **16 字节**。
- 帧头：`0xA5`；帧尾：`0x5A`。
- `uint16_t Azimuth` 按小端发送，即低字节在前、高字节在后。
- 校验：`Byte[0]` 到 `Byte[13]` 的逐字节 XOR，结果写入 `Byte[14]`。
- 每一帧始终包含完整的雷达参数字段；`CMDNum` 决定本帧是否同时执行转台动作。

### 3.2 字节布局

| 字节偏移 | 字段 | 类型/字节序 | 取值和现行含义 |
| --- | --- | --- | --- |
| 0 | `HeadFlag` | `uint8` | 固定 `0xA5`。 |
| 1 | `CMDNum` | `uint8` | 转台命令，见下表。 |
| 2-3 | `Azimuth` | `uint16`，小端 | 位置模式方位，单位为角度乘 100。 |
| 4 | `RangeVal` | `uint8` | 量程编号 `0..14`。 |
| 5 | `Gain` | `uint8` | 波束锐化：UI 当前使用 `0=关，1=低，2=中，3=高`。 |
| 6 | `GanRao` | `uint8` | 同频干扰抑制：UI 当前使用 `0=关，1=低，2=中，3=高`。 |
| 7 | `Level` | `uint8` | 截位/电平选择；`0=自动`，其余值由手动滑块传递。 |
| 8 | `SeaVal` | `uint8` | 海杂波；`0=自动`，其余值由手动滑块传递。 |
| 9 | `RainVal` | `uint8` | 雨雪杂波；`0=自动`，其余值由手动滑块传递。 |
| 10 | `CFAR` | `uint8` | 保留。显控当前固定为 `0`。 |
| 11 | `TXCtrl` | `uint8` | 发射控制：`0=关`，非零=开；显控下发 `0` 或 `1`。 |
| 12 | `Servo` | `uint8` | 转速档位 `0..8`。 |
| 13 | `MTD` | `uint8` | 保留。显控当前固定为 `0`。 |
| 14 | `CheckSum` | `uint8` | `Byte[0..13]` 的 XOR。 |
| 15 | `TailFlag` | `uint8` | 固定 `0x5A`。 |

### 3.3 `CMDNum` 定义

| 值 | 名称 | 设备侧预期动作 |
| --- | --- | --- |
| `0x00` | ParamsOnly | 仅更新雷达参数，不执行转台 UART 命令。 |
| `0x01` | Position | 更新雷达参数，并按 `Azimuth` 进入位置模式。 |
| `0x02` | Speed | 更新雷达参数，并按 `Servo` 档位进入速度模式。 |
| `0x03` | Stop | 更新雷达参数，并停止转台。 |
| `0x04` | Start | 更新雷达参数，并启动转台。 |

显控界面将 `0x00`、`0x01`、`0x02` 放在命令下拉框中；`0x03` 和 `0x04` 通过“停止”和“启动”按钮下发。无论命令为何值，显控都会构造并发送完整 16 字节帧。

### 3.4 方位和转速换算

**控制帧方位不是 8192 方位编号。** 控制下发仍使用“角度乘 100”：

```text
azimuthRaw = round(normalize(angleDeg, 0..360) * 100)
angleDeg   = azimuthRaw / 100.0
```

例如 `180.00 deg`：

```text
azimuthRaw = 18000 = 0x4650
Byte[2] = 0x50
Byte[3] = 0x46
```

`Servo` 为速度档位，代码中定义：

```text
rpm = min(Servo, 8) * 6.00
```

| `Servo` | 转速 |
| --- | --- |
| 0 | 0.00 rpm |
| 1 | 6.00 rpm |
| 2 | 12.00 rpm |
| 3 | 18.00 rpm |
| 4 | 24.00 rpm |
| 5 | 30.00 rpm |
| 6 | 36.00 rpm |
| 7 | 42.00 rpm |
| 8 | 48.00 rpm |

### 3.5 量程编号

当前代码只定义 `0..14` 共 15 档；超出该范围的回波帧会被丢弃，控制界面也不会产生超出范围的编号。

| 编号 | 最大显示量程 |
| --- | --- |
| 0 | 300 m |
| 1 | 500 m |
| 2 | 750 m |
| 3 | 1 km |
| 4 | 1.5 km |
| 5 | 2 km |
| 6 | 3 km |
| 7 | 4 km |
| 8 | 6 km |
| 9 | 10 km |
| 10 | 15 km |
| 11 | 30 km |
| 12 | 40 km |
| 13 | 60 km |
| 14 | 75 km |

### 3.6 控制帧示例

仅更新雷达参数，量程 4 km、发射开、其余参数为 0：

```text
A5 00 00 00 07 00 00 00 00 00 00 01 00 00 A3 5A
```

设置 3 档速度，同时下发同一组雷达参数：

```text
A5 02 00 00 07 00 00 00 00 00 00 01 03 00 A2 5A
```

该帧中的 `Servo=3` 对应 `18.00 rpm`。控制 Socket 的 `writeDatagram()` 成功只表示显控已将 UDP 数据交给本机网络栈；当前协议实现**没有控制确认/应答帧**，不能据此确认设备已执行。

## 4. 接口 4：雷达/伺服 -> 显控回波帧

### 4.1 基本约定

- 传输层：UDP。
- 完整逻辑帧固定头长度：**22 字节**，末尾还有 2 字节逻辑帧尾 `00 6B`。
- 逻辑帧总长度严格为 `24 + fftWordCount * 4` 字节。
- 大 FFT 逻辑帧在线上采用 24 字节应用层分片头传输；单个 UDP 数据报最大 1400 字节。
- 方位字段为 **8192 分辨率方位编号**，小端。
- `fftDataLen`、`packetNum`、`reserved` 均为大端。
- 每个距离单元为一个 **4 字节小端无符号幅值**。

### 4.2 字节布局

| 字节偏移 | 字段 | 字节序 | 现行解析规则 |
| --- | --- | --- | --- |
| 0 | `leadByte` | - | 必须为 `0x00`。 |
| 1 | `headFlag` | - | 必须为 `0xA5`。 |
| 2-3 | `aziLow`、`aziHigh` | 小端 | 方位编号 `bin = Byte[2] + Byte[3] * 256`。 |
| 4 | `style` | - | 仅接受 `0x00`、`0x01`、`0x02`。 |
| 5 | `hdrChecksum` | - | `Byte[0..4]` 的 XOR。 |
| 6 | `hdrTail` | - | 必须为 `0x5A`。 |
| 7 | `rangeCode` | - | 当前量程编号，必须为 `0..14`。 |
| 8 | `txState` | - | 非零视为发射开。 |
| 9 | `gain` | - | 当前增益状态。 |
| 10 | `level` | - | 当前电平/截位状态。 |
| 11 | `seaVal` | - | 当前海杂波状态。 |
| 12 | `rainVal` | - | 当前雨杂波状态。 |
| 13 | `ganRao` | - | 当前同频干扰抑制状态。 |
| 14 | `freqStatus` | - | 当前频率状态。 |
| 15 | `statusChecksum` | - | `Byte[7..14]` 的逐字节累加和低 8 位。 |
| 16-17 | `fftDataLenHigh`、`fftDataLenLow` | 大端 | `fftWordCount`，单位为 4 字节距离单元。 |
| 18-19 | `packetNumHigh`、`packetNumLow` | 大端 | 包序号。当前显控记录并传递该值，不用其做丢包重排。 |
| 20-21 | `reservedHigh`、`reservedLow` | 大端 | 保留。 |
| 22 起 | 回波幅值 | 每单元 4 字节小端 | 共 `fftWordCount` 个距离单元。 |
| FFT 数据后 2 字节 | `FrameTail` | - | 固定 `00 6B`；必须以 UDP 实际长度定位，不在 FFT 区搜索。 |

### 4.3 方位换算和显示映射

设备上报的方位编号定义为：

```text
bin      = Byte[2] + Byte[3] * 256
angleDeg = (bin mod 8192) * 360 / 8192
```

例：`Byte[2..3] = 00 08`，则 `bin=2048`，对应 `90.00 deg`。

PPI 渲染器内部为 4096 方位，因此显控进一步换算：

```text
renderIndex = ((bin mod 8192) * 4096) / 8192
```

即 8192 协议方位的相邻两个编号会映射到同一个 4096 渲染方位。该映射只影响显控内部存储和绘制，不改变设备协议格式。

### 4.4 幅值和长度

```text
echoBytes    = fftWordCount * 4
expectedSize = 22 + echoBytes + 2
```

现行代码接受的 `fftWordCount` 为 1 到 65535，因此：

```text
4 <= echoBytes <= 262140
echoBytes % 4 == 0
```

因此有效距离单元数为 **1 到 65535**。第 `i` 个单元的原始幅值按小端读取：

```text
magnitude = b0 + (b1 << 8) + (b2 << 16) + (b3 << 24)
displayAmplitude = min(magnitude, 255)
```

显控渲染缓存目前是 8 位幅值，故大于 `255` 的原始值会饱和为 `255`；这不是网络报文截断，而是显示端的幅值压缩行为。

### 4.5 接收校验和拒收规则

显控会直接丢弃以下回波报文：

1. 完整逻辑帧总长度小于 24 字节。
2. `leadByte`、`headFlag`、`hdrTail` 或头部 XOR 校验不正确。
3. `style` 不属于 `0x00`、`0x01`、`0x02`。
4. `rangeCode` 不属于 `0..14`。
5. `statusChecksum` 不是 Byte[7..14] 的累加和低 8 位。
6. `fftDataLen` 为 0，或 UDP 实际长度不严格等于 `22 + fftWordCount * 4 + 2`。
7. 逻辑帧最后两个字节不是 `00 6B`。

方位编号超过 8191 时，当前代码采用 `mod 8192` 回绕后继续绘制。

### 4.6 应用层分片传输

PS 端先构造包含 `00 6B` 帧尾的完整逻辑帧，再将其切分为多个 UDP 数据报。每个 UDP 数据报为 `24 字节分片头 + FragmentData`，总长度不得超过 1400 字节，单片数据最大 1376 字节。

| 偏移 | 字段 | 字节序 | 显控校验/处理 |
| --- | --- | --- | --- |
| 0-1 | `Magic` | - | 固定 ASCII `FR`，即 `46 52`。 |
| 2 | `Version` | - | 必须为 `0x01`。 |
| 3 | `Flags` | - | Bit0=首片、Bit1=尾片，其余位必须为 0。 |
| 4-5 | `FrameSeq` | 大端 | 同一逻辑帧的所有分片相同；范围循环 `0..65535`。 |
| 6-7 | `FragmentIndex` | 大端 | 从 0 开始，必须小于 `FragmentCount`。 |
| 8-9 | `FragmentCount` | 大端 | 总分片数，必须大于 0。 |
| 10-13 | `TotalDataLen` | 大端 | 分片前完整逻辑帧长度；显控限制为不大于 262164 字节。 |
| 14-17 | `DataOffset` | 大端 | 当前分片在完整逻辑帧中的字节偏移。 |
| 18-19 | `FragmentLen` | 大端 | 当前有效数据长度，`0..1376`。 |
| 20-23 | `Checksum` | 大端 | `CRC-32/ISO-HDLC(Byte[0..19] + FragmentData)`。 |
| 24 起 | `FragmentData` | 原始字节顺序 | 完整逻辑帧的一段。 |

显控允许乱序到达，按 `FrameSeq + FragmentIndex + DataOffset` 重组。重复片保留首份；不同片段出现字节区域重叠、同一 `FrameSeq` 的总长度或片数不一致、CRC 错误，都会丢弃该逻辑帧。重组状态超过 **100 ms** 未收齐也会整体丢弃，不绘制残缺 FFT 数据。

仅当全部片段收齐、字节覆盖完整、逻辑帧头/状态校验/长度/帧尾全部通过后，显控才发布状态并将回波交给 PPI。

## 5. 状态反馈与显示侧行为

每一条**已完整重组且校验通过**的回波线都携带状态字段。显控将 `rangeCode`、`txState`、`gain`、`level`、`seaVal`、`rainVal`、`ganRao` 和 `freqStatus` 提取为运行状态；只有其中任一值变化时才向界面发布状态更新。

有效回波线随后被发送给 `EchoRenderer::updateEchoLine()`。以下项目属于显示端行为，而不是外部 UDP 字段：

| 配置 | 作用 | 默认值 |
| --- | --- | --- |
| `marine.display.echo_decay_ms` | 回波余辉时长 | `10000` ms |
| `marine.display.colormap` | 色图 | `simrad` |
| `marine.display.sweep_history_rounds` | 同一显示方位保留的扫描圈数 | `1`，范围 `1..8` |
| `marine.display.echo_debug` | 接收和历史圈调试日志 | `false` |

量程切换会同步 PPI 几何范围和回波渲染范围。回波帧中的 `rangeCode` 同时用于确定该条回波数据对应的物理量程；因此设备在量程切换后应在上报帧中及时携带新的 `rangeCode`。

分片模式开始收到数据后，显控每 5 秒输出一条 `FRAGMENT-STATS`：已启动/已结算/已完成/超时/丢弃帧数，以及已结算分片的期望、收到、缺失数量和缺失率。该汇总不依赖 `echo_debug`；超时行还会列出最多 16 个缺失的 `FragmentIndex`。

显控还会在有效回波方位编号跨越 `8191 -> 0` 或 `0 -> 8191` 时输出一条 `SWEEP-STATS`。`durationMs` 和 `lineRateHz` 分别给出该圈耗时及实际观察到的逻辑回波线速率；`validLines` 是完整重组并通过校验的逻辑回波帧数，`droppedLines` 是该圈期间分片超时或主动丢弃的逻辑帧数。`sourceExpectedLines=not-declared` 表示当前协议没有声明每圈必须上报多少条回波线；`azimuthBinCapacity=8192` 只表示方位编号的地址空间。`uniqueAzimuthBins` 和 `azimuthCoverage` 分别表示去重后的实际方位覆盖。软件刚接入数据后的首圈会标记 `partial=1`，后续回绕完成的圈为 `partial=0`。

## 6. 与分片改造前代码的主要差异

| 项目 | 当前代码实现 | 变更影响 |
| --- | --- | --- |
| 控制帧 | 仍为 16 字节，未修改。 | PS 端和显控无需为控制方向切换分片。 |
| 回波逻辑帧 | 22 字节头 + `4*N` FFT 数据 + `00 6B` 帧尾。 | 旧的无帧尾回波不会被新显控接收。 |
| 状态校验 | Byte[7..14] 累加和低 8 位。 | 不再接受旧代码使用的 XOR。 |
| FFT 长度 | `N` 允许到 65535。 | 支持约 62 KB 及更大的逻辑回波帧。 |
| UDP 承载 | 使用 `FR` 分片头、CRC32 和 100 ms 重组超时。 | PS 与显控必须同步升级，不能只升级一端。 |
| 控制确认 | 当前显控没有协议级 ACK 解析。 | “发送成功”仅代表本地 UDP 写入成功。 |

## 7. 联调检查清单

1. 确认显控绑定的 `local_ip` 是实际网卡地址，而非另一网段或不存在的地址。
2. 设备应向 `local_ip:echo_port` 发送回波；控制帧目标为 `servo_ip:servo_port`。
3. PS 端必须先生成 `00 A5 <aziLow> <aziHigh> <style> <xor> 5A` 开头、`00 6B` 结尾的完整逻辑帧，再做分片；不得在 FFT 区搜索 `00 6B`。
4. `fftDataLen` 必须使用大端，并表示 4 字节距离单元的数量，不是字节数量。
5. `statusChecksum` 是 Byte[7..14] 的累加和低 8 位；回波载荷中的每个幅值仍是小端 4 字节整数。
6. 每个分片必须含 `FR`、版本 `0x01`、正确的 Big-Endian 元数据和 CRC-32/ISO-HDLC；最大 UDP 数据报 1400 字节。
7. 每个有效回波帧应携带正确的 `rangeCode`；设备量程切换后不可继续长期上报旧量程编号。
8. 控制方向的 `Azimuth` 与回波方向的方位字段编码不同：前者为角度乘 100，后者为 8192 方位编号。

## 8. 代码定位

- 报文结构、常量、校验和单位：`Basic/MarineProtocol.h`
- UDP 绑定、控制发送、CRC 校验、分片重组与回波解析：`Controller/MarineRadarManager.cpp`
- 网络和初始控制参数读取：`Controller/controller.cpp`、`config.toml`
- 界面控制命令与量程/速度取值：`mainPanel/mainoverlayout.cpp`
- 回波交给 PPI 渲染器的连接：`PolarDisp/ppisscene.cpp`
