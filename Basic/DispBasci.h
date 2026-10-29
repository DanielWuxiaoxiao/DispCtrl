/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@xidian.edu.cn
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-09-12 12:22:43
 * @Description: 
 */
#ifndef DISPBASCI_H
#define DISPBASCI_H

/**
 * @brief 软件版本号
 * @details 显示在主界面标题区（原英文名 SubtitleLabel 处）。
 *          发布新版本时只需修改此处一处。
 */
#define APP_VERSION_STR "V6.5"

#include <QColor>
#include <QString>
#include <qnamespace.h>
#include <QWidget>
#include <QScreen>
#include <QGridLayout>
#include <QGuiApplication>
#include <algorithm>
#include <cmath>
#include "Protocol.h"

#include "uiscale.h"

//COLOR
const QColor DET_COLOR = Qt::green;              // 检测点：绿色 (0, 255, 0)
const QColor TRA_COLOR = Qt::red;                // 航迹：红色
const QColor DRONE_COLOR = Qt::yellow;                // 无人机 ： 黄色
const QColor TBD_COLOR = QColor(160, 64, 255);    // TBD航迹：紫色
const QColor CO_TRACK_COLOR = QColor(0, 170, 255); // 协同航迹：青蓝色

//STRING
constexpr char APP_NAME[] = "雷达控制平台";
constexpr char APP_NAME_E[] = "Radar Control Platform";
//LABELS - 可通过CF_INS.targetLabel()获取配置值
constexpr char DET_LABEL[] = "检测点";
constexpr char TRA_LABEL[] = "跟踪点";
constexpr char TBD_LABEL[] = "TBD航迹";
constexpr char CO_TRACK_LABEL[] = "协同航迹";

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

inline QColor trackTypeColor(unsigned type) {
    switch (type) {
    case 3:
        return TBD_COLOR;
    case 4:
        return CO_TRACK_COLOR;
    case 2:
    default:
        return TRA_COLOR;
    }
}

inline QString trackTypeLabel(unsigned type) {
    switch (type) {
    case 3:
        return QString::fromUtf8(TBD_LABEL);
    case 4:
        return QString::fromUtf8(CO_TRACK_LABEL);
    case 2:
    default:
        return QString::fromUtf8(TRA_LABEL);
    }
}

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
 * @brief 从QGridLayout中隐藏并移除指定控件，使布局能紧凑化
 * @param grid 目标QGridLayout
 * @param widget 要隐藏的控件
 */
inline void hideFromGrid(QGridLayout* grid, QWidget* widget) {
    if (!grid || !widget) return;
    grid->removeWidget(widget);
    widget->hide();
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
