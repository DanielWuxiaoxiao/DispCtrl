/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-09-14 14:10:16
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-20 19:24:22
 * @Description: 
 */
#ifndef TESTTRACKGENERATOR_H
#define TESTTRACKGENERATOR_H

#include <QObject>
#include <QElapsedTimer>
#include <QTimer>

#include "Basic/Protocol.h"

/**
 * @brief 以低频定时方式生成多组普通 DBT 三维测试航迹
 * @details 不创建 UDP 套接字。每个 PointInfo 与 UDP 正常航迹使用相同的单位和类型：
 *          range 为米、azimuth/elevation 为度、type 为 PointType::Track；每条航迹在
 *          局地东北天坐标系中匀速积分，RAE 由三维位置实时反算。
 */
class TestTrackGenerator final : public QObject
{
    Q_OBJECT

public:
    explicit TestTrackGenerator(QObject* parent = nullptr);

    /** @brief 开始一轮生成；运行中再次调用不会重置当前航迹。 */
    bool start();
    /** @brief 主动终止本轮，并为已生成的全部测试批次发送消批点。 */
    bool stop();
    bool isRunning() const { return m_running; }

signals:
    /** @brief 交给 Controller::traInfoProcess 的普通航迹点或消批点。 */
    void trackGenerated(PointInfo info);
    /** @brief 启动测试时使用 [radar] 预存经纬高作为临时联调原点。 */
    void fallbackRadarPositionReady(double latitude, double longitude, double altitude);
    void generationStarted(int trackCount, int pointCount);
    void generationFinished();

private slots:
    void generateSample();

private:
    PointInfo makeTrackPoint(int trackIndex) const;
    void finishGeneration();
    void removeGeneratedTracks();

    QTimer m_timer;
    int m_trackCount = 10;
    int m_pointCount = 1200;
    int m_intervalMs = 500;
    double m_speedMps = 15.0;
    quint32 m_firstBatch = 101;
    int m_sampleIndex = 0;
    bool m_running = false;
    QElapsedTimer m_elapsedTimer;
};

#endif // TESTTRACKGENERATOR_H
