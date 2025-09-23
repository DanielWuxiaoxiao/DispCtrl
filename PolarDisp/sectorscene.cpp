/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:13
 * @Description: 
 */
/**
 * @file sectorscene.cpp
 * @brief 扇形显示场景实现
 * @details 实现专门用于扇形区域显示的图形场景管理系统
 * 
 * 实现要点：
 * 1. 组件集成：坐标轴、网格、数据管理器的统一管理
 * 2. 参数联动：扇形参数变化时的自动更新机制
 * 3. 场景适配：动态调整场景大小以适应扇形显示
 * 4. 信号转发：内部组件信号的统一对外接口
 * 5. 几何计算：扇形边界和场景矩形的精确计算
 * 
 * 核心算法：
 * - 扇形边界计算：根据角度范围确定最优场景矩形
 * - 像素密度调整：根据视图大小动态调整显示精度
 * - 信号级联：参数变化的层次化传播机制
 * 
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2024
 */

// sectorscene.cpp
#include "sectorscene.h"
#include "sectorpolargrid.h"
#include "PointManager/sectordetmanager.h"
#include "PointManager/sectortrackmanager.h"
#include "PolarDisp/polaraxis.h"
#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include <QtMath>
#include <QDebug>

/**
 * @brief 扇形场景构造函数
 * @param parent 父对象
 * @details 初始化扇形场景，创建并配置所有核心组件
 * 
 * 初始化流程：
 * 1. 创建极坐标轴：提供坐标转换和范围管理
 * 2. 初始化组件指针：为延迟创建做准备
 * 3. 调用组件初始化：创建网格和数据管理器
 * 4. 设置默认范围：60度扇形，500单位距离
 * 5. 连接信号转发：建立内部到外部的信号传播
 * 
 * 默认配置：
 * - 角度范围：-30°到+30°（适合前向雷达显示）
 * - 距离范围：0到500单位（可调整的监控范围）
 * - 坐标系统：标准的雷达极坐标系
 * 
 * 信号连接：
 * - 轴线范围变化信号转发给外部
 * - 确保内外部状态同步
 * - 支持组件间的响应式更新
 * 
 * 内存管理：
 * - Qt父子关系：自动管理子对象生命周期
 * - 显式删除：在析构函数中确保清理
 * - 异常安全：构造失败时的资源清理
 */
SectorScene::SectorScene(QObject* parent)
    : QGraphicsScene(parent),
      m_axis(new PolarAxis()),
      m_grid(nullptr),
      m_det(nullptr),
      m_track(nullptr)
{
    initLayerObjects();
    
    // 默认扇形范围：-30° 到 +30°，距离使用配置系统
    float minRange = CF_INS.range("min", MIN_RANGE) / 1000.0f;  // 转换为公里
    float maxRange = CF_INS.range("max", MAX_RANGE) / 1000.0f;  // 转换为公里
    setSectorRange(-30.0f, 30.0f, minRange, maxRange);
    
    // 确保轴的范围变化信号被转发
    connect(m_axis, &PolarAxis::rangeChanged, this, &SectorScene::rangeChanged);
}

/**
 * @brief 扇形场景析构函数
 * @details 清理场景资源，确保所有组件正确释放
 * 
 * 清理说明：
 * - Qt父子关系：大部分子对象自动清理
 * - 显式删除：确保核心组件m_axis的清理
 * - 防范措施：避免潜在的内存泄漏
 * 
 * 注意事项：
 * - 图形项由QGraphicsScene自动管理
 * - 数据管理器通过父子关系自动清理
 * - 信号连接在对象销毁时自动断开
 */
SectorScene::~SectorScene()
{
    // Qt的父子关系会自动删除子对象，但为了清晰还是显式删除
    delete m_axis;
}

/**
 * @brief 初始化图层对象
 * @details 创建并配置场景中的所有核心组件，建立组件间的通信机制
 * 
 * 组件创建顺序：
 * 1. 网格组件：创建SectorPolarGrid，依赖m_axis
 * 2. 添加到场景：将网格作为图形项加入场景
 * 3. 数据管理器：创建点迹和航迹管理器
 * 4. 信号连接：建立响应式更新机制
 * 
 * 信号连接策略：
 * - 范围变化 → 网格更新：距离范围变化时更新网格显示
 * - 范围变化 → 数据刷新：过滤显示范围外的数据
 * - 扇形变化 → 显示更新：角度范围变化时的完整更新
 * 
 * 组件依赖关系：
 * - 网格依赖坐标轴：进行距离到像素的转换
 * - 数据管理器依赖场景和坐标轴：数据显示和坐标转换
 * - 所有组件都响应范围变化信号
 * 
 * 更新传播链：
 * 参数变化 → SectorScene信号 → 各组件响应 → 视觉更新
 */
void SectorScene::initLayerObjects()
{
    // 创建网格
    m_grid = new SectorPolarGrid(m_axis);
    addItem(m_grid);
    
    // 创建点迹管理器
    m_det = new SectorDetManager(this, m_axis);
    m_track = new SectorTrackManager(this, m_axis);
    
    // 连接信号
    connect(this, &SectorScene::rangeChanged, m_grid, &SectorPolarGrid::updateGrid);
    connect(this, &SectorScene::rangeChanged, m_det, &SectorDetManager::refreshAll);
    connect(this, &SectorScene::rangeChanged, m_track, &SectorTrackManager::refreshAll);
    
    // 扇形范围改变时更新显示
    connect(this, &SectorScene::sectorChanged, this, &SectorScene::updateSectorDisplay);
}

void SectorScene::setSectorRange(float minAngle, float maxAngle, float minRange, float maxRange)
{
    // 参数验证
    if (minAngle >= maxAngle) {
        qWarning() << "Invalid angle range: minAngle should be less than maxAngle";
        return;
    }
    if (minRange < 0) minRange = 0;
    if (maxRange <= minRange) maxRange = minRange + 1;
    
    bool ifangleChanged = (m_minAngle != minAngle || m_maxAngle != maxAngle);
    bool ifrangeChanged = (m_minRange != minRange || m_maxRange != maxRange);
    
    m_minAngle = minAngle;
    m_maxAngle = maxAngle;
    m_minRange = minRange;
    m_maxRange = maxRange;
    
    // 更新轴的距离范围
    m_axis->setRange(minRange, maxRange);
    
    // 如果范围发生变化，更新网格的扇形范围
    if (ifangleChanged) {
        if (m_grid) {
            m_grid->setSectorRange(m_minAngle, m_maxAngle);
            // 角度范围改变时，更新场景矩形以匹配新的扇形范围
            QRectF gridBounds = m_grid->boundingRect();
            setSceneRect(gridBounds);
        }
    }
    
    if (ifangleChanged || ifrangeChanged) {
        emit sectorChanged(minAngle, maxAngle, minRange, maxRange);
    }
    
    if (ifrangeChanged) {
        emit rangeChanged(minRange, maxRange);
    }
}

void SectorScene::updateSceneSize(const QSize& newSize)
{
    // 计算扇形显示所需的场景矩形
    double radius = qMin(newSize.width(), newSize.height()) / 2.0 - 50; // 增加边距
    m_axis->setPixelsPerMeter(radius / m_axis->maxRange());
    
    // 使用网格的boundingRect来设置场景大小，确保一致性
    if (m_grid) {
        // 更新网格的扇形范围，确保boundingRect是最新的
        m_grid->setSectorRange(m_minAngle, m_maxAngle);
        QRectF gridBounds = m_grid->boundingRect();
        setSceneRect(gridBounds);
    } else {
        // 备用计算：简单的对称矩形
        double maxPixelRadius = m_axis->rangeToPixel(m_axis->maxRange());
        double margin = 50;
        setSceneRect(QRectF(-maxPixelRadius - margin, -maxPixelRadius - margin,
                           2 * (maxPixelRadius + margin), 2 * (maxPixelRadius + margin)));
    }
    
    emit rangeChanged(m_axis->minRange(), m_axis->maxRange());
}

void SectorScene::updateSectorDisplay()
{
    // 更新网格的扇形范围
    if (m_grid) {
        m_grid->setSectorRange(m_minAngle, m_maxAngle);
        m_grid->updateGrid();
    }
    
    // 更新点迹管理器的角度过滤范围
    if (m_det) {
        m_det->setAngleRange(m_minAngle, m_maxAngle);
        m_det->refreshAll();
    }
    
    if (m_track) {
        m_track->setAngleRange(m_minAngle, m_maxAngle);
        m_track->refreshAll();
    }
}