// RadarDataManager.h - 雷达数据统一管理器
#ifndef RADARDATAMANAGER_H
#define RADARDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QMutex>
#include <QTimer>
#include "Basic/Protocol.h"

// 雷达数据统一管理器
class RadarDataManager : public QObject
{
    Q_OBJECT
    
public:
    // 单例模式
    static RadarDataManager& instance();
    
    // 数据接收接口（从UDP管理器调用）
    void processDetection(const PointInfo& info);
    void processTrack(const PointInfo& info);
    
    // 视图注册接口
    void registerView(const QString& viewId, QObject* view);
    void unregisterView(const QString& viewId);
    
    // 数据查询接口（为性能优化）
    QList<PointInfo> getDetectionsInRange(float minRange, float maxRange, 
                                          float minAngle, float maxAngle) const;
    QList<PointInfo> getTracksInRange(float minRange, float maxRange, 
                                      float minAngle, float maxAngle, int batchId = -1) const;
    
    // 数据管理
    void clearAllData();
    void clearOldData(int maxAgeSeconds = 30);
    
    // 统计信息
    int getDetectionCount() const { return m_detections.size(); }
    int getTrackCount() const;
    
signals:
    // 数据更新信号 - 所有视图都会收到这些信号
    void detectionReceived(const PointInfo& info);
    void trackReceived(const PointInfo& info);
    void dataCleared();
    void oldDataCleared();
    
private:
    RadarDataManager(QObject* parent = nullptr);
    ~RadarDataManager() = default;
    
    // 禁用拷贝构造和赋值
    RadarDataManager(const RadarDataManager&) = delete;
    RadarDataManager& operator=(const RadarDataManager&) = delete;
    
    // 数据存储
    QList<PointInfo> m_detections;
    QMap<int, QList<PointInfo>> m_tracks; // batchId -> track points
    
    // 视图管理
    QMap<QString, QObject*> m_registeredViews;
    
    // 线程安全
    mutable QMutex m_dataMutex;
    
    // 自动清理定时器
    QTimer* m_cleanupTimer;
    
    // 辅助方法
    bool isInRange(const PointInfo& info, float minRange, float maxRange, 
                   float minAngle, float maxAngle) const;
    bool isValidPointInfo(const PointInfo& info) const;
    void setupCleanupTimer();
    
private slots:
    void performCleanup();
};

// 便捷宏定义
#define RADAR_DATA_MGR RadarDataManager::instance()

#endif // RADARDATAMANAGER_H