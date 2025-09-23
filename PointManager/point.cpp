/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:04
 * @Description: 
 */
/**
 * @file point.cpp
 * @brief 雷达点对象基类和派生类实现文件
 * @details 实现雷达显示系统中的点对象体系：
 *          - Point基类：提供通用点对象功能实现
 *          - DetPoint：检测点的具体显示实现
 *          - TrackPoint：航迹点的具体显示实现
 *          - 统一的悬停效果和工具提示机制
 * @author DispCtrl Team
 * @date 2024  
 */

#include "point.h"
#include <QGraphicsSceneHoverEvent>
#include <QPen>
#include "Basic/DispBasci.h"
#include "PolarDisp/tooltip.h"

/**
 * @brief Point基类构造函数实现
 * @param pi 雷达点信息结构体引用
 * @details 完成基类初始化：
 *          1. 保存雷达数据信息
 *          2. 根据点类型生成工具提示文本
 *          3. 设置图形项属性和Z值
 *          4. 启用鼠标悬停事件
 */
Point::Point(PointInfo &pi) : info(pi)
{
    // 根据点类型生成标签文本
    QString typeStr;
    switch (info.type) {
    case 1:
    {
        typeStr = DET_LABEL;  // 检测点标签
        break;
    }
    case 2:
    {
        typeStr = TRA_LABEL;  // 航迹点标签
        break;
    }
    }

    // 构建详细的工具提示文本
    if (info.type == 1) {
        // 检测点信息：不包含批次号
        text = QString("%1\nR:%2m\nA:%3°\nE:%4°\nSNR:%5dB\nV:%6m/s\nH:%7m\nAmp:%8")
                .arg(typeStr).arg(info.range).arg(info.azimuth).arg(info.elevation)
                .arg(info.SNR).arg(info.speed).arg(info.altitute).arg(info.amp);
    } else {
        // 航迹点信息：包含批次号
        text = QString("%1\nNum:%2\nR:%3m\nA:%4°\nE:%5°\nSNR:%6dB\nV:%7m/s\nH:%8m\nAmp:%9")
                .arg(typeStr).arg(info.batch).arg(info.range).arg(info.azimuth).arg(info.elevation)
                .arg(info.SNR).arg(info.speed).arg(info.altitute).arg(info.amp);
    }

    // 设置图形项属性
    setAcceptHoverEvents(true);  // 启用悬停事件
    setZValue(POINT_Z);          // 设置Z值，确保点在网格上层显示
}

/**
 * @brief 更新点的屏幕位置
 * @param x 新的X坐标(像素)
 * @param y 新的Y坐标(像素)
 * @details 更新点在场景中的位置：
 *          - 保存新的坐标值
 *          - 调用setSmallRect()更新图形矩形
 *          - 保持点的几何中心对齐
 */
void Point::updatePosition(float x, float y)
{
    mX = x; 
    mY = y;
    setSmallRect();  // 以新位置为中心重新设置矩形
}

/**
 * @brief 鼠标进入悬停事件处理
 * @param event 悬停事件对象
 * @details 鼠标悬停在点上时的响应：
 *          - 在鼠标位置显示详细信息工具提示
 *          - 放大点的显示尺寸以增强视觉反馈
 *          - 调用基类事件处理
 */
void Point::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    TOOL_TIP->showTooltip(event->screenPos(), text);  // 显示工具提示
    setBigRect();  // 切换到放大尺寸
    QGraphicsEllipseItem::hoverEnterEvent(event);
}

/**
 * @brief 鼠标移动悬停事件处理
 * @param event 悬停事件对象
 * @details 鼠标在点上移动时：
 *          - 更新工具提示位置跟随鼠标
 *          - 保持点的放大状态
 */
void Point::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    TOOL_TIP->showTooltip(event->screenPos(), text);  // 更新工具提示位置
    setBigRect();  // 确保保持放大状态
    QGraphicsEllipseItem::hoverMoveEvent(event);
}

/**
 * @brief 鼠标离开悬停事件处理
 * @param event 悬停事件对象
 * @details 鼠标离开点时的响应：
 *          - 隐藏工具提示(Linux下使用不同机制)
 *          - 恢复点的正常显示尺寸
 *          - 调用基类事件处理
 */
void Point::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
#ifdef Q_OS_LINUX
    TOOL_TIP->setHoldingState(false);  // Linux特殊处理
#else
    TOOL_TIP->setVisible(false);       // 直接隐藏工具提示
#endif
    setSmallRect();  // 恢复正常尺寸
    QGraphicsEllipseItem::hoverLeaveEvent(event);
}

/**
 * @brief 设置普通尺寸的椭圆矩形
 * @details 基类实现，以当前位置为中心设置普通大小的矩形：
 *          - 使用当前的w,h尺寸
 *          - 以mX,mY为几何中心
 */
void Point::setSmallRect()
{
    setRect(mX - w*0.5f, mY - h*0.5f, w, h);
}

/**
 * @brief 设置放大尺寸的椭圆矩形
 * @details 基类实现，以当前位置为中心设置放大的矩形：
 *          - 使用当前的W,H尺寸
 *          - 以mX,mY为几何中心
 */
void Point::setBigRect()
{
    setRect(mX - W*0.5f, mY - H*0.5f, W, H);
}

// ==================== DetPoint 检测点实现 ====================

/**
 * @brief DetPoint构造函数
 * @param info 检测点信息
 * @details 初始化检测点特有的显示参数：
 *          - 设置检测点的基础尺寸常量
 *          - 初始化当前显示尺寸
 *          - 应用检测点的默认颜色
 */

DetPoint::DetPoint  (PointInfo &info) : Point(info)
{
    // 设置检测点的基础尺寸参数
    baseSmallW = baseSmallH = DET_SIZE;        // 普通状态：小尺寸显示
    baseBigW   = baseBigH   = DET_BIG_SIZE;    // 悬停状态：适中放大尺寸
    
    // 初始化当前显示尺寸
    w = baseSmallW; h = baseSmallH;  // 普通尺寸
    W = baseBigW;   H = baseBigH;    // 放大尺寸
    
    // 应用检测点默认颜色
    setColor(DET_COLOR);
}

/**
 * @brief 检测点缩放功能实现
 * @param ratio 缩放比例(>0)，1.0为原始大小
 * @details 实现检测点的动态缩放：
 *          - 防护性检查：比例值必须大于0
 *          - 根据缩放比例计算新的显示尺寸
 *          - 更新当前尺寸并重绘矩形
 *          - 缩放比例越大，点显示越小(适应放大的视图)
 */
void DetPoint::resize(float ratio)
{
    if (ratio <= 0) ratio = 1.f;  // 防护性检查
    curRatio = ratio;              // 记录当前缩放比例
    
    // 根据缩放比例计算新尺寸(反比关系：视图放大时点相对变小)
    w = baseSmallW / ratio;
    h = baseSmallH / ratio;
    W = baseBigW   / ratio;
    H = baseBigH   / ratio;
    
    setSmallRect();  // 以新尺寸更新显示矩形
}

/**
 * @brief 检测点颜色设置实现
 * @param color 新的颜色值
 * @details 设置检测点的颜色外观：
 *          - 创建指定颜色的画笔
 *          - 设置1像素宽度的边框
 *          - 同时设置边框和填充颜色
 */
void DetPoint::setColor(QColor color)
{
    QPen pen(color);
    pen.setWidth(1);           // 细边框
    setPen(pen);               // 设置边框
    setBrush(QBrush(color));   // 设置填充
}

/**
 * @brief 检测点普通尺寸矩形设置
 * @details 重写基类方法，使用检测点特有的小尺寸
 */
void DetPoint::setSmallRect()
{
    setRect(mX - w*0.5f, mY - h*0.5f, w, h);
}

/**
 * @brief 检测点放大尺寸矩形设置
 * @details 重写基类方法，使用检测点特有的放大尺寸
 */
void DetPoint::setBigRect()
{
    setRect(mX - W*0.5f, mY - H*0.5f, W, H);
}

// ==================== TrackPoint 航迹点实现 ====================

/**
 * @brief TrackPoint构造函数
 * @param info 航迹点信息
 * @details 初始化航迹点特有的显示参数：
 *          - 设置航迹点的基础尺寸常量(比检测点更大)
 *          - 初始化当前显示尺寸
 *          - 应用航迹点的默认颜色
 */

TrackPoint::TrackPoint(PointInfo &info) : Point(info)
{
    // 设置航迹点的基础尺寸参数(比检测点更大更醒目)
    baseSmallW = baseSmallH = TRA_SIZE;        // 普通状态：中等尺寸
    baseBigW   = baseBigH   = TRA_BIG_SIZE;    // 悬停状态：大尺寸显示
    
    // 初始化当前显示尺寸
    w = baseSmallW; h = baseSmallH;  // 普通尺寸
    W = baseBigW;   H = baseBigH;    // 放大尺寸
    
    // 应用航迹点默认颜色
    setColor(TRA_COLOR);
}

/**
 * @brief 航迹点缩放功能实现
 * @param ratio 缩放比例(>0)，1.0为原始大小
 * @details 实现航迹点的动态缩放：
 *          - 防护性检查：比例值必须大于0
 *          - 根据缩放比例等比缩放所有尺寸
 *          - 避免之前drawline int限制导致的显示问题
 *          - 更新当前尺寸并重绘矩形
 */
void TrackPoint::resize(float ratio)
{
    if (ratio <= 0) ratio = 1.f;  // 防护性检查
    curRatio = ratio;              // 记录当前缩放比例
    
    // 航迹点也做等比缩放，避免显示问题
    w = baseSmallW / ratio;
    h = baseSmallH / ratio;
    W = baseBigW   / ratio;
    H = baseBigH   / ratio;
    
    setSmallRect();  // 以新尺寸更新显示矩形
}

/**
 * @brief 航迹点颜色设置实现
 * @param color 新的颜色值
 * @details 设置航迹点的颜色外观：
 *          - 创建指定颜色的画笔
 *          - 设置1像素宽度的边框
 *          - 同时设置边框和填充颜色
 */
void TrackPoint::setColor(QColor color)
{
    QPen pen(color);
    pen.setWidth(1);           // 细边框
    setPen(pen);               // 设置边框
    setBrush(QBrush(color));   // 设置填充
}

/**
 * @brief 航迹点普通尺寸矩形设置
 * @details 重写基类方法，使用航迹点特有的中等尺寸
 */
void TrackPoint::setSmallRect()
{
    setRect(mX - w*0.5f, mY - h*0.5f, w, h);
}

/**
 * @brief 航迹点放大尺寸矩形设置
 * @details 重写基类方法，使用航迹点特有的大尺寸
 */
void TrackPoint::setBigRect()
{
    setRect(mX - W*0.5f, mY - H*0.5f, W, H);
}
