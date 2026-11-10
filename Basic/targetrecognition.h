#ifndef TARGETRECOGNITION_H
#define TARGETRECOGNITION_H
#pragma pack(push, 8)
#include <QString>
#include "Basic/offlinerae.h"
#pragma pack(8)

namespace TargetRecognition {
// DD01/EE航迹识别值；只新增文字，着色和无人机筛选仍严格判断 result == 1。
inline QString text(unsigned result) {
    switch (result) {
    case 0: return QStringLiteral("未知");
    case 1: return QStringLiteral("无人机");
    case 2: return QStringLiteral("行人");
    case 3: return QStringLiteral("车辆");
    case 4: return QStringLiteral("鸟");
    case 5: return QStringLiteral("其它");
    default: return QStringLiteral("未知");
    }
}
inline QString text(const PointInfo& info) {
    // 离线记录借用该字段存颜色码，保留原有提示语义，不把颜色码解释成新类别。
    if (OfflineRae::isOffline(info)) return info.targetRecResult == 1 ? QStringLiteral("无人机") : QStringLiteral("其它");
    return text(info.targetRecResult);
}
}
#pragma pack(pop)
#endif
