/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-25 16:20:17
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-30 15:27:10
 * @Description: 
 */
/**
 * @file RadarSimulator.h
 * @brief 船用雷达回波模拟器
 * @details 模拟生成带余辉效果的船用雷达检测点数据，用于无实际协议数据时的演示
 */
#ifndef RADARSIMULATOR_H
#define RADARSIMULATOR_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QRandomGenerator>
#include "Basic/Protocol.h"
#include "Basic/MarineProtocol.h"

/**
 * @brief 模拟雷达目标（船只/岛屿/海岸线）
 */
struct SimTarget {
    double azimuth;     ///< 当前方位角（度）
    double range;       ///< 当前距离（米）
    double speed;       ///< 移动速度（米/秒）
    double heading;     ///< 运动航向（度）
    double arcSpan;     ///< 回波方位展宽（度），模拟目标大小
    int echoCount;      ///< 每次扫描产生的回波点数
    bool isStatic;      ///< 是否静态目标（岛屿/海岸线）
    float snr;          ///< 信噪比
};

/**
 * @class RadarSimulator
 * @brief 雷达回波模拟器，产生逼真的船用雷达PPI显示数据
 *
 * 模拟功能：
 * - 多个移动船只目标（不同速度、航向）
 * - 静态海岸线/岛屿回波（大面积弧形）
 * - 点迹按扫描角度产生（扫描线扫过目标时才产生回波）
 * - 回波具有距离展宽和方位展宽特性
 */
class RadarSimulator : public QObject
{
    Q_OBJECT
public:
    explicit RadarSimulator(QObject* parent = nullptr);
    ~RadarSimulator();

    void start();
    void stop();
    bool isRunning() const { return m_running; }

    /// 设置模拟量程（米）
    void setRangeMeter(double meters) { m_rangeMeter = meters; }

signals:
    /**
     * @brief 产生一个模拟检测点 (兼容旧管线)
     * @param info 检测点信息（含方位、距离、SNR等）
     */
    void simulatedDetection(PointInfo info);

    /**
     * @brief 产生一条模拟扫描线回波 (新管线——EchoRenderer)
     * @param line 回波线数据
     */
    void echoLineGenerated(const MarineEchoLine& line);

private slots:
    void onScanTick();

private:
    void initTargets();
    void updateTargetPositions(double dt);
    void generateEchoes(double sweepAzimuth);
    void generateEchoLine(double sweepAzimuth);
    PointInfo makeEchoPoint(double azi, double range, float snr, float amp);

    QTimer* m_timer = nullptr;
    QVector<SimTarget> m_targets;
    double m_sweepAngle = 0.0;        ///< 当前扫描方位角（度）
    double m_sweepSpeed = 24.0;       ///< 扫描速率（度/秒 ≈ 4rpm）
    double m_tickInterval = 50;       ///< 定时器间隔（毫秒）
    bool m_running = false;
    QRandomGenerator m_rng;
    double m_rangeMeter = 5000.0;     ///< 当前模拟量程(米)
    int m_rangeCells = 512;           ///< 距离单元数
};

#endif // RADARSIMULATOR_H
