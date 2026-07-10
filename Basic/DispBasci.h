/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2026-03-30 15:27:09
 * @Description: 
 */
#ifndef DISPBASCI_H
#define DISPBASCI_H

#include <QColor>
#include <QString>
#include <qnamespace.h>
#include <QWidget>
#include <QScreen>
#include <QGuiApplication>
#include <algorithm>
#include <cmath>

/**
 * @brief 屏幕分辨率与Windows DPI适配助手
 * @details ui.dpi_policy=fixed 时禁用 Qt 高DPI缩放，显控不跟随 Windows 文本/显示缩放；
 *          ui.dpi_policy=system 时保留 Qt 高DPI缩放，作为现场兼容回退。
 *          ui.ui_scale 在上述基础上对显控内部控件/文字做受控缩放。
 *
 *          在 main() 中 QApplication 创建后、setupFont 之前调用 init()
 */
class ScaleHelper {
public:
    /// 初始化：根据当前 Qt 坐标系屏幕尺寸计算布局缩放因子
    static void init(const QString& dpiPolicy = QStringLiteral("fixed"), double uiScale = 1.0) {
        s_dpiPolicy = dpiPolicy;
        s_uiScale = std::clamp(uiScale, 0.70, 1.60);
        QScreen* screen = QGuiApplication::primaryScreen();
        if (screen) {
            // fixed: size() 通常接近物理像素；system: size() 为 Qt 高DPI逻辑像素。
            // 后续布局始终使用同一 Qt 坐标系，避免混用物理/逻辑坐标。
            s_logicalW = screen->size().width();
            s_logicalH = screen->size().height();
            s_physicalW = static_cast<int>(std::round(screen->geometry().width() * screen->devicePixelRatio()));
            s_physicalH = static_cast<int>(std::round(screen->geometry().height() * screen->devicePixelRatio()));
            s_devicePixelRatio = screen->devicePixelRatio();
            s_factor = std::clamp(s_logicalH / 1080.0, 0.7, 2.0);
        }
    }

    /// 布局缩放因子 (逻辑1080p → 1.0)
    static double factor() { return s_factor; }

    /// 按分辨率布局缩放因子和内部UI缩放系数缩放整数值
    static int scaled(int base) {
        return static_cast<int>(std::round(base * s_factor * s_uiScale));
    }

    /// 仅按内部UI缩放系数缩放整数值（用于字体、图标、固定输入框）
    static int uiScaled(int base) {
        return static_cast<int>(std::round(base * s_uiScale));
    }

    /// 逻辑屏幕宽度
    static int logicalWidth() { return s_logicalW; }
    /// 逻辑屏幕高度
    static int logicalHeight() { return s_logicalH; }
    /// 屏幕设备像素比
    static double devicePixelRatio() { return s_devicePixelRatio; }
    /// 估算物理屏幕宽度
    static int physicalWidth() { return s_physicalW; }
    /// 估算物理屏幕高度
    static int physicalHeight() { return s_physicalH; }
    /// 当前DPI策略
    static QString dpiPolicy() { return s_dpiPolicy; }
    /// 当前内部UI缩放系数
    static double uiScale() { return s_uiScale; }

    static bool compactLayout() {
        return s_logicalW <= 1920 && s_logicalH <= 1080;
    }

    /// 左侧信息面板宽度 (逻辑屏幕宽度的 22%)
    static int leftPanelWidth() {
        const int base = compactLayout()
            ? std::clamp(static_cast<int>(std::round(s_logicalW * 0.195)), 350, 380)
            : std::clamp(static_cast<int>(std::round(s_logicalW * 0.22)), 380, 520);
        return uiScaled(base);
    }

    /// 右侧P显/B显面板宽度 (逻辑屏幕宽度的 20%)
    static int rightPanelWidth() {
        const int base = compactLayout()
            ? std::clamp(static_cast<int>(std::round(s_logicalW * 0.18)), 320, 350)
            : std::clamp(static_cast<int>(std::round(s_logicalW * 0.21)), 400, 440);
        return uiScaled(base);
    }

    /// 按钮最小高度 (基准40px按布局因子缩放)
    static int buttonHeight() { return scaled(40); }

    /// setTab 最大高度 (基准600px按布局因子缩放)
    static int setTabMaxHeight() { return scaled(600); }

    /// Logo 最大尺寸 (基准50px按布局因子缩放)
    static int logoSize() { return scaled(50); }

private:
    static inline double s_factor = 1.0;
    static inline double s_uiScale = 1.0;
    static inline int s_logicalW = 1920;
    static inline int s_logicalH = 1080;
    static inline int s_physicalW = 1920;
    static inline int s_physicalH = 1080;
    static inline double s_devicePixelRatio = 1.0;
    static inline QString s_dpiPolicy = QStringLiteral("fixed");
};

//COLOR
const QColor DET_COLOR = QColor(255, 180, 50);   // 检测点：暖橙色（船用雷达回波风格）
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
