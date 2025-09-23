/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:04
 * @Description: 
 */
/**
 * @file point.h
 * @brief 雷达点对象基类和派生类头文件
 * @details 定义雷达显示系统中的点对象体系：
 *          - Point基类：通用点对象功能
 *          - DetPoint：检测点(原始雷达回波)
 *          - TrackPoint：航迹点(处理后的目标轨迹)
 *          - 支持动态缩放、颜色变化、悬停效果
 * @author DispCtrl Team
 * @date 2024
 */

#ifndef POINT_H
#define POINT_H

#include <QGraphicsEllipseItem>
#include <QColor>
#include "Basic/Protocol.h"

/**
 * @defgroup PointSizeConstants 点尺寸常量定义
 * @brief 不同类型点的显示尺寸配置
 * @note 这些值现在可以通过config.toml配置文件调整
 * @{
 */
constexpr int DET_SIZE = 1;      ///< 检测点普通尺寸(像素) - 可通过CF_INS.pointSize("DET_SIZE", DET_SIZE)获取
constexpr int DET_BIG_SIZE = 3;  ///< 检测点放大尺寸(像素) - 可通过CF_INS.pointSize("DET_BIG_SIZE", DET_BIG_SIZE)获取
constexpr int TRA_SIZE = 3;      ///< 航迹点普通尺寸(像素) - 可通过CF_INS.pointSize("TRA_SIZE", TRA_SIZE)获取
constexpr int TRA_BIG_SIZE = 10; ///< 航迹点放大尺寸(像素) - 可通过CF_INS.pointSize("TRA_BIG_SIZE", TRA_BIG_SIZE)获取
/** @} */

/**
 * @class Point
 * @brief 雷达点对象基类
 * @details 所有雷达显示点的抽象基类，提供：
 *          - 统一的位置更新机制
 *          - 动态尺寸缩放功能
 *          - 鼠标悬停交互效果
 *          - 工具提示信息显示
 *          - 颜色和外观管理
 * 
 * 继承体系：
 * Point (基类)
 *  ├── DetPoint (检测点)
 *  └── TrackPoint (航迹点)
 * 
 * @example 基本使用流程：
 * @code
 * PointInfo info = getRadarData();
 * DetPoint* point = new DetPoint(info);
 * point->updatePosition(x, y);
 * point->resize(zoomRatio);
 * scene->addItem(point);
 * @endcode
 */
class Point : public QGraphicsEllipseItem
{
public:
    /**
     * @brief 构造函数
     * @param info 雷达点信息，包含位置、类型、批次等数据
     * @details 初始化点对象：
     *          - 保存雷达数据信息
     *          - 设置图形项属性
     *          - 启用鼠标悬停事件
     *          - 初始化尺寸参数
     */
    explicit Point(PointInfo &info);
    
    /**
     * @brief 虚析构函数
     * @details 确保派生类对象正确析构
     */
    virtual ~Point() = default;

    /**
     * @brief 更新点的屏幕位置
     * @param x 像素坐标X值
     * @param y 像素坐标Y值
     * @details 由PointManager统一调用，更新点在场景中的位置：
     *          - 转换雷达坐标到屏幕坐标
     *          - 更新图形项的位置
     *          - 保持点的几何中心对齐
     */
    void updatePosition(float x, float y);
    
    /**
     * @brief 调整点大小(纯虚函数)
     * @param ratio 缩放比例，1.0为原始大小
     * @details 派生类必须实现，用于：
     *          - 响应视图缩放变化
     *          - 保持点在不同缩放级别下的可见性
     *          - 维护视觉比例关系
     */
    virtual void resize(float ratio) = 0;
    
    /**
     * @brief 设置点颜色(纯虚函数)
     * @param color 新的颜色值
     * @details 派生类实现具体的颜色设置逻辑：
     *          - 根据点类型应用不同颜色方案
     *          - 支持状态相关的颜色变化
     *          - 更新画笔和画刷颜色
     */
    virtual void setColor(QColor color) = 0;

    /**
     * @brief 获取点信息引用
     * @return 雷达点信息的常量引用
     * @details 提供对原始雷达数据的只读访问
     */
    const PointInfo& infoRef() const { return info; }

protected:
    /**
     * @defgroup HoverEvents 鼠标悬停事件处理
     * @brief 鼠标悬停交互效果的事件处理方法
     * @{
     */
    
    /**
     * @brief 鼠标进入事件
     * @param event 悬停事件对象
     * @details 鼠标悬停时的响应：
     *          - 放大点的显示尺寸
     *          - 显示详细信息工具提示
     *          - 增强视觉反馈
     */
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    
    /**
     * @brief 鼠标移动事件
     * @param event 悬停事件对象
     * @details 鼠标在点上移动时更新工具提示位置
     */
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    
    /**
     * @brief 鼠标离开事件
     * @param event 悬停事件对象
     * @details 鼠标离开时的响应：
     *          - 恢复点的正常尺寸
     *          - 隐藏工具提示
     *          - 恢复默认外观
     */
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    
    /** @} */ // end of HoverEvents group

    /**
     * @defgroup SizeControl 尺寸控制方法
     * @brief 点尺寸变化的虚函数接口
     * @{
     */
    
    /**
     * @brief 设置普通尺寸矩形
     * @details 虚函数，子类可重写以定制普通状态的显示尺寸
     */
    virtual void setSmallRect();
    
    /**
     * @brief 设置放大尺寸矩形
     * @details 虚函数，子类可重写以定制悬停状态的放大尺寸
     */
    virtual void setBigRect();
    
    /** @} */ // end of SizeControl group

protected:
    // 核心数据
    PointInfo info;              ///< 雷达点信息数据
    QString text;                ///< 工具提示文本内容
    
    // 当前显示尺寸(像素)
    float w = 6.f, h = 6.f;      ///< 当前宽度和高度
    float W = 10.f, H = 10.f;    ///< 放大状态的宽度和高度

    // 屏幕坐标记录
    float mX = 0.f, mY = 0.f;    ///< 点中心的像素坐标

    // 基础尺寸配置 - 子类可重写
    float baseSmallW = 6.f, baseSmallH = 6.f;  ///< 基础普通尺寸
    float baseBigW   = 10.f, baseBigH   = 10.f; ///< 基础放大尺寸

    // 缩放状态
    float curRatio = 1.f;        ///< 当前缩放比例，避免重复计算
};

/**
 * @class DetPoint
 * @brief 检测点类
 * @details 表示雷达原始检测回波的点对象：
 *          - 较小的显示尺寸，反映原始数据特性
 *          - 通常数量较多，需要优化显示性能
 *          - 颜色通常表示回波强度或距离信息
 * 
 * 特点：
 * - 小尺寸显示(1-3像素)
 * - 快速更新频率
 * - 简单的几何形状
 */
class DetPoint : public Point
{
public:
    /**
     * @brief 构造函数
     * @param info 检测点信息
     * @details 初始化检测点特有的显示参数和外观
     */
    explicit DetPoint(PointInfo &info);
    
    /**
     * @brief 实现缩放功能
     * @param ratio 缩放比例
     * @details 检测点的缩放实现，保持较小的显示尺寸
     */
    void resize(float ratio) override;
    
    /**
     * @brief 实现颜色设置
     * @param color 颜色值
     * @details 检测点的颜色设置，通常用于表示回波强度
     */
    void setColor(QColor color) override;

protected:
    /**
     * @brief 设置检测点普通尺寸
     * @details 重写基类方法，设置检测点特有的小尺寸
     */
    void setSmallRect() override;
    
    /**
     * @brief 设置检测点放大尺寸
     * @details 重写基类方法，设置检测点悬停时的放大尺寸
     */
    void setBigRect() override;
};

/**
 * @class TrackPoint
 * @brief 航迹点类
 * @details 表示处理后的目标航迹点对象：
 *          - 较大的显示尺寸，突出重要目标
 *          - 通常数量较少但信息丰富
 *          - 颜色可能表示目标类型或威胁等级
 * 
 * 特点：
 * - 较大尺寸显示(3-10像素)
 * - 包含更多目标信息
 * - 可能有特殊标识符号
 */
class TrackPoint : public Point
{
public:
    /**
     * @brief 构造函数
     * @param info 航迹点信息
     * @details 初始化航迹点特有的显示参数和外观
     */
    explicit TrackPoint(PointInfo &info);
    
    /**
     * @brief 实现缩放功能
     * @param ratio 缩放比例
     * @details 航迹点的缩放实现，保持较大的显示尺寸
     */
    void resize(float ratio) override;
    
    /**
     * @brief 实现颜色设置
     * @param color 颜色值
     * @details 航迹点的颜色设置，通常用于表示目标类型
     */
    void setColor(QColor color) override;

protected:
    /**
     * @brief 设置航迹点普通尺寸
     * @details 重写基类方法，设置航迹点特有的中等尺寸
     */
    void setSmallRect() override;
    
    /**
     * @brief 设置航迹点放大尺寸
     * @details 重写基类方法，设置航迹点悬停时的大尺寸
     */
    void setBigRect() override;
};

#endif // POINT_H
