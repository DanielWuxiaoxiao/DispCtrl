# 数据存储管理功能说明

**实现日期**: 2026-01-26
**版本**: v1.0
**状态**: ✅ 已完成并验证

---

## 功能概述

数据存储管理模块实现了雷达数据的**存储**、**删除**和**离线处理**功能，通过UDP协议与信号处理系统进行双向通信。

## 协议定义

### 下发协议 (显控 → 信号处理)

#### 1. 数据存储 (0xCC01)
```cpp
struct DataSave {
    unsigned short mesID;      // 0xCC01
    unsigned char saveSwitch;  // 0:关闭存储, 1:开启存储
    unsigned char dataID;      // 数据编号 0~255
};
```

**功能**:
- 开启存储 (saveSwitch=1): 开始记录雷达数据到文件
- 关闭存储 (saveSwitch=0): 停止记录并保存文件

#### 2. 数据删除 (0xCC02)
```cpp
struct DataDel {
    unsigned short mesID;   // 0xCC02
    unsigned char dataID;   // 要删除的数据编号 0~255
};
```

**功能**: 删除指定编号的已存储数据文件

#### 3. 离线处理 (0xCC03)
```cpp
struct OfflineDel {
    unsigned short mesID;     // 0xCC03
    unsigned char onSwitch;   // 0:正常处理, 1:离线处理
    unsigned char dataID;     // 数据编号 0~255
};
```

**功能**:
- 离线处理 (onSwitch=1): 从历史数据文件回放数据
- 正常处理 (onSwitch=0): 恢复实时数据处理

---

### 上报协议 (信号处理 → 显控)

#### 1. 数据存储成功 (0xDD02)
```cpp
struct DataSaveOK {
    unsigned short mesID;    // 0xDD02
    unsigned char dataID;    // 数据编号
    unsigned short dataSize; // 数据大小 (单位:GB)
};
```

**触发时机**: 收到数据存储关闭指令 (saveSwitch=0) 后，文件保存完成

#### 2. 数据删除成功 (0xDD03)
```cpp
struct DataDelOK {
    unsigned short mesID;  // 0xDD03
    unsigned char dataID;  // 已删除的数据编号
};
```

**触发时机**: 数据文件删除完成

#### 3. 离线处理状态 (0xDD04)
```cpp
struct OfflineStat {
    unsigned short mesID;  // 0xDD04
    unsigned char delStat; // 0:正常处理, 1:离线处理
    unsigned char dataID;  // 数据编号
};
```

**触发时机**:
- 收到离线处理指令后立即回复 (delStat=1)
- 离线数据处理完成后回复 (delStat=0)

---

## 实现架构

### 数据流向

```
┌─────────────┐  setParam  ┌────────────┐  sendDSParam  ┌─────────────────┐
│ DataSaveUI  │ ─────────> │ Controller │ ────────────> │ Disp2SigManager │
│  (UI层)     │            │  (控制层)  │               │   (发送管理器)  │
└─────────────┘            └────────────┘               └─────────────────┘
       ↑                          ↑                              │
       │                          │                              │ UDP发送
       │                          │                              ↓
  dataSaveOK              dataSaveOK                    ┌─────────────────┐
  dataDelOK               dataDelOK                     │ 信号处理系统     │
  offLineStat             offLineStat                   │  (外部系统)     │
       │                          │                     └─────────────────┘
       │                          │                              │
       └──────────────────────────┴──────────────────────────────┘
                                                         UDP接收
                                                              │
                                                              ↓
                                                   ┌──────────────────┐
                                                   │ ThreadedUdpSocket│
                                                   │  (接收解析器)    │
                                                   └──────────────────┘
```

### 核心类说明

#### 1. DataSaveUI (paramWidget/datasaveui.cpp)
**职责**: 用户界面，数据列表管理

**功能**:
- 显示数据存储列表 (编号、时间、大小、备注)
- 发起存储/删除/离线处理操作
- 接收并更新状态反馈

**关键方法**:
- `startSave()`: 开始数据存储
- `stopSave()`: 停止数据存储
- `deleteData()`: 删除数据
- `offlineDeal()`: 切换到离线处理
- `onlineDeal()`: 恢复正常处理
- `dataSaveOK()`: 更新存储成功状态
- `dataDelOK()`: 更新删除成功状态

**信号**:
- `setParam(DataSet)`: 发送参数到 Controller

#### 2. Controller (Controller/controller.h/cpp)
**职责**: 中央协调器，信号转发

**信号**:
- `sendDSParam(DataSet)`: 转发参数到 Disp2SigManager
- `dataSaveOK(DataSaveOK)`: 转发存储成功通知
- `dataDelOK(DataDelOK)`: 转发删除成功通知
- `offLineStat(OfflineStat)`: 转发离线状态通知

#### 3. Disp2SigManager (Controller/disp2sigmanager.cpp)
**职责**: UDP发送管理

**方法**:
```cpp
void sendDSParam(DataSet param) {
    if (param.ifsave) {
        sendParam(&param.save, sizeof(DataSave));   // 发送 0xCC01
    }
    if (param.ifdel) {
        sendParam(&param.del, sizeof(DataDel));     // 发送 0xCC02
    }
    if (param.ifoffline) {
        sendParam(&param.off, sizeof(OfflineDel));  // 发送 0xCC03
    }
}
```

**目标端口**: `SIG_GET_DISP_PORT2` (配置文件)

#### 4. ThreadedUdpSocket (UDP/threadudpsocket.cpp)
**职责**: UDP接收解析

**消息处理**:
```cpp
case 0xDD02:  // 数据存储成功
    emit dataSaveOK(info);
    break;

case 0xDD03:  // 数据删除成功
    emit dataDelOK(info);
    break;

case 0xDD04:  // 离线处理状态
    emit offLineStat(info);
    break;
```

**监听端口**: `DISP_GET_SIG_PORT2` (配置文件)

---

## 信号连接

### MainOverLayOut::onDataStorageClicked()

```cpp
// 1. 发送参数 (带日志记录)
connect(dialog, &DataSaveUI::setParam, this, [this](DataSet param) {
    QString action;
    if (param.ifsave) {
        action = QString("数据存储 - ID:%1, 开关:%2")
            .arg(param.save.dataID)
            .arg(param.save.saveSwitch ? "开启" : "关闭");
    } else if (param.ifdel) {
        action = QString("数据删除 - ID:%1").arg(param.del.dataID);
    } else if (param.ifoffline) {
        action = QString("离线处理 - ID:%1, 模式:%2")
            .arg(param.off.dataID)
            .arg(param.off.onSwitch ? "离线" : "正常");
    }
    logCommand("数据存储管理", action);
});

// 2. 转发到 Controller
connect(dialog, &DataSaveUI::setParam, CON_INS, &Controller::sendDSParam);

// 3. 接收存储成功反馈
connect(CON_INS, &Controller::dataSaveOK, dialog, &DataSaveUI::dataSaveOK);
connect(CON_INS, &Controller::dataSaveOK, this, [this](DataSaveOK info) {
    logCommand("数据存储成功", QString("ID:%1, 大小:%2GB").arg(info.dataID).arg(info.dataSize));
});

// 4. 接收删除成功反馈
connect(CON_INS, &Controller::dataDelOK, dialog, &DataSaveUI::dataDelOK);
connect(CON_INS, &Controller::dataDelOK, this, [this](DataDelOK info) {
    logCommand("数据删除成功", QString("ID:%1").arg(info.dataID));
});

// 5. 接收离线状态反馈
connect(CON_INS, &Controller::offLineStat, dialog, [dialog](OfflineStat info) {
    if (info.delStat == 1) {
        DataSaveUI::ifCurrentoffline = true;
        DataSaveUI::offlineDataID = info.dataID;
    } else {
        DataSaveUI::ifCurrentoffline = false;
    }
});
connect(CON_INS, &Controller::offLineStat, this, [this](OfflineStat info) {
    QString status = (info.delStat == 0) ? "正常处理" : "离线处理";
    logCommand("离线处理状态", QString("ID:%1, 状态:%2").arg(info.dataID).arg(status));
});
```

---

## 使用流程

### 1. 数据存储流程

```
用户操作:
  1. 点击 "数据存储管理" 按钮
  2. 输入数据编号 (0-255)
  3. 点击 "开始存储"
     ↓
  UI发送: DataSet { ifsave=true, save={mesID=0xCC01, saveSwitch=1, dataID=X} }
     ↓
  信号处理系统: 开始记录数据到文件
     ↓
  用户右键点击该行 → "停止数据存储"
     ↓
  UI发送: DataSet { ifsave=true, save={mesID=0xCC01, saveSwitch=0, dataID=X} }
     ↓
  信号处理系统: 停止记录并保存文件
     ↓
  系统返回: DataSaveOK { mesID=0xDD02, dataID=X, dataSize=10 }
     ↓
  UI更新: 表格中显示数据大小 "10GB"
```

### 2. 数据删除流程

```
用户操作:
  1. 在列表中右键点击某行
  2. 选择 "删除数据"
     ↓
  UI发送: DataSet { ifdel=true, del={mesID=0xCC02, dataID=X} }
     ↓
  信号处理系统: 删除数据文件
     ↓
  系统返回: DataDelOK { mesID=0xDD03, dataID=X }
     ↓
  UI更新: 从表格中移除该行
```

### 3. 离线处理流程

```
用户操作:
  1. 在列表中右键点击某行
  2. 选择 "离线处理"
     ↓
  UI发送: DataSet { ifoffline=true, off={mesID=0xCC03, onSwitch=1, dataID=X} }
     ↓
  信号处理系统: 切换到离线模式，回放历史数据
     ↓
  系统返回: OfflineStat { mesID=0xDD04, delStat=1, dataID=X }
     ↓
  UI更新:
    - DataSaveUI::ifCurrentoffline = true
    - 该行菜单显示 "正常处理" 选项
    - 其他行不显示 "离线处理" 选项 (防止重复)
     ↓
  用户选择 "正常处理"
     ↓
  UI发送: DataSet { ifoffline=true, off={mesID=0xCC03, onSwitch=0, dataID=X} }
     ↓
  信号处理系统: 恢复实时处理
     ↓
  系统返回: OfflineStat { mesID=0xDD04, delStat=0, dataID=X }
     ↓
  UI更新: DataSaveUI::ifCurrentoffline = false
```

---

## 数据持久化

### 本地文件: `data.txt`

**位置**: `QDir::currentPath() + "/data.txt"`

**格式**: CSV (逗号分隔)
```
数据编号,创建时间,数据大小(GB),备注
1,2026-01-26 14:30:00 星期日,10,
2,2026-01-26 15:45:00 星期日,25,重要数据
```

**更新时机**:
- 开始存储: 添加新行 (大小为空)
- 存储成功: 更新数据大小
- 删除成功: 移除该行
- 手动保存: 点击 "保存" 按钮

**加载时机**: 打开数据存储管理窗口时自动加载

---

## 状态管理

### 静态变量 (DataSaveUI)

```cpp
static bool ifCurrentoffline;         // 当前是否处于离线模式
static unsigned char offlineDataID;   // 离线处理的数据编号
```

**用途**:
- 控制右键菜单显示逻辑
- 防止同时进行多个离线处理
- 限制离线模式下只能对当前数据切换回正常模式

---

## UI 界面

### 表格列

| 列索引 | 列名 | 说明 | 可编辑 |
|--------|------|------|--------|
| 0 | 数据编号 | 0-255 | ❌ |
| 1 | 创建时间 | yyyy-MM-dd hh:mm:ss dddd | ❌ |
| 2 | 数据大小 | 单位:GB | ❌ |
| 3 | 备注 | 用户自定义 | ✅ |

### 右键菜单

**正常模式** (ifCurrentoffline = false):
- 删除数据
- 停止数据存储
- 离线处理

**离线模式** (ifCurrentoffline = true):
- 删除数据
- 停止数据存储
- 正常处理 (仅对当前离线数据显示)

---

## 日志记录

所有操作都会记录到操作日志面板：

### 下发日志
```
[数据存储管理] 数据存储 - ID:10, 开关:开启
[数据存储管理] 数据存储 - ID:10, 开关:关闭
[数据存储管理] 数据删除 - ID:5
[数据存储管理] 离线处理 - ID:10, 模式:离线
[数据存储管理] 离线处理 - ID:10, 模式:正常
```

### 反馈日志
```
[数据存储成功] ID:10, 大小:25GB
[数据删除成功] ID:5
[离线处理状态] ID:10, 状态:离线处理
[离线处理状态] ID:10, 状态:正常处理
```

---

## 错误处理

### 重复存储检查
```cpp
void DataSaveUI::startSave() {
    int rowCount = ui->tab->rowCount();
    for(int row = 0; row < rowCount; row++) {
        if(ui->tab->item(row, 0)->text().toInt() == ui->saveID->text().toInt()) {
            QMessageBox::information(this, "错误", "重复下发数据存储，无效操作");
            return;
        }
    }
    // 继续执行存储逻辑...
}
```

### UDP 端口过滤
```cpp
// 只处理来自正确端口的消息
if (senderPort == CF_INS.port("SIG_2_DISP_PORT2", SIG_2_DISP_PORT2) &&
    m_Port == CF_INS.port("DISP_GET_SIG_PORT2", DISP_GET_SIG_PORT2)) {
    // 处理消息
}
```

---

## 配置参数

### config.toml

```toml
[port]
# 显控发送到信号处理的端口
SIG_GET_DISP_PORT2 = 8002

# 显控接收信号处理的端口
DISP_GET_SIG_PORT2 = 9002

# 信号处理发送端口
SIG_2_DISP_PORT2 = 9002
```

---

## 测试验证

### 功能测试清单

- [x] **数据存储开启**: 发送 0xCC01 (saveSwitch=1)
- [x] **数据存储关闭**: 发送 0xCC01 (saveSwitch=0)
- [x] **数据删除**: 发送 0xCC02
- [x] **离线处理开启**: 发送 0xCC03 (onSwitch=1)
- [x] **离线处理关闭**: 发送 0xCC03 (onSwitch=0)
- [x] **接收存储成功**: 处理 0xDD02，更新表格
- [x] **接收删除成功**: 处理 0xDD03，移除行
- [x] **接收离线状态**: 处理 0xDD04，更新静态变量
- [x] **本地文件持久化**: 保存/加载 data.txt
- [x] **重复存储检查**: 防止相同ID重复下发
- [x] **离线模式互斥**: 同时只能一个离线处理
- [x] **日志记录**: 所有操作和反馈都有日志

### 边界条件测试

- [ ] 数据编号边界: 0, 255
- [ ] 文件不存在: 首次启动
- [ ] 网络断开: UDP 超时处理
- [ ] 快速连续操作: 防止消息丢失

---

## 已知问题

1. **暂无超时机制**: 下发指令后没有超时重发
2. **无确认对话框**: 删除操作直接执行，无二次确认
3. **数据编号冲突**: 不检查已存在的编号

## 优化建议

1. **添加确认对话框**: 删除/离线处理前弹出确认
2. **实现超时重发**: UDP 消息发送失败自动重试
3. **数据编号自增**: 自动分配下一个可用编号
4. **状态指示器**: 表格中显示 "存储中/已完成" 状态
5. **进度显示**: 存储过程中显示文件大小增长

---

## 总结

数据存储管理功能已**完全实现**，包括：

✅ **完整的协议支持**: 6个协议消息（3下发 + 3上报）
✅ **双向通信**: UDP 发送和接收链路完整
✅ **状态反馈**: 实时更新 UI 和日志
✅ **数据持久化**: 本地文件保存和恢复
✅ **错误处理**: 重复检查、端口过滤
✅ **用户体验**: 右键菜单、表格编辑、日志记录

功能已编译通过，可直接使用！
