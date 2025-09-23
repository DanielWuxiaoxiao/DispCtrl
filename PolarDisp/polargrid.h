/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:09
 * @Description: 
 */
/**
 * @file polargrid.h
 * @brief 极坐标网格绘制器头文件
 * @details 雷达PPI显示的极坐标网格系统：
 *          - 绘制同心距离圆环和径向方位线
 *          - 提供距离和角度刻度标识
 *          - 支持可配置的角度范围显示
 *          - 包含雷达中心图标显示
 * @author DispCtrl Team
 * @date 2024
 */

#ifndef POLARGRID_H
#define POLARGRID_H

#pragma once
#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <vector>
#include <QPen>
#include "PolarDisp/polaraxis.h"

/**
 * @class PolarGrid
 * @brief 极坐标网格绘制器
 * @details 负责在PPI场景中绘制极坐标网格系统：
 *          - 距离圆环：表示不同距离的同心圆
 *          - 方位线：从中心向外的径向线，表示角度
 *          - 刻度标识：距离和角度的数值标注
 *          - 雷达图标：中心位置的雷达符号
 * 
 * 网格组成：
 * - 距离圆环：根据PolarAxis的距离范围绘制
 * - 方位线：根据设置的角度范围绘制径向线
 * - 文字标注：距离值和角度值的标识
 * - 雷达图标：中心的雷达位置标识
 * 
 * @example 基本使用：
 * @code
 * PolarGrid* grid = new PolarGrid(scene, axis);
 * grid->setAngleRange(0, 360);  // 设置全角度显示
 * grid->updateGrid();           // 更新网格显示
 * @endcode
 */
class PolarGrid : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief 构造函数
     * @param scene 图形场景指针，网格将绘制在此场景中
     * @param axis 极坐标轴指针，提供坐标变换和范围信息
     * @param parent 父对象指针
     * @details 初始化极坐标网格绘制器：
     *          - 关联图形场景和坐标轴
     *          - 设置默认角度范围(0-360度)
     *          - 准备图形项容器
     */
    explicit PolarGrid(QGraphicsScene* scene, PolarAxis* axis, QObject* parent = nullptr);

public slots:
    /**
     * @brief 更新网格显示
     * @details 重新绘制整个极坐标网格：
     *          1. 清除现有的网格图形项
     *          2. 根据当前坐标轴参数计算网格位置
     *          3. 绘制距离圆环和方位线
     *          4. 添加刻度文字标注
     *          5. 更新雷达中心图标位置
     * 
     * 调用时机：
     * - 坐标轴范围改变时
     * - 场景尺寸变化时
     * - 角度范围设置变化时
     */
    void updateGrid();
    
    /**
     * @brief 设置角度显示范围
     * @param startDeg 起始角度(度)，0度为正北方向
     * @param endDeg 结束角度(度)，顺时针方向
     * @details 设置要显示的角度扇区：
     *          - 支持任意角度范围(如90-270度显示半圆)
     *          - 影响方位线的绘制数量和位置
     *          - 设置后需调用updateGrid()生效
     * 
     * @example 扇区显示：
     * @code
     * grid->setAngleRange(270, 90);  // 显示前方180度扇区
     * grid->updateGrid();
     * @endcode
     */
    void setAngleRange(double startDeg, double endDeg);

private:
    // 关联组件
    QGraphicsScene* mScene;     ///< 图形场景指针
    PolarAxis* mAxis;          ///< 极坐标轴指针
    
    // 角度范围配置
    double m_angleStart = 0.0;   ///< 起始角度(度)
    double m_angleEnd = 360.0;   ///< 结束角度(度)
    
    // 图形项容器 - 用于管理绘制的图形元素生命周期
    QList<QGraphicsItem*> mCircleItems;  ///< 距离圆环图形项列表
    QList<QGraphicsItem*> mTickItems;    ///< 刻度线图形项列表  
    QList<QGraphicsItem*> mTextItems;    ///< 文字标注图形项列表
    QGraphicsPixmapItem* mRadarIcon = nullptr; ///< 雷达中心图标
};

#endif // POLARGRID_H
