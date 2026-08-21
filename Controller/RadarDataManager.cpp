/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 10:51:39
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:18
 * @Description: 
 */
/**
 * @file RadarDataManager.cpp
 * @brief 雷达数据统一管理器实现
 * @details 实现雷达数据的集中管理和分发：
 *          - 统一处理检测点和航迹数据
 *          - 线程安全的数据存储和访问
 *          - 多视图数据同步和通知
 *          - 数据验证和错误处理
 *          - 内存管理和数据清理
 * @author DispCtrl Team
 * @date 2024
 */

#include "Basic/log.h"

#include "RadarDataManager.h"
#include "ErrorHandler.h"
#include "../Basic/log.h"
#include <QDebug>
#include <QtMath>
#include <QDateTime>

namespace {

quint64 makeTrackKey(unsigned type, unsigned int batch)
{
    return (static_cast<quint64>(type) << 32) | static_cast<quint64>(batch);
}

}

/**
 * @brief RadarDataManager构造函数
 * @param parent 父对象指针，用于Qt对象树管理
 * @details 初始化雷达数据管理器：
 *          - 创建定时清理器，防止内存无限增长
 *          - 设置线程安全的数据结构
 *          - 准备错误处理机制
 */
RadarDataManager::RadarDataManager(QObject* parent)
    : QObject(parent), m_cleanupTimer(new QTimer(this))
{
    setupCleanupTimer();
}

/**
 * @brief 获取RadarDataManager单例实例
 * @return RadarDataManager的静态实例引用
 * @details 使用线程安全的单例模式，确保全局数据一致性
 */
RadarDataManager& RadarDataManager::instance()
{
    static RadarDataManager instance;
    return instance;
}

/**
 * @brief 处理雷达检测点数据
 * @param info 检测点信息，包含位置、类型、强度等数据
 * @details 实现功能：
 *          1. 数据有效性验证（范围、角度、类型检查）
 *          2. 线程安全的数据存储
 *          3. 内存管理（限制存储数量，防止内存泄漏）
 *          4. 信号发送（通知所有注册的视图）
 *          5. 异常处理（捕获并记录处理错误）
 */
void RadarDataManager::processDetection(const PointInfo& info)
{
    try {
        // 第一步：数据有效性验证
        if (!isValidPointInfo(info)) {
            ERROR_HANDLER.reportError("DATA_INVALID_DETECTION",
                                    QString("Invalid detection point: type=%1, range=%2, azimuth=%3")
                                    .arg(info.type).arg(info.range).arg(info.azimuth),
                                    ErrorSeverity::Warning, ErrorCategory::DataProcessing);
            return;
        }

        // 第二步：线程安全的数据操作
        QMutexLocker locker(&m_dataMutex);

        // 第三步：存储检测点数据
        m_detections.append(info);

        // 第四步：内存管理 - 限制检测点数量，避免内存无限增长
        if (m_detections.size() > kMaxCachedDetections) {
            m_detections.removeFirst();  // 移除最旧的数据
        }

        locker.unlock();

        // 第五步：通知所有注册的视图
        emit detectionReceived(info);

    } catch (const std::exception& e) {
        // 异常处理：记录处理错误但不影响程序运行
        ERROR_HANDLER.reportError("DATA_PROCESS_DETECTION_EXCEPTION",
                                QString("Exception processing detection: %1").arg(e.what()),
                                ErrorSeverity::Error, ErrorCategory::DataProcessing);
    }
}

/**
 * @brief 处理雷达航迹数据
 * @param info 航迹点信息，包含批次号、位置、速度等数据
 * @details 实现功能：
 *          1. 航迹数据有效性验证
 *          2. 按批次号组织航迹数据
 *          3. 航迹历史管理（保持适当的历史长度）
 *          4. 线程安全的数据存储
 *          5. 视图通知和异常处理
 */
void RadarDataManager::processTrack(const PointInfo& info)
{
    try {
        // 第一步：检查 statMethod==2，表示需要删除该批号的航迹
        // 必须在有效性验证之前处理，因为删除消息的 range/azimuth 字段可能为无效值
        if (info.statMethod == 2) {
            // 线程安全地从内存中删除该批次
            QMutexLocker locker(&m_dataMutex);
            m_tracks.remove(makeTrackKey(info.type, info.batch));
            locker.unlock();

            LOG_INFO(QString("[RadarDataManager::processTrack] statMethod==2: batch=%1 removed, emitting trackBatchRemoved").arg(info.batch));

            // 发出删除信号，通知所有视图删除该批次
            emit trackBatchRemoved(info.batch);
            return;
        }

        // 第二步：航迹数据有效性验证
        if (!isValidPointInfo(info)) {
            ERROR_HANDLER.reportError("DATA_INVALID_TRACK",
                                    QString("Invalid track point: type=%1, batch=%2, range=%3, azimuth=%4")
                                    .arg(info.type).arg(info.batch).arg(info.range).arg(info.azimuth),
                                    ErrorSeverity::Warning, ErrorCategory::DataProcessing);
            return;
        }

        // 第三步：线程安全的数据操作
        QMutexLocker locker(&m_dataMutex);
        const quint64 trackKey = makeTrackKey(info.type, info.batch);

        // 第三步：按航迹类型和批次号组织航迹数据，允许不同来源使用相同批号。
        if (!m_tracks.contains(trackKey)) {
            m_tracks[trackKey] = QList<PointInfo>();
        }

        // 第四步：添加航迹点到对应批次
        m_tracks[trackKey].append(info);

        // 第五步：航迹历史管理 - 限制每个批次的航迹点数量
        if (m_tracks[trackKey].size() > kMaxCachedTrackPointsPerBatch) {
            m_tracks[trackKey].removeFirst();  // 移除最旧的航迹点
        }

        locker.unlock();

        // 第六步：通知所有注册的视图
        emit trackReceived(info);

    } catch (const std::exception& e) {
        // 异常处理：记录航迹处理错误
        ERROR_HANDLER.reportError("DATA_PROCESS_TRACK_EXCEPTION",
                                QString("Exception processing track: %1").arg(e.what()),
                                ErrorSeverity::Error, ErrorCategory::DataProcessing);
    }
}

/**
 * @brief 注册视图到数据管理器
 * @param viewId 视图标识符（如"PPI_MAIN", "SECTOR_1"等）
 * @param view 视图对象指针
 * @details 实现功能：
 *          - 将视图注册到数据管理器
 *          - 后续数据更新时会通知所有注册的视图
 *          - 支持多视图同步显示
 *          - 线程安全的注册操作
 */
void RadarDataManager::registerView(const QString& viewId, QObject* view)
{
    QMutexLocker locker(&m_dataMutex);

    if (view && !m_registeredViews.contains(viewId)) {
        m_registeredViews[viewId] = view;
        LOG_DEBUG(QString("RadarDataManager: Registered view %1").arg(viewId));
    }
}

/**
 * @brief 从数据管理器注销视图
 * @param viewId 要注销的视图标识符
 * @details 实现功能：
 *          - 移除视图注册，停止接收数据更新通知
 *          - 用于视图销毁或切换时的清理
 *          - 线程安全的注销操作
 */
void RadarDataManager::unregisterView(const QString& viewId)
{
    QMutexLocker locker(&m_dataMutex);

    if (m_registeredViews.remove(viewId) > 0) {
        LOG_DEBUG(QString("RadarDataManager: Unregistered view %1").arg(viewId));
    }
}

/**
 * @brief 获取指定范围内的检测点
 * @param minRange 最小距离（公里）
 * @param maxRange 最大距离（公里）
 * @param minAngle 最小方位角（度）
 * @param maxAngle 最大方位角（度）
 * @return 符合条件的检测点列表
 * @details 实现功能：
 *          - 空间范围过滤（距离和角度条件）
 *          - 线程安全的数据访问
 *          - 用于区域显示和分析
 */
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
        // 查询特定批次的全部航迹类型。
        for (auto it = m_tracks.cbegin(); it != m_tracks.cend(); ++it) {
            for (const auto& track : it.value()) {
                if (track.batch != static_cast<unsigned int>(batchId)) {
                    continue;
                }
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
    LOG_DEBUG("RadarDataManager: All data cleared");
}

void RadarDataManager::clearOldData(int maxAgeSeconds)
{
    QMutexLocker locker(&m_dataMutex);

    // 简化版本：基于数据数量而不是时间戳来清理旧数据
    // 保留最新的一定数量的检测点和航迹点

    if (m_detections.size() > kMaxRetainedDetections) {
        int removeCount = m_detections.size() - kMaxRetainedDetections;
        for (int i = 0; i < removeCount; ++i) {
            m_detections.removeFirst();
        }
    }

    // 对每个航迹批次也做类似处理
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        auto& trackList = it.value();
        if (trackList.size() > kMaxRetainedTrackPointsPerBatch) {
            int removeCount = trackList.size() - kMaxRetainedTrackPointsPerBatch;
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
    try {
        clearOldData(300); // 清理5分钟前的数据
    } catch (const std::exception& e) {
        ERROR_HANDLER.reportError("DATA_CLEANUP_EXCEPTION",
                                QString("Exception during data cleanup: %1").arg(e.what()),
                                ErrorSeverity::Warning, ErrorCategory::DataProcessing);
    }
}

// 数据验证方法
bool RadarDataManager::isValidPointInfo(const PointInfo& info) const
{
    // 检查基本数值范围
    if (info.range < 0 || info.range > 1000000) { // 最大距离1000km
        return false;
    }

    if (info.azimuth < 0 || info.azimuth >= 360) {
        return false;
    }

    if (info.elevation < -90 || info.elevation > 90) {
        return false;
    }

    // 检查是否为NaN
    if (std::isnan(info.range) || std::isnan(info.azimuth) || std::isnan(info.elevation)) {
        return false;
    }

    return true;
}
