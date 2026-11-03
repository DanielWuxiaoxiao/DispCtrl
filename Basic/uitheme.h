#ifndef UITHEME_H
#define UITHEME_H

// Protocol.h 的旧 pack(1) 不能影响 Qt 类型；这里只定义 GUI 展示样式。
#pragma pack(push, 8)
#include <QString>

namespace UiTheme {
// 与 darkstyle.qss 对齐；点航迹类别颜色集中在 DispBasci.h，几何不受主题影响。
constexpr const char* Background = "#12171c";
constexpr const char* Surface = "#1b2228";
constexpr const char* Border = "#303b44";
constexpr const char* Text = "#e8edf1";
constexpr const char* Secondary = "#a6b2bd";
constexpr const char* Accent = "#78cbd8";
constexpr const char* Normal = "#78d4a5";
constexpr const char* NormalSurface = "#23322d";
constexpr const char* Fault = "#ea8c93";
constexpr const char* FaultSurface = "#35252d";

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
