#ifndef OFFLINERAE_H
#define OFFLINERAE_H

#include <QColor>
#include "uitheme.h"
#include "Basic/Protocol.h"

namespace OfflineRae {

constexpr unsigned char kStatMethod = 250;
constexpr unsigned kColorCyan = 101;
constexpr unsigned kColorRed = 102;
constexpr unsigned kColorBlue = 103;

// 离线分类代码不变；批绘制、最新点和 3D 使用同一组显示颜色。
inline const QColor& kCyanDisplayColor = UiTheme::radarPalette().offlineCyan;
inline const QColor& kRedDisplayColor = UiTheme::radarPalette().offlineRed;
inline const QColor& kBlueDisplayColor = UiTheme::radarPalette().offlineBlue;

inline bool isOffline(const PointInfo& info)
{
    return info.statMethod == kStatMethod;
}

inline QColor colorFor(const PointInfo& info, const QColor& fallback)
{
    if (!isOffline(info)) {
        return fallback;
    }

    switch (info.targetRecResult) {
    case kColorCyan:
        return kCyanDisplayColor;
    case kColorRed:
        return kRedDisplayColor;
    case kColorBlue:
        return kBlueDisplayColor;
    default:
        return fallback;
    }
}

} // namespace OfflineRae

#endif // OFFLINERAE_H
