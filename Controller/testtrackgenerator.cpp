/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-14 14:10:16
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-17 22:45:15
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
constexpr int kMaximumTrackCount = 10;
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
    bool recognizedDrone;
};

// 覆盖自动上报的高度/距离边界和非无人机分支；固定视线匀速运动使 PPI RAE、
// DDA4 经纬高和速度矢量完全一致。前两条、7 号满足默认范围，其他条分别用于
// 验证距离超限、高度超限和非无人机不自动上报。
constexpr TestTrackProfile kTrackProfiles[] = {
    {1200.0,  25.0, 2.0, true},   // 合格：高度约 42m，距离 1.2km。
    {2500.0,  90.0, 1.5, true},   // 合格：高度约 65m，距离 2.5km。
    {3600.0, 135.0, 1.0, true},   // 距离超限。
    {1800.0, 200.0, 5.0, true},   // 高度超限。
    {2200.0, 250.0, 2.0, false},  // 非无人机，几何范围合格但不自动上报。
    { 600.0, 300.0, 8.0, false},  // 非无人机且高度超限。
    {2900.0, 315.0, 1.0, true},   // 合格：高度约 51m，距离 2.9km。
    { 800.0,  45.0, 8.0, true},   // 高度超限。
    {4500.0, 160.0, 0.5, false},  // 非无人机且距离超限。
    {3100.0, 350.0, 1.0, true}    // 距离超限，用于方位跨 0 度联调。
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

    m_trackCount = qBound(kMinimumTrackCount, CF_INS.testTrackCount(10), kMaximumTrackCount);
    m_pointCount = qBound(kMinimumPointCount, CF_INS.testTrackPointCount(1200), kMaximumPointCount);
    m_intervalMs = qBound(kMinimumIntervalMs, CF_INS.testTrackIntervalMs(500), kMaximumIntervalMs);
    m_speedMps = CF_INS.testTrackSpeedMps(15.0);
    if (!std::isfinite(m_speedMps) || m_speedMps <= 0.0 || m_speedMps > 300.0) {
        LOG_WARNING(QString("[TestTrack] 非法 speed_mps=%1，回退为 15m/s").arg(m_speedMps));
        m_speedMps = 15.0;
    }
    m_firstBatch = CF_INS.testTrackFirstBatch(101);
    if (m_firstBatch == 0 || m_firstBatch > std::numeric_limits<quint32>::max() - m_trackCount) {
        LOG_WARNING(QString("[TestTrack] 非法 first_batch=%1，回退为 101").arg(m_firstBatch));
        m_firstBatch = 101;
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

bool TestTrackGenerator::stop()
{
    if (!m_running) {
        return false;
    }

    m_timer.stop();
    removeGeneratedTracks();
    m_running = false;
    LOG_INFO(QString("[TestTrack] 已主动终止：已生成 %1/%2 个采样，%3 条测试航迹均已消批")
             .arg(m_sampleIndex).arg(m_pointCount).arg(m_trackCount));
    emit generationFinished();
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
    point.targetRecResult = profile.recognizedDrone ? 1U : 0U;
    point.targetConfidence = profile.recognizedDrone ? 0.99f : 0.85f;
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
    removeGeneratedTracks();
    m_running = false;
    LOG_INFO(QString("[TestTrack] 完成：每条 %1 点，%2 条测试航迹均已消批")
             .arg(m_pointCount).arg(m_trackCount));
    emit generationFinished();
}

void TestTrackGenerator::removeGeneratedTracks()
{
    for (int trackIndex = 0; trackIndex < m_trackCount; ++trackIndex) {
        PointInfo cancelled;
        cancelled.type = PointType::Track;
        cancelled.batch = m_firstBatch + static_cast<quint32>(trackIndex);
        cancelled.statMethod = 2;
        cancelled.targetRecResult = kTrackProfiles[trackIndex].recognizedDrone ? 1U : 0U;
        cancelled.radarId = RADAR_ID_MIN;
        emit trackGenerated(cancelled);
        LOG_INFO(QString("[TestTrack] 自动消批 sourceBatch=%1").arg(cancelled.batch));
    }
}
