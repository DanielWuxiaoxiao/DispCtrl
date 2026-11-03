#ifndef OFFLINERAE_H
#define OFFLINERAE_H

#include <QColor>
#include "Basic/Protocol.h"

namespace OfflineRae {

constexpr unsigned char kStatMethod = 250;
constexpr unsigned kColorCyan = 101;
constexpr unsigned kColorRed = 102;
constexpr unsigned kColorBlue = 103;

// 离线分类代码不变；批绘制、最新点和 3D 使用同一组显示颜色。
const QColor kCyanDisplayColor(120, 203, 216);
const QColor kRedDisplayColor(234, 140, 147);
const QColor kBlueDisplayColor(130, 169, 220);

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
