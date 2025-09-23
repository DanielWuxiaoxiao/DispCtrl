/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:45:12
 * @Description: 
 */
/**
 * @file sectorpolargrid.cpp
 * @brief 扇形极坐标网格实现
 * @details 实现专门为扇形显示优化的网格渲染系统
 * 
 * 实现要点：
 * 1. 坐标系统转换：Qt坐标系与雷达坐标系的准确转换
 * 2. 智能网格算法：根据显示范围自动计算最佳网格密度
 * 3. 弧线绘制技术：使用QPainter::drawArc实现精确的圆弧绘制
 * 4. 标签布局算法：避免重叠的智能标签放置策略
 * 5. 性能优化：边界检查和选择性绘制提升渲染效率
 * 
 * 绘制流程：
 * 1. 距离圆弧：计算间隔并绘制同心圆弧
 * 2. 角度射线：绘制径向线条和边界线
 * 3. 文本标签：在最佳位置放置数值标签
 * 
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2024
 */

// sectorpolargrid.cpp
#include "sectorpolargrid.h"
#include "PolarDisp/polaraxis.h"
#include <QPainter>
#include <QtMath>
#include <QFont>

/**
 * @brief 扇形极坐标网格构造函数
 * @param axis 极坐标轴系统指针
 * @param parent 父图形项
 * @details 初始化扇形网格，配置绘制样式和层次关系
 * 
 * 初始化内容：
 * 1. 画笔配置：
 *    - 距离圈：中等灰色实线，用于距离参考
 *    - 角度线：较暗灰色实线，用于方向参考
 *    - 文本：浅色画笔，确保在暗背景下可见
 * 
 * 2. 层次设置：
 *    - setZValue(-1)：置于背景层
 *    - 确保网格不遮挡数据点和航迹
 *    - 提供清晰的背景参考
 * 
 * 3. 默认范围：
 *    - 角度范围：-30°到+30°（60度扇形）
 *    - 适合大多数雷达显示需求
 *    - 可通过setSectorRange()方法调整
 * 
 * 设计考虑：
 * - 颜色选择：适配深色雷达显示背景
 * - 线条粗细：保证可见性但不喧宾夺主
 * - 性能：轻量级初始化，避免不必要计算
 */
SectorPolarGrid::SectorPolarGrid(PolarAxis* axis, QGraphicsItem* parent)
    : QObject(nullptr), QGraphicsItem(parent), m_axis(axis)
{
    // 设置画笔样式 - 调整为更符合雷达显示的颜色
    m_rangePen = QPen(QColor(80, 80, 80), 1, Qt::SolidLine);      // 距离圈：较暗灰色
    m_anglePen = QPen(QColor(85, 85, 85), 1, Qt::SolidLine);      // 角度线：更暗灰色
    m_textPen = QPen(QColor(150, 150, 150), 1);                   // 文字：中等亮度灰色
    m_borderPen = QPen(QColor(100, 100, 100), 2, Qt::SolidLine);  // 边界线：稍亮一些

    setZValue(-1); // 确保网格在背景层
    
    // 默认扇形范围
    m_minAngle = -30.0f;
    m_maxAngle = 30.0f;
}

/**
 * @brief 设置扇形显示范围
 * @param minAngle 最小角度
 * @param maxAngle 最大角度
 * @details 更新扇形的角度范围，如有变化则触发重绘
 * 
 * 变化检测：
 * - 比较新旧角度值
 * - 只在实际变化时触发更新
 * - 避免不必要的重绘操作
 * 
 * 重绘触发：
 * - update()：标记图形项需要重绘
 * - Qt会在下一个绘制周期自动调用paint()
 * - 确保显示与设置同步
 * 
 * 应用场景：
 * - 用户调整扇形范围时
 * - 不同雷达模式切换时
 * - 动态扇形跟踪时
 */
void SectorPolarGrid::setSectorRange(float minAngle, float maxAngle)
{
    if (m_minAngle != minAngle || m_maxAngle != maxAngle) {
        m_minAngle = minAngle;
        m_maxAngle = maxAngle;
        update(); // 触发重绘
    }
}

/**
 * @brief 计算图形项边界矩形
 * @return 包含扇形网格内容的边界矩形
 * @details 计算包含扇形网格线条和标签的精确边界范围，而非整个圆形
 * 
 * 扇形边界计算：
 * 1. 角度范围：根据m_minAngle和m_maxAngle确定扇形边界
 * 2. 距离范围：根据轴的最大距离确定半径
 * 3. 精确包围：计算扇形所占的最小矩形区域
 * 4. 标签边距：额外空间确保标签完全可见
 * 
 * 算法步骤：
 * - 计算扇形的极值点（最远的x, y坐标）
 * - 考虑0°、90°、180°、270°等关键角度
 * - 确定扇形在各象限的延伸范围
 * - 添加标签显示所需的边距
 */
QRectF SectorPolarGrid::boundingRect() const
{
    if (!m_axis) return QRectF();

    double maxRange = m_axis->maxRange();
    double pixelRadius = m_axis->rangeToPixel(maxRange);
    double margin = 50; // 增加边距以容纳标签

    if (pixelRadius <= 0) return QRectF(-margin, -margin, 2*margin, 2*margin);

    // 计算扇形的边界
    // 初始化边界值
    double minX = 0, maxX = 0, minY = 0, maxY = 0;
    
    // 检查扇形边界点
    QVector<double> checkAngles;
    checkAngles << m_minAngle << m_maxAngle;
    
    // 添加关键角度点（0°, 90°, 180°, 270°）如果它们在扇形范围内
    for (int keyAngle = -180; keyAngle <= 180; keyAngle += 90) {
        if (keyAngle >= m_minAngle && keyAngle <= m_maxAngle) {
            checkAngles << (double)keyAngle;
        }
    }
    
    // 计算所有关键点的坐标
    for (double angle : checkAngles) {
        double rad = qDegreesToRadians(static_cast<double>(angle));
        double x = pixelRadius * qSin(rad);    // X坐标分量
        double y = pixelRadius * (-qCos(rad)); // Y坐标分量（0°向上）
        
        minX = qMin(minX, x);
        maxX = qMax(maxX, x);
        minY = qMin(minY, y);
        maxY = qMax(maxY, y);
    }
    
    // 确保包含原点（中心点）
    minX = qMin(minX, 0.0);
    maxX = qMax(maxX, 0.0);
    minY = qMin(minY, 0.0);
    maxY = qMax(maxY, 0.0);
    
    // 添加边距
    return QRectF(minX - margin, minY - margin,
                  (maxX - minX) + 2*margin, (maxY - minY) + 2*margin);
}

void SectorPolarGrid::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!m_axis) return;

    painter->setRenderHint(QPainter::Antialiasing, true);

    // 绘制扇形背景（轻微的填充效果）
    drawSectorBackground(painter);

    // 绘制距离同心圆
    drawRangeCircles(painter);

    // 绘制角度射线（只在扇形范围内）
    drawAngleLines(painter);

    // 绘制标签
    drawLabels(painter);
}

void SectorPolarGrid::drawRangeCircles(QPainter* painter)
{
    painter->setPen(m_rangePen);

    float minRange = m_axis->minRange();
    float maxRange = m_axis->maxRange();

    // 固定分割成4块，不管距离范围是多少
    float rangeSpan = maxRange - minRange;
    float rangeStep = rangeSpan / 4.0f; // 固定4等分
    int totalSteps = 4;

    // 绘制距离同心圆（只绘制扇形部分）- 固定4等分
    for (int i = 1; i <= totalSteps; ++i) {
        float range = minRange + i * rangeStep;
        if (range <= minRange || range > maxRange) continue;
        
        double pixelRadius = m_axis->rangeToPixel(range);
        if (pixelRadius <= 0) continue;

        // 绘制扇形弧线 - 修正角度计算
        QRectF rect(-pixelRadius, -pixelRadius, 2*pixelRadius, 2*pixelRadius);

        // Qt的角度系统：0度在3点钟方向，逆时针为正
        // 我们的雷达系统：0度在12点钟方向，顺时针为正
        // 转换公式：qt_angle = 90 - radar_angle
        double startAngleQt = 90.0 - m_maxAngle;  // 从最大角度开始（Qt中是逆时针）
        double spanAngleQt = m_maxAngle - m_minAngle;  // 跨度（正值，逆时针绘制）

        painter->drawArc(rect, startAngleQt * 16, spanAngleQt * 16);
    }
    
    // 绘制内边界圆弧（如果最小距离大于0）
    if (minRange > 0) {
        double minPixelRadius = m_axis->rangeToPixel(minRange);
        if (minPixelRadius > 0) {
            QRectF rect(-minPixelRadius, -minPixelRadius, 2*minPixelRadius, 2*minPixelRadius);
            double startAngleQt = 90.0 - m_maxAngle;
            double spanAngleQt = m_maxAngle - m_minAngle;
            painter->drawArc(rect, startAngleQt * 16, spanAngleQt * 16);
        }
    }
}

void SectorPolarGrid::drawAngleLines(QPainter* painter)
{
    painter->setPen(m_anglePen);

    float minRange = m_axis->minRange();
    float maxRange = m_axis->maxRange();
    double maxPixelRadius = m_axis->rangeToPixel(maxRange);
    double minPixelRadius = m_axis->rangeToPixel(minRange);

    // 使用固定的角度间隔 - 符合雷达显示习惯
    double angleStep = 10.0; // 固定10度间隔
    
    // 绘制角度射线（包括边界角度）- 使用与PolarAxis一致的坐标转换
    for (double angle = m_minAngle; angle <= m_maxAngle; angle += angleStep) {
        // 确保边界角度被绘制
        if (angle > m_maxAngle && angle - angleStep < m_maxAngle) {
            angle = m_maxAngle;
        }
        
        // 使用与PolarAxis::polarToScene相同的坐标转换公式
        double rad = qDegreesToRadians(static_cast<double>(angle));
        double sinA = qSin(rad);   // X坐标分量
        double cosA = -qCos(rad);  // Y坐标分量（注意负号，0°向上）

        QPointF innerPoint(minPixelRadius * sinA, minPixelRadius * cosA);
        QPointF outerPoint(maxPixelRadius * sinA, maxPixelRadius * cosA);

        painter->drawLine(innerPoint, outerPoint);
        
        if (angle == m_maxAngle) break; // 避免超出范围
    }

    // 绘制扇形边界线（使用更明显的画笔）
    painter->setPen(m_borderPen);

    // 最小角度边界 - 使用正确的坐标转换
    double minRadians = qDegreesToRadians(static_cast<double>(m_minAngle));
    QPointF minInner(minPixelRadius * qSin(minRadians), minPixelRadius * (-qCos(minRadians)));
    QPointF minOuter(maxPixelRadius * qSin(minRadians), maxPixelRadius * (-qCos(minRadians)));
    painter->drawLine(minInner, minOuter);

    // 最大角度边界 - 使用正确的坐标转换
    double maxRadians = qDegreesToRadians(static_cast<double>(m_maxAngle));
    QPointF maxInner(minPixelRadius * qSin(maxRadians), minPixelRadius * (-qCos(maxRadians)));
    QPointF maxOuter(maxPixelRadius * qSin(maxRadians), maxPixelRadius * (-qCos(maxRadians)));
    painter->drawLine(maxInner, maxOuter);
}

void SectorPolarGrid::drawLabels(QPainter* painter)
{
    painter->setPen(m_textPen);
    QFont font("Arial", 7); // 更小的字体
    painter->setFont(font);

    float minRange = m_axis->minRange();
    float maxRange = m_axis->maxRange();

    // 距离标签 - 固定4等分，分布在扇形左右边界，显示为公里单位
    float rangeSpan = maxRange - minRange;
    float rangeStep = rangeSpan / 4.0f;

    for (int i = 1; i <= 4; ++i) {
        float range = minRange + i * rangeStep;
        if (range <= minRange || range > maxRange) continue;
        
        double pixelRadius = m_axis->rangeToPixel(range);
        if (pixelRadius <= 0) continue;

        // 转换为公里显示（参考PPIView的显示方式）
        QString rangeText = QString::number((int)(range / 1000.0)) + "km";

        // 左边界标签（最小角度处）- 向圆心靠近一些
        double leftRadians = qDegreesToRadians(static_cast<double>(m_minAngle));
        double leftSinA = qSin(leftRadians);
        double leftCosA = -qCos(leftRadians);
        QPointF leftLabelPos(pixelRadius * leftSinA, pixelRadius * leftCosA);
        
        // 向圆心方向偏移，避免与角度刻度重合
        leftLabelPos += QPointF(leftSinA * 8, leftCosA * 8); // 减少偏移量
        
        QFontMetrics fm(font);
        QRect textRect = fm.boundingRect(rangeText);
        leftLabelPos -= QPointF(textRect.width() / 2, -textRect.height() / 2);
        
        painter->drawText(leftLabelPos, rangeText);

        // 右边界标签（最大角度处）- 向圆心靠近一些
        double rightRadians = qDegreesToRadians(static_cast<double>(m_maxAngle));
        double rightSinA = qSin(rightRadians);
        double rightCosA = -qCos(rightRadians);
        QPointF rightLabelPos(pixelRadius * rightSinA, pixelRadius * rightCosA);
        
        // 向圆心方向偏移，避免与角度刻度重合
        rightLabelPos += QPointF(rightSinA * 8, rightCosA * 8); // 减少偏移量
        rightLabelPos -= QPointF(textRect.width() / 2, -textRect.height() / 2);
        
        painter->drawText(rightLabelPos, rangeText);
    }

    // 角度标签 - 每10度一份
    double labelRadius = m_axis->rangeToPixel(maxRange) + 20; // 在外圈外侧
    
    // 计算角度范围内每10度的标签
    int startAngle = ((int)m_minAngle / 10) * 10; // 向下取整到10度的倍数
    int endAngle = ((int)m_maxAngle / 10) * 10;   // 向下取整到10度的倍数
    
    // 如果结束角度小于实际最大角度，加10度
    if (endAngle < m_maxAngle) {
        endAngle += 10;
    }
    
    for (int angle = startAngle; angle <= endAngle; angle += 10) {
        // 确保角度在扇形范围内
        if (angle < m_minAngle || angle > m_maxAngle) continue;

        double radians = qDegreesToRadians(static_cast<double>(angle));
        double sinAngle = qSin(radians);
        double cosAngle = -qCos(radians);
        
        QPointF labelPos(labelRadius * sinAngle, labelRadius * cosAngle);

        QString text = QString::number(angle) + "°";
        QFontMetrics fm(font);
        QRect textRect = fm.boundingRect(text);
        labelPos -= QPointF(textRect.width() / 2, -textRect.height() / 2);

        painter->drawText(labelPos, text);
    }
}

void SectorPolarGrid::updateGrid()
{
    update(); // 触发重绘
}

void SectorPolarGrid::drawSectorBackground(QPainter* painter)
{
    float minRange = m_axis->minRange();
    float maxRange = m_axis->maxRange();
    double maxPixelRadius = m_axis->rangeToPixel(maxRange);
    double minPixelRadius = m_axis->rangeToPixel(minRange);

    if (maxPixelRadius <= 0) return;

    // 创建扇形路径
    QPainterPath sectorPath;
    
    // 计算起始和结束角度（Qt坐标系）
    double startAngleQt = 90.0 - m_maxAngle;
    double endAngleQt = 90.0 - m_minAngle;
    double spanAngle = endAngleQt - startAngleQt;
    
    // 外圆弧
    QRectF outerRect(-maxPixelRadius, -maxPixelRadius, 2*maxPixelRadius, 2*maxPixelRadius);
    sectorPath.moveTo(0, 0);
    sectorPath.arcTo(outerRect, startAngleQt, spanAngle);
    sectorPath.lineTo(0, 0);
    
    // 如果有内圆，需要减去内圆部分
    if (minPixelRadius > 0) {
        QPainterPath innerPath;
        QRectF innerRect(-minPixelRadius, -minPixelRadius, 2*minPixelRadius, 2*minPixelRadius);
        innerPath.moveTo(0, 0);
        innerPath.arcTo(innerRect, startAngleQt, spanAngle);
        innerPath.lineTo(0, 0);
        
        sectorPath = sectorPath.subtracted(innerPath);
    }
    
    // 使用更明显的灰色半透明填充
    QBrush sectorBrush(QColor(80, 80, 80, 40)); // 更亮的灰色，稍高的透明度
    painter->setBrush(sectorBrush);
    painter->setPen(Qt::NoPen);
    painter->drawPath(sectorPath);
}

bool SectorPolarGrid::isAngleInSector(float angle) const
{
    // 简单的角度范围检查
    return (angle >= m_minAngle && angle <= m_maxAngle);
}
