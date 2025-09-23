/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:03
 * @Description: 
 */
/**
 * @file detmanager.cpp
 * @brief 检测点管理器实现文件
 * @details 实现雷达检测点的统一管理功能：
 *          - 与RadarDataManager集成的数据接收
 *          - 高效的批量点对象管理
 *          - 实时坐标变换和显示更新
 *          - 角度和距离范围的动态过滤
 * @author DispCtrl Team
 * @date 2024
 */

#include "detmanager.h"
#include "Basic/DispBasci.h"
#include "Controller/RadarDataManager.h"  // 雷达数据管理器头文件

/**
 * @brief DetManager构造函数实现
 * @param scene 图形场景指针
 * @param axis 极坐标轴指针
 * @param parent 父对象指针
 * @details 完成检测点管理器的初始化：
 *          1. 注册到统一数据管理器接收检测点数据
 *          2. 连接数据信号到对应的处理槽函数
 *          3. 设置默认的显示参数
 */

DetManager::DetManager(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), mScene(scene), mAxis(axis)
{
    // 注册到统一数据管理器，使用唯一标识符
    RADAR_DATA_MGR.registerView("DetManager_" + QString::number((quintptr)this), this);
    
    // 连接统一数据管理器的信号到本地处理函数
    connect(&RADAR_DATA_MGR, &RadarDataManager::detectionReceived, 
            this, &DetManager::addDetPoint);       // 接收检测点数据
    connect(&RADAR_DATA_MGR, &RadarDataManager::dataCleared, 
            this, &DetManager::clear);             // 响应数据清理
}

/**
 * @brief DetManager析构函数实现
 * @details 确保资源的正确清理：
 *          - 从统一数据管理器注销当前视图
 *          - 清理所有检测点对象
 *          - 断开信号连接
 */
DetManager::~DetManager()
{
    // 从统一数据管理器注销
    RADAR_DATA_MGR.unregisterView("DetManager_" + QString::number((quintptr)this));
    clear();  // 清理所有检测点
}

/**
 * @brief 添加检测点到管理器
 * @param info 检测点信息结构体
 * @details 完整的检测点创建流程：
 *          1. 复制并标记为检测点类型
 *          2. 创建DetPoint对象并配置外观
 *          3. 计算屏幕坐标位置
 *          4. 应用可见性过滤规则
 *          5. 添加到场景和内部容器
 */

void DetManager::addDetPoint(const PointInfo& info)
{
    // 创建检测点信息副本并设置类型
    PointInfo copy = info;
    copy.type = 1; // 标记为检测点类型
    
    // 创建检测点对象并配置外观
    auto* pt = new DetPoint(copy);
    pt->resize(mPointSizeRatio);      // 应用当前缩放比例
    pt->setColor(DET_COLOR);          // 设置检测点颜色

    // 计算屏幕坐标位置
    QPointF pos = polarToPixel(copy.range, copy.azimuth);
    pt->updatePosition(pos.x(), pos.y());
    
    // 应用可见性过滤：全局可见性 && 距离范围 && 角度范围
    bool vis = mVisible && inRange(copy.range) && inAngle(copy.azimuth);
    pt->setVisible(vis);
    
    // 添加到图形场景
    mScene->addItem(pt);

    // 创建节点并加入内部容器
    DetNode node;
    node.point = pt;
    mNodes.push_back(node);
}

/**
 * @brief 刷新所有检测点显示
 * @details 当坐标轴参数或显示设置变化时的批量更新：
 *          1. 重新计算每个检测点的屏幕坐标
 *          2. 更新点的显示位置
 *          3. 重新应用可见性过滤规则
 *          4. 确保显示状态与当前设置一致
 */
void DetManager::refreshAll()
{
    for (auto& n : mNodes) {
        if (!n.point) continue;  // 跳过无效节点
        
        // 获取检测点信息并重新计算位置
        const auto& pi = n.point->infoRef();
        QPointF pos = polarToPixel(pi.range, pi.azimuth);
        n.point->updatePosition(pos.x(), pos.y());
        
        // 重新应用可见性过滤
        bool vis = mVisible && inRange(pi.range) && inAngle(pi.azimuth);
        n.point->setVisible(vis);
    }
}

/**
 * @brief 设置全局可见性状态
 * @param vis 可见性标志
 * @details 批量控制所有检测点的显示/隐藏：
 *          - 更新全局可见性标志
 *          - 遍历所有检测点应用新状态
 *          - 结合距离和角度过滤条件
 */
void DetManager::setAllVisible(bool vis)
{
    mVisible = vis;
    for (auto& n : mNodes) {
        if (n.point) {
            // 综合考虑全局可见性和过滤条件
            bool in = inRange(n.point->infoRef().range) && inAngle(n.point->infoRef().azimuth);
            n.point->setVisible(mVisible && in);
        }
    }
}

/**
 * @brief 设置角度显示范围
 * @param startDeg 起始角度(度)
 * @param endDeg 结束角度(度)  
 * @details 设置检测点的角度过滤扇区：
 *          - 保存新的角度范围参数
 *          - 刷新所有检测点的显示状态
 *          - 支持跨越0度的角度范围
 */
void DetManager::setAngleRange(double startDeg, double endDeg)
{
    m_angleStart = startDeg;
    m_angleEnd = endDeg;
    // 更新所有点的显隐状态
    refreshAll();
}

/**
 * @brief 检查角度是否在显示扇区内
 * @param azimuthDeg 方位角(度)
 * @return true表示在扇区内
 * @details 角度范围判断逻辑：
 *          1. 将角度归一化到[0,360)范围
 *          2. 处理起始和结束角度的归一化
 *          3. 支持跨越0度的扇区(如350-010度)
 */
bool DetManager::inAngle(float azimuthDeg) const
{
    // 归一化角度到 [0,360) 范围
    double a = fmod(azimuthDeg, 360.0);
    if (a < 0) a += 360.0;
    
    double s = fmod(m_angleStart, 360.0);
    double e = fmod(m_angleEnd, 360.0);
    if (s < 0) s += 360.0;
    if (e < 0) e += 360.0;
    
    // 处理两种情况：普通扇区和跨越0度的扇区
    if (s <= e) {
        return (a >= s && a <= e);      // 普通扇区：如90-270度
    } else {
        return (a >= s || a <= e);      // 跨越0度：如350-010度
    }
}

/**
 * @brief 设置点尺寸缩放比例
 * @param ratio 缩放比例，1.0为默认大小
 * @details 批量调整所有检测点的显示尺寸：
 *          - 防护性检查确保比例值有效
 *          - 遍历所有检测点应用新比例
 *          - 刷新显示确保尺寸更新
 */
void DetManager::setPointSizeRatio(float ratio)
{
    if (ratio <= 0.f) ratio = 1.f;  // 防护性检查
    mPointSizeRatio = ratio;
    
    // 应用到所有检测点
    for (auto& n : mNodes) {
        if (n.point) n.point->resize(mPointSizeRatio);
    }
    refreshAll();  // 确保显示更新
}

/**
 * @brief 清理所有检测点
 * @details 完整的资源清理流程：
 *          1. 遍历所有检测点节点
 *          2. 从图形场景移除检测点
 *          3. 删除检测点对象释放内存
 *          4. 清空内部容器
 */
void DetManager::clear()
{
    for (auto& n : mNodes) {
        if (n.point) { 
            mScene->removeItem(n.point);  // 从场景移除
            delete n.point;               // 释放内存
            n.point = nullptr;            // 重置指针
        }
    }
    mNodes.clear();  // 清空容器
}

/**
 * @brief 极坐标转屏幕坐标
 * @param range 距离值(公里)
 * @param azimuthDeg 方位角(度)
 * @return 屏幕像素坐标
 * @details 通过PolarAxis进行坐标变换，考虑当前的缩放和偏移
 */
QPointF DetManager::polarToPixel(float range, float azimuthDeg) const
{
    return mAxis->polarToScene(range, azimuthDeg);
}

/**
 * @brief 检查距离是否在显示范围内
 * @param range 距离值(公里)
 * @return true表示在显示范围内
 * @details 根据PolarAxis的当前最小/最大距离设置判断
 */
bool DetManager::inRange(float range) const
{
    return (range >= mAxis->minRange() && range <= mAxis->maxRange());
}
