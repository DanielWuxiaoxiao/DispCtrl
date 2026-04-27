/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 10:19:56
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 10:23:32
 * @Description: 
 */
/**
 * @file recordmanager.h
 * @brief 雷达场景录制与回放管理器
 * @details 将 det/track/tbd 点迹流录制到二进制 .drec 文件，
 *          支持加载、回放（可调速）和按时间跳转。
 *
 * 文件格式：
 *   [4B magic "DREC"] + N × [RecordFrameHeader(16B) + PointInfo(可变)]
 *
 * ### 录制用法
 * @code
 * m_rec = new RecordManager(this);
 * connect(CON_INS, &Controller::detInfoProcess,   m_rec, &RecordManager::recordDet);
 * connect(CON_INS, &Controller::traInfoProcess,   m_rec, &RecordManager::recordTrack);
 * connect(CON_INS, &Controller::tbdInfoProcess,   m_rec, &RecordManager::recordTbd);
 * m_rec->startRecording("/path/to/scene.drec");
 * // ...
 * m_rec->stopRecording();
 * @endcode
 *
 * ### 回放用法
 * @code
 * m_rec->loadFile("/path/to/scene.drec");
 * connect(m_rec, &RecordManager::replayDet,   detManager, &DetManager::addDetPoint);
 * m_rec->startPlayback();
 * @endcode
 */
#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include <QObject>
#include <QFile>
#include <QTimer>
#include <QVector>
#include <QElapsedTimer>
#include "Basic/Protocol.h"

// ---------------------------------------------------------------------------
// 文件帧头（磁盘上每帧 16 字节 + sizeof(PointInfo)）
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct RecordFrameHeader {
    quint32 frameMagic;    ///< 0x4D524644 ("DFRM")
    quint8  type;          ///< 0=det  1=track  2=tbd
    quint8  reserved[3];
    quint64 timestampMs;   ///< 从录制开始的毫秒偏移
};
#pragma pack(pop)

// ---------------------------------------------------------------------------
// 内存中的一帧
// ---------------------------------------------------------------------------
struct RecordFrame {
    quint64   timestampMs;
    quint8    type;         ///< 0=det 1=track 2=tbd
    PointInfo point;
};

/**
 * @class RecordManager
 * @brief 场景录制 / 回放管理器
 */
class RecordManager : public QObject
{
    Q_OBJECT
public:
    explicit RecordManager(QObject* parent = nullptr);
    ~RecordManager() override;

    // ---- 录制 ----

    /// 开始录制（覆盖已有文件），返回 false 表示无法打开文件
    bool startRecording(const QString& filePath);

    /// 停止录制并关闭文件
    void stopRecording();

    bool isRecording() const { return m_recording; }

    // ---- 回放 ----

    /// 加载 .drec 文件到内存，返回 false 表示失败
    bool loadFile(const QString& filePath);

    void startPlayback();
    void pausePlayback();
    void resumePlayback();
    void stopPlayback();

    /// 设置回放速度（1.0 = 实时，2.0 = 二倍速，0.5 = 半倍速）
    void setPlaybackSpeed(double speed);

    /// 跳转到指定秒数
    void seekTo(double seconds);

    bool   isLoaded()  const { return !m_frames.isEmpty(); }
    bool   isPlaying() const { return m_playing; }
    double durationSec() const;

public slots:
    void recordDet  (const PointInfo& pt);
    void recordTrack(const PointInfo& pt);
    void recordTbd  (const PointInfo& pt);

signals:
    void replayDet  (const PointInfo& pt);
    void replayTrack(const PointInfo& pt);
    void replayTbd  (const PointInfo& pt);

    /// 定期回报回放进度（当前秒, 总秒）
    void playbackProgress(double currentSec, double totalSec);

    void playbackFinished();
    void logMessage(const QString& msg);

private slots:
    void onPlaybackTick();

private:
    void writeFrame(quint8 type, const PointInfo& pt);

    // 录制
    bool          m_recording  = false;
    QFile         m_file;
    QElapsedTimer m_recTimer;
    quint64       m_frameCount = 0;

    // 回放
    QVector<RecordFrame> m_frames;
    int                  m_playIdx       = 0;
    bool                 m_playing       = false;
    double               m_speed         = 1.0;
    QTimer*              m_timer         = nullptr;
    quint64              m_playStartMs   = 0;     ///< 回放起点帧时间戳
    QElapsedTimer        m_playElapsed;
};

#endif // RECORDMANAGER_H
