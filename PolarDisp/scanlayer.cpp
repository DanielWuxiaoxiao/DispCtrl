/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:22
 * @Description: 
 */
#include "scanlayer.h"
#include "polaraxis.h"
#include "../Basic/log.h"
#include <QPainter>
#include <QtMath>
#include <QGraphicsScene>
#include <QDebug>

ScanLayer::ScanLayer(PolarAxis* axis, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_axis(axis),
      m_angle(0), m_fixedStart(0), m_fixedEnd(360), // 初始化为0度，全圆范围
      m_direction(1), m_mode(Loop) // 初始方向和模式
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ScanLayer::advanceSweep);
    // 不要立即启动定时器，等待实时数据或外部控制
    // m_timer->start(50); // 注释掉自动启动，避免初始化竞争
}

QRectF ScanLayer::boundingRect() const {
    double r = m_axis->rangeToPixel(m_axis->maxRange());
    return QRectF(-r - 50, -r - 50, (r + 50) * 2, (r + 50) * 2);
}

void ScanLayer::paint(QPainter* painter,
                      const QStyleOptionGraphicsItem*,
                      QWidget*)
{
    // 【关键修复】在paint()开始时立即保存m_angle的快照
    // 避免在绘制过程中m_angle被onBITReport()修改导致余晖和扫描线不一致
    double currentAngle = m_angle;

    static int paintCount = 0;
    if (paintCount % 50 == 0) {  // 每50次打印一次，避免刷屏
        // qDebug() << "[ScanLayer::paint] Called, currentAngle=" << currentAngle
        //          << "m_useRealTimeAngle=" << m_useRealTimeAngle;
    }
    paintCount++;

    double r = m_axis->rangeToPixel(m_axis->maxRange());
    painter->setRenderHint(QPainter::Antialiasing);

    // === 1. 绘制固定扫描区域（可选的背景） ===
    QPainterPath scanAreaPath;
    scanAreaPath.moveTo(0, 0);
    // Qt坐标系：0度在3点钟方向，逆时针为正角度
    // 极坐标系：0度在12点钟方向，顺时针为正
    // 转换：Qt角度 = 90 - 极坐标角度
    double qtStartAngle = 90 - m_fixedStart;
    double qtEndAngle = 90 - m_fixedEnd;

    // 计算扫描范围的跨度角度
    double spanAngle;

    // 判断是否跨越0°（极坐标系中的0°，即正北方向）
    bool crossZero = (m_fixedStart > m_fixedEnd);

    if (crossZero) {
        // 跨越0°的情况（例如 315° 到 45°）
        // 实际扫描范围是：315° -> 360°/0° -> 45°
        // 总跨度 = (360 - 315) + 45 = 90°
        spanAngle = (360 - m_fixedStart) + m_fixedEnd;
    } else {
        // 正常情况（例如 -45° 到 45° 或 0° 到 90°）
        spanAngle = m_fixedEnd - m_fixedStart;
    }

    // Qt的arcTo使用逆时针为正，我们需要顺时针绘制（从start到end）
    // 所以使用负的spanAngle
    scanAreaPath.arcTo(-r, -r, 2*r, 2*r, qtStartAngle, -spanAngle);
    scanAreaPath.closeSubpath();

    // 绘制浅色背景表示扫描区域
    QColor areaColor(251, 159, 147, 30); // 很淡的灰色背景
    painter->fillPath(scanAreaPath, areaColor);

    // === 2. 绘制扫描余晖效果 ===
    // 余晖从当前扫描线位置向后延伸
    double afterglowAngle = 60.0; // 余晖延伸的角度范围
    double qtCurrentAngle = 90 - currentAngle; // 使用快照值，转换到Qt坐标系

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

    // 设置裁剪区域为扫描范围（仅用于余晖绘制）
    painter->save();
    painter->setClipPath(scanAreaPath);

    // 绘制余晖（在裁剪区域内）
    painter->setBrush(gradient);
    painter->setPen(Qt::NoPen);
    painter->drawPath(afterglowPath);

    painter->restore();  // 恢复裁剪，让扫描线可以在任意角度显示

    // === 3. 绘制扫描线（不受裁剪区域限制，可以超出扫描范围） ===
    // 使用快照值currentAngle，确保与余辉一致
    double rad = qDegreesToRadians(currentAngle);
    double x = r * qSin(rad);
    double y = -r * qCos(rad);

    QPen linePen(QColor(0, 255, 0, 255)); // 亮绿色扫描线
    linePen.setWidth(4);
    painter->setPen(linePen);
    painter->drawLine(QPointF(0, 0), QPointF(x, y));

    // === 4. 可选：绘制扫描区域边界线 ===
    painter->setPen(QPen(QColor(251, 159, 147, 100), 3));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(scanAreaPath);
}

void ScanLayer::advanceSweep() {
    // 如果使用实时角度更新，则不执行自动扫描
    if (m_useRealTimeAngle) {
        return;
    }

    // 判断是否跨越0°
    bool crossZero = (m_fixedStart > m_fixedEnd);

    // 根据扫描模式更新角度
    if (m_mode == Loop) {
        m_angle += m_direction * 2; // 每次转2度

        // 规范化角度到 0-360°
        if (m_angle >= 360) m_angle -= 360;
        if (m_angle < 0) m_angle += 360;

        if (crossZero) {
            // 跨越0°的情况（如 315° 到 45°）
            if (m_direction > 0) {
                // 正向扫描：315 -> 360 -> 0 -> 45
                // 当超过 fixedEnd 且不在 fixedStart 之后时，跳回起点
                if (m_angle > m_fixedEnd && m_angle < m_fixedStart) {
                    m_angle = m_fixedStart;
                }
            } else {
                // 反向扫描：45 -> 0 -> 360 -> 315
                if (m_angle < m_fixedStart && m_angle > m_fixedEnd) {
                    m_angle = m_fixedEnd;
                }
            }
        } else {
            // 正常范围（不跨越0°）
            if (m_direction > 0 && m_angle >= m_fixedEnd) {
                m_angle = m_fixedStart;
            } else if (m_direction < 0 && m_angle <= m_fixedStart) {
                m_angle = m_fixedEnd;
            }
        }
    } else if (m_mode == PingPong) {
        m_angle += m_direction * 2; // 每次转2度

        // 规范化角度到 0-360°
        if (m_angle >= 360) m_angle -= 360;
        if (m_angle < 0) m_angle += 360;

        if (crossZero) {
            // 跨越0°的往复模式
            if (m_direction > 0) {
                // 正向：检查是否超过 fixedEnd
                if (m_angle > m_fixedEnd && m_angle < m_fixedStart) {
                    m_angle = m_fixedEnd;
                    m_direction = -1;
                }
            } else {
                // 反向：检查是否低于 fixedStart
                if (m_angle < m_fixedStart && m_angle > m_fixedEnd) {
                    m_angle = m_fixedStart;
                    m_direction = 1;
                }
            }
        } else {
            // 正常范围的往复模式
            if (m_angle >= m_fixedEnd) {
                m_angle = m_fixedEnd;
                m_direction = -1;
            } else if (m_angle <= m_fixedStart) {
                m_angle = m_fixedStart;
                m_direction = 1;
            }
        }
    }

    update(); // 触发重绘
}

void ScanLayer::setHeadingAngle(double deg) {
    // 停止内部扫掠，直接按照外部航向角指向
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }

    // 禁用实时角度，改用外部设置
    m_useRealTimeAngle = false;
    m_angle = deg;

    LOG_DEBUG(QString("[ScanLayer::setHeadingAngle] Set to %1 degrees, timer stopped, m_useRealTimeAngle=false")
                  .arg(deg));

    update();
}

void ScanLayer::onBITReport(BITReport report) {
    // 使用实时扫描角度更新波束指向
    // scanAngle是0.01度量化，转换为度
    double scanDegree = report.scanAngle * 0.01;

    // qDebug() << "[ScanLayer] onBITReport called!";
    // qDebug() << "  - Raw scanAngle:" << report.scanAngle;
    // qDebug() << "  - Converted scanDegree:" << scanDegree;
    // qDebug() << "  - Current m_angle:" << m_angle;
    // qDebug() << "  - m_useRealTimeAngle before:" << m_useRealTimeAngle;

    // 【关键修复】首先停止定时器，避免与实时数据竞争
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
        //qDebug() << "  - Timer stopped to avoid race condition";
    }

    // 启用实时角度模式（停止自动扫描）
    m_useRealTimeAngle = true;

    // 更新角度（可以超出预设范围）
    m_angle = scanDegree;

    // qDebug() << "  - New m_angle:" << m_angle;
    // qDebug() << "  - m_useRealTimeAngle after:" << m_useRealTimeAngle;

    // 触发重绘
    update();

    //qDebug() << "[ScanLayer] update() called, expecting repaint...";
}

void ScanLayer::setSweepSpeed(int msPerStep) {
    if (msPerStep > 0) {
        m_timer->setInterval(msPerStep);
    }
}

void ScanLayer::setSweepRange(double startDeg, double endDeg) {
    // 输入语义：从 startDeg 顺时针旋转到 endDeg
    // 内部使用 0-360° 范围存储
    // m_fixedStart > m_fixedEnd 表示跨越0°（正北）的扫描范围
    // 特殊：m_fixedStart=0, m_fixedEnd=360 表示全圆

    LOG_DEBUG(QString("[ScanLayer::setSweepRange] input(%1,%2)").arg(startDeg).arg(endDeg));

    // 将角度转换为 [0, 360] 范围，保留 360 本身不归零
    auto normalize = [](double deg) -> double {
        while (deg < 0) deg += 360;
        while (deg > 360) deg -= 360;   // > 360 而非 >= 360，保留360
        return deg;
    };

    double normStart = normalize(startDeg);
    double normEnd   = normalize(endDeg);

    // 【全圆检测】跨度约360° 或输入为 0~360°
    double span = normEnd - normStart;
    if (span < 0) span += 360;
    // 特殊情况：normStart约0, normEnd约360 则 span=360
    if (qAbs(normStart) < 0.01 && qAbs(normEnd - 360.0) < 0.01) {
        span = 360.0;
    }
    if (qAbs(span) < 0.01 && qAbs(normEnd - 360.0) < 0.01) {
        span = 360.0;
    }
    if (qAbs(span - 360.0) < 0.01) {
        m_fixedStart = 0;
        m_fixedEnd   = 360;
        LOG_DEBUG("[ScanLayer::setSweepRange] Full-circle -> stored(0, 360)");
        if (!isAngleInRange(m_angle)) m_angle = 0;
        update();
        return;
    }

    // 全圆已处理，非全圆时将 360 视为 0
    if (qAbs(normEnd - 360.0) < 0.01) normEnd = 0;
    if (qAbs(normStart - 360.0) < 0.01) normStart = 0;

    // 判断是否跨越0°（正北）：
    // normStart > normEnd 意味着顺时针要经过正北
    bool crossZero = (normStart > normEnd) ||
                     (startDeg < 0 && endDeg > 0);

    if (crossZero) {
        m_fixedStart = normStart;
        m_fixedEnd = normEnd;
        LOG_DEBUG(QString("[ScanLayer::setSweepRange] Cross-zero -> stored(%1,%2)")
                  .arg(m_fixedStart)
                  .arg(m_fixedEnd));
    } else {
        // 正常情况：start < end，顺时针从start到end
        m_fixedStart = normStart;
        m_fixedEnd = normEnd;
        LOG_DEBUG(QString("[ScanLayer::setSweepRange] Normal -> stored(%1,%2)")
                  .arg(m_fixedStart)
                  .arg(m_fixedEnd));
    }

    // 确保当前角度在范围内
    if (!isAngleInRange(m_angle)) {
        m_angle = m_fixedStart;
        if (m_mode == PingPong) m_direction = 1;
    }

    update();
}

/**
 * @brief 判断角度是否在扫描范围内
 * @param angle 待检查的角度（0-360°）
 * @return true 如果在范围内
 */
bool ScanLayer::isAngleInRange(double angle) const {
    // 将角度规范化到 0-360° 范围
    while (angle < 0) angle += 360;
    while (angle >= 360) angle -= 360;

    if (m_fixedStart <= m_fixedEnd) {
        // 正常范围（不跨越0°）
        return angle >= m_fixedStart && angle <= m_fixedEnd;
    } else {
        // 跨越0°的范围（如 315° 到 45°）
        return angle >= m_fixedStart || angle <= m_fixedEnd;
    }
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
