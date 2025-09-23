/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:09
 * @Description: 
 */
/**
 * @file ppisscene.cpp
 * @brief PPI场景管理器实现文件
 * @details 实现雷达PPI显示系统的核心场景管理功能，包括组件初始化、
 *          场景尺寸管理、范围设置和信号转发等核心功能。
 * @author DispCtrl Development Team
 * @date 2024
 * @version 1.0
 */

#include "ppisscene.h"
#include "polaraxis.h"
#include <QGraphicsTextItem>
#include "polargrid.h"
#include "PointManager/trackmanager.h"
#include "PointManager/detmanager.h"
#include "tooltip.h"
#include "Basic/DispBasci.h"
#include "Basic/log.h"
#include "Basic/ConfigManager.h"
#include "scanlayer.h"

/**
 * @brief PPIScene构造函数
 * @details 初始化PPI显示场景和所有相关组件，建立组件间的协调关系
 * @param parent 父对象指针，用于Qt对象树管理
 * 
 * 初始化流程：
 * 1. 创建QGraphicsScene基类对象
 * 2. 创建极坐标轴对象作为坐标变换核心
 * 3. 初始化所有显示组件（网格、管理器、工具提示等）
 * 4. 从配置文件读取并设置默认显示范围
 * 5. 创建和配置扫描层组件
 * 6. 建立信号槽连接进行组件协调
 * 
 * 组件创建顺序：
 * - PolarAxis: 极坐标系统核心，提供坐标变换
 * - 显示组件: 通过initLayerObjects()创建
 * - ScanLayer: 雷达扫描线显示
 * 
 * 默认配置：
 * - 显示范围：从配置文件读取MIN_RANGE到MAX_RANGE
 * - 扫描扇区：-30°到+30°的60°扇区
 * - 扫描模式：循环扫描模式
 * 
 * @note 构造完成后场景立即可用，所有组件已正确初始化
 */
PPIScene::PPIScene(QObject *parent)
    : QGraphicsScene(parent),
      m_axis(new PolarAxis())
{
    initLayerObjects();
    setRange(CF_INS.range("min", MIN_RANGE), CF_INS.range("max", MAX_RANGE));

    m_scan = new ScanLayer(m_axis);
    addItem(m_scan);
    m_scan->setSweepRange(-30, 30);    // 固定粉色扇区（30°~60°）
    m_scan->setScanMode(ScanLayer::Loop);

    // ensure axis->rangeChanged is forwarded
    connect(m_axis, &PolarAxis::rangeChanged, this, &PPIScene::rangeChanged);
}

/**
 * @brief 更新场景尺寸
 * @details 响应视图窗口尺寸变化，重新计算场景矩形和像素密度，
 *          确保PPI显示始终保持正确的比例和居中对齐
 * @param newSize 新的场景尺寸
 * 
 * 计算逻辑：
 * 1. 场景矩形计算：以(0,0)为中心，尺寸为newSize的矩形
 * 2. 有效半径计算：取宽高中的较小值，减去边距得到可用半径
 * 3. 像素密度计算：有效半径除以最大显示距离得到像素/米比例
 * 4. 范围变化通知：发射信号通知所有组件更新显示
 * 
 * 边距处理：
 * - pviewMargin: 预留30像素边距，确保显示内容不触碰边界
 * - 保证网格、刻度等元素完整显示
 * 
 * 比例保持：
 * - 使用正方形显示区域（取宽高最小值）
 * - 确保极坐标显示的圆形特性
 * - 避免椭圆变形影响距离和角度判读
 * 
 * 自动适配：
 * - 窗口放大：显示更精细，像素密度增加
 * - 窗口缩小：显示更粗略，像素密度减少
 * - 保持相对比例关系不变
 * 
 * @note 该函数通常由视图窗口的resize事件触发调用
 */
void PPIScene::updateSceneSize(const QSize &newSize) {
    setSceneRect(QRectF(-newSize.width()/2, -newSize.height()/2, newSize.width(), newSize.height()));

    double radius = qMin(newSize.width(), newSize.height())/2.0 - pviewMargin; // margin=20
    m_axis->setPixelsPerMeter(radius / m_axis->maxRange());

    emit rangeChanged(m_axis->minRange(), m_axis->maxRange());
}


/**
 * @brief 初始化图层对象
 * @details 创建PPI显示所需的所有图层组件，建立组件间的协调关系，
 *          确保各组件能够响应场景范围变化并同步更新显示
 * 
 * 创建的组件：
 * 1. PolarGrid - 极坐标网格：绘制距离圆环和方位线
 * 2. DetManager - 检测点管理器：管理雷达原始检测点显示
 * 3. TrackManager - 航迹管理器：管理目标航迹显示和更新
 * 4. Tooltip - 工具提示：提供鼠标悬停时的信息显示
 * 
 * 组件关系：
 * - 所有管理器都依赖PolarAxis进行坐标转换
 * - 通过信号槽机制实现组件间的同步更新
 * - 范围变化时各组件自动刷新显示
 * 
 * 信号槽连接：
 * - rangeChanged → PolarGrid::updateGrid: 网格重绘
 * - rangeChanged → DetManager::refreshAll: 检测点重定位/隐藏
 * - rangeChanged → TrackManager::refreshAll: 航迹重定位/隐藏
 * 
 * 组件初始化参数：
 * - this: 将场景对象作为图形容器传递
 * - m_axis: 将极坐标轴作为坐标转换引擎传递
 * 
 * 生命周期管理：
 * - 所有组件通过Qt父子关系自动管理内存
 * - 场景销毁时自动清理所有子组件
 * 
 * @note 该函数在构造函数中调用，确保所有组件在场景使用前完成初始化
 */
void PPIScene::initLayerObjects()
{
    m_grid = new PolarGrid(this, m_axis);
    m_det = new DetManager(this, m_axis);
    m_track = new TrackManager(this, m_axis);
    m_tooltip = new Tooltip(); //
    // 联动：有 range 改变时，网格重绘，点迹重定位/隐藏
    connect(this, &PPIScene::rangeChanged, m_grid, &PolarGrid::updateGrid);
    connect(this, &PPIScene::rangeChanged, m_det, &DetManager::refreshAll);
    connect(this, &PPIScene::rangeChanged, m_track, &TrackManager::refreshAll);
}

/**
 * @brief 设置PPI显示范围
 * @details 更新PPI显示的距离范围，确保参数有效性并通知所有组件同步更新
 * @param minR 最小显示距离（公里），通常为0
 * @param maxR 最大显示距离（公里），如5表示5公里探测范围
 * 
 * 参数验证：
 * 1. 最小距离检查：确保minR不小于0，负值自动修正为0
 * 2. 最大距离检查：确保maxR大于minR，无效时自动设为minR+1
 * 3. 保证显示范围的逻辑合理性
 * 
 * 更新流程：
 * 1. 参数有效性验证和修正
 * 2. 更新极坐标轴的距离范围
 * 3. 发射rangeChanged信号通知所有相关组件
 * 
 * 影响的组件：
 * - PolarAxis: 更新坐标转换的距离基准
 * - PolarGrid: 重新绘制距离圆环和刻度
 * - DetManager: 重新计算检测点位置，隐藏超范围点
 * - TrackManager: 重新计算航迹位置，隐藏超范围航迹
 * - 其他监听rangeChanged信号的组件
 * 
 * 典型用法：
 * - 雷达模式切换：不同模式有不同的探测范围
 * - 用户缩放：通过界面控制调整显示范围
 * - 自适应调整：根据目标分布自动调整合适范围
 * 
 * 范围限制：
 * - 最小范围：0公里（雷达中心位置）
 * - 最大范围：由雷达系统性能和配置决定
 * - 范围差值：至少为1公里，确保有效显示区域
 * 
 * @example 常用范围设置：
 * @code
 * setRange(0, 5);    // 0-5公里短程显示
 * setRange(0, 50);   // 0-50公里中程显示  
 * setRange(0, 200);  // 0-200公里远程显示
 * @endcode
 * 
 * @note 范围变化会立即生效，所有显示元素自动重新定位
 */
void PPIScene::setRange(float minR, float maxR)
{
    if (minR < 0)
        minR = 0;
    if (maxR <= minR)
        maxR = minR + 1;
    m_axis->setRange(minR, maxR);
    emit rangeChanged(minR, maxR);
}
