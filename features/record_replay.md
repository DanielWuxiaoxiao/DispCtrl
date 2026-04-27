# 场景数据录制与回放

## 功能描述

将探测到的目标数据（检测点、航迹点、状态帧）以及系统控制指令记录到文件，之后可以脱机回放，用于：
- 事后分析：回顾某次告警事件前后的目标运动轨迹
- 测试回归：记录真实场景用于算法比对
- 演示展示：用录制数据做现场演示

## 实现文件

- `Controller/recordmanager.h / .cpp`

## RecordManager 接口

```cpp
class RecordManager : public QObject {
    Q_OBJECT
public:
    // 录制控制
    bool startRecording(const QString& filePath);
    void stopRecording();
    bool isRecording() const;

    // 回放控制
    bool loadFile(const QString& filePath);
    void startPlayback();
    void pausePlayback();
    void stopPlayback();
    void setPlaybackSpeed(double factor);   // 1.0=实时, 2.0=2x, 0.5=慢放
    bool isPlaying() const;

    // 回放进度
    double duration() const;               // 秒
    double currentPosition() const;        // 秒
    void seekTo(double seconds);

signals:
    // 回放时，重新发出与Controller相同的信号
    void detInfoPlayback(const PointInfo& info);
    void traInfoPlayback(const PointInfo& info);
    void tbdInfoPlayback(const PointInfo& info);
    void playbackStateChanged(bool playing);
    void playbackProgress(double seconds, double total);
    void recordingStateChanged(bool recording);
};
```

## 文件格式

二进制文件 `.drec`（DispCtrl Recording）：

```
文件头:
  [4B] magic: 0x44524543 ("DREC")
  [4B] version: 1
  [8B] startTime: Unix ms timestamp
  [4B] totalEntries

数据帧（重复）:
  [4B] offsetMs: 相对开始时间的偏移（ms）
  [2B] type:  0=DetPoint 1=TrackPoint 2=TBDPoint 3=MonitorParam
  [2B] size: 数据字节数
  [size]B data: 原始结构体
```

## UI 集成

在主面板底部工具栏或状态栏添加录制按钮（⏺ / ⏸ / ⏹ / ▶），
在 `MainOverLayOut::setupMarineControls` 中初始化：

```cpp
m_recordManager = new RecordManager(this);
// 录制时连接 Controller 信号
connect(CON_INS, &Controller::detInfoProcess, m_recordManager, &RecordManager::onDetPoint);
// 回放时连接回放信号到场景
connect(m_recordManager, &RecordManager::detInfoPlayback,
        m_ppiScene->detManager(), &DetManager::addDetPoint);
```

## 注意事项

- 录制文件应限制单文件大小（默认不超过 500MB）
- 回放时需暂停真实数据输入（或双画面对比模式）
- 文件名自动加时间戳：`record_20260401_143000.drec`
