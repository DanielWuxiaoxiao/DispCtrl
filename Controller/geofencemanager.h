/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-04-27 11:21:00
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:49
 * @Description: 
 */
/**
 * @file geofencemanager.h
 * @brief 电子围栏管理器
 * @details 维护多边形围栏列表（极坐标：方位°, 距离m），
 *          对每个到来的 PointInfo 执行射线法 in-poly 检测，入栏时发出 fenceAlert 信号。
 */
#ifndef GEOFENCEMANAGER_H
#define GEOFENCEMANAGER_H

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QString>
#include "Basic/Protocol.h"

/**
 * @brief 单个电子围栏定义
 *
 * 顶点坐标为极坐标 QPointF(azimuthDeg, rangeM)。
 * 方位角为 0~360°（北方 = 0），距离为斜距米。
 */
struct GeoFenceDef {
    int      id      = 0;       ///< 围栏唯一 ID（由 GeoFenceManager 分配）
    QString  name;              ///< 围栏名称（显示用）
    bool     enabled = true;    ///< 是否启用检测

    /// 顶点列表，每个顶点 QPointF(azimuthDeg, rangeM)，至少 3 个点
    QVector<QPointF> vertices;
};

/**
 * @class GeoFenceManager
 * @brief 电子围栏管理器：保存围栏列表，检测目标是否入栏
 *
 * ### 典型用法（在 MainOverLayOut 中）
 * @code
 * m_fenceMgr = new GeoFenceManager(this);
 * connect(CON_INS, &Controller::detInfoProcess,
 *         m_fenceMgr, &GeoFenceManager::checkPoint);
 * connect(CON_INS, &Controller::traInfoProcess,
 *         m_fenceMgr, &GeoFenceManager::checkPoint);
 * connect(m_fenceMgr, &GeoFenceManager::fenceAlert,
 *         m_logPanel, &LogPanel::onFenceAlert);
 * @endcode
 */
class GeoFenceManager : public QObject
{
    Q_OBJECT
public:
    explicit GeoFenceManager(QObject* parent = nullptr);

    // ---- 围栏管理 ----

    /// 添加围栏并返回分配的 ID
    int  addFence(const GeoFenceDef& fence);

    /// 按 ID 移除围栏
    void removeFence(int id);

    /// 更新围栏顶点
    void updateFence(int id, const QVector<QPointF>& vertices);

    /// 设置围栏启用/禁用
    void setFenceEnabled(int id, bool enabled);

    /// 获取所有围栏（只读）
    const QVector<GeoFenceDef>& fences() const { return m_fences; }

    /// 清空所有围栏
    void clearAll();

public slots:
    /**
     * @brief 检查目标点是否进入任何启用的围栏
     * @param pt 检测点 / 航迹点
     */
    void checkPoint(const PointInfo& pt);

signals:
    /**
     * @brief 目标进入围栏时触发
     * @param message  人类可读警告文本
     * @param fenceId  触发的围栏 ID
     * @param pt       触发的目标点
     */
    void fenceAlert(const QString& message, int fenceId, const PointInfo& pt);

private:
    /// 射线法判断点 (azDeg, rangM) 是否在多边形内
    static bool pointInPolygon(double azDeg, double rangM,
                               const QVector<QPointF>& vertices);

    QVector<GeoFenceDef> m_fences;
    int m_nextId = 1;
};

#endif // GEOFENCEMANAGER_H
