/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 10:19:13
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 10:23:32
 * @Description: 
 */
/**
 * @file geofenceitem.h
 * @brief PPI 电子围栏多边形图形项
 * @details 以极坐标顶点（方位°, 距离m）描述围栏边界，
 *          转换为 PPIScene 像素坐标后渲染为半透明黄色多边形。
 *          告警时自动变为红色虚线。
 */
#ifndef GEOFENCEITEM_H
#define GEOFENCEITEM_H

#include <QGraphicsPolygonItem>
#include <QVector>
#include <QPointF>
#include <QString>

/**
 * @class GeoFenceItem
 * @brief PPI 上的电子围栏多边形图形项
 *
 * z-value = 50（回波层之上、检测点层之下）
 *
 * ### 典型用法
 * @code
 * auto* item = new GeoFenceItem(fenceId, nullptr);
 * item->setFenceName("围栏A");
 * item->setVerticesPolar(vertices, pxPerMeter);  // 设置顶点并更新多边形
 * ppiScene->addItem(item);
 * // 告警时
 * item->setAlert(true);
 * @endcode
 */
class GeoFenceItem : public QGraphicsPolygonItem
{
public:
    explicit GeoFenceItem(int fenceId, QGraphicsItem* parent = nullptr);

    int fenceId() const { return m_id; }

    void setFenceName(const QString& name);

    /**
     * @brief 设置极坐标顶点并更新 QGraphicsPolygonItem
     * @param vertices  顶点列表，每个 QPointF(azimuthDeg, rangeM)
     * @param pxPerMeter 当前 PPI 场景中每米对应的像素数
     * @details PPI 以 (0,0) 为中心，正北为 -Y 轴。
     *          转换公式：x = range*sin(az), y = -range*cos(az)
     */
    void setVerticesPolar(const QVector<QPointF>& vertices, double pxPerMeter);

    /// 进入告警状态（红色虚线）/ 正常状态（黄色实线）
    void setAlert(bool alert);

    /// 编辑模式开关：允许整体拖拽并高亮顶点手柄
    void setEditMode(bool edit);

protected:
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

private:
    int     m_id    = -1;
    QString m_name;
    bool    m_alert = false;
    bool    m_edit  = false;
};

#endif // GEOFENCEITEM_H
