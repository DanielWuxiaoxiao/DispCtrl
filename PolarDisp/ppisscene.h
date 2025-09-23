/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:09
 * @Description: 
 */
/**
 * @file ppisscene.h
 * @brief PPI(平面位置显示器)场景管理器头文件
 * @details PPI显示系统的核心场景管理类：
 *          - 管理极坐标显示的所有图形元素
 *          - 协调极坐标轴、网格、航迹、检测点等组件
 *          - 提供场景尺寸和范围管理
 *          - 支持动态缩放和视图更新
 * @author DispCtrl Team
 * @date 2024
 */

#ifndef PPISSCENE_H
#define PPISSCENE_H

#include <QGraphicsScene>
#include "Basic/Protocol.h"

// 前向声明 - 避免头文件循环依赖
class PolarAxis;    ///< 极坐标轴绘制器
class PolarGrid;    ///< 极坐标网格绘制器
class TrackManager; ///< 航迹管理器
class DetManager;   ///< 检测点管理器
class Tooltip;      ///< 工具提示组件
class ScanLayer;    ///< 扫描层组件

/**
 * @class PPIScene
 * @brief PPI显示场景管理器
 * @details 雷达PPI显示的核心场景类，负责：
 *          - 管理所有极坐标显示组件的生命周期
 *          - 协调组件间的交互和数据传递
 *          - 处理场景范围和尺寸变化
 *          - 提供统一的图形渲染管理
 * 
 * 组件层次结构：
 * PPIScene (场景容器)
 *  ├── PolarAxis (极坐标轴)
 *  ├── PolarGrid (极坐标网格)
 *  ├── ScanLayer (扫描层)
 *  ├── DetManager (检测点管理)
 *  ├── TrackManager (航迹管理)
 *  └── Tooltip (工具提示)
 * 
 * @example 基本使用：
 * @code
 * PPIScene* scene = new PPIScene();
 * scene->setRange(0, 100);  // 设置显示范围0-100km
 * view->setScene(scene);
 * @endcode
 */
class PPIScene : public QGraphicsScene {
    Q_OBJECT
    
public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     * @details 初始化PPI场景和所有显示组件：
     *          - 创建极坐标轴和网格
     *          - 初始化航迹和检测点管理器
     *          - 设置扫描层和工具提示
     */
    explicit PPIScene(QObject* parent=nullptr);
    
    /**
     * @defgroup ComponentAccessors 组件访问器
     * @brief 获取场景中各个组件的访问器方法
     * @{
     */
    
    /// 获取极坐标轴组件
    PolarAxis* axis() const { return m_axis; }
    
    /// 获取极坐标网格组件
    PolarGrid* grid() const { return m_grid; }
    
    /// 获取航迹管理器组件
    TrackManager* track() const { return m_track; }
    
    /// 获取检测点管理器组件
    DetManager* det() const { return m_det; }
    
    /// 获取工具提示组件
    Tooltip* tooltip() const { return m_tooltip; }
    
    /** @} */ // end of ComponentAccessors group

public slots:
    /**
     * @brief 设置显示范围
     * @param minR 最小距离(公里)
     * @param maxR 最大距离(公里)
     * @details 更新PPI显示的距离范围：
     *          - 同步更新所有子组件的显示范围
     *          - 重新计算坐标变换
     *          - 发送范围变化信号通知其他组件
     */
    void setRange(float minR, float maxR);
    
    /**
     * @brief 更新场景尺寸
     * @param newSize 新的场景尺寸
     * @details 响应视图尺寸变化：
     *          - 重新计算场景矩形
     *          - 更新所有组件的布局
     *          - 保持显示比例和居中对齐
     */
    void updateSceneSize(const QSize& newSize);

signals:
    /**
     * @brief 范围变化信号
     * @param minR 新的最小距离
     * @param maxR 新的最大距离
     * @details 当PPI显示范围改变时发送，通知其他组件同步更新
     */
    void rangeChanged(float minR, float maxR);

private:
    // 核心显示组件
    PolarAxis* m_axis;       ///< 极坐标轴 - 绘制方位角和距离刻度
    PolarGrid* m_grid;       ///< 极坐标网格 - 绘制距离圆和方位线
    TrackManager* m_track;   ///< 航迹管理器 - 管理目标航迹显示
    DetManager* m_det;       ///< 检测点管理器 - 管理雷达检测点显示
    Tooltip* m_tooltip;      ///< 工具提示 - 鼠标悬停信息显示
    ScanLayer* m_scan;       ///< 扫描层 - 雷达扫描线显示
    
    int pviewMargin = 30;    ///< 视图边距(像素) - 预留显示边界
    
    /**
     * @brief 初始化图层对象
     * @details 创建并配置所有显示组件：
     *          - 设置组件间的父子关系
     *          - 建立信号槽连接
     *          - 配置默认显示参数
     */
    void initLayerObjects();
};

#endif // PPISSCENE_H
