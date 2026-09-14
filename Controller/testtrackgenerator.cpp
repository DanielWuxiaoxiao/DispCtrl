/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-13 09:35:23
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-14 14:10:17
 * @Description: 
 */
#include "testtrackgenerator.h"

#include "Basic/ConfigManager.h"
#include "Basic/log.h"

#include <QtGlobal>
#include <cmath>
#include <limits>

namespace {

constexpr int kMinimumTrackCount = 2;
constexpr int kMaximumTrackCount = 3;
constexpr int kMinimumPointCount = 1;
constexpr int kMaximumPointCount = 10000;
constexpr int kMinimumIntervalMs = 100;
constexpr int kMaximumIntervalMs = 2000;
constexpr int kLogSampleInterval = 50;
constexpr double kPi = 3.14159265358979323846;

struct TestTrackProfile {
    double initialRangeM;
    double azimuthDeg;
    double elevationDeg;
};

// 固定视线的匀速运动使 PPI RAE、DDA4 经纬高和 DDA4 速度矢量完全一致。
constexpr TestTrackProfile kTrackProfiles[] = {
    {2800.0, 25.0, 3.5},
    {4300.0, 330.0, 5.0},
    {3400.0, 170.0, 4.2}
};

float normalizeAzimuth(double azimuthDeg)
{
    double normalized = std::fmod(azimuthDeg, 360.0);
    if (normalized < 0.0) {
        normalized += 360.0;
    }
    return static_cast<float>(normalized);
}

} // namespace

TestTrackGenerator::TestTrackGenerator(QObject* parent)
    : QObject(parent)
{
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &TestTrackGenerator::generateSample);
}

bool TestTrackGenerator::start()
{
    if (m_running) {
        LOG_WARNING("[TestTrack] 上一轮测试航迹尚未结束，忽略重复生成请求");
        return false;
    }

    m_trackCount = qBound(kMinimumTrackCount, CF_INS.testTrackCount(2), kMaximumTrackCount);
    m_pointCount = qBound(kMinimumPointCount, CF_INS.testTrackPointCount(300), kMaximumPointCount);
    m_intervalMs = qBound(kMinimumIntervalMs, CF_INS.testTrackIntervalMs(500), kMaximumIntervalMs);
    m_speedMps = CF_INS.testTrackSpeedMps(15.0);
    if (!std::isfinite(m_speedMps) || m_speedMps <= 0.0 || m_speedMps > 300.0) {
        LOG_WARNING(QString("[TestTrack] 非法 speed_mps=%1，回退为 15m/s").arg(m_speedMps));
        m_speedMps = 15.0;
    }
    m_firstBatch = CF_INS.testTrackFirstBatch(65000);
    if (m_firstBatch == 0 || m_firstBatch > std::numeric_limits<quint32>::max() - m_trackCount) {
        LOG_WARNING(QString("[TestTrack] 非法 first_batch=%1，回退为 65000").arg(m_firstBatch));
        m_firstBatch = 65000;
    }

    m_sampleIndex = 0;
    m_running = true;
    m_elapsedTimer.start();

    // 测试不应依赖现场 DD05；该位置只作为启动本轮联调的临时原点。
    const double latitude = CF_INS.latitude("latitude", 34.225249343);
    const double longitude = CF_INS.longitude("longitude", 109.123922221);
    const double altitude = CF_INS.altitude("altitude", 781.48);
    emit fallbackRadarPositionReady(latitude, longitude, altitude);
    emit generationStarted(m_trackCount, m_pointCount);
    LOG_INFO(QString("[TestTrack] 开始：tracks=%1 points=%2 intervalMs=%3 speed=%4m/s batches=%5..%6 fallbackRadar=[lon=%7 lat=%8 alt=%9]")
             .arg(m_trackCount).arg(m_pointCount).arg(m_intervalMs).arg(m_speedMps, 0, 'f', 2)
             .arg(m_firstBatch).arg(m_firstBatch + static_cast<quint32>(m_trackCount) - 1)
             .arg(longitude, 0, 'f', 8).arg(latitude, 0, 'f', 8).arg(altitude, 0, 'f', 1));

    generateSample();
    if (m_running) {
        m_timer.start(m_intervalMs);
    }
    return true;
}

void TestTrackGenerator::generateSample()
{
    if (!m_running) {
        return;
    }

    for (int trackIndex = 0; trackIndex < m_trackCount; ++trackIndex) {
        emit trackGenerated(makeTrackPoint(trackIndex));
    }

    ++m_sampleIndex;
    if (m_sampleIndex == 1 || m_sampleIndex % kLogSampleInterval == 0) {
        LOG_INFO(QString("[TestTrack] 已生成 sample=%1/%2，每采样并行航迹数=%3")
                 .arg(m_sampleIndex).arg(m_pointCount).arg(m_trackCount));
    }
    if (m_sampleIndex >= m_pointCount) {
        finishGeneration();
    }
}

PointInfo TestTrackGenerator::makeTrackPoint(int trackIndex) const
{
    const TestTrackProfile& profile = kTrackProfiles[trackIndex];
    const double elapsedSeconds = static_cast<double>(m_elapsedTimer.elapsed()) / 1000.0;
    const double rangeM = profile.initialRangeM + m_speedMps * elapsedSeconds;

    PointInfo point;
    point.type = PointType::Track;
    point.batch = m_firstBatch + static_cast<quint32>(trackIndex);
    point.statMethod = 0;               // 正常滤波更新，与普通 DBT 航迹一致。
    point.targetRecResult = 1;          // 识别为无人机，便于自动上报总控联调。
    point.targetConfidence = 0.99f;
    point.radarId = RADAR_ID_MIN;

    point.range = static_cast<float>(rangeM);
    point.azimuth = normalizeAzimuth(profile.azimuthDeg);
    point.elevation = static_cast<float>(profile.elevationDeg);
    // 现场 PointInfo 的高度字段表示相对雷达的上向量；与 elevation/range 保持一致。
    point.altitute = static_cast<float>(rangeM * std::sin(profile.elevationDeg * kPi / 180.0));
    point.speed = static_cast<float>(m_speedMps);
    point.SNR = 24.0f;
    point.amp = 1.0f;
    return point;
}

void TestTrackGenerator::finishGeneration()
{
    m_timer.stop();
    for (int trackIndex = 0; trackIndex < m_trackCount; ++trackIndex) {
        PointInfo cancelled;
        cancelled.type = PointType::Track;
        cancelled.batch = m_firstBatch + static_cast<quint32>(trackIndex);
        cancelled.statMethod = 2;
        cancelled.targetRecResult = 1;
        cancelled.radarId = RADAR_ID_MIN;
        emit trackGenerated(cancelled);
        LOG_INFO(QString("[TestTrack] 自动消批 sourceBatch=%1").arg(cancelled.batch));
    }
    m_running = false;
    LOG_INFO(QString("[TestTrack] 完成：每条 %1 点，%2 条测试航迹均已消批")
             .arg(m_pointCount).arg(m_trackCount));
    emit generationFinished();
}
