# 参数保存功能实现指南

## 概述

本文档说明如何为 `paramWidget` 目录下的参数配置窗口添加"保存参数到配置文件"功能。

## 已完成的示例

- ✅ **servocontrol** - 伺服控制参数保存功能已实现

## 功能说明

1. **保存到配置文件**：点击"保存参数到配置文件"按钮后，当前UI的参数会保存到 `config.toml`
2. **启动时加载**：下次打开程序时，参数窗口会自动从 `config.toml` 加载上次保存的参数作为默认值
3. **降级支持**：如果 `config.toml` 中没有相应参数，则使用 `Protocol.h` 中定义的硬编码默认值

## 实现步骤（以其他窗口为例）

### 步骤1：扩展 ConfigManager（如需要新参数类型）

已经在 `Basic/ConfigManager.h` 中添加了以下参数类型的支持：

- ✅ 伺服控制参数 (`params.servo.*`)
- ✅ 扫描范围参数 (`params.scanrange.*`)
- ✅ 波形控制参数 (`params.beamcontrol.*`)
- ✅ 信号处理参数 (`params.sigpro.*`)
- ✅ 数据处理参数 (`params.datapro.*`)
- ✅ 数据保存参数 (`params.datasave.*`)

如需添加新的参数类型，参考以下模式：

```cpp
// 保存方法
void saveXxxParam(参数列表) {
    saveValue("params.xxx.field1", value1);
    saveValue("params.xxx.field2", value2);
    // ...
}

// 读取方法
类型 xxxField1(类型 def = 默认值) const {
    return getValue("params.xxx.field1", def).to类型();
}
```

### 步骤2：修改头文件 (*.h)

```cpp
// 在 private slots: 中添加
private slots:
    void onSaveToConfig();  // 保存参数到配置文件
```

### 步骤3：修改实现文件 (*.cpp)

#### 3.1 添加头文件

```cpp
#include "Basic/ConfigManager.h"
#include "cusWidgets/custommessagebox.h"
```

#### 3.2 在构造函数中加载默认值

```cpp
构造函数::构造函数(QWidget *parent) : ... {
    // ... 原有初始化代码 ...

    // 从配置文件加载默认值
    ui->field1->setValue(CF_INS.xxxField1(默认值));
    ui->field2->setValue(CF_INS.xxxField2(默认值));
    // ...

    // 连接保存按钮
    if (ui->saveButton) {
        connect(ui->saveButton, &QPushButton::clicked, this, &类名::onSaveToConfig);
    }
}
```

#### 3.3 实现保存函数

```cpp
void 类名::onSaveToConfig()
{
    // 1. 获取当前UI参数
    类型1 field1 = ui->field1->获取值();
    类型2 field2 = ui->field2->获取值();
    // ...

    // 2. 保存到ConfigManager
    CF_INS.saveXxxParam(field1, field2, ...);

    // 3. 保存到文件并提示
    if (CF_INS.save()) {
        CustomMessageBox::showInfo(this, tr("保存成功"),
                                  tr("参数已保存到配置文件！\n下次启动将自动加载这些参数。"));
    } else {
        CustomMessageBox::showWarning(this, tr("保存失败"),
                                     tr("无法保存配置文件，请检查文件权限。"));
    }
}
```

### 步骤4：修改UI文件 (*.ui)

在 `</layout>` 之前、`<widget class="QDialogButtonBox">` 之前添加：

```xml
   <item>
    <widget class="QPushButton" name="saveButton">
     <property name="text">
      <string>保存参数到配置文件</string>
     </property>
    </widget>
   </item>
```

## 各窗口的具体实现指导

### 1. scanrangeui.cpp/h - 扫描范围设置

**ConfigManager 方法**：
- 保存：`CF_INS.saveScanRangeParam(workMode)`
- 读取：`CF_INS.scanRangeWorkMode(默认值)`

**UI字段**：
- `ui->workModeCombo` - 工作模式

### 2. waveandsample.cpp/h - 波形与采样控制

**ConfigManager 方法**：
- 保存：`CF_INS.saveBeamControlParam(...)` （参数较多，见ConfigManager.h）
- 读取：`CF_INS.beamFreqID()`, `CF_INS.beamType()`, `CF_INS.beamAziStart()` 等

**UI字段**：
- `ui->freq` - 频率ID
- `ui->type` - 波形类型
- `ui->azistart`, `ui->aziend`, `ui->azistep` - 方位参数
- `ui->pulseNum1` - 积累脉冲数
- `ui->enable1/2/3`, `ui->wave1/2/3` - 波形使能和编码
- `ui->samplestart1/2/3`, `ui->samplelen1/2/3` - 采样参数
- `ui->elestart1/2/3`, `ui->eleend1/2/3`, `ui->elestep1/2/3` - 俯仰参数

### 3. sigparamui.cpp/h - 信号处理参数

**ConfigManager 方法**：
- 保存：`CF_INS.saveSigProParam(...)`
- 读取：`CF_INS.sigProNoise()` 等

**UI字段**：
- 噪声、门限、CFAR参数等（参考 `SigProParam` 结构体）

### 4. dataprocessui.cpp/h - 数据处理参数

**ConfigManager 方法**：
- 保存：`CF_INS.saveDataProParam(...)`
- 读取：`CF_INS.dataProStartWinLen()` 等

**UI字段**：
- 航迹起始窗口、航迹门限、凝聚门等（参考 `DataProParam` 结构体）

### 5. datasaveui.cpp/h - 数据保存参数

**ConfigManager 方法**：
- 保存：`CF_INS.saveDataSaveParam(saveSwitch, dataID)`
- 读取：`CF_INS.dataSaveSaveSwitch()`, `CF_INS.dataSaveDataID()`

### 6. twsmodedialog.cpp/h 和 tasmodedialog.cpp/h

这两个对话框包含三种参数（扫描范围+波形控制+伺服控制），可以复用已有的方法：

```cpp
void onSaveToConfig() {
    // 保存三种参数
    CF_INS.saveScanRangeParam(workMode);
    CF_INS.saveBeamControlParam(...);
    CF_INS.saveServoParam(cmd, speed, az);

    if (CF_INS.save()) {
        CustomMessageBox::showInfo(this, tr("保存成功"),
                                  tr("TWS/TAS参数已保存！"));
    }
}
```

## config.toml 文件结构

保存功能会自动在 `config.toml` 中添加如下段落（如果不存在）：

```toml
[params.servo]
# 伺服控制参数 Servo Control Parameters
cmd = 0
speed = 3
az = 0

[params.scanrange]
# 扫描范围参数 Scan Range Parameters
workMode = 0

[params.beamcontrol]
# 波形控制参数 Beam Control Parameters
freqID = 4
type = 2
aziStart = -4500
aziEnd = 4500
# ... 更多参数

[params.sigpro]
# 信号处理参数 Signal Processing Parameters
noise = 1000
# ... 更多参数

[params.datapro]
# 数据处理参数 Data Processing Parameters
startWinLen = 3
# ... 更多参数

[params.datasave]
# 数据保存参数 Data Save Parameters
saveSwitch = 0
dataID = 0
```

## 测试步骤

1. 打开参数配置窗口
2. 修改参数值
3. 点击"保存参数到配置文件"按钮
4. 确认看到"保存成功"提示
5. 关闭程序
6. 打开 `config.toml` 文件，确认参数已保存
7. 重新启动程序
8. 再次打开参数配置窗口，确认参数已恢复为上次保存的值

## 注意事项

1. **参数单位转换**：保存到 config.toml 的值是协议中使用的原始值（如角度 * 100），不是UI显示值
2. **默认值链**：`config.toml` → `ConfigManager默认参数` → `Protocol.h硬编码值`
3. **文件权限**：确保程序对 `config.toml` 有写权限
4. **线程安全**：ConfigManager 是单例，多窗口同时保存时后面的会覆盖前面的，建议保存前读取最新值

## 参考文件

- **ConfigManager**: `d:\DispCtrl\DispCtrl\Basic\ConfigManager.h`
- **示例实现**: `d:\DispCtrl\DispCtrl\paramWidget\servocontrol.cpp`
- **协议定义**: `d:\DispCtrl\DispCtrl\Basic\Protocol.h`
- **配置文件**: `d:\DispCtrl\DispCtrl\config.toml`
