/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-27 11:21:00
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-27 11:45:27
 * @Description: 
 */
/**
 * @file beamschedulewidget.h
 * @brief 波束调度甘特图显示控件
 * @details
 *  - BeamScheduleChart: 纯 QPainter 甘特图，X轴=帧内µs时间，行=任务类型(搜索/跟踪/空闲)
 *  - BeamScheduleWidget: 外层容器，含顶部状态栏（帧序/槽数/帧长）+ 甘特图
 *
 * ### 颜色约定
 *  - 搜索 taskType=0: 蓝色 #508cdc
 *  - 跟踪 taskType=1: 橙色 #ff8c28
 *  - 空闲 taskType=2: 深灰 #505050
 *  - 其他:            紫色 #a050c8
 */
#ifndef BEAMSCHEDULEWIDGET_H
#define BEAMSCHEDULEWIDGET_H

#include <QWidget>
#include <QVector>
#include "Basic/Protocol.h"

class QLabel;

// ============================================================================

/**
 * @class BeamScheduleChart
 * @brief 波束调度甘特图画布
 */
class BeamScheduleChart : public QWidget
{
    Q_OBJECT
public:
    explicit BeamScheduleChart(QWidget* parent = nullptr);

    /// 更新数据并重绘
    void setSchedule(const BeamScheduleReport& hdr, const QVector<BeamSlot>& beamSlots);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    BeamScheduleReport m_header{};
    QVector<BeamSlot>  m_slots;
    bool               m_hasData = false;

    static QColor  slotColor(quint8 taskType);
    static QString taskLabel(quint8 taskType);
};

// ============================================================================

/**
 * @class BeamScheduleWidget
 * @brief 波束调度可视化面板
 *
 * ### 集成方式
 * @code
 * m_beamWidget = new BeamScheduleWidget(this);
 * connect(m_beamMgr, &BeamScheduleManager::beamScheduleReceived,
 *         m_beamWidget, &BeamScheduleWidget::onBeamSchedule);
 * @endcode
 */
class BeamScheduleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BeamScheduleWidget(QWidget* parent = nullptr);

public slots:
    void onBeamSchedule(const BeamScheduleReport& hdr, const QVector<BeamSlot>& beamSlots);

private:
    BeamScheduleChart* m_chart      = nullptr;
    QLabel*            m_lblSeq     = nullptr;
    QLabel*            m_lblSlotNum = nullptr;
    QLabel*            m_lblFrameUs = nullptr;
};

#endif // BEAMSCHEDULEWIDGET_H
