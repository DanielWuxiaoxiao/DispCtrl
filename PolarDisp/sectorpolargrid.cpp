// sectorpolargrid.cpp
#include "sectorpolargrid.h"
#include "PolarDisp/polaraxis.h"
#include <QPainter>
#include <QtMath>
#include <QFont>

SectorPolarGrid::SectorPolarGrid(PolarAxis* axis, QGraphicsItem* parent)
    : QObject(nullptr), QGraphicsItem(parent), m_axis(axis)
{
    // 设置画笔样式
    m_rangePen = QPen(QColor(100, 100, 100), 1, Qt::SolidLine);
    m_anglePen = QPen(QColor(80, 80, 80), 1, Qt::SolidLine);
    m_textPen = QPen(QColor(200, 200, 200), 1);

    setZValue(-1); // 确保网格在背景层
}

void SectorPolarGrid::setSectorRange(float minAngle, float maxAngle)
{
    if (m_minAngle != minAngle || m_maxAngle != maxAngle) {
        m_minAngle = minAngle;
        m_maxAngle = maxAngle;
        update(); // 触发重绘
    }
}

QRectF SectorPolarGrid::boundingRect() const
{
    if (!m_axis) return QRectF();

    double maxRange = m_axis->maxRange();
    double pixelRadius = m_axis->rangeToPixel(maxRange);

    // 扩展边界以包含标签
    double margin = 30;
    return QRectF(-pixelRadius - margin, -pixelRadius - margin,
                  2 * (pixelRadius + margin), 2 * (pixelRadius + margin));
}

void SectorPolarGrid::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!m_axis) return;

    painter->setRenderHint(QPainter::Antialiasing, true);

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

    // 计算合适的距离间隔
    float rangeSpan = maxRange - minRange;
    int steps = 5; // 默认5个距离圈
    if (rangeSpan > 1000) steps = 10;
    else if (rangeSpan < 100) steps = 4;

    float rangeStep = rangeSpan / steps;

    // 绘制距离同心圆（只绘制扇形部分）
    for (int i = 1; i <= steps; ++i) {
        float range = minRange + i * rangeStep;
        double pixelRadius = m_axis->rangeToPixel(range);

        if (pixelRadius <= 0) continue;

        // 如果扇形跨度小于360度，绘制扇形弧线
        double angleSpan = qAbs(m_maxAngle - m_minAngle);
        if (angleSpan < 360.0) {
            // 绘制扇形弧
            QRectF rect(-pixelRadius, -pixelRadius, 2*pixelRadius, 2*pixelRadius);

            // Qt的角度系统：0度在3点钟方向，逆时针为正
            // 我们的角度系统：0度在12点钟方向，顺时针为正
            // 需要转换：qt_angle = 90 - our_angle
            double startAngleQt = 90.0 - m_maxAngle; // Qt角度
            double spanAngleQt = -(m_maxAngle - m_minAngle); // Qt中逆时针为正

            painter->drawArc(rect, startAngleQt * 16, spanAngleQt * 16);
        } else {
            // 绘制完整圆
            painter->drawEllipse(QPointF(0, 0), pixelRadius, pixelRadius);
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

    // 计算合适的角度间隔
    double angleSpan = qAbs(m_maxAngle - m_minAngle);
    double angleStep = 10.0; // 默认10度间隔
    if (angleSpan > 180) angleStep = 30.0;
    else if (angleSpan > 90) angleStep = 15.0;
    else if (angleSpan < 30) angleStep = 5.0;

    // 绘制角度射线
    for (double angle = m_minAngle; angle <= m_maxAngle; angle += angleStep) {
        if (!isAngleInSector(angle)) continue;

        double radians = qDegreesToRadians(angle);
        double cosA = qCos(radians);
        double sinA = qSin(radians);

        QPointF innerPoint(minPixelRadius * cosA, minPixelRadius * sinA);
        QPointF outerPoint(maxPixelRadius * cosA, maxPixelRadius * sinA);

        painter->drawLine(innerPoint, outerPoint);
    }

    // 绘制扇形边界线（如果不是完整圆）
    if (angleSpan < 360.0) {
        painter->setPen(QPen(m_anglePen.color(), 2)); // 边界线稍粗一些

        // 最小角度边界
        double minRadians = qDegreesToRadians(m_minAngle);
        QPointF minInner(minPixelRadius * qCos(minRadians), minPixelRadius * qSin(minRadians));
        QPointF minOuter(maxPixelRadius * qCos(minRadians), maxPixelRadius * qSin(minRadians));
        painter->drawLine(minInner, minOuter);

        // 最大角度边界
        double maxRadians = qDegreesToRadians(m_maxAngle);
        QPointF maxInner(minPixelRadius * qCos(maxRadians), minPixelRadius * qSin(maxRadians));
        QPointF maxOuter(maxPixelRadius * qCos(maxRadians), maxPixelRadius * qSin(maxRadians));
        painter->drawLine(maxInner, maxOuter);
    }
}

void SectorPolarGrid::drawLabels(QPainter* painter)
{
    painter->setPen(m_textPen);
    QFont font("Arial", 8);
    painter->setFont(font);

    float minRange = m_axis->minRange();
    float maxRange = m_axis->maxRange();

    // 距离标签
    float rangeSpan = maxRange - minRange;
    int steps = 5;
    if (rangeSpan > 1000) steps = 10;
    else if (rangeSpan < 100) steps = 4;

    float rangeStep = rangeSpan / steps;

    for (int i = 1; i <= steps; ++i) {
        float range = minRange + i * rangeStep;
        double pixelRadius = m_axis->rangeToPixel(range);

        if (pixelRadius <= 0) continue;

        // 在扇形中央位置放置距离标签
        double centerAngle = (m_minAngle + m_maxAngle) / 2.0;
        double radians = qDegreesToRadians(centerAngle);

        QPointF labelPos(pixelRadius * qCos(radians), pixelRadius * qSin(radians));

        // 调整标签位置避免与射线重叠
        labelPos += QPointF(10, -5);

        painter->drawText(labelPos, QString::number((int)range));
    }

    // 角度标签
    double angleSpan = qAbs(m_maxAngle - m_minAngle);
    double angleStep = 10.0;
    if (angleSpan > 180) angleStep = 30.0;
    else if (angleSpan > 90) angleStep = 15.0;
    else if (angleSpan < 30) angleStep = 5.0;

    double labelRadius = m_axis->rangeToPixel(maxRange) + 20; // 标签距离外圆一定距离

    for (double angle = m_minAngle; angle <= m_maxAngle; angle += angleStep) {
        if (!isAngleInSector(angle)) continue;

        double radians = qDegreesToRadians(angle);
        QPointF labelPos(labelRadius * qCos(radians), labelRadius * qSin(radians));

        // 调整文本对齐方式
        QRectF textRect = painter->fontMetrics().boundingRect(QString::number((int)angle) + "°");
        labelPos -= QPointF(textRect.width() / 2, textRect.height() / 2);

        painter->drawText(labelPos, QString::number((int)angle) + "°");
    }
}

void SectorPolarGrid::updateGrid()
{
    update(); // 触发重绘
}

bool SectorPolarGrid::isAngleInSector(float angle) const
{
    // 简单的角度范围检查
    return (angle >= m_minAngle && angle <= m_maxAngle);
}
