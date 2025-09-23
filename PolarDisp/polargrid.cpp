/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:09
 * @Description: 
 */
/**
 * @file polargrid.cpp
 * @brief 极坐标网格绘制器实现文件
 * @details 实现PPI雷达显示的极坐标网格绘制功能，包括距离圆环、方位线、
 *          刻度标注和雷达图标的绘制与管理。支持动态更新和自定义角度范围显示。
 * @author DispCtrl Development Team
 * @date 2024
 * @version 1.0
 */

#include "polargrid.h"
#include <QPen>
#include <QFont>
#include <QGraphicsTextItem>
#include <QContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QtMath>
#include <cmath>
#include "Basic/DispBasci.h"
#include "polaraxis.h"

/**
 * @brief PolarGrid构造函数
 * @details 初始化极坐标网格绘制器，建立与场景和坐标轴的关联，
 *          连接坐标轴范围变化信号以实现自动更新功能。
 * @param scene 图形场景指针，所有网格元素将绘制在此场景中
 * @param axis 极坐标轴指针，提供坐标转换和距离范围信息
 * @param parent 父对象指针，用于Qt对象树管理
 * 
 * 初始化流程：
 * 1. 保存场景和坐标轴引用
 * 2. 连接坐标轴的rangeChanged信号到updateGrid槽
 * 3. 调用updateGrid()进行初始化绘制
 * 
 * 信号连接：
 * - 当坐标轴范围改变时自动重新绘制网格
 * - 确保网格始终与当前坐标系设置同步
 */
PolarGrid::PolarGrid(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), mScene(scene), mAxis(axis)
{
    connect(mAxis, &PolarAxis::rangeChanged, this, &PolarGrid::updateGrid);
    updateGrid();
}

/**
 * @brief 更新极坐标网格显示
 * @details 完整重绘极坐标网格系统，包括清理旧元素和绘制新的网格组件。
 *          该函数是网格显示的核心，负责所有可视化元素的创建和布局。
 * 
 * 绘制流程：
 * 1. 清理阶段：移除场景中所有旧的网格图形项
 * 2. 参数计算：根据坐标轴获取最大显示半径
 * 3. 雷达图标：在中心位置绘制雷达标识
 * 4. 距离圆环：绘制最外层边界圆和内部等距圆环
 * 5. 方位刻度：绘制径向刻度线（5°间隔）
 * 6. 角度标注：添加10°间隔的角度数值标识
 * 7. 十字分割线：绘制主要方向的径向线（90°间隔）
 * 8. 距离标注：在右侧显示距离刻度值
 * 
 * 网格组成说明：
 * - 外层实线圆：定义最大显示范围边界
 * - 内层虚线圆：5个等距的距离参考圆环
 * - 刻度线：360个5°间隔的短线，10°处加粗显示
 * - 十字线：0°、90°、180°、270°的主方向指示线
 * - 文字标注：角度值（每10°）和距离值（右侧）
 * 
 * 颜色方案：
 * - 主要元素：绿色 RGB(0, 255, 136)
 * - 辅助元素：灰色虚线
 * - 文字标注：白色和绿色
 * 
 * @note 考虑角度范围限制，只在指定扇区内绘制刻度和分割线
 */
void PolarGrid::updateGrid() {
    // 清理旧元素
    for (auto item : mCircleItems) mScene->removeItem(item);
    for (auto item : mTickItems) mScene->removeItem(item);
    for (auto item : mTextItems) mScene->removeItem(item);
    mCircleItems.clear();
    mTickItems.clear();
    mTextItems.clear();

    if (!mScene) return;

    double radius = mAxis->rangeToPixel(mAxis->maxRange());
    QPointF center(0, 0);

    // === 中心雷达图标绘制 ===
    // 在坐标原点放置雷达天线图标，作为PPI显示的中心标识
    if (!mRadarIcon) {
        QPixmap pix(":/resources/icon/array.png");
        pix = pix.scaled(40,40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        mRadarIcon = mScene->addPixmap(pix);
        mRadarIcon->setZValue(100); // 确保在最上层显示
        mRadarIcon->setOffset(-pix.width()/2, -pix.height()/2); // 图标居中对齐
    }

    // === 1. 最外层边界圆绘制 ===
    // 绘制PPI显示的最大范围边界，使用绿色实线突出显示
    QPen outerPen(QColor(0, 255, 136));
    outerPen.setWidth(2);
    QGraphicsEllipseItem* outerCircle =
            mScene->addEllipse(-radius, -radius, radius*2, radius*2, outerPen);
    mCircleItems.append(outerCircle);

    // === 2. 内部距离参考圆环绘制 ===
    // 绘制5个等距的虚线圆环，作为距离估算的参考标识
    QPen dashPen(Qt::gray);
    dashPen.setStyle(Qt::DashLine);
    int ringCount = 5;  // 固定绘制5个距离圆环
    for (int i=1; i<=ringCount; ++i) {
        double r = radius * i / ringCount;  // 等距分布计算
        QGraphicsEllipseItem* ring =
                mScene->addEllipse(-r, -r, r*2, r*2, dashPen);
        mCircleItems.append(ring);
    }

    // === 3. 径向方位刻度线绘制（外圈，5°间隔） ===
    // 在最外层圆周绘制方位角刻度线，仅在指定角度扇区内显示
    for (int angle=0; angle<360; angle+=5) {
        double ang = angle;
        
        // 角度范围检查：判断当前角度是否在显示扇区内
        double as = m_angleStart;
        double ae = m_angleEnd;
        bool inSector;
        if (as <= ae) {
            // 正常角度范围（如0-180°）
            inSector = (ang >= as && ang <= ae);
        } else {
            // 跨越0°的角度范围（如270-90°）
            inSector = (ang >= as || ang <= ae);
        }
        if (!inSector) continue;  // 跳过扇区外的角度
        
        // 极坐标到直角坐标转换
        double rad = qDegreesToRadians((double)angle);
        double x1, y1, x2, y2;
        int len;
        QPen tickPen(QColor(0, 255, 136));

        // 刻度线长度和粗细设置：10°倍数用粗线，其他用细线
        if (angle % 10 == 0) {
            len = 15;           // 主刻度线长度
            tickPen.setWidth(2); // 主刻度线粗细
        } else {
            len = 8;            // 次刻度线长度
            tickPen.setWidth(1); // 次刻度线粗细
        }

        // 计算刻度线的起点和终点坐标
        x1 = qSin(rad) * (radius - len);  // 内侧起点
        y1 = -qCos(rad) * (radius - len);
        x2 = qSin(rad) * radius;          // 外侧终点
        y2 = -qCos(rad) * radius;

        QGraphicsLineItem* tick = mScene->addLine(x1, y1, x2, y2, tickPen);
        mTickItems.append(tick);

        // === 4. 角度数值标注绘制（每10°显示） ===
        // 在主刻度线位置添加角度数值，便于方位角读取
        if (angle % 10 == 0) {
            // 计算文字标注的位置（在刻度线外侧）
            double tx = qSin(rad) * (radius + 20);
            double ty = -qCos(rad) * (radius + 20);
            
            // 创建角度数值文本
            QGraphicsSimpleTextItem* text =
                    mScene->addSimpleText(QString::number(angle));
            text->setBrush(QColor(0, 255, 136));  // 设置文字颜色为绿色
            
            // 文字居中对齐到计算位置
            text->setPos(tx - text->boundingRect().width()/2,
                         ty - text->boundingRect().height()/2);
            mTextItems.append(text);
        }
    }

    // === 5. 主方向十字分割线绘制（90°间隔） ===
    // 绘制四条主要方向的径向线：北(0°)、东(90°)、南(180°)、西(270°)
    QPen crossPen(QColor(0, 255, 136, 128));  // 半透明绿色
    crossPen.setStyle(Qt::DashLine);
    for (int angle=0; angle<360; angle+=90) {
        double ang = angle;
        
        // 检查主方向线是否在显示扇区内
        double as = m_angleStart;
        double ae = m_angleEnd;
        bool inSector;
        if (as <= ae) {
            inSector = (ang >= as && ang <= ae);
        } else {
            inSector = (ang >= as || ang <= ae);
        }
        if (!inSector) continue;
        
        // 计算从中心到边界的直线坐标
        double rad = qDegreesToRadians((double)angle);
        double x = qSin(rad) * radius;
        double y = -qCos(rad) * radius;
        
        QGraphicsLineItem* line =
                mScene->addLine(center.x(), center.y(), x, y, crossPen);
        mTickItems.append(line);
    }

    // === 6. 右侧距离刻度标注绘制 ===
    // 在屏幕右侧显示距离值，对应各个距离圆环的实际距离
    double maxRange = mAxis->maxRange();
    int rangeStep = maxRange / ringCount;  // 计算每个圆环对应的距离步长
    
    for (int i=1; i<=ringCount; ++i) {
        double r = radius * i / ringCount;  // 当前圆环的像素半径
        
        // 距离标签格式化：第一个圆环显示"km"单位，其他仅显示数值
        QString label;
        if(i == 1) {
            label = QString::number(i * rangeStep / 1000.0, 'f', 1) + " km";
        } else {
            label = QString::number(i * rangeStep / 1000.0, 'f', 1);
        }
        
        // 创建距离标注文本并定位到右侧
        QGraphicsSimpleTextItem* txt = mScene->addSimpleText(label);
        txt->setBrush(Qt::white);  // 白色文字便于识别
        txt->setPos(r-25, -txt->boundingRect().height()/2);  // 右侧居中对齐
        mTextItems.append(txt);
    }
}

/**
 * @brief 设置角度显示范围
 * @details 配置极坐标网格的可见角度扇区，支持任意角度范围的显示控制
 * @param startDeg 起始角度（度），0°表示正北方向
 * @param endDeg 结束角度（度），顺时针方向递增
 * 
 * 功能说明：
 * - 设置网格的可见角度范围，影响刻度线和分割线的绘制
 * - 支持跨越0°的角度范围（如270°-90°表示前方180°扇区）
 * - 设置后自动调用updateGrid()重新绘制网格
 * 
 * 应用场景：
 * - 扇区雷达显示：只显示特定方向的探测区域
 * - 局部监控：聚焦于某个角度范围内的目标
 * - 导航雷达：显示前方或侧方的扇形区域
 * 
 * 角度约定：
 * - 0° = 正北方向（屏幕正上方）
 * - 90° = 正东方向（屏幕右侧）
 * - 180° = 正南方向（屏幕正下方）
 * - 270° = 正西方向（屏幕左侧）
 * 
 * @example 常用角度范围设置：
 * @code
 * setAngleRange(0, 360);    // 全圆显示
 * setAngleRange(270, 90);   // 前方180°扇区
 * setAngleRange(315, 45);   // 前方90°扇区
 * @endcode
 * 
 * @note 角度范围变化会立即生效，无需手动调用更新函数
 */
void PolarGrid::setAngleRange(double startDeg, double endDeg)
{
    m_angleStart = startDeg;
    m_angleEnd = endDeg;
    updateGrid();  // 立即更新网格显示以反映新的角度范围
}



