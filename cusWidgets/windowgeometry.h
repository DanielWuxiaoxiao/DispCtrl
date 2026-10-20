#ifndef WINDOWGEOMETRY_H
#define WINDOWGEOMETRY_H

#include <QRect>
#include <QVector>

namespace CusWindowGeometry {

struct Screen {
    QRect geometry;
    QRect availableGeometry;
};

// All rectangles use Qt global logical coordinates, including negative origins.
// Prefer the native screen only for ties or an entirely off-screen window.
inline QRect maximizedGeometry(const QRect& windowRect, const QVector<Screen>& screens,
                               int preferredScreen = 0)
{
    if (screens.isEmpty()) return {};

    const auto overlapArea = [&windowRect](const QRect& screenRect) -> qint64 {
        const QRect overlap = windowRect.intersected(screenRect);
        return overlap.isEmpty() ? 0 : qint64(overlap.width()) * overlap.height();
    };
    int selected = preferredScreen >= 0 && preferredScreen < screens.size() ? preferredScreen : 0;
    qint64 selectedArea = overlapArea(screens[selected].geometry);
    for (int index = 0; index < screens.size(); ++index) {
        const qint64 area = overlapArea(screens[index].geometry);
        if (area > selectedArea) {
            selected = index;
            selectedArea = area;
        }
    }

    const Screen& target = screens[selected];
    return target.availableGeometry.isValid() ? target.availableGeometry : target.geometry;
}

} // namespace CusWindowGeometry

#endif // WINDOWGEOMETRY_H
