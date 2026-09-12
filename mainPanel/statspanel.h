/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-04-27 11:21:00
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:57
 * @Description: 
 */
/**
 * @file statspanel.h
 * @brief 多目标统计面板
 * @details 以滚动折线图实时展示：检测点数/秒、DBT航迹数、TBD航迹数，
 *          并显示当前各类目标数量的数字摘要。
 *          数据来源：Controller::detInfoProcess / traInfoProcess / tbdInfoProcess
 */
#ifndef STATSPANEL_H
#define STATSPANEL_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QVector>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include "Basic/Protocol.h"

/**
 * @class StatsChart
 * @brief 内嵌滚动折线图控件（纯 QPainter 实现）
 */
class StatsChart : public QWidget
{
    Q_OBJECT
public:
    explicit StatsChart(QWidget* parent = nullptr);
    void setWindowSec(int seconds);  ///< 时间窗口（默认60s）
    void setMaxY(int maxY);          ///< Y轴上限（0=自动）

    void pushSample(int dets, int tracks, int tbds);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Sample {
        int dets;
        int tracks;
        int tbds;
    };

    QVector<Sample> m_samples;
    int m_windowSec = 60;
    int m_maxY      = 0;  // 0=自动
};

/**
 * @class StatsPanel
 * @brief 多目标统计面板
 *
 * 使用方式：
 * @code
 * m_statsPanel = new StatsPanel(this);
 * connect(CON_INS, &Controller::detInfoProcess,
 *         m_statsPanel, &StatsPanel::onDetPoint);
 * connect(CON_INS, &Controller::traInfoProcess,
 *         m_statsPanel, &StatsPanel::onTrackPoint);
 * connect(CON_INS, &Controller::tbdInfoProcess,
 *         m_statsPanel, &StatsPanel::onTbdPoint);
 * @endcode
 */
class StatsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit StatsPanel(QWidget* parent = nullptr);
    ~StatsPanel() override = default;

public slots:
    void onDetPoint(const PointInfo& info);
    void onTrackPoint(const PointInfo& info);
    void onTbdPoint(const PointInfo& info);

private slots:
    void onSecondTick();

private:
    void applyStyle();
    void updateNumbers();

    StatsChart* m_chart        = nullptr;

    // 数字摘要标签
    QLabel* m_lblDetVal        = nullptr;
    QLabel* m_lblTrackVal      = nullptr;
    QLabel* m_lblTbdVal        = nullptr;
    QLabel* m_lblDetRate       = nullptr;
    QLabel* m_lblTrackRate     = nullptr;
    QLabel* m_lblTbdRate       = nullptr;

    // 当前秒内计数
    int m_detCount   = 0;
    int m_trackCount = 0;
    int m_tbdCount   = 0;

    // 累计总数（去重批号统计留给外部，此处简单计算当前帧率）
    int m_totalDet   = 0;
    int m_totalTrack = 0;
    int m_totalTbd   = 0;

    QTimer* m_secTimer = nullptr;
};

#endif // STATSPANEL_H
