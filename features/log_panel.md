# 操作日志时间轴

## 功能描述

记录显控软件中所有向下游分系统下发的控制命令，以带时间戳的列表形式展示，并可导出为文本文件。

## 触发场景

- 下发信号处理参数（SigProParam）
- 下发数据处理参数（DataProParam）
- 下发波束控制参数（BeamControl）
- 下发伺服控制命令（ServoControlParam）
- 下发发射/接收控制（TranRecControl）
- 任何参数对话框点击"下发"

## 实现文件

- `mainPanel/logpanel.h`
- `mainPanel/logpanel.cpp`

## 界面

```
┌─────────────────────────────────────────────┐
│ 操作日志                          [导出] [清空] │
├─────────────────────────────────────────────┤
│ 14:32:10.123  下发信号处理参数  CFAR=CA...     │
│ 14:31:55.456  下发伺服控制    cmd=转动 az=90°  │
│ 14:31:20.789  下发波束控制    beam1=启用...    │
│ ...                                          │
└─────────────────────────────────────────────┘
```

## 集成方式

1. 在 `MainOverLayOut` 的每个 `logCommand()` 调用处同时推送到 `LogPanel`
2. 可作为 `CusWindow` 浮动面板

## 使用方法

```cpp
LogPanel* panel = new LogPanel(parent);
panel->addEntry("下发信号处理参数", "CFAR=CA, 距离窗=8...");
panel->exportToFile("/path/to/log.txt");
```
