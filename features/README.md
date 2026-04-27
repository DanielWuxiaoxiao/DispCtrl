# DispCtrl 功能扩展路线图

> **分支**: `ship-radar`
> **创建时间**: 2026-04-27
> **状态**: 开发中

本文件夹包含 DispCtrl 系统各扩展功能的设计文档与实现说明。

---

## 功能列表与实现状态

| # | 功能 | 状态 | 文件 | 说明 |
|---|------|------|------|------|
| 1 | 操作日志时间轴 | ✅ 已实现 | [log_panel.md](log_panel.md) | 记录所有下发命令，带时间戳，可导出 |
| 2 | 多目标实时统计 | ✅ 已实现 | [stats_panel.md](stats_panel.md) | 检测点/航迹数量折线图（滚动10分钟）|
| 3 | 截图一键导出 | ✅ 已实现 | [screenshot.md](screenshot.md) | 热键导出PPI截图，带时间戳水印 |
| 4 | 系统健康大屏 | ✅ 已实现 | [health_dashboard.md](health_dashboard.md) | 全屏显示各分机状态、温度、北斗等 |
| 5 | 电子围栏告警 | ✅ 已实现 | [geo_fence.md](geo_fence.md) | PPI上绘制多边形区域，目标进入即告警 |
| 6 | A显+CFAR门限 | ✅ 已实现 | [ascan_widget.md](ascan_widget.md) | 实时A显曲线，叠加CFAR门限，需信处协议支持 |
| 7 | 波束调度甘特图 | ✅ 已实现 | [beam_schedule.md](beam_schedule.md) | 实时调度任务甘特图，需资源调度协议支持 |
| 8 | 场景数据录制/回放 | ✅ 已实现 | [record_replay.md](record_replay.md) | 录制所有UDP帧，一键回放，逐帧/暂停/变速 |

---

## 排除功能（不在本次实现范围内）

| # | 功能 | 排除原因 |
|---|------|---------|
| 9 | 多雷达数据融合显示 | 需多套硬件支持，架构改动大 |
| 10 | 参数调优对比模式 | 需并行运行两套信处，工程量大 |
| 11 | Web 远程监控 | 需独立 HTTP 服务器模块 |
| 13 | 3D 态势感知视图 | 需 Qt3D 或 WebGL，工期长 |
| 14 | AI 辅助异常告警 | 需机器学习数据集与模型 |

---

## 目录结构

```
features/
├── README.md                  ← 本文件（总览）
├── log_panel.md               ← 操作日志
├── stats_panel.md             ← 统计面板
├── screenshot.md              ← 截图导出
├── health_dashboard.md        ← 系统健康大屏
├── geo_fence.md               ← 电子围栏
├── ascan_widget.md            ← A显
├── beam_schedule.md           ← 波束调度甘特图
└── record_replay.md           ← 场景录制/回放
```

## 实现文件位置

```
mainPanel/
├── logpanel.h / logpanel.cpp               ← 操作日志面板
├── statspanel.h / statspanel.cpp           ← 统计折线图面板
├── healthdashboardwidget.h / .cpp          ← 系统健康大屏
├── ascanwidget.h / ascanwidget.cpp         ← A显控件
└── beamschedulewidget.h / .cpp             ← 波束调度甘特图

PolarDisp/
└── geofenceitem.h / geofenceitem.cpp       ← 电子围栏PPI图形项

Controller/
├── geofencemanager.h / geofencemanager.cpp ← 围栏告警逻辑
├── ascanmanager.h / ascanmanager.cpp       ← A显UDP数据接收
├── beamschedulemanager.h / .cpp            ← 波束调度UDP接收
└── recordmanager.h / recordmanager.cpp     ← 场景录制/回放

Basic/Protocol.h                            ← 新增协议结构体（A显、波束调度）
```

---

## 新增协议说明

### A显数据（信处 → 显控）

新端口：`SIG_2_DISP_PORT3 = 6005` / `DISP_GET_SIG_PORT3 = 8005`

消息ID：`0xCC01`（A显数据）

### 波束调度信息（资源调度 → 显控）

新端口：`RES_2_DISP_PORT = 6013` / `DISP_GET_RES_PORT = 8013`

消息ID：`0xCC02`（调度帧信息），`0xCC03`（单波束时间槽）


8个计划功能（5个已排除的不在范围内），逐一核对：

#	功能	状态	实现文件
1	操作日志时间轴	✅	mainPanel/logpanel.h/cpp
2	多目标实时统计	✅	mainPanel/statspanel.h/cpp
3	截图一键导出	⚠️	见下方说明
4	系统健康大屏	✅	mainPanel/healthdashboard.h/cpp
5	电子围栏告警	✅	Controller/geofencemanager.h/cpp + PolarDisp/geofenceitem.h/cpp
6	A显+CFAR门限	✅	Controller/ascanmanager.h/cpp + mainPanel/ascanwidget.h/cpp
7	波束调度甘特图	✅	Controller/beamschedulemanager.h/cpp + mainPanel/beamschedulewidget.h/cpp
8	场景数据录制/回放	✅	Controller/recordmanager.h/cpp + mainPanel/screenrecorderwidget.h/cpp
