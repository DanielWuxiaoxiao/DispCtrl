/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2026-03-30 11:48:47
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-30 15:27:10
 * @Description: 
 */
/**
 * @file colorbarwidget.h
 * @brief 色阶条控件
 * @details PPI右下角显示的颜色图例, 展示回波幅值(0~255)到颜色的映射关系
 */
#ifndef COLORBARWIDGET_H
#define COLORBARWIDGET_H

#include <QWidget>
#include <QImage>
#include <array>
#include <QRgb>

class ColorBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ColorBarWidget(QWidget* parent = nullptr);

    /// 更新颜色映射表 (从 EchoRenderer 同步)
    void setColorLUT(const std::array<QRgb, 256>& lut);

    QSize sizeHint() const override { return QSize(80, 320); }
    QSize minimumSizeHint() const override { return QSize(60, 200); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::array<QRgb, 256> m_colorLUT{};
    bool m_hasLUT = false;
};

#endif // COLORBARWIDGET_H
