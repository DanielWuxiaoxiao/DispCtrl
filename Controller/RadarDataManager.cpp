// RadarDataManager.cpp - 雷达数据统一管理器实现
#include "CentralDataManager.h"
#include <QDebug>
#include <QtMath>
#include <QDateTime>

RadarDataManager::RadarDataManager(QObject* parent)
    : QObject(parent), m_cleanupTimer(new QTimer(this))
{
    setupCleanupTimer();
}

RadarDataManager& RadarDataManager::instance()
{
    static RadarDataManager instance;
    return instance;
}

void RadarDataManager::processDetection(const PointInfo& info)
{
    QMutexLocker locker(&m_dataMutex);
    
    // 直接存储检测点（不使用timestamp）
    m_detections.append(info);
    
    // 限制检测点数量，避免内存无限增长
    const int MAX_DETECTIONS = 10000;
    if (m_detections.size() > MAX_DETECTIONS) {
        m_detections.removeFirst();
    }
    
    // 发出信号通知所有视图
    emit detectionReceived(info);
}

void RadarDataManager::processTrack(const PointInfo& info)
{
    QMutexLocker locker(&m_dataMutex);
    
    // 直接存储航迹点（不使用timestamp）
    if (!m_tracks.contains(info.batch)) {
        m_tracks[info.batch] = QList<PointInfo>();
    }
    
    m_tracks[info.batch].append(info);
    
    // 限制每个批次的航迹点数量
    const int MAX_TRACK_POINTS = 1000;
    if (m_tracks[info.batch].size() > MAX_TRACK_POINTS) {
        m_tracks[info.batch].removeFirst();
    }
    
    // 发出信号通知所有视图
    emit trackReceived(info);
}

void RadarDataManager::registerView(const QString& viewId, QObject* view)
{
    QMutexLocker locker(&m_dataMutex);
    
    if (view && !m_registeredViews.contains(viewId)) {
        m_registeredViews[viewId] = view;
        qDebug() << "RadarDataManager: Registered view" << viewId;
    }
}

void RadarDataManager::unregisterView(const QString& viewId)
{
    QMutexLocker locker(&m_dataMutex);
    
    if (m_registeredViews.remove(viewId) > 0) {
        qDebug() << "RadarDataManager: Unregistered view" << viewId;
    }
}

QList<PointInfo> RadarDataManager::getDetectionsInRange(float minRange, float maxRange, 
                                                        float minAngle, float maxAngle) const
{
    QMutexLocker locker(&m_dataMutex);
    
    QList<PointInfo> result;
    for (const auto& detection : m_detections) {
        if (isInRange(detection, minRange, maxRange, minAngle, maxAngle)) {
            result.append(detection);
        }
    }
    return result;
}

QList<PointInfo> RadarDataManager::getTracksInRange(float minRange, float maxRange, 
                                                     float minAngle, float maxAngle, int batchId) const
{
    QMutexLocker locker(&m_dataMutex);
    
    QList<PointInfo> result;
    
    if (batchId >= 0) {
        // 查询特定批次
        if (m_tracks.contains(batchId)) {
            for (const auto& track : m_tracks[batchId]) {
                if (isInRange(track, minRange, maxRange, minAngle, maxAngle)) {
                    result.append(track);
                }
            }
        }
    } else {
        // 查询所有批次
        for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
            for (const auto& track : it.value()) {
                if (isInRange(track, minRange, maxRange, minAngle, maxAngle)) {
                    result.append(track);
                }
            }
        }
    }
    
    return result;
}

void RadarDataManager::clearAllData()
{
    QMutexLocker locker(&m_dataMutex);
    
    m_detections.clear();
    m_tracks.clear();
    
    emit dataCleared();
    qDebug() << "RadarDataManager: All data cleared";
}

void RadarDataManager::clearOldData(int maxAgeSeconds)
{
    QMutexLocker locker(&m_dataMutex);
    
    // 简化版本：基于数据数量而不是时间戳来清理旧数据
    // 保留最新的一定数量的检测点和航迹点
    
    const int MAX_KEEP_DETECTIONS = 5000;
    if (m_detections.size() > MAX_KEEP_DETECTIONS) {
        int removeCount = m_detections.size() - MAX_KEEP_DETECTIONS;
        for (int i = 0; i < removeCount; ++i) {
            m_detections.removeFirst();
        }
    }
    
    // 对每个航迹批次也做类似处理
    const int MAX_KEEP_TRACKS = 500;
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        auto& trackList = it.value();
        if (trackList.size() > MAX_KEEP_TRACKS) {
            int removeCount = trackList.size() - MAX_KEEP_TRACKS;
            for (int i = 0; i < removeCount; ++i) {
                trackList.removeFirst();
            }
        }
        
        // 如果某个批次的航迹点全部被清理，则删除该批次
        if (trackList.isEmpty()) {
            it = m_tracks.erase(it);
        } else {
            ++it;
        }
    }
    
    emit oldDataCleared();
}

int RadarDataManager::getTrackCount() const
{
    QMutexLocker locker(&m_dataMutex);
    
    int count = 0;
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        count += it.value().size();
    }
    return count;
}

bool RadarDataManager::isInRange(const PointInfo& info, float minRange, float maxRange, 
                                 float minAngle, float maxAngle) const
{
    // 距离检查
    if (info.range < minRange || info.range > maxRange) {
        return false;
    }
    
    // 角度检查（处理跨越0度的情况）
    double angle = fmod(info.azimuth, 360.0);
    if (angle < 0) angle += 360.0;
    
    double minAngleNorm = fmod(minAngle, 360.0);
    double maxAngleNorm = fmod(maxAngle, 360.0);
    if (minAngleNorm < 0) minAngleNorm += 360.0;
    if (maxAngleNorm < 0) maxAngleNorm += 360.0;
    
    if (minAngleNorm <= maxAngleNorm) {
        return (angle >= minAngleNorm && angle <= maxAngleNorm);
    } else {
        // 跨越0度的扇形
        return (angle >= minAngleNorm || angle <= maxAngleNorm);
    }
}

void RadarDataManager::setupCleanupTimer()
{
    // 每30秒自动清理一次超过5分钟的旧数据
    m_cleanupTimer->setInterval(30000); // 30秒
    m_cleanupTimer->setSingleShot(false);
    connect(m_cleanupTimer, &QTimer::timeout, this, &RadarDataManager::performCleanup);
    m_cleanupTimer->start();
}

void RadarDataManager::performCleanup()
{
    clearOldData(300); // 清理5分钟前的数据
}