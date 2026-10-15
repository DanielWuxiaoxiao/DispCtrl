#ifndef ERRORLOGLIMITER_H
#define ERRORLOGLIMITER_H

#include "ErrorHandler.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>

#include <limits>

namespace ErrorLogInternal {

struct Key {
    QString code;
    ErrorSeverity severity;
    ErrorCategory category;

    bool operator==(const Key& other) const
    {
        return code == other.code && severity == other.severity && category == other.category;
    }
};

inline uint qHash(const Key& key, uint seed = 0)
{
    return ::qHash(key.code, seed)
        ^ ::qHash((static_cast<uint>(key.category) << 8) | static_cast<uint>(key.severity));
}

class BoundedLogLimiter {
public:
    struct Decision {
        bool shouldLog;
        quint64 suppressedCount;
    };

    static constexpr qint64 kSuppressWindowMs = 5000;
    static constexpr int kMaxStates = 256;

    // nowMs is elapsed monotonic time, supplied by the caller for deterministic tests.
    Decision check(const ErrorInfo& error, qint64 nowMs)
    {
        if (error.severity >= ErrorSeverity::Critical) {
            return {true, 0};
        }

        QMutexLocker locker(&m_mutex);
        const Key key{error.code, error.severity, error.category};
        auto it = m_states.find(key);
        if (it != m_states.end()) {
            // Concurrent callers can acquire this lock in a different order
            // from reading the clock. An older sample must not reopen a window.
            nowMs = qMax(nowMs, it->lastSeenMs);
            it->lastSeenMs = nowMs;
            if (nowMs - it->lastLogMs < kSuppressWindowMs) {
                if (it->suppressedCount < (std::numeric_limits<quint64>::max)()) {
                    ++it->suppressedCount;
                }
                return {false, 0};
            }

            const quint64 suppressedCount = it->suppressedCount;
            it->suppressedCount = 0;
            it->lastLogMs = nowMs;
            return {true, suppressedCount};
        }

        // Codes should be fixed literals. Bound retention even if a caller
        // accidentally supplies dynamic codes instead of putting details in message.
        if (m_states.size() >= kMaxStates) {
            auto oldest = m_states.begin();
            for (auto candidate = m_states.begin(); candidate != m_states.end(); ++candidate) {
                if (candidate->lastSeenMs < oldest->lastSeenMs) {
                    oldest = candidate;
                }
            }
            m_states.erase(oldest);
        }
        m_states.insert(key, {nowMs, nowMs, 0});
        return {true, 0};
    }

    int stateCount() const
    {
        QMutexLocker locker(&m_mutex);
        return m_states.size();
    }

private:
    struct State {
        qint64 lastLogMs;
        qint64 lastSeenMs;
        quint64 suppressedCount;
    };

    mutable QMutex m_mutex;
    QHash<Key, State> m_states;
};

} // namespace ErrorLogInternal

#endif // ERRORLOGLIMITER_H
