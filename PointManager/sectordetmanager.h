/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 10:04:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:06
 * @Description: 
 */
/**
 * @file sectordetmanager.h
 * @brief 扇形检测点管理器头文件
 * @details 专门用于扇形显示区域的检测点管理系统：
 *          - 管理扇形区域内的雷达原始检测点
 *          - 提供扇形角度范围的动态过滤
 *          - 优化扇形显示的性能和视觉效果
 *          - 与主检测点管理器形成互补关系
 * @author DispCtrl Team
 * @date 2024
 */

#ifndef SECTORDETMANAGER_H
#define SECTORDETMANAGER_H

#include <QObject>
#include <QGraphicsScene>
#include <QVector>
#include "point.h"
#include "PolarDisp/polaraxis.h"

/**
 * @struct SectorDetNode
 * @brief 扇形检测点节点结构
 * @details 封装扇形区域内检测点对象的容器结构：
 *          - 管理DetPoint对象的生命周期
 *          - 专门用于扇形显示优化
 */
struct SectorDetNode {
    DetPoint* point = nullptr;   ///< 检测点对象指针
};

/**
 * @class SectorDetManager
 * @brief 扇形检测点管理器
 * @details 专用于扇形显示区域的检测点管理器，负责：
 *          - 扇形区域内检测点的创建、更新和销毁
 *          - 扇形角度范围的实时过滤和显示
 *          - 与扇形网格系统的协调显示
 *          - 扇形区域的性能优化管理
 * 
 * 功能特点：
 * - 专门针对扇形显示区域优化
 * - 支持动态角度范围调整
 * - 高效的扇形过滤算法
 * - 与主PPI显示的分离管理
 * 
 * @example 基本使用：
 * @code
 * SectorDetManager* mgr = new SectorDetManager(scene, axis);
 * mgr->setAngleRange(-30, 30);     // 设置扇形范围
 * mgr->addDetPoint(pointInfo);     // 添加检测点
 * mgr->refreshAll();               // 刷新显示
 * @endcode
 */
class SectorDetManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param scene 图形场景指针，检测点将添加到此场景
     * @param axis 极坐标轴指针，提供坐标变换参数
     * @param parent 父对象指针
     * @details 初始化扇形检测点管理器：
     *          - 关联图形场景和坐标轴
     *          - 设置默认的扇形角度范围
     *          - 初始化扇形点容器
     */
    explicit SectorDetManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent = nullptr);
    
    /**
     * @brief 虚析构函数
     * @details 清理所有扇形检测点对象和相关资源
     */
    virtual ~SectorDetManager();

    /**
     * @brief 添加检测点到扇形区域
     * @param info 检测点信息，包含极坐标位置、强度等数据
     * @details 向扇形区域添加新的检测点：
     *          1. 验证检测点是否在扇形范围内
     *          2. 创建DetPoint对象并配置外观
     *          3. 计算屏幕坐标并更新位置
     *          4. 应用扇形过滤和显示设置
     */
    void addDetPoint(const PointInfo& info);
    
    /**
     * @brief 刷新所有扇形检测点显示
     * @details 当坐标轴参数或扇形范围变化时调用：
     *          - 重新计算所有检测点的屏幕坐标
     *          - 重新应用扇形角度过滤
     *          - 更新点的显示位置和可见性
     */
    void refreshAll();
    
    /**
     * @brief 设置扇形区域全局可见性
     * @param visible true显示所有扇形检测点，false隐藏所有扇形检测点
     * @details 批量控制扇形区域内所有检测点的可见性
     */
    void setAllVisible(bool visible);
    
    /**
     * @brief 设置扇形检测点尺寸比例
     * @param ratio 缩放比例，1.0为默认大小
     * @details 批量调整扇形区域内所有检测点的显示尺寸
     */
    void setPointSizeRatio(float ratio);
    
    /**
     * @brief 设置扇形角度范围
     * @param minAngle 最小角度(度)
     * @param maxAngle 最大角度(度)
     * @details 设置扇形显示的角度范围：
     *          - 只显示指定角度扇形内的检测点
     *          - 实时过滤现有和新增的检测点
     *          - 通常用于雷达扇形扫描显示
     */
    void setAngleRange(float minAngle, float maxAngle);
    
    /**
     * @brief 清理所有扇形检测点
     * @details 删除扇形区域内的所有检测点对象：
     *          - 从场景中移除所有点
     *          - 释放相关内存资源
     *          - 重置内部状态
     */
    void clear();
    
    /**
     * @brief 获取扇形检测点数量
     * @return 当前扇形区域内的检测点总数
     * @details 用于性能监控和状态查询
     */
    int pointCount() const { return m_nodes.size(); }

private:
    /**
     * @brief 极坐标转屏幕坐标
     * @param range 距离值(公里)
     * @param azimuthDeg 方位角(度)
     * @return 屏幕像素坐标点
     * @details 利用PolarAxis进行坐标变换
     */
    QPointF polarToPixel(float range, float azimuthDeg) const;
    
    /**
     * @brief 检查距离是否在显示范围内
     * @param range 距离值(公里)
     * @return true表示在显示范围内
     * @details 根据PolarAxis的最小/最大距离判断
     */
    bool inRange(float range) const;
    
    /**
     * @brief 检查角度是否在扇形范围内
     * @param azimuthDeg 方位角(度)
     * @return true表示在扇形范围内
     * @details 判断检测点是否在设置的扇形角度范围内
     */
    bool inAngle(float azimuthDeg) const;
    
    /**
     * @brief 检查点是否应该可见
     * @param info 检测点信息
     * @return true表示点应该可见
     * @details 综合判断点的距离和角度是否都在显示范围内
     */
    bool isPointVisible(const PointInfo& info) const;

private:
    // 核心组件引用
    QGraphicsScene* m_scene;                ///< 图形场景指针
    PolarAxis* m_axis;                     ///< 极坐标轴指针
    
    // 扇形检测点管理
    QVector<SectorDetNode> m_nodes;        ///< 扇形检测点节点容器
    
    // 显示控制参数
    bool m_visible = true;                 ///< 全局可见性标志
    float m_pointSizeRatio = 1.0f;         ///< 点尺寸缩放比例
    
    // 扇形角度范围参数
    float m_minAngle = -30.0f;             ///< 扇形最小角度(度)
    float m_maxAngle = 30.0f;              ///< 扇形最大角度(度)
};

#endif // SECTORDETMANAGER_H