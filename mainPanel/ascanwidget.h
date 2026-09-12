/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-04-27 11:21:00
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:56
 * @Description: 
 */
/**
 * @file ascanwidget.h
 * @brief A显（距离–幅度）波形显示控件
 * @details 接收来自信号处理分系统的一条方位线回波幅度数据（AScanFrame），
 *          以折线图形式绘制：
 *          - 蓝色：PC后回波幅度（dB）
 *          - 绿色：MTD后幅度（dB）
 *          - 橙色虚线：CFAR门限
 *
 *  数据来源：AScanManager（UDP端口 DISP_GET_SIG_PORT3=8005）
 *
 *  集成步骤（简要）：
 *  @code
 *  // 1. 创建管理器和控件
 *  m_ascanMgr    = new AScanManager(this);
 *  m_ascanWidget = new AScanWidget(this);
 *
 *  // 2. 连接信号
 *  connect(m_ascanMgr, &AScanManager::ascanReceived,
 *          m_ascanWidget, &AScanWidget::onAScanData);
 *
 *  // 3. 初始化 UDP 监听（在 controller.cpp init 中）
 *  m_ascanMgr->init(CF_INS.ip("DISP_CTRL_IP","192.168.64.4"), DISP_GET_SIG_PORT3);
 *  @endcode
 */
#ifndef ASCANWIDGET_H
#define ASCANWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QVector>
#include "Basic/Protocol.h"

/**
 * @class AScanPlot
 * @brief 纯 QPainter 绘制的 A显波形区域
 */
class AScanPlot : public QWidget
{
    Q_OBJECT
public:
    explicit AScanPlot(QWidget* parent = nullptr);

    void setData(int pointNum, float rangeResM, float cfar,
                 const QVector<float>& pcAmps,
                 const QVector<float>& mtdAmps);
    void setShowPC(bool v)   { m_showPC  = v; update(); }
    void setShowMTD(bool v)  { m_showMTD = v; update(); }
    void setYRange(float lo, float hi) { m_yLo = lo; m_yHi = hi; update(); }

protected:
    void paintEvent(QPaintEvent*) override;
    QSize sizeHint() const override { return {600, 220}; }

private:
    QVector<float> m_pcAmps;
    QVector<float> m_mtdAmps;
    float m_cfar    = 80.f;
    float m_rangeRes = 10.f;  // metres per cell
    float m_yLo = -10.f;
    float m_yHi = 160.f;
    bool  m_showPC  = true;
    bool  m_showMTD = true;
};

/**
 * @class AScanWidget
 * @brief A显完整控件（含工具栏 + 波形区）
 */
class AScanWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AScanWidget(QWidget* parent = nullptr);
    ~AScanWidget() override = default;

public slots:
    /** @brief 接收 AScanManager 发射的完整帧数据 */
    void onAScanData(const AScanFrame& frame,
                     const QVector<float>& pcAmps,
                     const QVector<float>& mtdAmps);

private:
    void applyStyle();

    AScanPlot*  m_plot     = nullptr;
    QLabel*     m_lblInfo  = nullptr;
    QCheckBox*  m_chkPC    = nullptr;
    QCheckBox*  m_chkMTD   = nullptr;
};

#endif // ASCANWIDGET_H
