/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-14 14:10:16
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-20 19:24:22
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
    double courseAzimuthDeg;
    double climbAngleDeg;
    bool recognizedDrone;
};

// 覆盖自动上报的高度/距离边界和非无人机分支。每条航迹都在东北天坐标系中沿不同
// 航向、爬升角匀速运动，再反算 PPI 所需的 RAE；因此方位、俯仰、经纬高和速度来自
// 同一个三维位置模型。前两条、7 号初始满足默认范围，其他条分别用于验证距离超限、
// 高度超限和非无人机不自动上报。
constexpr TestTrackProfile kTrackProfiles[] = {
    {1200.0,  25.0, 2.0, 110.0,  4.0, true},   // 合格：横向上升，方位/俯仰均变化。
    {2500.0,  90.0, 1.5,  20.0, -3.0, true},   // 合格：横向下降。
    {3600.0, 135.0, 1.0, 230.0,  2.0, true},   // 距离超限，斜向上升。
    {1800.0, 200.0, 5.0, 290.0, -4.0, true},   // 高度超限，斜向下降。
    {2200.0, 250.0, 2.0, 340.0,  3.0, false},  // 非无人机，几何范围合格但不自动上报。
    { 600.0, 300.0, 8.0,  50.0, -6.0, false},  // 非无人机且高度超限。
    {2900.0, 315.0, 1.0,  65.0,  5.0, true},   // 合格：横向上升，便于范围联调。
    { 800.0,  45.0, 8.0, 150.0, -5.0, true},   // 高度超限。
    {4500.0, 160.0, 0.5, 260.0,  1.0, false},  // 非无人机且距离超限。
    {3100.0, 350.0, 1.0,  80.0,  3.0, true}    // 距离超限，跨正北方位联调。
};
static_assert(sizeof(kTrackProfiles) / sizeof(kTrackProfiles[0]) >= kMaximumTrackCount,
              "测试航迹配置上限不得超过预设三维航迹数量");

float normalizeAzimuth(double azimuthDeg)
{
    double normalized = std::fmod(azimuthDeg, 360.0);
    if (normalized < 0.0) {
        normalized += 360.0;
    }
    return static_cast<float>(normalized);
}

double degreesToRadians(double degrees)
{
    return degrees * kPi / 180.0;
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
    LOG_INFO(QString("[TestTrack] 开始：tracks=%1 points=%2 intervalMs=%3 speed=%4m/s batches=%5..%6 model=ENU-3D fallbackRadar=[lon=%7 lat=%8 alt=%9]")
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
    const double initialAzimuthRad = degreesToRadians(profile.azimuthDeg);
    const double initialElevationRad = degreesToRadians(profile.elevationDeg);
    const double courseAzimuthRad = degreesToRadians(profile.courseAzimuthDeg);
    const double climbAngleRad = degreesToRadians(profile.climbAngleDeg);

    // 本项目 RAE 定义为正北=0°、正东=90°、俯仰向上为正。先由初始 RAE 得到
    // 东北天位置，再按航向和爬升角积分，避免单独篡改角度而产生不真实的速度。
    const double initialHorizontalM = profile.initialRangeM * std::cos(initialElevationRad);
    double eastM = initialHorizontalM * std::sin(initialAzimuthRad);
    double northM = initialHorizontalM * std::cos(initialAzimuthRad);
    double upM = profile.initialRangeM * std::sin(initialElevationRad);

    const double horizontalSpeedMps = m_speedMps * std::cos(climbAngleRad);
    eastM += horizontalSpeedMps * std::sin(courseAzimuthRad) * elapsedSeconds;
    northM += horizontalSpeedMps * std::cos(courseAzimuthRad) * elapsedSeconds;
    upM += m_speedMps * std::sin(climbAngleRad) * elapsedSeconds;

    const double horizontalRangeM = std::hypot(eastM, northM);
    const double rangeM = std::hypot(horizontalRangeM, upM);
    const double azimuthDeg = std::atan2(eastM, northM) * 180.0 / kPi;
    const double elevationDeg = std::atan2(upM, horizontalRangeM) * 180.0 / kPi;

    PointInfo point;
    point.type = PointType::Track;
    point.batch = m_firstBatch + static_cast<quint32>(trackIndex);
    point.statMethod = 0;               // 正常滤波更新，与普通 DBT 航迹一致。
    point.targetRecResult = profile.recognizedDrone ? 1U : 0U;
    point.targetConfidence = profile.recognizedDrone ? 0.99f : 0.85f;
    point.radarId = RADAR_ID_MIN;

    point.range = static_cast<float>(rangeM);
    point.azimuth = normalizeAzimuth(azimuthDeg);
    point.elevation = static_cast<float>(elevationDeg);
    // 现场 PointInfo 的高度字段表示相对雷达的上向量，直接复用三维位置的 up 分量。
    point.altitute = static_cast<float>(upM);
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
