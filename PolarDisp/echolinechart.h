/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-04-09 16:45:56
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-20 11:30:45
 * @Description: 
 */
/**
 * @file echolinechart.h
 * @brief SIMRAD风格回波幅值折线图控件
 * @details 显示单条扫描线的回波幅值数据（距离单元 vs 幅度 0~255），
 *          支持实时更新、SIMRAD主题配色、方位角标注。
 */
#ifndef ECHOLINECHART_H
#define ECHOLINECHART_H

#include <QWidget>
#include <QVector>
#include <QColor>
#include <QTimer>
#include "Basic/MarineProtocol.h"

/**
 * @class EchoLineChart
 * @brief SIMRAD主题回波A显折线图
 *
 * 用于在PPI视图上叠加或切换显示当前扫描线的回波幅值数据。
 * 横轴：距离单元索引（对应实际距离），纵轴：回波幅度 0~255。
 */
class EchoLineChart : public QWidget
{
    Q_OBJECT
public:
    explicit EchoLineChart(QWidget* parent = nullptr);
    ~EchoLineChart() override = default;

    /// 设置当前量程(米)，用于X轴标注
    void setRangeMeters(double meters);

public slots:
    /// 更新回波数据（来自 MarineEchoLine）
    void updateEchoLine(const MarineEchoLine& line);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // 当前显示数据
    QVector<uint8_t> m_amplitudes;     ///< 幅值数组 (0~255)
    double  m_azimuthDeg = 0.0;        ///< 当前方位角(度)
    uint16_t m_packetNum = 0;          ///< 包序号
    double  m_rangeMeters = 3704.0;    ///< 当前量程(米)

    // SIMRAD 主题色
    static constexpr QRgb CLR_BG       = 0xFF0a0a0a; ///< 背景色
    static constexpr QRgb CLR_GRID     = 0xFF333333; ///< 网格线
    static constexpr QRgb CLR_AXIS     = 0xFF888888; ///< 坐标轴
    static constexpr QRgb CLR_TEXT     = 0xFF999999; ///< 刻度文字
    static constexpr QRgb CLR_LINE     = 0xFF00CC66; ///< 回波折线（绿色）
    static constexpr QRgb CLR_FILL_TOP = 0x4400CC66; ///< 填充顶色(半透明绿)
    static constexpr QRgb CLR_FILL_BOT = 0x0500CC66; ///< 填充底色(近透明)
    static constexpr QRgb CLR_BORDER   = 0xFFff8800; ///< 边框(SIMRAD橙)
    static constexpr QRgb CLR_TITLE    = 0xFFff8800; ///< 标题(SIMRAD橙)

    // 布局边距
    int m_marginLeft   = 50;
    int m_marginRight  = 15;
    int m_marginTop    = 30;
    int m_marginBottom = 35;
};

#endif // ECHOLINECHART_H
