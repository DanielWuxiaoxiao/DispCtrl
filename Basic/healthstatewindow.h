#ifndef HEALTHSTATEWINDOW_H
#define HEALTHSTATEWINDOW_H
#include "Basic/Protocol.h"
#pragma pack(push, 8)
#include <array>
#include <limits>

// GUI-thread log gate only; health snapshots and fault detection remain at 5 seconds.
class HealthFaultLogGate {
public:
    bool sample(std::size_t slot, quint64 faults, qint64 nowMs) {
        if (slot >= m_slots.size()) return false;
        auto& state = m_slots[slot];
        if (!faults) {
            // Missing reports are not recovery; require consecutive normal snapshots.
            if (state.normalSinceMs < 0 || nowMs - state.lastNormalMs > 10000)
                state.normalSinceMs = nowMs;
            state.lastNormalMs = nowMs;
            if (nowMs - state.normalSinceMs >= 60000) state.logged = false;
            return false;
        }
        state.normalSinceMs = -1;
        // No periodic reminders for a persistent fault. Changing power masks must
        // not turn every five-second health window into another log entry.
        if (state.logged && (faults == state.lastFaults || nowMs - state.lastMs < 300000))
            return false;
        state.lastFaults = faults;
        state.lastMs = nowMs;
        state.logged = true;
        return true;
    }
private:
    struct Slot {
        quint64 lastFaults = 0;
        qint64 lastMs = 0;
        qint64 normalSinceMs = -1;
        qint64 lastNormalMs = 0;
        bool logged = false;
    };
    std::array<Slot, 5> m_slots{}; // Four panels and software, no growing cache.
};

// Fixed-size aggregation only: no frame history, queues, timestamps, or per-frame allocations.
class HealthStateWindow {
public:
    static constexpr int IntervalMs = 5000;
    struct BitSnapshot {
        BITReport report;
        bool powerConfirmed = false;
        quint32 powerSampleCount = 0;
    };
    void pushBit(const BITReport& report) {
        if (report.mesID != 0xDE02 || report.radarId > RADAR_ID_MAX) return;
        auto& p = m_panels[report.radarId];
        p.latest.radarId = report.radarId;
        p.latest.fpgaTemp = report.fpgaTemp;
        p.latest.panelTemp = report.panelTemp;
        if (report.yaw <= 36000) p.latest.yaw = report.yaw;
        if (report.scanAngle <= 36000) p.latest.scanAngle = report.scanAngle;
        p.latest.bitGroup = report.bitGroup;
        p.latest.powerState = report.powerState;
        if (!p.pending) {
            p.alarms = 0;
            p.normalBits = 0x0F;
            p.normalPower = 0x03;
        }
        p.pending = true;
        p.alarms |= report.bitGroup & 0x60; // duty/pulse alarms: 1=fault
        p.normalBits &= report.bitGroup & 0x0F; // hardware status: 0=fault
        p.normalPower &= report.powerState & 0x03;
        // Each valid BIT contains all 36 channels. Count frames, not elapsed time
        // or fault runs; the five-second timer closes this non-overlapping window.
        if (p.powerSamples < std::numeric_limits<quint32>::max()) {
            ++p.powerSamples;
            for (int i = 0; i < 36; ++i)
                if (!(report.subArrayPower[i / 8] & (1U << (i % 8)))) ++p.powerFaults[i];
        }
    }
    bool takeBit(int panelId, BitSnapshot& snapshot) {
        if (panelId < 0 || panelId >= 4) return false;
        auto& p = m_panels[panelId];
        if (!p.pending) return false; // No packet is not proof of recovery.
        snapshot.report = p.latest;
        // TX/RX enable bits are latest switches, not latched alarms.
        snapshot.report.bitGroup = (p.latest.bitGroup & 0x90) | p.alarms | p.normalBits;
        snapshot.report.powerState = (p.latest.powerState & 0xFC) | p.normalPower;
        snapshot.powerConfirmed = p.powerSamples > 0;
        snapshot.powerSampleCount = p.powerSamples;
        for (auto& byte : snapshot.report.subArrayPower) byte = 0;
        for (int i = 0; i < 36; ++i) {
            // Strictly greater than 90%; 9/10 faults is still green. Widen before multiplication.
            const bool fault = quint64(p.powerFaults[i]) * 10 > quint64(p.powerSamples) * 9;
            if (!fault) snapshot.report.subArrayPower[i / 8] |= (1U << (i % 8));
        }
        p.pending = false;
        p.powerSamples = 0;
        p.powerFaults.fill(0);
        return true;
    }
    static bool bitFault(const BitSnapshot& s) {
        if ((s.report.bitGroup & 0x60) || (s.report.bitGroup & 0x0F) != 0x0F
            || (s.report.powerState & 0x03) != 0x03) return true;
        if (s.powerConfirmed) for (int i = 0; i < 5; ++i)
            if ((s.report.subArrayPower[i] & mask(i)) != mask(i)) return true;
        return false;
    }
    static bool softwareFault(unsigned char state) { return state != 0 && state != 2 && state != 4; }
    void pushSoftware(const MonitorParam& report) {
        if (report.mesID != 0xCF01) return;
        merge(m_software.sigProSta, report.sigProSta);
        merge(m_software.dataProSta, report.dataProSta);
        merge(m_software.beamConSta, report.beamConSta);
        merge(m_software.targetRecSta, report.targetRecSta);
        m_softwarePending = true;
    }
    bool takeSoftware(MonitorParam& result) {
        if (!m_softwarePending) return false;
        result = m_software;
        m_softwarePending = false;
        return true;
    }
private:
    static unsigned char mask(int byte) { return byte == 4 ? 0x0F : 0xFF; }
    void merge(unsigned char& current, unsigned char incoming) {
        if (incoming > 5) incoming = 1;
        if (!m_softwarePending || softwareFault(incoming) || !softwareFault(current)) current = incoming;
    }
    struct Panel {
        BITReport latest;
        std::array<quint32, 36> powerFaults{};
        quint32 powerSamples = 0;
        bool pending = false;
        unsigned char alarms = 0, normalBits = 0x0F, normalPower = 0x03;
    };
    std::array<Panel, 4> m_panels{};
    MonitorParam m_software;
    bool m_softwarePending = false;
};
#pragma pack(pop)
#endif
