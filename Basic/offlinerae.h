#ifndef OFFLINERAE_H
#define OFFLINERAE_H

#include <QColor>
#include "Basic/Protocol.h"

namespace OfflineRae {

constexpr unsigned char kStatMethod = 250;
constexpr unsigned kColorCyan = 101;
constexpr unsigned kColorRed = 102;
constexpr unsigned kColorBlue = 103;

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
        return QColor(0, 255, 255);
    case kColorRed:
        return QColor(255, 0, 0);
    case kColorBlue:
        return QColor(0, 0, 255);
    default:
        return fallback;
    }
}

} // namespace OfflineRae

#endif // OFFLINERAE_H
