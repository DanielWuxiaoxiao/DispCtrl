/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 10:20:21
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 10:23:32
 * @Description: 
 */
/**
 * @file recordmanager.cpp
 * @brief 雷达场景录制与回放管理器实现
 */
#include "recordmanager.h"
#include <QDebug>
#include <algorithm>

// 文件头 magic：little-endian "DREC"
static constexpr quint32 kFileMagic  = 0x43455244u;
// 帧头 magic：little-endian "DFRM"
static constexpr quint32 kFrameMagic = 0x4D524644u;

// ============================================================================
// 构造 / 析构
// ============================================================================

RecordManager::RecordManager(QObject* parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(20);  // 20ms tick → 最小回放分辨率
    connect(m_timer, &QTimer::timeout, this, &RecordManager::onPlaybackTick);
}

RecordManager::~RecordManager()
{
    stopRecording();
    stopPlayback();
}

// ============================================================================
// 录制
// ============================================================================

bool RecordManager::startRecording(const QString& filePath)
{
    stopRecording();

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit logMessage(QStringLiteral("RecordManager: 无法创建文件 %1 — %2")
                        .arg(filePath, m_file.errorString()));
        return false;
    }

    // 写文件头
    const quint32 magic = kFileMagic;
    m_file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

    m_recTimer.start();
    m_recording  = true;
    m_frameCount = 0;

    emit logMessage(QStringLiteral("RecordManager: 开始录制 → %1").arg(filePath));
    return true;
}

void RecordManager::stopRecording()
{
    if (!m_recording) return;
    m_recording = false;
    m_file.flush();
    m_file.close();
    emit logMessage(QStringLiteral("RecordManager: 停止录制，共 %1 帧").arg(m_frameCount));
}

void RecordManager::writeFrame(quint8 type, const PointInfo& pt)
{
    if (!m_recording || !m_file.isOpen()) return;

    RecordFrameHeader hdr;
    hdr.frameMagic   = kFrameMagic;
    hdr.type         = type;
    hdr.reserved[0]  = hdr.reserved[1] = hdr.reserved[2] = 0;
    hdr.timestampMs  = static_cast<quint64>(m_recTimer.elapsed());

    m_file.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    m_file.write(reinterpret_cast<const char*>(&pt),  sizeof(pt));
    ++m_frameCount;
}

void RecordManager::recordDet  (const PointInfo& pt) { writeFrame(0, pt); }
void RecordManager::recordTrack(const PointInfo& pt) { writeFrame(1, pt); }
void RecordManager::recordTbd  (const PointInfo& pt) { writeFrame(2, pt); }

// ============================================================================
// 文件加载
// ============================================================================

bool RecordManager::loadFile(const QString& filePath)
{
    stopPlayback();
    m_frames.clear();

    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        emit logMessage(QStringLiteral("RecordManager: 无法打开 %1 — %2")
                        .arg(filePath, f.errorString()));
        return false;
    }

    // 验证文件头
    quint32 magic = 0;
    if (f.read(reinterpret_cast<char*>(&magic), sizeof(magic)) != sizeof(magic) ||
        magic != kFileMagic) {
        emit logMessage(QStringLiteral("RecordManager: 文件格式无效 %1").arg(filePath));
        return false;
    }

    // 逐帧读取
    while (!f.atEnd()) {
        RecordFrameHeader hdr;
        if (f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr)) != sizeof(hdr)) break;
        if (hdr.frameMagic != kFrameMagic) break;

        PointInfo pt;
        if (f.read(reinterpret_cast<char*>(&pt), sizeof(pt)) != sizeof(pt)) break;

        m_frames.append({ hdr.timestampMs, hdr.type, pt });
    }

    emit logMessage(QStringLiteral("RecordManager: 加载完成  %1 帧  时长 %.1f s")
                    .arg(m_frames.size()).arg(durationSec()));
    return !m_frames.isEmpty();
}

double RecordManager::durationSec() const
{
    if (m_frames.isEmpty()) return 0.0;
    return m_frames.last().timestampMs / 1000.0;
}

// ============================================================================
// 回放控制
// ============================================================================

void RecordManager::startPlayback()
{
    if (m_frames.isEmpty()) return;

    m_playIdx     = 0;
    m_playing     = true;
    m_playStartMs = m_frames.first().timestampMs;
    m_playElapsed.start();
    m_timer->start();

    emit logMessage(QStringLiteral("RecordManager: 开始回放  速度=%.1fx  共 %1 帧")
                    .arg(m_speed).arg(m_frames.size()));
}

void RecordManager::pausePlayback()
{
    if (!m_playing) return;
    m_playing = false;
    m_timer->stop();
}

void RecordManager::resumePlayback()
{
    if (m_playing || m_frames.isEmpty() || m_playIdx >= m_frames.size()) return;

    m_playing     = true;
    // 从当前帧重新对齐时间基准
    m_playStartMs = m_frames[m_playIdx].timestampMs;
    m_playElapsed.start();
    m_timer->start();
}

void RecordManager::stopPlayback()
{
    m_playing = false;
    m_timer->stop();
    m_playIdx = 0;
}

void RecordManager::setPlaybackSpeed(double speed)
{
    m_speed = qMax(0.1, speed);
}

void RecordManager::seekTo(double seconds)
{
    const quint64 targetMs = static_cast<quint64>(seconds * 1000.0);
    m_playIdx = 0;
    for (int i = 0; i < m_frames.size(); ++i) {
        if (m_frames[i].timestampMs >= targetMs) {
            m_playIdx = i;
            break;
        }
    }
    if (m_playing) {
        m_playStartMs = m_frames[m_playIdx].timestampMs;
        m_playElapsed.restart();
    }
}

// ============================================================================
// 回放 tick
// ============================================================================

void RecordManager::onPlaybackTick()
{
    if (!m_playing) return;

    if (m_playIdx >= m_frames.size()) {
        stopPlayback();
        emit playbackFinished();
        return;
    }

    // 计算当前回放时间（经过的"回放时间" = elapsed × speed）
    const quint64 elapsedScaled =
        static_cast<quint64>(m_playElapsed.elapsed() * m_speed);
    const quint64 targetMs = m_playStartMs + elapsedScaled;

    // 发送所有时间戳 <= targetMs 的帧
    while (m_playIdx < m_frames.size() &&
           m_frames[m_playIdx].timestampMs <= targetMs)
    {
        const RecordFrame& rf = m_frames[m_playIdx];
        switch (rf.type) {
            case 0: emit replayDet  (rf.point); break;
            case 1: emit replayTrack(rf.point); break;
            case 2: emit replayTbd  (rf.point); break;
            default: break;
        }
        ++m_playIdx;
    }

    // 进度回报
    const int idx = qMin(m_playIdx, m_frames.size() - 1);
    emit playbackProgress(m_frames[idx].timestampMs / 1000.0, durationSec());
}
