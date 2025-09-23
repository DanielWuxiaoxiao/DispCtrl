/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:03
 * @Description: 
 */
/**
 * @file detmanager.h
 * @brief 检测点管理器头文件
 * @details 雷达检测点的统一管理系统：
 *          - 管理所有雷达原始检测点的生命周期
 *          - 提供坐标变换和显示控制
 *          - 支持角度范围过滤和可见性控制
 *          - 优化大量检测点的显示性能
 * @author DispCtrl Team
 * @date 2024
 */

#ifndef DET_MANAGER_H
#define DET_MANAGER_H

#pragma once
#include <QObject>
#include <QGraphicsScene>
#include <QVector>
#include "point.h"
#include "PolarDisp/polaraxis.h"

/**
 * @struct DetNode
 * @brief 检测点节点结构
 * @details 封装检测点对象的容器结构：
 *          - 管理DetPoint对象的生命周期
 *          - 提供统一的访问接口
 *          - 支持批量操作和内存管理
 */
struct DetNode {
    DetPoint* point = nullptr;   ///< 检测点对象指针
};

/**
 * @class DetManager
 * @brief 检测点管理器
 * @details 雷达原始检测点的统一管理器，负责：
 *          - 检测点的创建、更新和销毁
 *          - 极坐标到屏幕坐标的转换
 *          - 角度范围和距离范围的过滤
 *          - 显示状态和尺寸的批量控制
 *          - 性能优化的批量操作
 * 
 * 功能特点：
 * - 高效的大量点对象管理
 * - 实时坐标变换
 * - 角度扇区过滤
 * - 动态显示控制
 * 
 * @example 基本使用：
 * @code
 * DetManager* mgr = new DetManager(scene, axis);
 * mgr->setAngleRange(0, 180);  // 设置显示扇区
 * mgr->addDetPoint(pointInfo); // 添加检测点
 * mgr->refreshAll();           // 刷新显示
 * @endcode
 */
class DetManager : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 构造函数
     * @param scene 图形场景指针，检测点将添加到此场景
     * @param axis 极坐标轴指针，提供坐标变换参数
     * @param parent 父对象指针
     * @details 初始化检测点管理器：
     *          - 关联图形场景和坐标轴
     *          - 设置默认显示参数
     *          - 初始化点容器
     */
    explicit DetManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     * @details 清理所有检测点对象和相关资源
     */
    ~DetManager();

    /**
     * @brief 添加检测点
     * @param info 检测点信息，包含极坐标位置、强度等数据
     * @details 创建新的检测点对象：
     *          1. 验证检测点数据有效性
     *          2. 检查是否在显示范围内
     *          3. 创建DetPoint对象并添加到场景
     *          4. 计算屏幕坐标并更新位置
     *          5. 应用当前的显示设置
     */
    void addDetPoint(const PointInfo& info);

    /**
     * @brief 刷新所有检测点
     * @details 当坐标轴参数变化时调用：
     *          - 重新计算所有检测点的屏幕坐标
     *          - 更新点的显示位置
     *          - 应用距离和角度过滤
     *          - 重新应用可见性设置
     * 
     * 调用时机：
     * - PolarAxis的最小/最大距离变化
     * - 像素范围(pixelRange)变化
     * - 场景尺寸变化
     */
    void refreshAll();

    /**
     * @brief 设置全局显示状态
     * @param vis true显示所有检测点，false隐藏所有检测点
     * @details 批量控制所有检测点的可见性：
     *          - 用于检测点图层的开关控制
     *          - 优化显示性能
     *          - 支持分层显示管理
     */
    void setAllVisible(bool vis);

    /**
     * @brief 设置点尺寸缩放比例
     * @param ratio 缩放比例，1.0为默认大小
     * @details 批量调整所有检测点的显示尺寸：
     *          - 配合视图缩放级别调整
     *          - 保持检测点在不同缩放下的可见性
     *          - 维护视觉比例关系
     */
    void setPointSizeRatio(float ratio);

    /**
     * @brief 清理所有检测点
     * @details 删除所有检测点对象：
     *          - 从场景中移除所有检测点
     *          - 释放相关内存资源
     *          - 重置内部状态
     *          - 用于数据重置或场景切换
     */
    void clear();

    /**
     * @brief 设置角度显示范围
     * @param startDeg 起始角度(度)，0度为正北方向
     * @param endDeg 结束角度(度)，顺时针方向
     * @details 设置检测点的角度过滤范围：
     *          - 只显示指定角度扇区内的检测点
     *          - 支持任意角度范围(如90-270度)
     *          - 实时过滤现有检测点
     *          - 影响后续添加的检测点
     * 
     * @example 前方半圆显示：
     * @code
     * manager->setAngleRange(270, 90);  // 显示前方180度
     * @endcode
     */
    void setAngleRange(double startDeg, double endDeg);

private:
    /**
     * @brief 极坐标转屏幕坐标
     * @param range 距离值(公里)
     * @param azimuthDeg 方位角(度)
     * @return 屏幕像素坐标点
     * @details 利用PolarAxis进行坐标变换：
     *          - 将雷达极坐标转换为屏幕像素坐标
     *          - 考虑当前的显示范围和缩放级别
     *          - 处理坐标系旋转和偏移
     */
    QPointF polarToPixel(float range, float azimuthDeg) const;
    
    /**
     * @brief 检查距离是否在显示范围内
     * @param range 距离值(公里)
     * @return true表示在显示范围内
     * @details 根据PolarAxis的最小/最大距离判断点是否可见
     */
    bool inRange(float range) const;

    /**
     * @brief 检查角度是否在显示扇区内
     * @param azimuthDeg 方位角(度)
     * @return true表示在显示扇区内
     * @details 判断检测点是否在设置的角度范围内：
     *          - 处理角度跨越0度的情况
     *          - 支持任意角度扇区定义
     */
    bool inAngle(float azimuthDeg) const;

private:
    // 核心组件引用
    QGraphicsScene* mScene = nullptr;     ///< 图形场景指针
    PolarAxis* mAxis = nullptr;          ///< 极坐标轴指针
    
    // 检测点管理
    QVector<DetNode> mNodes;             ///< 检测点节点容器
    
    // 显示控制参数
    float mPointSizeRatio = 1.f;         ///< 点尺寸缩放比例
    bool mVisible = true;                ///< 全局可见性标志
    
    // 角度过滤参数
    double m_angleStart = 0.0;           ///< 起始角度(度)
    double m_angleEnd = 360.0;           ///< 结束角度(度)
};

#endif // DET_MANAGER_H
