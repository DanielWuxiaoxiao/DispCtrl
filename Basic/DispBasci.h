/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-04-20 11:30:44
 * @Description: 
 */
#ifndef DISPBASCI_H
#define DISPBASCI_H

#include <QColor>
#include <qnamespace.h>
#include <QWidget>
#include <QScreen>
#include <QGridLayout>
#include <QGuiApplication>
#include <algorithm>
#include <cmath>

/**
 * @brief 屏幕分辨率自适应布局助手（方案B-v2）
 * @details 仅处理面板宽度、按钮高度等布局尺寸的自适应
 *          字体大小由 Qt AA_EnableHighDpiScaling 自动处理，不在此重复缩放
 *
 *          计算方式：取物理像素高度 / devicePixelRatio 得到逻辑高度
 *          以逻辑 1080 为基准 factor=1.0
 *          面板宽度直接取逻辑屏幕宽度的百分比
 *
 *          在 main() 中 QApplication 创建后、setupFont 之前调用 init()
 */
class ScaleHelper {
public:
    /// 初始化：根据逻辑屏幕尺寸计算布局缩放因子
    static void init() {
        QScreen* screen = QGuiApplication::primaryScreen();
        if (screen) {
            // size() 在 AA_EnableHighDpiScaling 下返回逻辑像素
            s_logicalW = screen->size().width();
            s_logicalH = screen->size().height();
            s_factor = std::clamp(s_logicalH / 1080.0, 0.7, 2.0);
        }
    }

    /// 布局缩放因子 (逻辑1080p → 1.0)
    static double factor() { return s_factor; }

    /// 按布局缩放因子缩放整数值（仅用于按钮高度、间距等布局尺寸）
    static int scaled(int base) {
        return static_cast<int>(std::round(base * s_factor));
    }

    /// 逻辑屏幕宽度
    static int logicalWidth() { return s_logicalW; }
    /// 逻辑屏幕高度
    static int logicalHeight() { return s_logicalH; }

    /// 左侧信息面板宽度 (逻辑屏幕宽度的 22%)
    static int leftPanelWidth() {
        return static_cast<int>(s_logicalW * 0.22);
    }

    /// 右侧P显/B显面板宽度 (逻辑屏幕宽度的 28%)
    static int rightPanelWidth() {
        return static_cast<int>(s_logicalW * 0.28);
    }

    /// 按钮最小高度 (基准40px按布局因子缩放)
    static int buttonHeight() { return scaled(40); }

    /// setTab 最大高度 (基准600px按布局因子缩放)
    static int setTabMaxHeight() { return scaled(600); }

    /// Logo 最大尺寸 (基准50px按布局因子缩放)
    static int logoSize() { return scaled(50); }

private:
    static inline double s_factor = 1.0;
    static inline int s_logicalW = 1920;
    static inline int s_logicalH = 1080;
};

//COLOR
const QColor DET_COLOR = Qt::green;              // 检测点：绿色 (0, 255, 0)
const QColor TRA_COLOR = Qt::red;                // 航迹默认色（保留兼容）
const QColor TBD_COLOR = Qt::yellow;             // TBD航迹：黄色 (255, 255, 0)
const QColor TRA_DRONE_COLOR = Qt::red;          // 无人机目标航迹：红色
const QColor TRA_OTHER_COLOR = QColor(0, 120, 255); // 非无人机目标航迹：蓝色

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
