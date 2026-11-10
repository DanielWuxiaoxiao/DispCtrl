#ifndef CLASSICUI_H
#define CLASSICUI_H
#include <QAbstractButton>
#include <QTabWidget>
#include <QWidget>
#include "uitheme.h"

namespace UiTheme {
// mainoverlayout.ui 的 x576/f338142 图标引用，按对象名恢复。
inline void applyClassicMainIcons(QWidget* root)
{
    struct Entry { const char* name; const char* path; const char* graphite; };
    static const Entry entries[] = {
        {"tab_2", ":/resources/icon/log.png", ":/resources/icon/ui/log.svg"},
        {"tab_4", ":/resources/icon/mode.png", ":/resources/icon/ui/radar.svg"},
        {"tab", ":/resources/icon/param.png", ":/resources/icon/ui/sliders.svg"},
        {"tab_5", ":/resources/icon/track.png", ":/resources/icon/ui/track.svg"},
        {"droneTrackTab", ":/resources/icon/track.png", ":/resources/icon/ui/track.svg"},
        {"CloseButton", ":/resources/icon/close.png", ":/resources/icon/ui/close.svg"},
        {"minButton", ":/resources/icon/min.png", ":/resources/icon/ui/minus.svg"},
        {"FuncToolButton", ":/resources/icon/control.png", ":/resources/icon/ui/sliders.svg"},
        {"radarsystem", ":/resources/icon/radararray.png", ":/resources/icon/ui/radar.svg"},
        {"FuncToolButton_2", ":/resources/icon/data.png", ":/resources/icon/ui/awareness.svg"},
        {"toolButton_3", ":/resources/icon/record.png", ":/resources/icon/ui/replay.svg"},
    };
    for (const auto& entry : entries) {
        QWidget* widget = root->findChild<QWidget*>(QString::fromLatin1(entry.name));
        if (!widget) continue;
        const QIcon icon(QString::fromLatin1(classicEnabled ? entry.path : entry.graphite));
        if (auto* button = qobject_cast<QAbstractButton*>(widget)) {
            button->setIcon(icon);
        } else {
            for (auto* tabs : root->findChildren<QTabWidget*>()) {
                const int index = tabs->indexOf(widget);
                if (index >= 0) tabs->setTabIcon(index, icon);
            }
        }
    }
}
}
#endif
