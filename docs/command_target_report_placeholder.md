# 指控无人机目标上报占位接口

本接口用于在对方正式协议尚未发布前，向指控系统连续上报本雷达已识别为无人机的**普通航迹**。TBD 和协同航迹不进入本模块。

## 配置与运行开关

配置段为 `[command_target_report]`：

```toml
enabled = true
transport = "udp"       # udp | tcp
payload = "json"        # json | binary
byte_order = "little"   # 仅 binary 生效；little | big
local_ip = "0.0.0.0"
local_port = 0
remote_ip = "127.0.0.1"
remote_port = 21002
tcp_reconnect_interval_ms = 3000
```

`enabled` 是启动默认值。主界面“参数设置”区的“指控无人机上报：开/关”只改变本次运行，不改写 `config.toml`；重启后恢复配置默认值。

健康管理窗口显示该链路状态：TCP 仅在已连接时显示正常；UDP 正常仅表示本地 socket 已绑定、发送已被本机网络栈接受，不代表对端已收到。

## 上报触发与时间语义

每收到一个普通航迹点，只要 `PointInfo.targetRecResult == 1`，或该批次已收到 `TargetClaRes.claRes == 1`，就立刻发送一次。分类结果晚于航迹点到达时，会立即补发缓存的最新点。

现有 X576 的 `PointInfo` 未携带原始航迹时间，因此 `track_timestamp_utc_ms` 是本显控收到该航迹点时的 UTC 毫秒；`report_timestamp_utc_ms` 是当前组包时的 UTC 毫秒。正式协议若要求雷达源时间，应仅在 `CommandTargetReporter::TimedTrack` 的赋值处替换时间来源。

经纬高由 `[radar]` 的经纬高和航迹的距离/方位/俯仰按现有 PPI 北向方位语义换算。该处不重复叠加 `radar.yaw`。

## JSON 占位载荷

TCP 下每条 JSON 后增加换行符作为分帧；UDP 一包一条。

```json
{
  "version": 1,
  "report_timestamp_utc_ms": 1760000000123,
  "track_timestamp_utc_ms": 1760000000100,
  "target_id": 15,
  "longitude_deg": 109.123922,
  "latitude_deg": 34.225249,
  "altitude_m": 781.5
}
```

## Binary 占位载荷

`payload = "binary"` 时为固定 52 字节记录，使用 `byte_order` 统一编码整数和 IEEE-754 `double`：

| Offset | Type | Field |
| --- | --- | --- |
| 0 | u32 | magic（小端在线字节为 `55 41 56 31`，即 `UAV1`） |
| 4 | u16 | version（1） |
| 6 | u16 | packet_size（52） |
| 8 | u64 | report_timestamp_utc_ms |
| 16 | u64 | track_timestamp_utc_ms |
| 24 | u32 | target_id |
| 28 | f64 | longitude_deg |
| 36 | f64 | latitude_deg |
| 44 | f64 | altitude_m |

TCP binary 为无分隔的连续 52 字节记录。对端协议确定后，优先仅修改 `Controller/commandtargetreportprotocol.h` 中的序列化函数；筛选、坐标换算、状态显示和传输管理保持不变。
