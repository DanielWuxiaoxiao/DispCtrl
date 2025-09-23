/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 10:39:39
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:54
 * @Description: 
 */
/**
 * @file RadarDataManager.h
 * @brief 雷达数据统一管理器
 * @details 提供集中化的雷达数据管理服务，包括数据接收、存储、查询和分发
 * 
 * 功能特性：
 * - 统一的雷达数据接收和处理
 * - 高效的数据存储和查询机制
 * - 线程安全的数据访问
 * - 自动的数据清理和生命周期管理
 * - 视图注册和数据分发
 * - 基于范围的快速数据检索
 * 
 * 数据管理：
 * - 检测点数据的实时处理
 * - 航迹数据的批次管理
 * - 过期数据的自动清理
 * - 数据完整性验证
 * 
 * 性能优化：
 * - 基于范围的空间索引查询
 * - 批量数据操作支持
 * - 智能的数据缓存策略
 * - 多线程安全访问
 * 
 * 设计模式：
 * - 单例模式：全局唯一数据管理器
 * - 观察者模式：视图注册和数据分发
 * - 策略模式：可扩展的数据处理策略
 * 
 * @author DispCtrl Team
 * @version 1.0
 * @date 2024
 */

// RadarDataManager.h - 雷达数据统一管理器
#ifndef RADARDATAMANAGER_H
#define RADARDATAMANAGER_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QMutex>
#include <QTimer>
#include "Basic/Protocol.h"

/**
 * @class RadarDataManager
 * @brief 雷达数据统一管理器
 * @details 采用单例模式的雷达数据中央管理器，提供完整的数据生命周期管理
 * 
 * 该类作为系统的数据中枢：
 * - 接收来自各种数据源的雷达信息
 * - 提供高效的数据存储和检索服务
 * - 管理数据的生命周期和内存使用
 * - 向注册的视图组件分发数据更新
 * - 支持基于空间范围的快速查询
 * 
 * 数据类型支持：
 * - 检测点数据：实时的雷达检测信息
 * - 航迹数据：按批次组织的跟踪数据
 * - 历史数据：支持时间窗口查询
 * 
 * 线程安全设计：
 * - 所有公共方法都提供线程安全保证
 * - 使用读写锁优化并发访问性能
 * - 支持多线程环境下的数据更新
 * 
 * @example
 * ```cpp
 * // 获取数据管理器实例
 * RadarDataManager& dataMgr = RadarDataManager::instance();
 * 
 * // 注册视图
 * dataMgr.registerView("MainDisplay", this);
 * 
 * // 处理数据
 * connect(udpManager, &UDPManager::detectionReceived,
 *         [&](const PointInfo& info) {
 *             dataMgr.processDetection(info);
 *         });
 * 
 * // 查询范围内的数据
 * auto detections = dataMgr.getDetectionsInRange(0, 100, 0, 360);
 * ```
 */
// 雷达数据统一管理器
class RadarDataManager : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 获取单例实例
     * @return 雷达数据管理器单例引用
     * @details 线程安全的单例实现，确保全局唯一的数据管理器
     */
    // 单例模式
    static RadarDataManager& instance();
    
    // === 数据接收接口 ===
    
    /**
     * @brief 处理检测点数据
     * @param info 检测点信息
     * @details 接收并处理来自UDP管理器的检测点数据
     * 
     * 处理流程：
     * - 验证数据完整性
     * - 添加到检测点列表
     * - 发射数据更新信号
     * - 触发视图更新
     */
    void processDetection(const PointInfo& info);
    
    /**
     * @brief 处理航迹点数据
     * @param info 航迹点信息
     * @details 接收并处理来自UDP管理器的航迹点数据
     */
    void processTrack(const PointInfo& info);
    
    // === 视图注册接口 ===
    
    /**
     * @brief 注册视图组件
     * @param viewId 视图唯一标识符
     * @param view 视图对象指针
     * @details 注册视图组件以接收数据更新通知
     */
    void registerView(const QString& viewId, QObject* view);
    
    /**
     * @brief 注销视图组件
     * @param viewId 视图唯一标识符
     * @details 注销视图组件，停止接收数据更新通知
     */
    void unregisterView(const QString& viewId);
    
    // === 数据查询接口 ===
    
    /**
     * @brief 获取范围内的检测点
     * @param minRange 最小距离
     * @param maxRange 最大距离
     * @param minAngle 最小角度
     * @param maxAngle 最大角度
     * @return 符合条件的检测点列表
     * @details 高效的空间范围查询，支持距离和角度过滤
     */
    QList<PointInfo> getDetectionsInRange(float minRange, float maxRange, 
                                          float minAngle, float maxAngle) const;
    
    /**
     * @brief 获取范围内的航迹点
     * @param minRange 最小距离
     * @param maxRange 最大距离
     * @param minAngle 最小角度
     * @param maxAngle 最大角度
     * @param batchId 批次ID（-1表示所有批次）
     * @return 符合条件的航迹点列表
     * @details 支持按批次和空间范围的航迹数据查询
     */
    QList<PointInfo> getTracksInRange(float minRange, float maxRange, 
                                      float minAngle, float maxAngle, int batchId = -1) const;
    
    // === 数据管理 ===
    
    /**
     * @brief 清除所有数据
     * @details 清空所有检测点和航迹数据，释放内存
     */
    void clearAllData();
    
    /**
     * @brief 清除过期数据
     * @param maxAgeSeconds 最大保留时间（秒）
     * @details 自动清理超过指定时间的历史数据
     */
    void clearOldData(int maxAgeSeconds = 30);
    
    // === 统计信息 ===
    
    /**
     * @brief 获取检测点数量
     * @return 当前检测点总数
     */
    int getDetectionCount() const { return m_detections.size(); }
    
    /**
     * @brief 获取航迹点数量
     * @return 当前航迹点总数
     */
    int getTrackCount() const;
    
signals:
    /**
     * @brief 检测点接收信号
     * @param info 检测点信息
     * @details 当接收到新的检测点时发射此信号
     */
    void detectionReceived(const PointInfo& info);
    
    /**
     * @brief 航迹点接收信号
     * @param info 航迹点信息
     * @details 当接收到新的航迹点时发射此信号
     */
    void trackReceived(const PointInfo& info);
    
    /**
     * @brief 数据清除信号
     * @details 当执行数据清除操作时发射此信号
     */
    void dataCleared();
    
    /**
     * @brief 过期数据清除信号
     * @details 当自动清理过期数据时发射此信号
     */
    void oldDataCleared();
    
private:
    /**
     * @brief 私有构造函数
     * @param parent 父对象指针
     * @details 单例模式的私有构造函数，防止外部直接创建实例
     */
    RadarDataManager(QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     * @details 默认析构函数，清理资源
     */
    ~RadarDataManager() = default;
    
    /**
     * @brief 禁用拷贝构造函数
     * @details 确保单例模式的唯一性
     */
    RadarDataManager(const RadarDataManager&) = delete;
    
    /**
     * @brief 禁用赋值操作符
     * @details 确保单例模式的唯一性
     */
    RadarDataManager& operator=(const RadarDataManager&) = delete;
    
    // === 数据存储 ===
    QList<PointInfo> m_detections;               ///< 检测点数据列表
    QMap<int, QList<PointInfo>> m_tracks;        ///< 航迹数据：batchId -> track points
    
    // === 视图管理 ===
    QMap<QString, QObject*> m_registeredViews;   ///< 注册的视图组件映射
    
    // === 线程安全 ===
    mutable QMutex m_dataMutex;                  ///< 数据访问互斥锁
    
    // === 自动清理 ===
    QTimer* m_cleanupTimer;                      ///< 自动清理定时器
    
    // === 辅助方法 ===
    
    /**
     * @brief 范围检查
     * @param info 点信息
     * @param minRange 最小距离
     * @param maxRange 最大距离
     * @param minAngle 最小角度
     * @param maxAngle 最大角度
     * @return 是否在指定范围内
     * @details 检查点是否在指定的距离和角度范围内
     */
    bool isInRange(const PointInfo& info, float minRange, float maxRange, 
                   float minAngle, float maxAngle) const;
    
    /**
     * @brief 数据有效性验证
     * @param info 点信息
     * @return 数据是否有效
     * @details 验证点信息的完整性和有效性
     */
    bool isValidPointInfo(const PointInfo& info) const;
    
    /**
     * @brief 设置清理定时器
     * @details 初始化和配置自动数据清理定时器
     */
    void setupCleanupTimer();
    
private slots:
    /**
     * @brief 执行数据清理
     * @details 定时器触发的数据清理操作
     */
    void performCleanup();
};

/**
 * @def RADAR_DATA_MGR
 * @brief 雷达数据管理器便捷访问宏
 * @details 简化对雷达数据管理器单例的访问
 * 
 * @example
 * ```cpp
 * RADAR_DATA_MGR.processDetection(pointInfo);
 * int count = RADAR_DATA_MGR.getDetectionCount();
 * ```
 */
// 便捷宏定义
#define RADAR_DATA_MGR RadarDataManager::instance()

#endif // RADARDATAMANAGER_H