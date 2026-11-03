#ifndef UIFONTSCALE_H
#define UIFONTSCALE_H
// Legacy Protocol.h leaves pack(1) enabled; isolate Qt declarations from that state.
#pragma pack(push, 8)
#include <QFont>
#include <QRegularExpression>
#include <QString>
#include "Basic/uiscale.h"

namespace UiFontScale {
// V7 可读性微调：仅字号增加 8%，不改变 ui_scale 的布局/图标/雷达几何含义。
constexpr double ReadabilityFactor = 1.08;
inline double factor() { return ScaleHelper::uiScale() * ReadabilityFactor; }
inline int pixels(double designPixels)
{
    return qMax(1, qRound(designPixels * factor()));
}
// Only font declarations are transformed: padding, border widths and radar geometry stay intact.
// 显式 factor 参数仍按传入值精确缩放；默认入口统一应用可读性系数一次。
inline QString style(const QString& baseStyle, double factor = UiFontScale::factor())
{
    if (qFuzzyCompare(factor, 1.0) || !baseStyle.contains(QStringLiteral("font"), Qt::CaseInsensitive))
        return baseStyle;
    static const QRegularExpression declaration(
        QStringLiteral(R"((\b(?:font-size|font)\s*:\s*[^;{}]*?)(\d+(?:\.\d+)?)\s*(px|pt)\b)"),
        QRegularExpression::CaseInsensitiveOption);
    QString result;
    int cursor = 0;
    auto matches = declaration.globalMatch(baseStyle);
    while (matches.hasNext()) {
        const auto match = matches.next();
        const double value = match.captured(2).toDouble() * factor;
        const bool pixels = match.captured(3).compare(QStringLiteral("px"), Qt::CaseInsensitive) == 0;
        result += baseStyle.mid(cursor, match.capturedStart(2) - cursor);
        result += pixels ? QString::number(qMax(1, qRound(value))) : QString::number(value, 'g', 8);
        result += match.captured(3);
        cursor = match.capturedEnd(3);
    }
    if (cursor == 0) return baseStyle;
    result += baseStyle.mid(cursor);
    return result;
}

inline int pointPixels(double points)
{
    // Explicit painter fonts use the same 96-DPI design baseline as the fixed UI policy.
    return pixels(points * (96.0 / 72.0));
}
inline QFont pointFont(const QString& family, double points, int weight = QFont::Normal)
{
    QFont font(family, -1, weight);
    font.setPixelSize(pointPixels(points));
    return font;
}
} // namespace UiFontScale
#pragma pack(pop)
#endif
