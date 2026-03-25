/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-23 11:52:58
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-25 16:20:18
 * @Description: 
 */
/**
 * @file RadarSimulator.cpp
 * @brief 船用雷达回波模拟器实现
 * @details 产生逼真的船用雷达PPI回波数据，包括移动船只、静态海岸线和岛屿
 */
#include "RadarSimulator.h"
#include <QtMath>
#include <QDebug>

RadarSimulator::RadarSimulator(QObject* parent)
    : QObject(parent), m_rng(QRandomGenerator::securelySeeded())
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &RadarSimulator::onScanTick);
    initTargets();
}

RadarSimulator::~RadarSimulator()
{
    stop();
}

void RadarSimulator::start()
{
    if (m_running) return;
    m_running = true;
    m_timer->start(static_cast<int>(m_tickInterval));
    qInfo() << "[RadarSimulator] Started: sweep speed" << m_sweepSpeed << "deg/s";
}

void RadarSimulator::stop()
{
    m_running = false;
    m_timer->stop();
}

/**
 * @brief 初始化模拟目标场景
 * @details 创建一组逼真的船用雷达场景：移动船只 + 海岸线 + 岛屿
 */
void RadarSimulator::initTargets()
{
    m_targets.clear();

    // ===== 移动船只（5~8艘） =====

    // 大型货轮 - 近距离，慢速
    m_targets.append({45.0, 800, 3.0, 225.0, 4.0, 8, false, 25.0f});

    // 集装箱船 - 中距离
    m_targets.append({120.0, 1800, 5.0, 310.0, 5.0, 10, false, 22.0f});

    // 渔船群 - 远距离，分散
    m_targets.append({200.0, 3200, 2.0, 80.0, 3.0, 5, false, 15.0f});
    m_targets.append({210.0, 3000, 1.5, 75.0, 2.5, 4, false, 14.0f});
    m_targets.append({195.0, 3400, 2.5, 90.0, 2.0, 4, false, 13.0f});

    // 快艇 - 近距离，快速
    m_targets.append({310.0, 600, 12.0, 150.0, 2.0, 4, false, 20.0f});

    // 油轮 - 远距离，慢速
    m_targets.append({75.0, 4000, 4.0, 180.0, 6.0, 12, false, 18.0f});

    // 帆船 - 中距离
    m_targets.append({270.0, 1500, 3.5, 45.0, 1.5, 3, false, 12.0f});

    // ===== 海岸线（大弧度静态目标） =====

    // 北侧海岸线（大段弧形回波）
    m_targets.append({350.0, 3800, 0, 0, 30.0, 40, true, 30.0f});
    m_targets.append({15.0, 3500, 0, 0, 25.0, 35, true, 28.0f});

    // 东南侧海岸线
    m_targets.append({130.0, 4200, 0, 0, 35.0, 45, true, 32.0f});
    m_targets.append({155.0, 4500, 0, 0, 20.0, 25, true, 26.0f});

    // ===== 岛屿（中等弧度静态目标） =====

    // 近岛
    m_targets.append({240.0, 2200, 0, 0, 10.0, 15, true, 35.0f});

    // 远岛
    m_targets.append({85.0, 3600, 0, 0, 8.0, 12, true, 20.0f});
}

/**
 * @brief 更新所有移动目标的位置
 */
void RadarSimulator::updateTargetPositions(double dt)
{
    for (auto& t : m_targets) {
        if (t.isStatic) continue;

        // 将航向转换为弧度
        double headingRad = qDegreesToRadians(t.heading);

        // 计算位移（极坐标增量）
        double dx = t.speed * dt * qSin(headingRad);
        double dy = t.speed * dt * qCos(headingRad);

        // 当前位置转直角坐标
        double aziRad = qDegreesToRadians(t.azimuth);
        double x = t.range * qSin(aziRad) + dx;
        double y = t.range * qCos(aziRad) + dy;

        // 转回极坐标
        t.range = qSqrt(x * x + y * y);
        t.azimuth = qRadiansToDegrees(qAtan2(x, y));
        if (t.azimuth < 0) t.azimuth += 360.0;

        // 如果移出检测范围，重新生成位置
        if (t.range > 5000 || t.range < 100) {
            t.azimuth = m_rng.bounded(360.0);
            t.range = 500 + m_rng.bounded(4000.0);
            t.heading = m_rng.bounded(360.0);
        }
    }
}

/**
 * @brief 定时器回调：推进扫描角并生成回波
 */
void RadarSimulator::onScanTick()
{
    double dt = m_tickInterval / 1000.0;

    // 推进扫描角
    double sweepStep = m_sweepSpeed * dt;
    m_sweepAngle += sweepStep;
    if (m_sweepAngle >= 360.0) m_sweepAngle -= 360.0;

    // 更新移动目标
    updateTargetPositions(dt);

    // 在当前扫描角附近生成回波
    generateEchoes(m_sweepAngle);
}

/**
 * @brief 在当前扫描角处为所有命中目标生成回波点
 */
void RadarSimulator::generateEchoes(double sweepAzimuth)
{
    for (const auto& t : m_targets) {
        // 判断当前扫描线是否扫到该目标
        double halfArc = t.arcSpan / 2.0;
        double angleDiff = sweepAzimuth - t.azimuth;

        // 规范化到 [-180, 180]
        while (angleDiff > 180.0) angleDiff -= 360.0;
        while (angleDiff < -180.0) angleDiff += 360.0;

        if (qAbs(angleDiff) > halfArc) continue;

        // 目标被扫到，生成多个回波点（距离+方位散布）
        int numEchoes = t.echoCount;

        // 根据距离近远调整回波强度
        double rangeFactor = qMax(0.3, 1.0 - t.range / 6000.0);

        for (int i = 0; i < numEchoes; ++i) {
            // 方位散布：在目标弧度范围内分布
            double aziOffset = (m_rng.generateDouble() - 0.5) * t.arcSpan;
            double echoAzi = t.azimuth + aziOffset;
            if (echoAzi < 0) echoAzi += 360.0;
            if (echoAzi >= 360.0) echoAzi -= 360.0;

            // 距离散布：小范围抖动 ± 目标大小相关
            double rangeSpread = t.arcSpan * 8.0; // 方位展宽越大，距离展宽也越大
            double echoRange = t.range + (m_rng.generateDouble() - 0.5) * rangeSpread;
            if (echoRange < 50) echoRange = 50;

            // SNR散布
            float echoSnr = static_cast<float>(t.snr * rangeFactor
                            + (m_rng.generateDouble() - 0.5) * 5.0);

            // 幅度：模拟雷达回波强度（用于颜色映射）
            float amp = static_cast<float>(echoSnr * rangeFactor * 0.8
                        + m_rng.generateDouble() * 3.0);

            emit simulatedDetection(makeEchoPoint(echoAzi, echoRange, echoSnr, amp));
        }
    }
}

/**
 * @brief 构造一个PointInfo检测点
 */
PointInfo RadarSimulator::makeEchoPoint(double azi, double range, float snr, float amp)
{
    PointInfo p;
    memset(&p, 0, sizeof(p));
    p.type = 1;  // Detection
    p.azimuth = static_cast<float>(azi);
    p.range = static_cast<float>(range);
    p.elevation = 0.0f;
    p.SNR = snr;
    p.amp = amp;
    p.speed = 0.0f;
    p.altitute = 0.0f;
    p.batch = 0;
    p.statMethod = 0;
    return p;
}
