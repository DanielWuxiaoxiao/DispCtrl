/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 10:04:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:07
 * @Description: 
 */
/**
 * @file sectortrackmanager.h
 * @brief 扇形航迹管理器头文件
 * @details 专门用于扇形显示区域的航迹管理系统：
 *          - 管理扇形区域内的多条航迹数据
 *          - 提供扇形角度范围的动态过滤
 *          - 支持航迹连线和动态标签功能
 *          - 优化扇形显示的航迹可视化效果
 * @author DispCtrl Team
 * @date 2024
 */

#ifndef SECTORTRACKMANAGER_H
#define SECTORTRACKMANAGER_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QMap>
#include <QVector>
#include "point.h"
#include "PolarDisp/polaraxis.h"

/**
 * @class SectorDraggableLabel
 * @brief 扇形区域可拖拽航迹标签
 * @details 专用于扇形显示区域的航迹信息标签：
 *          - 支持用户交互拖拽移动
 *          - 自动保持与锚点的连线
 *          - 适配扇形显示的布局需求
 *          - 与主PPI区域标签系统分离
 */
class SectorDraggableLabel : public QGraphicsTextItem
{
public:
    /**
     * @brief 构造函数
     * @param parent 父图形项指针
     * @details 初始化扇形区域的可拖拽标签
     */
    SectorDraggableLabel(QGraphicsItem* parent = nullptr);
    
    /**
     * @brief 设置锚点关联
     * @param anchor 锚点图形项（通常是航迹点）
     * @param tether 连接线图形项
     * @details 建立标签与锚点的关联关系
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
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QGraphicsItem* m_anchor = nullptr;      ///< 锚点图形项指针
    QGraphicsLineItem* m_tether = nullptr;  ///< 连接线图形项指针
};

/**
 * @struct SectorTrackNode
 * @brief 扇形航迹节点结构
 * @details 封装扇形区域内单个航迹点及其连线信息：
 *          - 管理航迹点对象
 *          - 管理与前一点的连线
 *          - 专门用于扇形显示优化
 */
struct SectorTrackNode {
    TrackPoint* point = nullptr;                ///< 航迹点对象指针
    QGraphicsLineItem* lineFromPrev = nullptr; ///< 与前一节点的连线
};

/**
 * @struct SectorTrackSeries
 * @brief 扇形航迹序列结构
 * @details 表示扇形区域内一条完整的航迹路径：
 *          - 包含该航迹在扇形区域的所有历史点
 *          - 管理扇形航迹的可见性和颜色
 *          - 提供扇形区域的动态标签显示
 *          - 支持扇形航迹的批量操作
 */
struct SectorTrackSeries {
    QVector<SectorTrackNode> nodes;             ///< 扇形航迹节点序列
    SectorDraggableLabel* label = nullptr;      ///< 最新点标签
    QGraphicsLineItem* labelLine = nullptr;     ///< 标签到最新点的连线
    bool visible = true;                        ///< 航迹可见性标志
    QColor color;                               ///< 航迹颜色
};

/**
 * @class SectorTrackManager
 * @brief 扇形航迹管理器
 * @details 专用于扇形显示区域的航迹管理器，负责：
 *          - 扇形区域内多条航迹的创建、更新和销毁
 *          - 扇形范围内的航迹点连线绘制
 *          - 扇形区域的动态标签显示和交互
 *          - 扇形角度范围的实时过滤
 *          - 与主PPI航迹管理器的分离协作
 * 
 * 功能特点：
 * - 专门针对扇形显示区域优化
 * - 支持多航迹并发显示
 * - 航迹点自动连线
 * - 可拖拽的动态标签
 * - 扇形角度范围过滤
 * 
 * @example 基本使用：
 * @code
 * SectorTrackManager* mgr = new SectorTrackManager(scene, axis);
 * mgr->setAngleRange(-45, 45);         // 设置扇形范围
 * mgr->addTrackPoint(trackInfo);       // 添加航迹点
 * mgr->setBatchColor(1, Qt::red);      // 设置航迹颜色
 * @endcode
 */
class SectorTrackManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param scene 图形场景指针，航迹将添加到此场景
     * @param axis 极坐标轴指针，提供坐标变换参数
     * @param parent 父对象指针
     * @details 初始化扇形航迹管理器：
     *          - 关联图形场景和坐标轴
     *          - 设置默认的扇形角度范围
     *          - 初始化航迹容器
     */
    explicit SectorTrackManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent = nullptr);
    
    /**
     * @brief 虚析构函数
     * @details 清理所有扇形航迹对象和相关资源
     */
    virtual ~SectorTrackManager();

    /**
     * @brief 添加航迹点到扇形区域
     * @param info 航迹点信息，包含批次ID、位置等数据
     * @details 向扇形区域的指定批次添加新的航迹点：
     *          1. 验证航迹点是否在扇形范围内
     *          2. 根据批次ID查找或创建航迹序列
     *          3. 创建TrackPoint对象并配置外观
     *          4. 与前一点建立连线关系
     *          5. 更新扇形区域的动态标签显示
     */
    void addTrackPoint(const PointInfo& info);
    
    /**
     * @brief 刷新所有扇形航迹显示
     * @details 当坐标轴参数或扇形范围变化时调用：
     *          - 重新计算所有航迹点的屏幕坐标
     *          - 更新所有连线的几何形状
     *          - 重新应用扇形角度过滤
     *          - 更新标签位置和连线
     */
    void refreshAll();
    
    /**
     * @brief 设置指定批次的可见性
     * @param batchID 批次ID
     * @param visible true显示该批次，false隐藏该批次
     * @details 控制扇形区域内单条航迹的显示状态
     */
    void setBatchVisible(int batchID, bool visible);
    
    /**
     * @brief 设置扇形区域全局可见性
     * @param visible true显示所有航迹，false隐藏所有航迹
     * @details 批量控制扇形区域内所有航迹的可见性
     */
    void setAllVisible(bool visible);
    
    /**
     * @brief 设置扇形航迹点尺寸比例
     * @param ratio 缩放比例，1.0为默认大小
     * @details 批量调整扇形区域内所有航迹点的显示尺寸
     */
    void setPointSizeRatio(float ratio);
    
    /**
     * @brief 设置指定批次的颜色
     * @param batchID 批次ID
     * @param color 新的颜色值
     * @details 定制扇形区域内单条航迹的颜色
     */
    void setBatchColor(int batchID, const QColor& color);
    
    /**
     * @brief 设置扇形角度范围
     * @param minAngle 最小角度(度)
     * @param maxAngle 最大角度(度)
     * @details 设置扇形航迹的角度过滤范围：
     *          - 只显示指定角度扇形内的航迹
     *          - 影响航迹点、连线和标签的显示
     *          - 实时过滤现有和新增的航迹数据
     */
    void setAngleRange(float minAngle, float maxAngle);
    
    /**
     * @brief 删除指定批次的航迹
     * @param batchID 要删除的批次ID
     * @details 完全删除扇形区域内一条航迹
     */
    void removeBatch(int batchID);
    
    /**
     * @brief 清理所有扇形航迹
     * @details 删除扇形区域内的所有航迹对象
     */
    void clear();
    
    /**
     * @brief 获取航迹批次数量
     * @return 当前扇形区域内的航迹批次总数
     * @details 用于状态查询和性能监控
     */
    int batchCount() const { return m_series.size(); }
    
    /**
     * @brief 获取所有批次ID列表
     * @return 当前扇形区域内所有批次ID的列表
     * @details 用于遍历和管理所有航迹批次
     */
    QList<int> batchIDs() const { return m_series.keys(); }

private:
    /**
     * @brief 确保航迹序列存在
     * @param batchID 批次ID
     * @details 根据批次ID查找或创建对应的扇形航迹序列
     */
    void ensureSeries(int batchID);
    
    /**
     * @brief 更新最新点标签
     * @param batchID 批次ID
     * @details 更新指定扇形航迹的动态标签显示和连线
     */
    void updateLatestLabel(int batchID);
    
    /**
     * @brief 更新批次可见性
     * @param batchID 批次ID
     * @details 批量更新指定扇形航迹的所有元素可见性
     */
    void updateBatchVisibility(int batchID);
    
    /**
     * @brief 更新连线几何形状
     * @param line 连线对象指针
     * @param a 起点坐标
     * @param b 终点坐标
     * @details 更新扇形区域内连线的几何形状和层级设置
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
     * @details 根据PolarAxis的最小/最大距离判断
     */
    bool inRange(float range) const;
    
    /**
     * @brief 检查角度是否在扇形范围内
     * @param azimuthDeg 方位角(度)
     * @return true表示在扇形范围内
     * @details 判断航迹点是否在设置的扇形角度范围内
     */
    bool inAngle(float azimuthDeg) const;
    
    /**
     * @brief 检查点是否应该可见
     * @param info 航迹点信息
     * @return true表示点应该可见
     * @details 综合判断点的距离和角度是否都在扇形范围内
     */
    bool isPointVisible(const PointInfo& info) const;

private:
    // 核心组件引用
    QGraphicsScene* m_scene;                    ///< 图形场景指针
    PolarAxis* m_axis;                         ///< 极坐标轴指针
    
    // 扇形航迹管理
    QMap<int, SectorTrackSeries> m_series;     ///< 批次ID到扇形航迹序列的映射
    
    // 显示控制参数
    float m_pointSizeRatio = 1.0f;             ///< 点尺寸缩放比例
    
    // 扇形角度范围参数
    float m_minAngle = -30.0f;                 ///< 扇形最小角度(度)
    float m_maxAngle = 30.0f;                  ///< 扇形最大角度(度)
};

#endif