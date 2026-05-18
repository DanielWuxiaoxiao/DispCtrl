/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-23 09:44:52
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-05-18 15:26:20
 * @Description: 
 */
#ifndef MOUSEPOSITIONINFO_H
#define MOUSEPOSITIONINFO_H

#include <QWidget>

class QCheckBox;

namespace Ui {
class MousePositionInfo;
}

class MousePositionInfo : public QWidget
{
    Q_OBJECT

public:
    explicit MousePositionInfo(QWidget *parent = nullptr);
    ~MousePositionInfo();

    // 更新鼠标位置信息
    void updatePosition(double distance, double azimuth);

    // 获取当前状态的方法
    bool isDetectionVisible() const;
    bool isTrackVisible() const;
    bool isTbdTrackVisible() const;
    bool isCooperativeTrackVisible() const;
    double getDetectionSize() const;
    double getTrackSize() const;

signals:
    /**
     * @brief 检测点可见性变化信号
     * @param visible true表示显示，false表示隐藏
     */
    void detectionVisibilityChanged(bool visible);

    /**
     * @brief 跟踪点可见性变化信号
     * @param visible true表示显示，false表示隐藏
     */
    void trackVisibilityChanged(bool visible);

    /**
     * @brief TBD航迹可见性变化信号
     * @param visible true表示显示，false表示隐藏
     */
    void tbdTrackVisibilityChanged(bool visible);

    /**
     * @brief 协同航迹可见性变化信号
     * @param visible true表示显示，false表示隐藏
     */
    void cooperativeTrackVisibilityChanged(bool visible);

    /**
     * @brief 检测点大小变化信号
     * @param ratio 大小倍数（0.5~3.0）
     */
    void detectionSizeChanged(double ratio);

    /**
     * @brief 航迹点大小变化信号
     * @param ratio 大小倍数（0.5~3.0）
     */
    void trackSizeChanged(double ratio);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    Ui::MousePositionInfo *ui;
    QCheckBox* m_tbdTrackCheckBox = nullptr;
    QCheckBox* m_cooperativeTrackCheckBox = nullptr;
};

#endif // MOUSEPOSITIONINFO_H
