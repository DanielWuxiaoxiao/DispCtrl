/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:20
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

#include "Basic/log.h"

#include "point.h"
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPen>
#include <QDebug>
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
    rebuildTooltipText();

    // 设置图形项属性
    setAcceptHoverEvents(true);  // 启用悬停事件
    setZValue(POINT_Z);          // 设置Z值，确保点在网格上层显示
}

void Point::setInfo(const PointInfo& nextInfo)
{
    info = nextInfo;
    rebuildTooltipText();
}

void Point::rebuildTooltipText()
{
    const bool isDetection = (info.type == PointType::Detection);
    const QString typeStr = isDetection ? QString::fromUtf8(DET_LABEL)
                                        : trackTypeLabel(info.type);

    if (isDetection) {
        text = QString("%1\nR:%2m\nA:%3°\nE:%4°\nSNR:%5dB\nV:%6m/s\nH:%7m\nAmp:%8")
                .arg(typeStr).arg(info.range).arg(info.azimuth).arg(info.elevation)
                .arg(info.SNR).arg(info.speed).arg(info.altitute).arg(info.amp);
        return;
    }

    const QString targetRecStr = (info.targetRecResult == 1) ? "无人机" : "其它";
    text = QString("%1\nNum:%2\nR:%3m\nA:%4°\nE:%5°\nSNR:%6dB\nV:%7m/s\nH:%8m\nAmp:%9\n识别:%10")
            .arg(typeStr).arg(info.batch).arg(info.range).arg(info.azimuth).arg(info.elevation)
            .arg(info.SNR).arg(info.speed).arg(info.altitute).arg(info.amp).arg(targetRecStr);
}

/**
 * @brief 更新点的屏幕位置
 * @param x 新的X坐标(像素)
 * @param y 新的Y坐标(像素)
 * @details 更新点在场景中的位置：
 *          - 使用setPos()设置图形项的场景位置
 *          - 调用setSmallRect()更新椭圆几何形状（以(0,0)为中心）
 *          - 这样scenePos()能正确返回点的位置，连线才能正确绑定
 */
void Point::updatePosition(float x, float y)
{
    mX = x;
    mY = y;
    setPos(x, y);    // 设置图形项的场景位置
    setSmallRect();  // 更新椭圆几何形状（现在以(0,0)为中心）
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
    // 使用场景坐标而不是屏幕坐标，并偏移一点避免遮挡鼠标
    QPointF tooltipPos = event->scenePos() + QPointF(15, 15);
    TOOL_TIP->showTooltip(tooltipPos, text);  // 显示工具提示
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
    // 使用场景坐标而不是屏幕坐标，并偏移一点避免遮挡鼠标
    QPointF tooltipPos = event->scenePos() + QPointF(15, 15);
    TOOL_TIP->showTooltip(tooltipPos, text);  // 更新工具提示位置
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
    TOOL_TIP->setVisible(false);  // Linux特殊处理
#else
    TOOL_TIP->setVisible(false);       // 直接隐藏工具提示
#endif
    setSmallRect();  // 恢复正常尺寸
    QGraphicsEllipseItem::hoverLeaveEvent(event);
}

/**
 * @brief 鼠标按下事件处理
 * @param event 鼠标事件对象
 * @details 点击点时获取点信息，用于选中该航迹批次
 *          通过 scene() 获取场景，然后通过 views() 获取视图来传递信息
 */
void Point::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        LOG_DEBUG(QString("[Point::mousePressEvent] Point clicked, batch: %1 type: %2")
                  .arg(info.batch)
                  .arg(info.type));

        // 发送自定义事件或通过属性传递信息
        // 设置一个标记在 scene 的属性中
        if (scene()) {
            scene()->setProperty("selectedBatchID", info.batch);
            scene()->setProperty("selectedPointInfo", QVariant::fromValue(info));
        }

        // 调用基类处理
        QGraphicsEllipseItem::mousePressEvent(event);

        // 接受事件，阻止传播
        event->accept();
    } else {
        QGraphicsEllipseItem::mousePressEvent(event);
    }
}

/**
 * @brief 设置普通尺寸的椭圆矩形
 * @details 基类实现，以(0,0)为中心设置普通大小的矩形：
 *          - 使用当前的w,h尺寸
 *          - 图形项的实际位置由setPos()控制
 */
void Point::setSmallRect()
{
    setRect(-w*0.5f, -h*0.5f, w, h);
}

/**
 * @brief 设置放大尺寸的椭圆矩形
 * @details 基类实现，以(0,0)为中心设置放大的矩形：
 *          - 使用当前的W,H尺寸
 *          - 图形项的实际位置由setPos()控制
 */
void Point::setBigRect()
{
    setRect(-W*0.5f, -H*0.5f, W, H);
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
 *          - 注意：不在这里调用setSmallRect()，由updatePosition负责更新位置
 */
void DetPoint::resize(float ratio)
{
    if (ratio <= 0) ratio = 1.f;  // 防护性检查
    curRatio = ratio;              // 记录当前缩放比例

    // 根据缩放比例计算新尺寸(正比关系：ratio越大点越大)
    w = baseSmallW * ratio;
    h = baseSmallH * ratio;
    W = baseBigW   * ratio;
    H = baseBigH   * ratio;

    // 注意：不在这里调用 setSmallRect()
    // 因为在创建检测点时，mX和mY还未设置，会导致点被错误放置到(0,0)
    // setSmallRect() 会在 updatePosition() 中被调用
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
 *          矩形以(0,0)为中心，实际位置由setPos()控制
 */
void DetPoint::setSmallRect()
{
    setRect(-w*0.5f, -h*0.5f, w, h);
}

/**
 * @brief 检测点放大尺寸矩形设置
 * @details 重写基类方法，使用检测点特有的放大尺寸
 *          矩形以(0,0)为中心，实际位置由setPos()控制
 */
void DetPoint::setBigRect()
{
    setRect(-W*0.5f, -H*0.5f, W, H);
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
 *          - 注意：不在这里调用setSmallRect()，由updatePosition负责更新位置
 */
void TrackPoint::resize(float ratio)
{
    if (ratio <= 0) ratio = 1.f;  // 防护性检查
    curRatio = ratio;              // 记录当前缩放比例

    // 航迹点也做正比缩放，ratio越大点越大
    w = baseSmallW * ratio;
    h = baseSmallH * ratio;
    W = baseBigW   * ratio;
    H = baseBigH   * ratio;

    // 注意：不在这里调用 setSmallRect()
    // 因为在创建航迹点时，mX和mY还未设置，会导致点被错误放置到(0,0)
    // setSmallRect() 会在 updatePosition() 中被调用
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
    m_color = color;

    QPen pen(color);
    pen.setWidth(m_focused ? 2 : 1);
    setPen(pen);
    setBrush(m_focused ? QBrush(Qt::NoBrush) : QBrush(color));
    update();
}

void TrackPoint::setFocused(bool focused)
{
    if (m_focused == focused) {
        return;
    }

    m_focused = focused;
    setSmallRect();
    setColor(m_color);
}

void TrackPoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (!m_focused) {
        QGraphicsEllipseItem::paint(painter, option, widget);
        return;
    }

    Q_UNUSED(option)
    Q_UNUSED(widget)

    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen focusPen(m_color);
    focusPen.setWidth(2);
    painter->setPen(focusPen);
    painter->setBrush(Qt::NoBrush);

    const QRectF r = rect();
    QPolygonF triangle;
    triangle << QPointF(r.center().x(), r.top())
             << QPointF(r.right(), r.bottom())
             << QPointF(r.left(), r.bottom());
    painter->drawPolygon(triangle);
}

/**
 * @brief 航迹点普通尺寸矩形设置
 * @details 重写基类方法，使用航迹点特有的中等尺寸
 *          矩形以(0,0)为中心，实际位置由setPos()控制
 */
void TrackPoint::setSmallRect()
{
    const float scale = m_focused ? 1.8f : 1.0f;
    const float drawW = w * scale;
    const float drawH = h * scale;
    QRectF newRect(-drawW*0.5f, -drawH*0.5f, drawW, drawH);
    // qCritical() << "[TrackPoint::setSmallRect]"
    //             << "batch=" << info.batch
    //             << "pos()=" << pos()
    //             << "w=" << w << "h=" << h
    //             << "rect=(" << newRect.x() << "," << newRect.y()
    //             << "," << newRect.width() << "," << newRect.height() << ")";
    setRect(newRect);
}

/**
 * @brief 航迹点放大尺寸矩形设置
 * @details 重写基类方法，使用航迹点特有的大尺寸
 *          矩形以(0,0)为中心，实际位置由setPos()控制
 */
void TrackPoint::setBigRect()
{
    const float scale = m_focused ? 1.8f : 1.0f;
    setRect(-W*scale*0.5f, -H*scale*0.5f, W*scale, H*scale);
}
