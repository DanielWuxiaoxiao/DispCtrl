/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-25 16:20:17
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-30 15:27:10
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

    // ===== 移动船只（多样化SNR，覆盖全色阶） =====

    // 大型货轮 - 近距离，强回波（红/橙色）
    m_targets.append({45.0, 800, 3.0, 225.0, 5.0, 10, false, 38.0f});

    // 集装箱船 - 中距离，较强回波（黄/橙色）
    m_targets.append({120.0, 1800, 5.0, 310.0, 6.0, 12, false, 30.0f});

    // 渔船群 - 远距离，中等回波（绿色）
    m_targets.append({200.0, 3200, 2.0, 80.0, 3.5, 6, false, 20.0f});
    m_targets.append({210.0, 3000, 1.5, 75.0, 3.0, 5, false, 18.0f});
    m_targets.append({195.0, 3400, 2.5, 90.0, 2.5, 5, false, 16.0f});

    // 快艇 - 近距离，强回波（红色）
    m_targets.append({310.0, 600, 12.0, 150.0, 3.0, 6, false, 35.0f});

    // 油轮 - 远距离，中强回波（黄/绿色）
    m_targets.append({75.0, 4000, 4.0, 180.0, 7.0, 14, false, 24.0f});

    // 帆船 - 中距离，弱回波（青/蓝色）
    m_targets.append({270.0, 1500, 3.5, 45.0, 2.0, 4, false, 12.0f});

    // 小型游艇 - 近距离，弱回波（深蓝色）
    m_targets.append({160.0, 500, 6.0, 270.0, 1.5, 3, false, 8.0f});

    // 拖轮 - 中距离，中等回波（绿/青过渡）
    m_targets.append({330.0, 2200, 4.0, 120.0, 3.5, 6, false, 22.0f});

    // ===== 海岸线（强回波，红/橙色大片区域） =====

    // 北侧海岸线（大段弧形回波，极强）
    m_targets.append({350.0, 3800, 0, 0, 35.0, 50, true, 40.0f});
    m_targets.append({15.0, 3500, 0, 0, 30.0, 45, true, 38.0f});
    m_targets.append({0.0, 3600, 0, 0, 20.0, 30, true, 36.0f});

    // 东南侧海岸线（中强，延伸面积大）
    m_targets.append({130.0, 4200, 0, 0, 40.0, 55, true, 35.0f});
    m_targets.append({155.0, 4500, 0, 0, 25.0, 35, true, 32.0f});
    m_targets.append({145.0, 4000, 0, 0, 15.0, 20, true, 28.0f});

    // 西南侧海岸线（近距、强回波 = 红色）
    m_targets.append({220.0, 2000, 0, 0, 25.0, 35, true, 42.0f});
    m_targets.append({235.0, 2300, 0, 0, 18.0, 25, true, 36.0f});

    // ===== 岛屿（中等弧度，颜色丰富） =====

    // 近岛（强回波，红/黄色）
    m_targets.append({240.0, 2200, 0, 0, 12.0, 18, true, 38.0f});

    // 远岛（中回波，绿/黄色）
    m_targets.append({85.0, 3600, 0, 0, 10.0, 15, true, 25.0f});

    // 小礁石（弱回波，青/蓝色）
    m_targets.append({290.0, 1800, 0, 0, 5.0, 8, true, 14.0f});

    // ===== 雨云/天气杂波（中等SNR，黄/绿色大面积模糊回波） =====
    m_targets.append({60.0, 2500, 0.5, 180.0, 30.0, 40, false, 18.0f});
    m_targets.append({50.0, 2800, 0.3, 200.0, 25.0, 35, false, 15.0f});
    m_targets.append({70.0, 2200, 0.4, 160.0, 20.0, 30, false, 20.0f});
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

    // 生成扫描线回波数据 (新管线)
    generateEchoLine(m_sweepAngle);
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

/**
 * @brief 生成一条完整的扫描线回波数据（给EchoRenderer用）
 * @details 将所有目标投影到当前方位角的距离单元数组中
 */
void RadarSimulator::generateEchoLine(double sweepAzimuth)
{
    MarineEchoLine line;
    // 方位角 → raw (0~4095)
    double normAzi = sweepAzimuth;
    while (normAzi < 0) normAzi += 360.0;
    while (normAzi >= 360.0) normAzi -= 360.0;
    line.azimuthRaw = static_cast<uint16_t>(normAzi / 360.0 * MARINE_AZI_STEPS);
    line.azimuthDeg = normAzi;
    line.style = 0x01;
    line.packetNum = 0;

    // 初始化距离单元（全零 = 噪底）
    line.amplitudes.resize(m_rangeCells);
    line.amplitudes.fill(0);

    // 添加少量噪声
    for (int r = 0; r < m_rangeCells; ++r) {
        line.amplitudes[r] = static_cast<uint8_t>(m_rng.bounded(12)); // 0~11 噪底
    }

    // 将各目标投影到距离单元
    double cellSpacing = m_rangeMeter / m_rangeCells;

    for (const auto& t : m_targets) {
        double halfArc = t.arcSpan / 2.0;
        double angleDiff = sweepAzimuth - t.azimuth;
        while (angleDiff > 180.0) angleDiff -= 360.0;
        while (angleDiff < -180.0) angleDiff += 360.0;

        if (qAbs(angleDiff) > halfArc)
            continue; // 不在扫描线覆盖范围内

        // 目标被扫到: 计算幅值(越接近中心越强)
        double angularFactor = 1.0 - qAbs(angleDiff) / halfArc;
        double rangeFactor = qMax(0.2, 1.0 - t.range / (m_rangeMeter * 1.2));
        double baseAmp = t.snr * rangeFactor * angularFactor;

        // 目标在距离上的展开(多个距离单元)
        double rangeSpread = t.arcSpan * 5.0; // 展宽(米)
        int centerCell = static_cast<int>(t.range / cellSpacing);
        int spreadCells = static_cast<int>(rangeSpread / cellSpacing) + 1;

        for (int dr = -spreadCells; dr <= spreadCells; ++dr) {
            int cell = centerCell + dr;
            if (cell < 0 || cell >= m_rangeCells) continue;

            double distFactor = 1.0 - qAbs(dr) / (spreadCells + 1.0);
            double amp = baseAmp * distFactor + m_rng.generateDouble() * 5.0;
            int ampInt = qBound(0, static_cast<int>(amp * 8.0), 255);

            // 取最大值(多目标叠加)
            if (ampInt > line.amplitudes[cell]) {
                line.amplitudes[cell] = static_cast<uint8_t>(ampInt);
            }
        }
    }

    emit echoLineGenerated(line);
}
