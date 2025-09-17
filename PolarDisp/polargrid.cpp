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

PolarGrid::PolarGrid(QGraphicsScene* scene, PolarAxis* axis, QObject* parent)
    : QObject(parent), mScene(scene), mAxis(axis)
{
    connect(mAxis, &PolarAxis::rangeChanged, this, &PolarGrid::updateGrid);
    updateGrid();
}

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

    // === 中心雷达图标 ===
    if (!mRadarIcon) {
        QPixmap pix(":/resources/icon/array.png");
        pix = pix.scaled(40,40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        mRadarIcon = mScene->addPixmap(pix);
        mRadarIcon->setZValue(100); // 确保在最上层
        mRadarIcon->setOffset(-pix.width()/2, -pix.height()/2); // 居中
    }

    // === 1. 最外层大圆 ===
    QPen outerPen(QColor(0, 255, 136));
    outerPen.setWidth(2);
    QGraphicsEllipseItem* outerCircle =
            mScene->addEllipse(-radius, -radius, radius*2, radius*2, outerPen);
    mCircleItems.append(outerCircle);

    // === 2. 七层虚线圆 ===
    QPen dashPen(Qt::gray);
    dashPen.setStyle(Qt::DashLine);
    int ringCount = 5;
    for (int i=1; i<=ringCount; ++i) {
        double r = radius * i / ringCount;
        QGraphicsEllipseItem* ring =
                mScene->addEllipse(-r, -r, r*2, r*2, dashPen);
        mCircleItems.append(ring);
    }

    // === 3. 刻度线（外圈，5°一条），仅绘制在指定扇区内 ===
    for (int angle=0; angle<360; angle+=5) {
        double ang = angle;
        // normalize angles
        double as = m_angleStart;
        double ae = m_angleEnd;
        // handle wrap-around
        bool inSector;
        if (as <= ae) inSector = (ang >= as && ang <= ae);
        else inSector = (ang >= as || ang <= ae);
        if (!inSector) continue;
        double rad = qDegreesToRadians((double)angle);
        double x1, y1, x2, y2;
        int len;
        QPen tickPen(QColor(0, 255, 136));

        if (angle % 10 == 0) {
            len = 15;
            tickPen.setWidth(2);
        } else {
            len = 8;
            tickPen.setWidth(1);
        }

        x1 = qSin(rad) * (radius - len);
        y1 = -qCos(rad) * (radius - len);
        x2 = qSin(rad) * radius;
        y2 = -qCos(rad) * radius;

        QGraphicsLineItem* tick = mScene->addLine(x1, y1, x2, y2, tickPen);
        mTickItems.append(tick);

        // === 4. 刻度值（每 10°） ===
        if (angle % 10 == 0) {
            double tx = qSin(rad) * (radius + 20);
            double ty = -qCos(rad) * (radius + 20);
            QGraphicsSimpleTextItem* text =
                    mScene->addSimpleText(QString::number(angle));
            text->setBrush(QColor(0, 255, 136));
            text->setPos(tx - text->boundingRect().width()/2,
                         ty - text->boundingRect().height()/2);
            mTextItems.append(text);
        }
    }

    // === 5. 四条分割直线 ===
    QPen crossPen(QColor(0, 255, 136, 128));
    crossPen.setStyle(Qt::DashLine);
    for (int angle=0; angle<360; angle+=90) {
        // only draw major cross lines if within sector
        double ang = angle;
        double as = m_angleStart;
        double ae = m_angleEnd;
        bool inSector;
        if (as <= ae) inSector = (ang >= as && ang <= ae);
        else inSector = (ang >= as || ang <= ae);
        if (!inSector) continue;
        double rad = qDegreesToRadians((double)angle);
        double x = qSin(rad) * radius;
        double y = -qCos(rad) * radius;
        QGraphicsLineItem* line =
                mScene->addLine(center.x(), center.y(), x, y, crossPen);
        mTickItems.append(line);
    }

    // === 6. 右侧距离标注 ===
    double maxRange = mAxis->maxRange();
    int rangeStep = maxRange / ringCount;
    for (int i=1; i<=ringCount; ++i) {
        double r = radius * i / ringCount;
        QString label;
        if(i == 1)
            label  = QString::number(i * rangeStep / 1000.0, 'f', 1) + " km";
        else
            label = QString::number(i * rangeStep / 1000.0, 'f', 1);
        QGraphicsSimpleTextItem* txt =
                mScene->addSimpleText(label);
        txt->setBrush(Qt::white);
        txt->setPos(r-25, -txt->boundingRect().height()/2);
        mTextItems.append(txt);
    }
}

void PolarGrid::setAngleRange(double startDeg, double endDeg)
{
    m_angleStart = startDeg;
    m_angleEnd = endDeg;
    updateGrid();
}



