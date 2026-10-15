#ifndef DIAGNOSTICLOGWINDOW_H
#define DIAGNOSTICLOGWINDOW_H

#include <cstdint>
#include <limits>

// One fixed diagnostic category; no allocation or message-dependent cache keys.
// The caller owns the clock and serializes access (Controller uses the GUI thread).
class DiagnosticLogWindow {
public:
    struct Decision {
        bool write = false;
        std::uint64_t suppressed = 0;
    };

    Decision sample(std::int64_t nowMs, std::int64_t intervalMs)
    {
        if (!m_started || intervalMs <= 0 || nowMs < m_lastMs
            || nowMs - m_lastMs >= intervalMs) {
            const Decision decision{true, m_suppressed};
            m_started = true;
            m_lastMs = nowMs;
            m_suppressed = 0;
            return decision;
        }
        if (m_suppressed != (std::numeric_limits<std::uint64_t>::max)()) {
            ++m_suppressed;
        }
        return {};
    }

private:
    std::int64_t m_lastMs = 0;
    std::uint64_t m_suppressed = 0;
    bool m_started = false;
};

#endif // DIAGNOSTICLOGWINDOW_H
