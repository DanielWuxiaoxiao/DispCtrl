/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:14
 * @Description: 
 */
/**
 * @file sectorscene.h
 * @brief 扇形显示场景定义
 * @details 提供雷达扇形区域的专用场景管理，支持角度和距离范围的灵活配置
 * 
 * 功能特性：
 * 1. 扇形范围控制：支持任意角度范围和距离范围的扇形显示
 * 2. 坐标系统：专门优化的极坐标系统，适配扇形显示需求
 * 3. 数据管理：集成点迹和航迹管理器，专门处理扇形区域内的目标
 * 4. 动态更新：支持实时调整扇形参数和场景尺寸
 * 5. 信号通信：提供扇形参数变化的通知机制
 * 
 * 应用场景：
 * - 雷达扇形扫描显示：模拟真实雷达的扫描范围
 * - 局部区域监控：专注于特定角度和距离范围的目标监控
 * - 分屏显示：作为主PPI显示的补充，提供局部详细视图
 * - 多雷达融合：不同雷达的扇形覆盖区域显示
 * 
 * 技术架构：
 * - 继承QGraphicsScene：复用Qt图形框架的高效渲染
 * - 组合设计模式：集成轴线、网格、数据管理器等组件
 * - 观察者模式：通过信号槽实现参数变化通知
 * 
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2024
 */

// sectorscene.h
#ifndef SECTORSCENE_H
#define SECTORSCENE_H

#include <QGraphicsScene>
#include <QObject>

class PolarAxis;
class SectorPolarGrid;
class SectorDetManager;
class SectorTrackManager;

/**
 * @class SectorScene
 * @brief 扇形显示场景类
 * @details 专门用于扇形区域显示的图形场景，集成坐标系统、网格显示和数据管理功能
 * 
 * 核心功能：
 * 1. 扇形范围管理：
 *    - 角度范围：支持任意角度的扇形设置（minAngle到maxAngle）
 *    - 距离范围：可配置的径向距离范围（minRange到maxRange）
 *    - 实时调整：支持动态修改扇形参数
 * 
 * 2. 组件集成：
 *    - PolarAxis: 提供扇形坐标系统支持
 *    - SectorPolarGrid: 专用的扇形网格显示
 *    - SectorDetManager: 扇形区域内的点迹管理
 *    - SectorTrackManager: 扇形区域内的航迹管理
 * 
 * 3. 动态适配：
 *    - 场景尺寸自适应：根据窗口大小调整场景范围
 *    - 参数联动更新：修改扇形参数时自动更新所有相关组件
 *    - 信号通知机制：扇形参数变化时通知外部组件
 * 
 * 坐标系统：
 * - 极坐标原点：通常位于扇形的顶点（雷达位置）
 * - 角度定义：相对于北方向（或指定参考方向）的角度
 * - 距离单位：根据应用需求配置（米、公里、海里等）
 * 
 * 性能优化：
 * - 视野裁剪：只渲染扇形范围内的图形元素
 * - 层次管理：合理组织各组件的层次关系
 * - 更新策略：智能的局部更新机制
 */
class SectorScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit SectorScene(QObject* parent = nullptr);
    virtual ~SectorScene();

    /**
     * @brief 设置扇形显示范围
     * @param minAngle 最小角度（度）
     * @param maxAngle 最大角度（度）
     * @param minRange 最小距离
     * @param maxRange 最大距离
     * @details 配置扇形的角度和距离范围，所有相关组件将自动更新
     */
    // 设置扇形显示范围
    void setSectorRange(float minAngle, float maxAngle, float minRange, float maxRange);
    
    /**
     * @brief 获取最小角度
     * @return 当前设置的最小角度值
     */
    // 获取当前扇形参数
    float minAngle() const { return m_minAngle; }
    
    /**
     * @brief 获取最大角度
     * @return 当前设置的最大角度值
     */
    float maxAngle() const { return m_maxAngle; }
    
    /**
     * @brief 获取最小距离
     * @return 当前设置的最小距离值
     */
    float minRange() const { return m_minRange; }
    
    /**
     * @brief 获取最大距离
     * @return 当前设置的最大距离值
     */
    float maxRange() const { return m_maxRange; }
    
    /**
     * @brief 更新场景大小
     * @param newSize 新的场景尺寸
     * @details 当容器窗口大小变化时，调整场景的显示范围和组件布局
     */
    // 更新场景大小
    void updateSceneSize(const QSize& newSize);
    
    /**
     * @brief 获取点迹管理器
     * @return SectorDetManager指针
     * @details 提供对扇形区域内点迹数据的访问接口
     */
    // 获取管理器
    SectorDetManager* detManager() const { return m_det; }
    
    /**
     * @brief 获取航迹管理器
     * @return SectorTrackManager指针
     * @details 提供对扇形区域内航迹数据的访问接口
     */
    SectorTrackManager* trackManager() const { return m_track; }
    
    /**
     * @brief 获取坐标轴系统
     * @return PolarAxis指针
     * @details 提供对扇形坐标系统的访问接口
     */
    PolarAxis* axis() const { return m_axis; }

signals:
    /**
     * @brief 扇形参数改变信号
     * @param minAngle 新的最小角度
     * @param maxAngle 新的最大角度
     * @param minRange 新的最小距离
     * @param maxRange 新的最大距离
     * @details 当扇形的任何参数发生变化时发出，通知外部组件更新
     */
    void sectorChanged(float minAngle, float maxAngle, float minRange, float maxRange);
    
    /**
     * @brief 距离范围改变信号
     * @param minRange 新的最小距离
     * @param maxRange 新的最大距离
     * @details 当距离范围发生变化时发出，用于距离相关的组件更新
     */
    void rangeChanged(float minRange, float maxRange);

private slots:
    /**
     * @brief 更新扇形显示
     * @details 当扇形参数变化时，更新所有相关组件的显示状态
     */
    void updateSectorDisplay();

private:
    /**
     * @brief 初始化图层对象
     * @details 创建并配置坐标轴、网格、数据管理器等核心组件
     */
    void initLayerObjects();

private:
    PolarAxis* m_axis;               ///< 极坐标轴系统
    SectorPolarGrid* m_grid;         ///< 扇形极坐标网格
    SectorDetManager* m_det;         ///< 扇形区域点迹管理器
    SectorTrackManager* m_track;     ///< 扇形区域航迹管理器
    
    float m_minAngle = -30.0f;       ///< 最小角度（默认-30度）
    float m_maxAngle = 30.0f;        ///< 最大角度（默认30度）
    float m_minRange = 0.0f;         ///< 最小距离（默认0）
    float m_maxRange = 500.0f;       ///< 最大距离（默认500单位）
};

#endif // SECTORSCENE_H