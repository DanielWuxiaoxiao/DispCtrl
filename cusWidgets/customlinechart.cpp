/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-30 11:45:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:22
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-01-28
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-01-28
 * @Description: 自定义线性图表实现
 */

#include "customlinechart.h"
#include "../Basic/DispBasci.h"
#include <QResizeEvent>
#include <QDebug>
#include <cmath>

namespace {

int decimalsForInterval(double interval)
{
    if (interval <= 0) {
        return 0;
    }

    int decimals = 0;
    double scaled = interval;
    while (decimals < 3 && std::fabs(std::round(scaled) - scaled) > 0.001) {
        scaled *= 10.0;
        ++decimals;
    }
    return decimals;
}

QString formatAxisValue(double value, double interval, const QString& unit)
{
    QString text = QString::number(value, 'f', decimalsForInterval(interval));
    if (text.contains('.')) {
        while (text.endsWith('0')) {
            text.chop(1);
        }
        if (text.endsWith('.')) {
            text.chop(1);
        }
    }

    if (!unit.isEmpty()) {
        text += unit;
    }
    return text;
}

}

CustomLineChart::CustomLineChart(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
{
    initScene();

    // 设置视图属性
    setRenderHint(QPainter::Antialiasing, true);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 设置背景色
    setBackgroundBrush(QBrush(m_bgColor));

    // 默认X轴配置（方位角0-360°）
    m_xAxisConfig.minValue = 0;
    m_xAxisConfig.maxValue = 360;
    m_xAxisConfig.majorTickInterval = 45;
    m_xAxisConfig.minorTickInterval = 15;
    m_xAxisConfig.label = "方位角";
    m_xAxisConfig.unit = "°";

    // 默认Y轴配置（距离0-5000m）
    m_yAxisConfig.minValue = 0;
    m_yAxisConfig.maxValue = 5000;
    m_yAxisConfig.majorTickInterval = 1000;
    m_yAxisConfig.minorTickInterval = 500;
    m_yAxisConfig.label = "距离";
    m_yAxisConfig.unit = "m";

    rebuild();
}

CustomLineChart::~CustomLineChart()
{
    clearPoints();
    clearGrid();
    clearAxisLabels();
}

void CustomLineChart::initScene()
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
}

void CustomLineChart::setXAxisConfig(const ChartAxisConfig& config)
{
    m_xAxisConfig = config;
    rebuild();
}

void CustomLineChart::setYAxisConfig(const ChartAxisConfig& config)
{
    m_yAxisConfig = config;
    rebuild();
}

QGraphicsEllipseItem* CustomLineChart::addPoint(double x, double y, const QColor& color, double size)
{
    QPointF scenePos = dataToScene(x, y);

    double halfSize = size / 2.0;
    QGraphicsEllipseItem* point = new QGraphicsEllipseItem(
        scenePos.x() - halfSize,
        scenePos.y() - halfSize,
        size,
        size
    );

    point->setBrush(QBrush(color));
    point->setPen(QPen(color));

    m_scene->addItem(point);
    m_dataPoints.append(point);

    return point;
}

void CustomLineChart::clearPoints()
{
    for (auto* point : m_dataPoints) {
        if (point) {
            m_scene->removeItem(point);
            delete point;
        }
    }
    m_dataPoints.clear();
}

void CustomLineChart::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    for (auto* line : m_gridLines) {
        if (line) {
            line->setVisible(visible);
        }
    }
}

void CustomLineChart::setAxisVisible(bool visible)
{
    m_axisVisible = visible;
    for (auto* label : m_axisLabels) {
        if (label) {
            label->setVisible(visible);
        }
    }
}

void CustomLineChart::drawGrid()
{
    clearGrid();

    if (!m_gridVisible) {
        return;
    }

    double width = this->width() - m_leftMargin - m_rightMargin;
    double height = this->height() - m_topMargin - m_bottomMargin;

    QPen majorPen(m_gridMajorColor, 1);
    QPen minorPen(m_gridMinorColor, 1);

    // 绘制垂直网格线（X轴）
    if (m_xAxisConfig.majorTickInterval > 0) {
        double xRange = m_xAxisConfig.maxValue - m_xAxisConfig.minValue;

        // 主网格线
        for (double x = m_xAxisConfig.minValue;
             x <= m_xAxisConfig.maxValue;
             x += m_xAxisConfig.majorTickInterval) {
            double ratio = (x - m_xAxisConfig.minValue) / xRange;
            double sceneX = m_leftMargin + ratio * width;

            QGraphicsLineItem* line = new QGraphicsLineItem(
                sceneX, m_topMargin,
                sceneX, m_topMargin + height
            );
            line->setPen(majorPen);
            m_scene->addItem(line);
            m_gridLines.append(line);
        }

        // 次网格线
        if (m_xAxisConfig.minorTickInterval > 0) {
            for (double x = m_xAxisConfig.minValue;
                 x <= m_xAxisConfig.maxValue;
                 x += m_xAxisConfig.minorTickInterval) {
                // 跳过主网格线位置
                double remainder = fmod(x - m_xAxisConfig.minValue, m_xAxisConfig.majorTickInterval);
                if (fabs(remainder) < 0.01) continue;

                double ratio = (x - m_xAxisConfig.minValue) / xRange;
                double sceneX = m_leftMargin + ratio * width;

                QGraphicsLineItem* line = new QGraphicsLineItem(
                    sceneX, m_topMargin,
                    sceneX, m_topMargin + height
                );
                line->setPen(minorPen);
                m_scene->addItem(line);
                m_gridLines.append(line);
            }
        }
    }

    // 绘制水平网格线（Y轴）
    if (m_yAxisConfig.majorTickInterval > 0) {
        double yRange = m_yAxisConfig.maxValue - m_yAxisConfig.minValue;

        // 主网格线
        for (double y = m_yAxisConfig.minValue;
             y <= m_yAxisConfig.maxValue;
             y += m_yAxisConfig.majorTickInterval) {
            double ratio = (y - m_yAxisConfig.minValue) / yRange;
            double sceneY = m_topMargin + height - ratio * height; // Y轴向上

            QGraphicsLineItem* line = new QGraphicsLineItem(
                m_leftMargin, sceneY,
                m_leftMargin + width, sceneY
            );
            line->setPen(majorPen);
            m_scene->addItem(line);
            m_gridLines.append(line);
        }

        // 次网格线
        if (m_yAxisConfig.minorTickInterval > 0) {
            for (double y = m_yAxisConfig.minValue;
                 y <= m_yAxisConfig.maxValue;
                 y += m_yAxisConfig.minorTickInterval) {
                // 跳过主网格线位置
                double remainder = fmod(y - m_yAxisConfig.minValue, m_yAxisConfig.majorTickInterval);
                if (fabs(remainder) < 0.01) continue;

                double ratio = (y - m_yAxisConfig.minValue) / yRange;
                double sceneY = m_topMargin + height - ratio * height;

                QGraphicsLineItem* line = new QGraphicsLineItem(
                    m_leftMargin, sceneY,
                    m_leftMargin + width, sceneY
                );
                line->setPen(minorPen);
                m_scene->addItem(line);
                m_gridLines.append(line);
            }
        }
    }
}

void CustomLineChart::drawAxes()
{
    clearAxisLabels();

    if (!m_axisVisible) {
        return;
    }

    double width = this->width() - m_leftMargin - m_rightMargin;
    double height = this->height() - m_topMargin - m_bottomMargin;

    QPen axisPen(m_axisColor, 2);
    QFont labelFont(QStringLiteral("微软雅黑"));
    labelFont.setPixelSize(ScaleHelper::uiScaled(12));

    // 绘制X轴刻度标签
    if (m_xAxisConfig.majorTickInterval > 0) {
        double xRange = m_xAxisConfig.maxValue - m_xAxisConfig.minValue;

        for (double x = m_xAxisConfig.minValue;
             x <= m_xAxisConfig.maxValue;
             x += m_xAxisConfig.majorTickInterval) {
            double ratio = (x - m_xAxisConfig.minValue) / xRange;
            double sceneX = m_leftMargin + ratio * width;

            QString labelText = formatAxisValue(x, m_xAxisConfig.majorTickInterval, m_xAxisConfig.unit);

            QGraphicsTextItem* label = new QGraphicsTextItem(labelText);
            label->setFont(labelFont);
            label->setDefaultTextColor(m_textColor);

            // 居中对齐
            QRectF textRect = label->boundingRect();
            label->setPos(sceneX - textRect.width() / 2,
                         m_topMargin + height + 5);

            m_scene->addItem(label);
            m_axisLabels.append(label);
        }
    }

    // 绘制Y轴刻度标签
    if (m_yAxisConfig.majorTickInterval > 0) {
        double yRange = m_yAxisConfig.maxValue - m_yAxisConfig.minValue;

        for (double y = m_yAxisConfig.minValue;
             y <= m_yAxisConfig.maxValue;
             y += m_yAxisConfig.majorTickInterval) {
            double ratio = (y - m_yAxisConfig.minValue) / yRange;
            double sceneY = m_topMargin + height - ratio * height;

            QString labelText = formatAxisValue(y, m_yAxisConfig.majorTickInterval, m_yAxisConfig.unit);

            QGraphicsTextItem* label = new QGraphicsTextItem(labelText);
            label->setFont(labelFont);
            label->setDefaultTextColor(m_textColor);

            // 右对齐
            QRectF textRect = label->boundingRect();
            label->setPos(m_leftMargin - textRect.width() - 5,
                         sceneY - textRect.height() / 2);

            m_scene->addItem(label);
            m_axisLabels.append(label);
        }
    }

    // 绘制X轴标题
    if (!m_xAxisConfig.label.isEmpty()) {
        QGraphicsTextItem* xTitle = new QGraphicsTextItem(m_xAxisConfig.label);
        QFont titleFont(QStringLiteral("微软雅黑"));
        titleFont.setBold(true);
        titleFont.setPixelSize(ScaleHelper::uiScaled(14));
        xTitle->setFont(titleFont);
        xTitle->setDefaultTextColor(m_textColor);

        QRectF titleRect = xTitle->boundingRect();
        xTitle->setPos(m_leftMargin + width / 2 - titleRect.width() / 2,
                      m_topMargin + height + 25);

        m_scene->addItem(xTitle);
        m_axisLabels.append(xTitle);
    }

    // 绘制Y轴标题（垂直）
    if (!m_yAxisConfig.label.isEmpty()) {
        QGraphicsTextItem* yTitle = new QGraphicsTextItem(m_yAxisConfig.label);
        QFont titleFont(QStringLiteral("微软雅黑"));
        titleFont.setBold(true);
        titleFont.setPixelSize(ScaleHelper::uiScaled(14));
        yTitle->setFont(titleFont);
        yTitle->setDefaultTextColor(m_textColor);

        QRectF titleRect = yTitle->boundingRect();
        yTitle->setPos(5, m_topMargin + height / 2 + titleRect.width() / 2);
        yTitle->setRotation(-90);

        m_scene->addItem(yTitle);
        m_axisLabels.append(yTitle);
    }
}

QPointF CustomLineChart::dataToScene(double dataX, double dataY) const
{
    double width = this->width() - m_leftMargin - m_rightMargin;
    double height = this->height() - m_topMargin - m_bottomMargin;

    double xRange = m_xAxisConfig.maxValue - m_xAxisConfig.minValue;
    double yRange = m_yAxisConfig.maxValue - m_yAxisConfig.minValue;

    double xRatio = (dataX - m_xAxisConfig.minValue) / xRange;
    double yRatio = (dataY - m_yAxisConfig.minValue) / yRange;

    double sceneX = m_leftMargin + xRatio * width;
    double sceneY = m_topMargin + height - yRatio * height; // Y轴向上

    return QPointF(sceneX, sceneY);
}

QPointF CustomLineChart::sceneToData(double sceneX, double sceneY) const
{
    double width = this->width() - m_leftMargin - m_rightMargin;
    double height = this->height() - m_topMargin - m_bottomMargin;

    double xRange = m_xAxisConfig.maxValue - m_xAxisConfig.minValue;
    double yRange = m_yAxisConfig.maxValue - m_yAxisConfig.minValue;

    double xRatio = (sceneX - m_leftMargin) / width;
    double yRatio = (m_topMargin + height - sceneY) / height;

    double dataX = m_xAxisConfig.minValue + xRatio * xRange;
    double dataY = m_yAxisConfig.minValue + yRatio * yRange;

    return QPointF(dataX, dataY);
}

void CustomLineChart::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);

    // 更新场景大小
    m_scene->setSceneRect(0, 0, event->size().width(), event->size().height());

    // 重建图表
    rebuild();
}

void CustomLineChart::clearGrid()
{
    for (auto* line : m_gridLines) {
        if (line) {
            m_scene->removeItem(line);
            delete line;
        }
    }
    m_gridLines.clear();
}

void CustomLineChart::clearAxisLabels()
{
    for (auto* label : m_axisLabels) {
        if (label) {
            m_scene->removeItem(label);
            delete label;
        }
    }
    m_axisLabels.clear();
}

void CustomLineChart::rebuild()
{
    // 保存现有数据点
    QVector<QPointF> dataPositions;
    QVector<QColor> dataColors;
    QVector<double> dataSizes;

    for (auto* point : m_dataPoints) {
        if (point) {
            QRectF rect = point->rect();
            QPointF sceneCenter(rect.center().x(), rect.center().y());
            QPointF dataPos = sceneToData(sceneCenter.x(), sceneCenter.y());

            dataPositions.append(dataPos);
            dataColors.append(point->brush().color());
            dataSizes.append(rect.width());
        }
    }

    // 清除旧内容
    clearPoints();
    clearGrid();
    clearAxisLabels();

    // 重绘网格和坐标轴
    drawGrid();
    drawAxes();

    // 恢复数据点
    for (int i = 0; i < dataPositions.size(); ++i) {
        addPoint(dataPositions[i].x(), dataPositions[i].y(),
                dataColors[i], dataSizes[i]);
    }
}

// ---------- 颜色主题 setters ----------

void CustomLineChart::setChartBgColor(const QColor& color)
{
    m_bgColor = color;
    setBackgroundBrush(QBrush(m_bgColor));
}

void CustomLineChart::setChartGridMajorColor(const QColor& color)
{
    m_gridMajorColor = color;
    rebuild();
}

void CustomLineChart::setChartGridMinorColor(const QColor& color)
{
    m_gridMinorColor = color;
    rebuild();
}

void CustomLineChart::setChartAxisColor(const QColor& color)
{
    m_axisColor = color;
    rebuild();
}

void CustomLineChart::setChartTextColor(const QColor& color)
{
    m_textColor = color;
    rebuild();
}
