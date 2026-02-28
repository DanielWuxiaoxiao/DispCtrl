/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-02-28 16:46:30
 * @Description: 
 */
#ifndef DISPBASCI_H
#define DISPBASCI_H

#include <QColor>
#include <qnamespace.h>
#include <QWidget>
#include <QScreen>
#include <QGuiApplication>


//COLOR
const QColor DET_COLOR = Qt::green;              // 检测点：绿色 (0, 255, 0)
const QColor TRA_COLOR = Qt::red;                // 航迹：红色
const QColor TBD_COLOR = Qt::yellow;              // TBD航迹：黄色 (255, 255, 0)

//STRING
constexpr char APP_NAME[] = "雷达控制平台";
constexpr char APP_NAME_E[] = "Radar Control Platform";
//LABELS - 可通过CF_INS.targetLabel()获取配置值
constexpr char DET_LABEL[] = "检测点";
constexpr char TRA_LABEL[] = "跟踪点";

//RANGE - 可通过CF_INS.range()获取配置值
constexpr float MIN_RANGE = 0.0;
constexpr float MAX_RANGE = 5000.0;  //单位m

//ZVALUE - 可通过CF_INS.zValue()获取配置值
constexpr int TOOL_TIP_Z = 1000;
constexpr int POINT_Z = 10;
constexpr int INFO_Z = 50;
constexpr int LINE_Z = 9;
constexpr int MAP_Z = -5;

//font - 可通过CF_INS.fontSize()获取配置值
constexpr int MAIN_FONT_SIZE = 9;

/**
 * @brief 将窗口居中显示在屏幕中央
 * @param widget 需要居中的窗口指针
 * @details 获取主屏幕的几何信息，计算窗口居中位置并移动窗口
 *          如果窗口有父窗口，则相对于父窗口居中
 *          如果没有父窗口，则相对于屏幕居中
 */
inline void centerWidget(QWidget* widget) {
    if (!widget) return;

    // 确保窗口已经有了正确的尺寸
    widget->adjustSize();

    QRect screenGeometry;
    if (widget->parentWidget()) {
        // 如果有父窗口，相对于父窗口居中
        screenGeometry = widget->parentWidget()->geometry();
    } else {
        // 如果没有父窗口，相对于主屏幕居中
        QScreen* screen = QGuiApplication::primaryScreen();
        if (screen) {
            screenGeometry = screen->availableGeometry();
        }
    }

    int x = screenGeometry.x() + (screenGeometry.width() - widget->width()) / 2;
    int y = screenGeometry.y() + (screenGeometry.height() - widget->height()) / 2;

    widget->move(x, y);
}

/**
 * @brief 将窗口居中显示在屏幕中央（忽略父窗口，始终相对于屏幕居中）
 * @param widget 需要居中的窗口指针
 */
inline void centerWidgetOnScreen(QWidget* widget) {
    if (!widget) return;

    widget->adjustSize();

    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int x = screenGeometry.x() + (screenGeometry.width() - widget->width()) / 2;
        int y = screenGeometry.y() + (screenGeometry.height() - widget->height()) / 2;
        widget->move(x, y);
    }
}

#endif // DISPBASCI_H
