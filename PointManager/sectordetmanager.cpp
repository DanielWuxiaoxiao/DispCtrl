/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 10:04:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:20
 * @Description: 
 */
/**
 * @file sectordetmanager.cpp
 * @brief 扇形检测点管理器实现文件
 * @details 实现扇形显示区域的检测点管理功能：
 *          - 与RadarDataManager集成的数据接收
 *          - 扇形角度范围的动态过滤算法
 *          - 高效的扇形区域点对象管理
 *          - 与主PPI显示系统的协调工作
 * @author DispCtrl Team
 * @date 2024
 */

#include "Basic/log.h"

#include "sectordetmanager.h"
#include "Basic/ConfigManager.h"
#include "Basic/DispBasci.h"
#include "Controller/RadarDataManager.h"  // 雷达数据管理器头文件
#include <QtMath>
#include <QDebug>

/**
 * @brief SectorDetManager构造函数实现
 * @param scene 图形场景指针
 * @param axis 极坐标轴指针
 * @param parent 父对象指针
 * @details 完成扇形检测点管理器的初始化：
 *          1. 注册到统一数据管理器接收检测点数据
 *          2. 连接数据信号到对应的处理槽函数
 *          3. 设置默认的扇形角度范围
 */

SectorDetManager::SectorDetManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), m_scene(scene), m_axis(axis)
{
    m_maxPoints = CF_INS.displayConfig("max_points", 1000);

    // 注册到统一数据管理器，使用唯一标识符
    RADAR_DATA_MGR.registerView("SectorDetManager_" + QString::number((quintptr)this), this);

        // 检测点数据由 MainOverLayOut 通过 Controller::detInfoProcess 统一分发，
        // 这里仅保留清空通知，避免扇区检测点被重复绘制。
        connect(&RADAR_DATA_MGR, &RadarDataManager::dataCleared,
            this, &SectorDetManager::clear);           // 响应数据清理
}

/**
 * @brief SectorDetManager析构函数实现
 * @details 确保资源的正确清理：
 *          - 从统一数据管理器注销当前视图
 *          - 清理所有扇形检测点对象
 *          - 断开信号连接
 */
SectorDetManager::~SectorDetManager()
{
    // 从统一数据管理器注销
    RADAR_DATA_MGR.unregisterView("SectorDetManager_" + QString::number((quintptr)this));
    clear();  // 清理所有扇形检测点
}

/**
 * @brief 添加检测点到扇形区域
 * @param info 检测点信息结构体
 * @details 完整的扇形检测点创建流程：
 *          1. 复制并标记为检测点类型
 *          2. 创建DetPoint对象并配置外观
 *          3. 计算屏幕坐标位置
 *          4. 应用扇形可见性过滤规则
 *          5. 添加到场景和内部容器
 */

void SectorDetManager::addDetPoint(const PointInfo& info)
{
    // 创建检测点
    PointInfo copy = info;
    copy.type = 1; // 检测点类型

    DetPoint* pt = new DetPoint(copy);
    pt->resize(m_pointSizeRatio);
    pt->setColor(DET_COLOR);

    // 计算位置
    QPointF pos = polarToPixel(copy.range, copy.azimuth);
    pt->updatePosition(pos.x(), pos.y());

    // 设置可见性（只有在扇形范围内的点才可见）
    bool visible = m_visible && isPointVisible(copy);
    setPointSceneVisible(pt, visible);

    // 保存节点
    SectorDetNode node;
    node.point = pt;
    m_nodes.push_back(node);

    while (m_nodes.size() > m_maxPoints) {
        SectorDetNode& oldNode = m_nodes.first();
        if (oldNode.point) {
            if (oldNode.point->scene() == m_scene) {
                m_scene->removeItem(oldNode.point);
            }
            delete oldNode.point;
        }
        m_nodes.removeFirst();
    }

}

void SectorDetManager::refreshAll()
{
    int visibleCount = 0;
    int hiddenCount = 0;

    for (auto& node : m_nodes) {
        if (!node.point) continue;

        const PointInfo& info = node.point->infoRef();

        // 更新位置
        QPointF pos = polarToPixel(info.range, info.azimuth);
        node.point->updatePosition(pos.x(), pos.y());

        // 更新可见性
        bool visible = m_visible && isPointVisible(info);
        setPointSceneVisible(node.point, visible);

        if (visible) visibleCount++;
        else hiddenCount++;
    }

    Q_UNUSED(visibleCount)
    Q_UNUSED(hiddenCount)
}

void SectorDetManager::setAllVisible(bool visible)
{
    m_visible = visible;

    for (auto& node : m_nodes) {
        if (node.point) {
            bool shouldShow = m_visible && isPointVisible(node.point->infoRef());
            setPointSceneVisible(node.point, shouldShow);
        }
    }
}

void SectorDetManager::setPointSizeRatio(float ratio)
{
    if (ratio <= 0.0f) ratio = 1.0f;

    m_pointSizeRatio = ratio;

    for (auto& node : m_nodes) {
        if (node.point) {
            node.point->resize(m_pointSizeRatio);
        }
    }
}

void SectorDetManager::setMaxPoints(int maxPoints)
{
    if (maxPoints < 100) {
        maxPoints = 100;
    }

    m_maxPoints = maxPoints;
    while (m_nodes.size() > m_maxPoints) {
        SectorDetNode& oldNode = m_nodes.first();
        if (oldNode.point) {
            if (oldNode.point->scene() == m_scene) {
                m_scene->removeItem(oldNode.point);
            }
            delete oldNode.point;
        }
        m_nodes.removeFirst();
    }
}

void SectorDetManager::setAngleRange(float minAngle, float maxAngle)
{
    m_minAngle = minAngle;
    m_maxAngle = maxAngle;

    // 更新所有点的可见性
    refreshAll();
}

void SectorDetManager::clear()
{
    LOG_DEBUG(QString("[SectorDetManager::clear] Clearing %1 detection points")
                  .arg(m_nodes.size()));

    for (auto& node : m_nodes) {
        if (node.point) {
            if (node.point->scene() == m_scene) {
                m_scene->removeItem(node.point);
            }
            delete node.point;
            node.point = nullptr;
        }
    }
    m_nodes.clear();

    LOG_DEBUG(QString("[SectorDetManager::clear] Clear complete, nodes count: %1")
                  .arg(m_nodes.size()));
}

QPointF SectorDetManager::polarToPixel(float range, float azimuthDeg) const
{
    return m_axis->polarToScene(range, azimuthDeg);
}

bool SectorDetManager::inRange(float range) const
{
    return (range >= m_axis->minRange() && range <= m_axis->maxRange());
}

bool SectorDetManager::inAngle(float azimuthDeg) const
{
    // 归一化角度到 [0, 360)
    double angle = fmod(azimuthDeg, 360.0);
    if (angle < 0) angle += 360.0;

    double minAngle = fmod(m_minAngle, 360.0);
    double maxAngle = fmod(m_maxAngle, 360.0);
    if (minAngle < 0) minAngle += 360.0;
    if (maxAngle < 0) maxAngle += 360.0;

    // 处理跨越0度的情况
    if (minAngle <= maxAngle) {
        return (angle >= minAngle && angle <= maxAngle);
    } else {
        // 跨越0度的扇形
        return (angle >= minAngle || angle <= maxAngle);
    }
}

bool SectorDetManager::isPointVisible(const PointInfo& info) const
{
    return inRange(info.range) && inAngle(info.azimuth);
}

void SectorDetManager::setPointSceneVisible(DetPoint* point, bool visible)
{
    if (!point || !m_scene) {
        return;
    }

    point->setVisible(visible);
    if (visible) {
        if (!point->scene()) {
            m_scene->addItem(point);
        }
        return;
    }

    if (point->scene() == m_scene) {
        m_scene->removeItem(point);
    }
}
