#ifndef UITHEMERUNTIME_H
#define UITHEMERUNTIME_H

#pragma pack(push, 8)
#include "uitheme.h"
#include <QApplication>
#include <QAbstractButton>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QHash>
#include <QPointer>
#include <QThread>
#include <QTabWidget>
#include <QSplitter>
#include <QTimer>
#include <QWidget>
#include <functional>
#include <utility>

namespace UiTheme {

struct IconPair { QIcon classic, graphite; };
inline QHash<QString, IconPair>& iconPairs() {
    static QHash<QString, IconPair> pairs;
    return pairs;
}
inline QIcon themedIcon(const QString& classic, const QString& graphite) {
    const QString key = classic + QLatin1Char('|') + graphite;
    auto& pairs = iconPairs();
    if (!pairs.contains(key)) pairs.insert(key, {QIcon(classic), QIcon(graphite)});
    return classicEnabled ? pairs.value(key).classic : pairs.value(key).graphite;
}
inline QIcon refreshedIcon(const QIcon& icon) {
    if (icon.isNull()) return icon;
    for (const auto& pair : iconPairs()) {
        if (icon.cacheKey() == pair.classic.cacheKey() || icon.cacheKey() == pair.graphite.cacheKey())
            return classicEnabled ? pair.classic : pair.graphite;
    }
    return icon;
}

// GUI-thread registrations are refreshed only on an explicit theme change.
// Each style factory retains its design-size source, so fonts are scaled once.
class Runtime final : public QObject {
public:
    using Refresh = std::function<void()>;
    using Style = std::function<QString()>;

    static Runtime& instance() {
        static Runtime runtime;
        return runtime;
    }

    void setApplicationRefresh(Refresh refresh) { m_applicationRefresh = std::move(refresh); }

    void setStyle(QWidget* widget, Style factory) {
        if (!widget) return;
        if (!m_styles.contains(widget)) {
            connect(widget, &QObject::destroyed, this, [this, widget] { m_styles.remove(widget); });
        }
        m_styles.insert(widget, std::move(factory));
        const QString style = m_styles.value(widget)();
        if (widget->styleSheet() != style) widget->setStyleSheet(style);
    }

    void onChanged(QObject* owner, Refresh refresh) {
        if (!owner) return;
        if (!m_refresh.contains(owner)) {
            connect(owner, &QObject::destroyed, this, [this, owner] { m_refresh.remove(owner); });
        }
        m_refresh[owner].append(std::move(refresh));
    }

    bool apply(const QString& theme) {
        Q_ASSERT(!qApp || QThread::currentThread() == qApp->thread());
        if (m_switching || (theme != QLatin1String("classic") && theme != QLatin1String("graphite"))) return false;
        if (theme == name()) return true;
        m_switching = true;
        QVector<std::pair<QPointer<QSplitter>, QList<int>>> splitterSizes;
        for (QWidget* widget : QApplication::allWidgets()) {
            if (auto* splitter = qobject_cast<QSplitter*>(widget))
                splitterSizes.append({QPointer<QSplitter>(splitter), splitter->sizes()});
        }
        configure(theme);
        if (m_applicationRefresh) m_applicationRefresh();

        // Callbacks can replace a health-window presentation or update bindings.
        // Guard copied owners so their destruction cannot leave a stale callback.
        QVector<std::pair<QPointer<QWidget>, Style>> styles;
        for (auto it = m_styles.cbegin(); it != m_styles.cend(); ++it)
            styles.append({QPointer<QWidget>(it.key()), it.value()});
        for (const auto& entry : styles) {
            if (!entry.first) continue;
            const QString style = entry.second();
            if (entry.first->styleSheet() != style) entry.first->setStyleSheet(style);
        }
        QVector<std::pair<QPointer<QObject>, QVector<Refresh>>> callbacks;
        for (auto it = m_refresh.cbegin(); it != m_refresh.cend(); ++it)
            callbacks.append({QPointer<QObject>(it.key()), it.value()});
        for (const auto& entry : callbacks) {
            for (const auto& refresh : entry.second) {
                if (!entry.first) break;
                refresh();
            }
        }
        for (QWidget* widget : QApplication::allWidgets()) {
            if (auto* button = qobject_cast<QAbstractButton*>(widget)) button->setIcon(refreshedIcon(button->icon()));
            if (auto* tabs = qobject_cast<QTabWidget*>(widget)) {
                for (int i = 0; i < tabs->count(); ++i) tabs->setTabIcon(i, refreshedIcon(tabs->tabIcon(i)));
            }
            widget->update();
            if (auto* view = qobject_cast<QGraphicsView*>(widget)) {
                if (view->scene()) view->scene()->invalidate();
            }
        }
        for (const auto& entry : splitterSizes) {
            if (!entry.first) continue;
            entry.first->setSizes(entry.second);
            QTimer::singleShot(0, entry.first.data(), [entry] {
                if (entry.first) entry.first->setSizes(entry.second);
            });
        }
        m_switching = false;
        return true;
    }

private:
    QHash<QWidget*, Style> m_styles;
    QHash<QObject*, QVector<Refresh>> m_refresh;
    Refresh m_applicationRefresh;
    bool m_switching = false;
};

inline void setStyle(QWidget* widget, Runtime::Style factory) {
    Runtime::instance().setStyle(widget, std::move(factory));
}
inline void onChanged(QObject* owner, Runtime::Refresh refresh) {
    Runtime::instance().onChanged(owner, std::move(refresh));
}
} // namespace UiTheme
#pragma pack(pop)
#endif
