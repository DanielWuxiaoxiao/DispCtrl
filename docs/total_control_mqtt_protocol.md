# X576 目标识别结果 MQTT 接入协议

## 1. 概述

本文档定义 X576 雷达系统向总控终端发布目标识别结果的 MQTT 通信协议。

总控终端作为 MQTT 订阅端，订阅指定 Topic 后接收每个航迹目标的雷达识别结果和光电识别结果。雷达侧不做融合判决，总控端按业务策略自行融合。

## 2. 通信参数

| 参数 | 默认值 | 说明 |
|---|---:|---|
| 协议 | MQTT 3.1.1 | TCP 长连接 |
| Broker IP | `192.168.1.30` | 总控侧提供或现场协商 |
| Broker Port | `1883` | MQTT 标准端口，可协商 |
| Client ID | `DispCtrl-X576` | 发布端客户端 ID |
| Topic | `x576/target/result` | 总控端订阅该 Topic |
| QoS | `0` | 最多一次投递 |
| Retain | `false` | 不保留历史消息 |
| 编码 | JSON UTF-8 | 每条 MQTT payload 为一个 JSON 对象 |
| 发布周期 | `4000 ms` | 周期性发布当前目标结果 |

## 3. 消息规则

- 每个目标单独发布一条 MQTT 消息。
- 同一目标在多次发布中保持相同 `track_id`。
- `radar` 字段承载雷达侧目标信息和雷达侧识别结果。
- `optical` 字段承载边缘计算终端/光电系统回传的识别结果。
- `radar` 和 `optical` 可能不一致，总控端不要假定二者已经融合。
- 尚未收到光电结果时，`optical` 为 `null`。
- 如果只收到光电结果但当前没有对应雷达目标缓存，`radar` 为 `null`，总控端仍可按 `track_id` 保存该光电结果。

## 4. Payload 示例

```json
{
  "type": "target_recognition",
  "source": "x576",
  "timestamp_ms": 1780000000000,
  "track_id": "12",
  "radar": {
    "target_type": 1,
    "azimuth": 35.4,
    "elevation": 4.1,
    "distance": 1280.0,
    "speed": 12.3
  },
  "optical": {
    "is_drone": true,
    "count": 1,
    "detections": [
      {
        "class": "drone",
        "confidence": 0.91,
        "bbox": [120, 80, 300, 250]
      }
    ],
    "timestamp": 1710000000.0,
    "received_timestamp_ms": 1780000000000
  }
}
```

无光电结果时：

```json
{
  "type": "target_recognition",
  "source": "x576",
  "timestamp_ms": 1780000000000,
  "track_id": "12",
  "radar": {
    "target_type": 1,
    "azimuth": 35.4,
    "elevation": 4.1,
    "distance": 1280.0,
    "speed": 12.3
  },
  "optical": null
}
```

仅有光电结果时：

```json
{
  "type": "target_recognition",
  "source": "x576",
  "timestamp_ms": 1780000000000,
  "track_id": "12",
  "radar": null,
  "optical": {
    "is_drone": false,
    "count": 0,
    "detections": [],
    "timestamp": 1710000000.0,
    "received_timestamp_ms": 1780000000000
  }
}
```

## 5. 顶层字段

| 字段 | 类型 | 必填 | 单位 | 说明 |
|---|---|---|---|---|
| `type` | string | 是 | - | 固定为 `target_recognition` |
| `source` | string | 是 | - | 固定为 `x576` |
| `timestamp_ms` | number | 是 | ms | MQTT 消息发布时间，Unix 毫秒时间戳 |
| `track_id` | string | 是 | - | 航迹编号，同一目标保持不变 |
| `radar` | object/null | 是 | - | 雷达侧目标信息和识别结果 |
| `optical` | object/null | 是 | - | 光电系统识别结果 |

## 6. radar 字段

| 字段 | 类型 | 必填 | 单位 | 说明 |
|---|---|---|---|---|
| `target_type` | int | 是 | - | 雷达侧目标类型，见目标类型枚举 |
| `azimuth` | number | 是 | 度 | 目标方位角，正北 0 度，顺时针增加 |
| `elevation` | number | 是 | 度 | 目标俯仰角 |
| `distance` | number | 是 | m | 目标斜距 |
| `speed` | number | 是 | m/s | 目标速度 |

## 7. optical 字段

| 字段 | 类型 | 必填 | 单位 | 说明 |
|---|---|---|---|---|
| `is_drone` | bool | 是 | - | 光电系统是否识别到无人机 |
| `count` | int | 是 | - | 光电检测目标数量 |
| `detections` | array | 是 | - | 光电检测框列表，无目标时为空数组 |
| `timestamp` | number | 是 | s | 光电系统生成识别结果的 Unix 秒时间戳 |
| `received_timestamp_ms` | number | 是 | ms | X576 收到该光电结果的 Unix 毫秒时间戳 |

`detections` 中每个对象字段：

| 字段 | 类型 | 必填 | 单位 | 说明 |
|---|---|---|---|---|
| `class` | string | 是 | - | 光电识别类别，例如 `drone` |
| `confidence` | number | 是 | - | 光电识别置信度，范围 `0.0-1.0` |
| `bbox` | array | 是 | 像素 | 识别框 `[x1, y1, x2, y2]`，基于图像左上角坐标系 |

## 8. 目标类型枚举

| 值 | 类型 | 说明 |
|---:|---|---|
| `0` | 鸟 | Bird |
| `1` | 无人机 | Drone |

## 9. 总控端订阅示例

```bash
mosquitto_sub -h 192.168.1.30 -p 1883 -t x576/target/result -v
```

## 10. 处理建议

- 总控端按 `track_id` 维护目标状态。
- 当 `radar` 和 `optical` 都存在时，总控端进行融合判决。
- 当 `optical` 为 `null` 时，表示当前只有雷达侧结果。
- 当 `radar` 为 `null` 时，表示该消息只转发了光电侧结果，总控端可暂存并等待同一 `track_id` 的雷达消息。
