#include "scanlayer.h"
#include "polaraxis.h"
#include <QPainter>
#include <QtMath>
#include <QGraphicsScene>
#include <QDebug>

ScanLayer::ScanLayer(PolarAxis* axis, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_axis(axis),
      m_angle(30), m_fixedStart(30), m_fixedEnd(150), // 初始角度和范围
      m_direction(1), m_mode(Loop) // 初始方向和模式
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ScanLayer::advanceSweep);
    m_timer->start(50); // 默认 50ms 更新一次
}

QRectF ScanLayer::boundingRect() const {
    double r = m_axis->rangeToPixel(m_axis->maxRange());
    return QRectF(-r - 50, -r - 50, (r + 50) * 2, (r + 50) * 2);
}

void ScanLayer::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem*,
                      QWidget*)
{
    double r = m_axis->rangeToPixel(m_axis->maxRange());
    painter->setRenderHint(QPainter::Antialiasing);

    // === 1. 绘制固定扫描区域（可选的背景） ===
    QPainterPath scanAreaPath;
    scanAreaPath.moveTo(0, 0);
    // Qt坐标系：0度在3点钟方向，顺时针为正
    // 极坐标系：0度在12点钟方向，顺时针为正
    // 转换：Qt角度 = 90 - 极坐标角度
    double qtStartAngle = 90 - m_fixedStart;
    double qtEndAngle = 90 - m_fixedEnd;
    double spanAngle = qtStartAngle - qtEndAngle; // 因为是从start到end顺时针

    scanAreaPath.arcTo(-r, -r, 2*r, 2*r, qtStartAngle, -spanAngle);
    scanAreaPath.closeSubpath();

    // 绘制浅色背景表示扫描区域
    QColor areaColor(251, 159, 147, 30); // 很淡的灰色背景
    painter->fillPath(scanAreaPath, areaColor);

    // === 2. 绘制扫描余晖效果 ===
    // 余晖从当前扫描线位置向后延伸
    double afterglowAngle = 60.0; // 余晖延伸的角度范围
    double qtCurrentAngle = 90 - m_angle; // 转换到Qt坐标系

    // 创建余晖扇形路径
    QPainterPath afterglowPath;
    afterglowPath.moveTo(0, 0);

    QConicalGradient gradient(0, 0, 0); // 中心点在原点

    if (m_direction > 0) {
        // 正向扫描，余晖在扫描线后面（逆时针方向）
        // 从当前角度向后（增大角度方向）延伸
        afterglowPath.arcTo(-r, -r, 2*r, 2*r, qtCurrentAngle, afterglowAngle);

        // 设置渐变：从扫描线位置开始向后渐变
        gradient.setAngle(qtCurrentAngle);
        gradient.setColorAt(0.0, QColor(0, 255, 0, 180));  // 扫描线位置最亮
        gradient.setColorAt(0.02, QColor(0, 255, 0, 150));
        gradient.setColorAt(0.05, QColor(0, 255, 0, 100));
        gradient.setColorAt(0.10, QColor(0, 255, 0, 60));
        gradient.setColorAt(0.15, QColor(0, 255, 0, 30));
        gradient.setColorAt(0.20, QColor(0, 255, 0, 10));
        gradient.setColorAt(0.25, QColor(0, 255, 0, 0));   // 完全透明
    } else {
        // 反向扫描，余晖在扫描线后面（顺时针方向）
        // 从当前角度向前（减小角度方向）延伸
        afterglowPath.arcTo(-r, -r, 2*r, 2*r, qtCurrentAngle - afterglowAngle, afterglowAngle);

        // 设置渐变：反向时需要调整渐变方向
        gradient.setAngle(qtCurrentAngle - afterglowAngle);
        // 反向渐变：从远端透明到扫描线位置最亮
        gradient.setColorAt(0.0, QColor(0, 255, 0, 0));    // 远端完全透明
        gradient.setColorAt(0.05, QColor(0, 255, 0, 10));
        gradient.setColorAt(0.10, QColor(0, 255, 0, 30));
        gradient.setColorAt(0.15, QColor(0, 255, 0, 60));
        gradient.setColorAt(0.18, QColor(0, 255, 0, 100));
        gradient.setColorAt(0.20, QColor(0, 255, 0, 150));
        gradient.setColorAt(0.22, QColor(0, 255, 0, 180)); // 扫描线位置最亮
    }

    afterglowPath.closeSubpath();

    // 设置裁剪区域为扫描范围
    painter->save();
    painter->setClipPath(scanAreaPath);

    // 绘制余晖
    painter->setBrush(gradient);
    painter->setPen(Qt::NoPen);
    painter->drawPath(afterglowPath);

    // === 3. 绘制扫描线 ===
    double rad = qDegreesToRadians(m_angle);
    double x = r * qSin(rad);
    double y = -r * qCos(rad);

    QPen linePen(QColor(0, 255, 0, 255)); // 亮绿色扫描线
    linePen.setWidth(4);
    painter->setPen(linePen);
    painter->drawLine(QPointF(0, 0), QPointF(x, y));

    painter->restore();

    // === 4. 可选：绘制扫描区域边界线 ===
    painter->setPen(QPen(QColor(251, 159, 147, 100), 3));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(scanAreaPath);
}

void ScanLayer::advanceSweep() {
    // 根据扫描模式更新角度
    if (m_mode == Loop) {
        m_angle += m_direction * 2; // 每次转2度

        // 循环模式：到达终点后跳回起点
        if (m_direction > 0 && m_angle >= m_fixedEnd) {
            m_angle = m_fixedStart;
        } else if (m_direction < 0 && m_angle <= m_fixedStart) {
            m_angle = m_fixedEnd;
        }
    } else if (m_mode == PingPong) {
        m_angle += m_direction * 2; // 每次转2度

        // 往复模式：到达边界后反向
        if (m_angle >= m_fixedEnd) {
            m_angle = m_fixedEnd;
            m_direction = -1; // 反向
        } else if (m_angle <= m_fixedStart) {
            m_angle = m_fixedStart;
            m_direction = 1; // 正向
        }
    }

    update(); // 触发重绘
}

void ScanLayer::setSweepSpeed(int msPerStep) {
    if (msPerStep > 0) {
        m_timer->setInterval(msPerStep);
    }
}

void ScanLayer::setSweepRange(double startDeg, double endDeg) {
    // 确保start < end
    if (startDeg > endDeg) {
        qSwap(startDeg, endDeg);
    }

    m_fixedStart = startDeg;
    m_fixedEnd = endDeg;

    // 确保当前角度在范围内
    if (m_angle < m_fixedStart) {
        m_angle = m_fixedStart;
        if (m_mode == PingPong) m_direction = 1;
    } else if (m_angle > m_fixedEnd) {
        m_angle = m_fixedEnd;
        if (m_mode == PingPong) m_direction = -1;
    }

    update();
}

void ScanLayer::setScanMode(ScanMode mode) {
    m_mode = mode;

    // 重置方向
    if (m_mode == Loop) {
        m_direction = 1; // 循环模式默认正向
    } else if (m_mode == PingPong) {
        // 往复模式根据当前位置决定初始方向
        if (m_angle >= m_fixedEnd) {
            m_direction = -1;
        } else {
            m_direction = 1;
        }
    }

    update();
}

ScanLayer::~ScanLayer() {
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
}
