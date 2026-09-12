/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2026-01-15 14:23:10
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:55
 * @Description: 
 */
/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-01-12
 * @Description: Custom SpinBox with styled arrows
 */
/**
 * @file customspinbox.h
 * @brief 自定义SpinBox组件头文件
 * @details 提供带有自定义绿色箭头的SpinBox组件，支持整数和浮点数
 *
 * 主要特性：
 * 1. 自定义箭头绘制：与CustomComboBox风格一致的绿色箭头
 * 2. 完整功能保留：继承标准QSpinBox/QDoubleSpinBox的所有功能
 * 3. 状态响应：根据hover状态动态调整箭头颜色
 * 4. 透明按钮背景：按钮区域透明，箭头清晰可见
 *
 * @author DispCtrl Development Team
 * @version 1.0
 * @date 2025
 */

#ifndef CUSTOMSPINBOX_H
#define CUSTOMSPINBOX_H

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPainter>
#include <QPaintEvent>

/**
 * @class CustomSpinBox
 * @brief 带有自定义箭头的整数SpinBox
 * @details 重写paintEvent实现自定义上下箭头绘制
 */
class CustomSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口组件
     */
    explicit CustomSpinBox(QWidget *parent = nullptr);

protected:
    /**
     * @brief 重写绘制事件
     * @param event 绘制事件对象
     * @details 绘制标准SpinBox并叠加自定义箭头
     */
    void paintEvent(QPaintEvent *event) override;

private:
    /**
     * @brief 绘制自定义上箭头
     * @param painter 绘制器对象
     * @param rect 上按钮区域
     */
    void drawUpArrow(QPainter *painter, const QRect &rect);

    /**
     * @brief 绘制自定义下箭头
     * @param painter 绘制器对象
     * @param rect 下按钮区域
     */
    void drawDownArrow(QPainter *painter, const QRect &rect);
};

/**
 * @class CustomDoubleSpinBox
 * @brief 带有自定义箭头的浮点数SpinBox
 * @details 重写paintEvent实现自定义上下箭头绘制
 */
class CustomDoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口组件
     */
    explicit CustomDoubleSpinBox(QWidget *parent = nullptr);

protected:
    /**
     * @brief 重写绘制事件
     * @param event 绘制事件对象
     * @details 绘制标准DoubleSpinBox并叠加自定义箭头
     */
    void paintEvent(QPaintEvent *event) override;

private:
    /**
     * @brief 绘制自定义上箭头
     * @param painter 绘制器对象
     * @param rect 上按钮区域
     */
    void drawUpArrow(QPainter *painter, const QRect &rect);

    /**
     * @brief 绘制自定义下箭头
     * @param painter 绘制器对象
     * @param rect 下按钮区域
     */
    void drawDownArrow(QPainter *painter, const QRect &rect);
};

#endif // CUSTOMSPINBOX_H
