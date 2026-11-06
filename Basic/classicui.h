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
    if (!classicEnabled) return;
    struct Entry { const char* name; const char* path; };
    static const Entry entries[] = {
        {"tab_2", ":/resources/icon/log.png"},
        {"tab_4", ":/resources/icon/mode.png"},
        {"tab", ":/resources/icon/param.png"},
        {"tab_5", ":/resources/icon/track.png"},
        {"droneTrackTab", ":/resources/icon/track.png"},
        {"CloseButton", ":/resources/icon/close.png"},
        {"minButton", ":/resources/icon/min.png"},
        {"FuncToolButton", ":/resources/icon/control.png"},
        {"radarsystem", ":/resources/icon/radararray.png"},
        {"FuncToolButton_2", ":/resources/icon/data.png"},
        {"toolButton_3", ":/resources/icon/record.png"},
    };
    for (const auto& entry : entries) {
        QWidget* widget = root->findChild<QWidget*>(QString::fromLatin1(entry.name));
        if (!widget) continue;
        const QIcon icon(QString::fromLatin1(entry.path));
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
