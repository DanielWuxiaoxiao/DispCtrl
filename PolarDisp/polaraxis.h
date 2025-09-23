/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:08
 * @Description: 
 */
/**
 * @file polaraxis.h
 * @brief 极坐标轴系统头文件
 * @details 定义了极坐标轴系统类，用于雷达显示系统中极坐标与场景坐标的转换。
 *          提供距离-像素转换、极坐标-场景坐标转换等核心功能，支持PPI雷达显示。
 * @author DispCtrl Development Team
 * @date 2024
 * @version 1.0
 */

#ifndef POLARAXIS_H
#define POLARAXIS_H
#pragma once
#include <cmath>
#include <QPointF>
#include <QObject>
// 坐标系约定
// 转换关系
// 0°方位角 → 负Y轴方向（正北）
// 90°方位角 → 正X轴方向（正东）
// 180°方位角 → 正Y轴方向（正南）
// 270°方位角 → 负X轴方向（正西）
// 雷达坐标系：     Qt场景坐标系：
//      0°               Y
//      ↑                ↓
// 270°←●→90°      X ←---●
//      ↓               
//     180°
/**
 * @class PolarAxis
 * @brief 极坐标轴系统类
 * @details 为雷达PPI显示提供极坐标系统支持，实现物理距离与屏幕像素、
 *          极坐标与场景坐标之间的双向转换功能。支持自定义距离范围和像素密度设置。
 * 
 * 主要功能：
 * - 设置和管理距离范围（最小/最大距离）
 * - 配置像素密度（每米像素数）
 * - 距离与像素的双向转换
 * - 极坐标与场景坐标的双向转换
 * - 坐标系统变化事件通知
 * 
 * 坐标系约定：
 * - 极坐标：距离(米) + 方位角(度，0°为正北，顺时针递增)
 * - 场景坐标：右手坐标系，X轴向右，Y轴向下
 * - 转换关系：0°方位角对应负Y轴方向
 * 
 * @note 继承自QObject，支持Qt信号槽机制
 */
class PolarAxis : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针，用于Qt对象树管理
     */
    explicit PolarAxis(QObject* parent = nullptr);

    /**
     * @brief 设置距离范围
     * @param minR 最小显示距离（米）
     * @param maxR 最大显示距离（米）
     * 
     * 功能说明：
     * - 设置PPI显示的有效距离范围
     * - 发射rangeChanged信号通知范围变化
     * - 用于限制显示内容和优化渲染性能
     */
    void setRange(double minR, double maxR);
    
    /**
     * @brief 获取最小距离范围
     * @return double 最小显示距离（米）
     */
    double minRange() const { return m_minRange; }
    
    /**
     * @brief 获取最大距离范围  
     * @return double 最大显示距离（米）
     */
    double maxRange() const { return m_maxRange; }
    
    /**
     * @brief 设置像素密度
     * @param pixelsPerMeter 每米对应的像素数
     * 
     * 用途：
     * - 控制显示比例尺
     * - 影响距离-像素转换精度
     * - 适配不同分辨率的显示设备
     */
    void setPixelsPerMeter(double pixelsPerMeter);

    /**
     * @brief 物理距离转像素距离
     * @param distance 物理距离（米）
     * @return double 对应的像素距离
     * 
     * 转换公式：像素距离 = 物理距离 × 像素密度
     */
    double rangeToPixel(double distance) const;
    
    /**
     * @brief 像素距离转物理距离
     * @param pixel 像素距离
     * @return double 对应的物理距离（米）
     * 
     * 转换公式：物理距离 = 像素距离 ÷ 像素密度
     */
    double pixelToRange(double pixel) const;

    /**
     * @brief 极坐标转场景坐标
     * @param distance 距离（米）
     * @param azimuthDeg 方位角（度，0°为正北，顺时针为正）
     * @return QPointF 场景坐标点(x, y)
     * 
     * 转换说明：
     * - 将雷达极坐标转换为Qt场景坐标系
     * - X轴：向右为正，计算公式 x = r × sin(θ)
     * - Y轴：向下为正，计算公式 y = -r × cos(θ)
     * - 0°方位角对应负Y轴方向（正北）
     */
    QPointF polarToScene(double distance, double azimuthDeg) const;
    
    /**
     * @brief 极坐标结构体
     * @details 用于返回场景坐标转极坐标的结果
     */
    struct PolarCoord {
        double distance;    ///< 距离（米）
        double azimuthDeg;  ///< 方位角（度，0-360°）
    };
    
    /**
     * @brief 场景坐标转极坐标
     * @param scenePos 场景坐标点
     * @return PolarCoord 极坐标{距离, 方位角}
     * 
     * 转换说明：
     * - 距离计算：r = √(x² + y²)
     * - 方位角计算：θ = atan2(x, -y)，转换为0-360°范围
     * - 自动处理负角度，确保方位角在[0, 360°)范围内
     */
    PolarCoord sceneToPolar(const QPointF& scenePos) const;

signals:
    /**
     * @brief 距离范围变化信号
     * @param minR 新的最小距离
     * @param maxR 新的最大距离
     * 
     * 触发时机：
     * - 调用setRange()函数时发射
     * - 用于通知依赖组件更新显示范围
     */
    void rangeChanged(double minR, double maxR);

private:
    double m_minRange = 0.0;         ///< 最小显示距离（米）
    double m_maxRange = 5000.0;      ///< 最大显示距离（米），默认5km
    double m_pixelsPerMeter = 1.0;   ///< 像素密度（像素/米）
};
#endif // POLARAXIS_H


