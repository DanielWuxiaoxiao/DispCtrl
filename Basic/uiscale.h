#ifndef UISCALE_H
#define UISCALE_H
#pragma pack(push, 8)
#include <QGuiApplication>
#include <QScreen>
#include <QString>
#include <algorithm>
#include <cmath>

/**
 * @brief 屏幕分辨率自适应布局助手（方案B-v3）
 * @details 仅处理面板宽度、按钮高度等布局尺寸的自适应
 *
 *          ui.dpi_policy=fixed 时禁用 Qt 高DPI缩放，软件不跟随 Windows 显示/文本缩放；
 *          ui.dpi_policy=system 时保留 Qt 高DPI缩放，跟随系统DPI。
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
            s_physicalW = screen->geometry().width() * screen->devicePixelRatio();
            s_physicalH = screen->geometry().height() * screen->devicePixelRatio();
            s_devicePixelRatio = screen->devicePixelRatio();
            s_factor = std::clamp(s_logicalH / 1080.0, 0.7, 2.0);
        }
    }

    /// 布局缩放因子 (逻辑1080p → 1.0)
    static double factor() { return s_factor; }

    /// 按布局缩放因子和内部UI缩放系数缩放整数值
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

    // A 1920-wide 16:10 laptop has more vertical room, not more room for sidebars/fonts.
    static bool compactSize(int width, int height) {
        return width <= 1920 && height <= 1200;
    }
    static bool compactLayout() {
        return compactSize(s_logicalW, s_logicalH);
    }

    // Left panel width.
    static int leftPanelWidth() {
        if (compactLayout()) {
            return uiScaled(std::clamp(static_cast<int>(std::round(s_logicalW * 0.195)), 350, 380));
        }
        return uiScaled(std::clamp(static_cast<int>(std::round(s_logicalW * 0.22)), 380, 520));
    }

    // Right P/B/H panel width.
    static int rightPanelWidth() {
        if (compactLayout()) {
            return uiScaled(std::clamp(static_cast<int>(std::round(s_logicalW * 0.18)), 320, 350));
        }
        return uiScaled(std::clamp(static_cast<int>(std::round(s_logicalW * 0.21)), 400, 440));
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
#pragma pack(pop)
#endif
