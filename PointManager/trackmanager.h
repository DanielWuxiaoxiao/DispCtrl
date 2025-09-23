/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:07
 * @Description: 
 */
/**
 * @file trackmanager.h
 * @brief 航迹点管理器头文件
 * @details 雷达航迹数据的统一管理系统：
 *          - 管理多条航迹的生命周期和显示
 *          - 提供航迹点连线和标签功能
 *          - 支持批次管理和动态标签
 *          - 优化航迹数据的可视化效果
 * @author DispCtrl Team
 * @date 2024
 */

#pragma once
#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMap>
#include <QVector>
#include "point.h"
#include "PolarDisp/polaraxis.h"

/**
 * @class DraggableLabel
 * @brief 可拖拽的航迹标签
 * @details 航迹信息标签，支持：
 *          - 用户交互拖拽移动
 *          - 自动保持与锚点的连线
 *          - 动态更新连线几何形状
 *          - 高层级显示（在点和线之上）
 * 
 * 特点：
 * - 继承自QGraphicsTextItem，支持文本显示
 * - 可拖拽移动，提升用户体验
 * - 智能连线管理，保持视觉关联
 */
class DraggableLabel : public QGraphicsTextItem
{
public:
    /**
     * @brief 构造函数
     * @param parent 父图形项指针
     * @details 初始化可拖拽标签：
     *          - 启用拖拽和几何变化监听
     *          - 设置合适的Z值层级
     */
    DraggableLabel(QGraphicsItem* parent = nullptr);
    
    /**
     * @brief 设置锚点关联
     * @param anchor 锚点图形项（通常是航迹点）
     * @param tether 连接线图形项
     * @details 建立标签与锚点的关联关系，用于自动更新连线
     */
    void setAnchorItem(QGraphicsItem* anchor, QGraphicsLineItem* tether);

protected:
    /**
     * @brief 图形项变化事件处理
     * @param change 变化类型
     * @param value 变化值
     * @return 处理后的值
     * @details 监听位置变化，自动更新与锚点的连线几何
     */
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QGraphicsItem* anchor = nullptr;        ///< 锚点图形项指针
    QGraphicsLineItem* tether = nullptr;    ///< 连接线图形项指针
};

/**
 * @struct TrackNode
 * @brief 航迹节点结构
 * @details 封装单个航迹点及其连线信息：
 *          - 管理航迹点对象
 *          - 管理与前一点的连线
 *          - 支持时间序列的航迹重建
 */
struct TrackNode {
    TrackPoint* point = nullptr;                ///< 航迹点对象指针
    QGraphicsLineItem* lineFromPrev = nullptr; ///< 与前一节点的连线
    // 如需时间戳/序列号，可在此处再加字段
};

/**
 * @struct TrackSeries  
 * @brief 航迹序列结构
 * @details 表示一条完整的航迹路径：
 *          - 包含该航迹的所有历史点
 *          - 管理航迹的可见性和颜色
 *          - 提供动态标签显示功能
 *          - 支持航迹的批量操作
 */
struct TrackSeries {
    QVector<TrackNode> nodes;                   ///< 航迹节点序列
    DraggableLabel* label = nullptr;            ///< 最新点标签
    QGraphicsLineItem* labelLine = nullptr;     ///< 标签到最新点的连线
    bool visible = true;                        ///< 航迹可见性标志
    QColor color;                               ///< 航迹颜色
};

/**
 * @class TrackManager
 * @brief 航迹管理器
 * @details 雷达航迹数据的统一管理器，负责：
 *          - 多条航迹的创建、更新和销毁
 *          - 航迹点之间的连线绘制
 *          - 动态标签的显示和交互
 *          - 批次分组和颜色管理
 *          - 角度范围和距离范围过滤
 * 
 * 功能特点：
 * - 支持多航迹并发显示
 * - 航迹点自动连线
 * - 可拖拽的动态标签
 * - 批次颜色定制
 * - 实时过滤功能
 * 
 * @example 基本使用：
 * @code
 * TrackManager* mgr = new TrackManager(scene, axis);
 * mgr->addTrackPoint(trackInfo);          // 添加航迹点
 * mgr->setBatchColor(1, Qt::red);         // 设置航迹颜色
 * mgr->setBatchVisible(1, true);          // 控制航迹可见性
 * @endcode
 */
class TrackManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param scene 图形场景指针，航迹将添加到此场景
     * @param axis 极坐标轴指针，提供坐标变换参数
     * @param parent 父对象指针
     * @details 初始化航迹管理器：
     *          - 关联图形场景和坐标轴
     *          - 设置默认显示参数
     *          - 初始化航迹容器
     */
    explicit TrackManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     * @details 清理所有航迹对象和相关资源
     */
    ~TrackManager();

    /**
     * @brief 添加航迹点
     * @param info 航迹点信息，包含批次ID、位置等数据
     * @details 向指定批次添加新的航迹点：
     *          1. 根据批次ID查找或创建航迹序列
     *          2. 创建TrackPoint对象并配置外观
     *          3. 与前一点建立连线关系
     *          4. 更新动态标签显示
     *          5. 应用当前的过滤和显示设置
     */
    void addTrackPoint(const PointInfo& info);

    /**
     * @brief 刷新所有航迹显示
     * @details 当坐标轴参数变化时调用：
     *          - 重新计算所有航迹点的屏幕坐标
     *          - 更新所有连线的几何形状
     *          - 重新应用距离和角度过滤
     *          - 更新标签位置和连线
     * 
     * 调用时机：
     * - PolarAxis的最小/最大距离变化
     * - 像素范围(pixelRange)变化
     * - 场景尺寸变化
     */
    void refreshAll();

    /**
     * @brief 设置指定批次的可见性
     * @param batchID 批次ID
     * @param vis true显示该批次，false隐藏该批次
     * @details 控制单条航迹的显示状态：
     *          - 影响航迹点、连线和标签的可见性
     *          - 不影响其他批次的显示状态
     */
    void setBatchVisible(int batchID, bool vis);
    
    /**
     * @brief 设置全局可见性
     * @param vis true显示所有航迹，false隐藏所有航迹
     * @details 批量控制所有航迹的可见性：
     *          - 用于航迹图层的开关控制
     *          - 保持各批次的相对可见性状态
     */
    void setAllVisible(bool vis);

    /**
     * @brief 删除指定批次的航迹
     * @param batchID 要删除的批次ID
     * @details 完全删除一条航迹：
     *          - 清理所有航迹点和连线
     *          - 删除动态标签
     *          - 释放相关内存资源
     */
    void removeBatch(int batchID);
    
    /**
     * @brief 清理所有航迹
     * @details 删除所有航迹对象：
     *          - 遍历删除所有批次
     *          - 释放所有相关资源
     *          - 重置管理器状态
     */
    void clear();

    /**
     * @brief 设置点尺寸缩放比例
     * @param ratio 缩放比例，1.0为默认大小
     * @details 批量调整所有航迹点的显示尺寸：
     *          - 配合视图缩放级别调整
     *          - 保持航迹点在不同缩放下的可见性
     */
    void setPointSizeRatio(float ratio);
    
    /**
     * @brief 设置指定批次的颜色
     * @param batchID 批次ID
     * @param c 新的颜色值
     * @details 定制单条航迹的颜色：
     *          - 同时影响航迹点、连线和标签颜色
     *          - 支持航迹的视觉区分
     */
    void setBatchColor(int batchID, const QColor& c);

    /**
     * @brief 设置角度显示范围
     * @param startDeg 起始角度(度)，0度为正北方向
     * @param endDeg 结束角度(度)，顺时针方向
     * @details 设置航迹的角度过滤范围：
     *          - 只显示指定角度扇区内的航迹点
     *          - 影响航迹点、连线和标签的显示
     *          - 支持跨越0度的角度范围
     */
    void setAngleRange(double startDeg, double endDeg);

private:
    /**
     * @brief 确保航迹序列存在
     * @param batchID 批次ID
     * @details 根据批次ID查找或创建对应的航迹序列
     */
    void ensureSeries(int batchID);
    
    /**
     * @brief 更新最新点标签
     * @param batchID 批次ID
     * @details 更新指定航迹的动态标签显示和连线
     */
    void updateLatestLabel(int batchID);
    
    /**
     * @brief 更新节点可见性
     * @param node 航迹节点引用
     * @details 根据当前过滤条件更新单个节点的可见性
     */
    void updateNodeVisibility(TrackNode& node);
    
    /**
     * @brief 更新批次可见性
     * @param batchID 批次ID
     * @details 批量更新指定航迹的所有元素可见性
     */
    void updateBatchVisibility(int batchID);
    
    /**
     * @brief 更新连线几何形状
     * @param line 连线对象指针
     * @param a 起点坐标
     * @param b 终点坐标
     * @details 更新连线的几何形状和层级设置
     */
    void updateLineGeometry(QGraphicsLineItem* line, const QPointF& a, const QPointF& b);

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
     * @details 根据PolarAxis的最小/最大距离判断点是否可见
     */
    bool inRange(float range) const;

private:
    // 核心组件引用
    QGraphicsScene* mScene = nullptr;           ///< 图形场景指针
    PolarAxis* mAxis = nullptr;                ///< 极坐标轴指针
    
    // 航迹管理
    QMap<int, TrackSeries> mSeries;            ///< 批次ID到航迹序列的映射
    
    // 显示控制参数
    float mPointSizeRatio = 1.f;               ///< 点尺寸缩放比例
    
    // 角度过滤参数
    double m_angleStart = 0.0;                 ///< 起始角度(度)
    double m_angleEnd = 360.0;                 ///< 结束角度(度)
    
    /**
     * @brief 检查角度是否在显示扇区内
     * @param azimuthDeg 方位角(度)
     * @return true表示在显示扇区内
     * @details 处理角度跨越0度的情况，支持任意角度扇区
     */
    bool inAngle(float azimuthDeg) const;
};
