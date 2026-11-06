#ifndef UITHEME_H
#define UITHEME_H

// Protocol.h 的旧 pack(1) 不能影响 Qt 类型；这里只定义 GUI 展示样式。
#pragma pack(push, 8)
#include <QColor>
#include <QString>

namespace UiTheme {
// 与 darkstyle.qss 对齐；点航迹类别颜色集中在 DispBasci.h，几何不受主题影响。
inline const char* Background = "#12171c";
inline const char* Surface = "#1b2228";
inline const char* Border = "#303b44";
inline const char* Text = "#e8edf1";
inline const char* Secondary = "#a6b2bd";
inline const char* Accent = "#78cbd8";
inline const char* Normal = "#78d4a5";
inline const char* NormalSurface = "#23322d";
inline const char* Fault = "#ea8c93";
inline const char* FaultSurface = "#35252d";
inline bool classicEnabled = false;

struct RadarPalette
{
    QColor detection{120, 212, 165};
    QColor track{234, 140, 147};
    QColor other{223, 189, 130};
    QColor tbd{181, 154, 223};
    QColor cooperative{120, 203, 216};
    QColor accent{120, 203, 216};
    QColor grid{126, 153, 165, 160};
    QColor text{232, 237, 241};
    QColor measure{223, 189, 130};
    QColor offlineCyan{120, 203, 216};
    QColor offlineRed{234, 140, 147};
    QColor offlineBlue{130, 169, 220};
};

inline RadarPalette& radarPalette()
{
    static RadarPalette palette;
    return palette;
}

inline QString name()
{
    return classicEnabled ? QStringLiteral("classic") : QStringLiteral("graphite");
}

// 在创建 Controller/UI 之前调用一次。颜色引用地址稳定，绘制热路径直接读取颜色。
inline void configure(const QString& theme)
{
    classicEnabled = theme == QLatin1String("classic");
    Background = classicEnabled ? "#101818" : "#12171c";
    Surface = classicEnabled ? "#101818" : "#1b2228";
    Border = classicEnabled ? "#66b880" : "#303b44";
    Text = classicEnabled ? "#66ffcc" : "#e8edf1";
    Secondary = classicEnabled ? "#66ffcc" : "#a6b2bd";
    Accent = classicEnabled ? "#00ff88" : "#78cbd8";
    Normal = classicEnabled ? "#00ff00" : "#78d4a5";
    NormalSurface = classicEnabled ? "#113522" : "#23322d";
    Fault = classicEnabled ? "#ff0000" : "#ea8c93";
    FaultSurface = classicEnabled ? "#5a1717" : "#35252d";

    RadarPalette palette;
    if (classicEnabled) {
        // 893b491 的父提交 f338142 中的点迹类别和雷达网格配色。
        palette.detection = Qt::green;
        palette.track = Qt::red;
        palette.other = Qt::yellow;
        palette.tbd = QColor(160, 64, 255);
        palette.cooperative = QColor(0, 170, 255);
        palette.accent = QColor(0, 255, 136);
        palette.grid = QColor(Qt::gray);
        palette.text = Qt::white;
        palette.measure = Qt::yellow;
        palette.offlineCyan = Qt::cyan;
        palette.offlineRed = Qt::red;
        palette.offlineBlue = Qt::blue;
    }
    radarPalette() = palette;
}

// 保留统一入口；classic 使用 x576 原始 QSS 与逐控件原始颜色，不做近似映射。
inline QColor color(const QColor& value) { return value; }
inline QString styleColors(const QString& value) { return value; }

enum class Status { Normal, Fault, Inactive, Unknown };

// 返回未缩放 QSS。调用方沿用 UiFontScale::style/差异比较，避免重复缩放和 repolish。
inline QString statusStyle(Status status, bool prominent = false)
{
    const char* background = Surface;
    const char* foreground = Secondary;
    const char* marker = Border;
    switch (status) {
    case Status::Normal:
        foreground = Normal;
        marker = Border;
        break;
    case Status::Fault:
        background = FaultSurface;
        foreground = Fault;
        marker = Fault;
        break;
    case Status::Inactive:
        break;
    case Status::Unknown:
        // 未知不是故障，不再把整片未上报状态染成黄块。
        break;
    }
    // 显式覆盖 disabled：状态块不可点击，但正文不能被系统调色板淡化。
    return QStringLiteral(
        "QPushButton, QPushButton:disabled { background:%1; color:%2; "
        "border:1px solid %3; border-radius:4px; "
        "padding:%4px; font-size:%5px; font-weight:normal; }")
        .arg(QLatin1String(background), QLatin1String(foreground), QLatin1String(marker))
        .arg(prominent ? 10 : 6).arg(prominent ? 15 : 13);
}
} // namespace UiTheme
#pragma pack(pop)
#endif
