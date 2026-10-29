#ifndef HEALTHSTATEWINDOW_H
#define HEALTHSTATEWINDOW_H
#include "Basic/Protocol.h"
#pragma pack(push, 8)
#include <array>

// GUI-thread log gate only; health snapshots and fault detection remain at 5 seconds.
class HealthFaultLogGate {
public:
    bool sample(std::size_t slot, quint64 faults, qint64 nowMs) {
        if (slot >= m_slots.size()) return false;
        auto& state = m_slots[slot];
        if (!faults) { state.lastFaults = 0; return false; }
        const qint64 interval = faults == state.lastFaults ? 300000 : 30000;
        if (state.logged && nowMs - state.lastMs < interval) return false;
        state = {faults, nowMs, true};
        return true;
    }
private:
    struct Slot { quint64 lastFaults = 0; qint64 lastMs = 0; bool logged = false; };
    std::array<Slot, 5> m_slots{}; // Four panels and software, no growing cache.
};

// Fixed-size aggregation only: no frame history, queues, timestamps, or per-frame allocations.
class HealthStateWindow {
public:
    static constexpr int IntervalMs = 5000;
    static constexpr int ZeroThreshold = 1000;
    struct BitSnapshot {
        BITReport report;
        bool powerConfirmed = false;
        int consecutiveZeroFrames = 0;
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
        bool allZero = true;
        for (int i = 0; i < 5; ++i) allZero = allZero && ((report.subArrayPower[i] & mask(i)) == 0);
        if (allZero) {
            if (p.zeroFrames < ZeroThreshold) ++p.zeroFrames;
            if (p.zeroFrames >= ZeroThreshold) {
                p.confirmedPower.fill(0);
                p.hasConfirmedPower = true;
            }
        } else {
            p.zeroFrames = 0;
            for (int i = 0; i < 5; ++i) p.confirmedPower[i] = report.subArrayPower[i] & mask(i);
            p.hasConfirmedPower = true;
        }
        if (p.hasConfirmedPower) {
            if (!p.windowPowerValid) p.windowPower = p.confirmedPower;
            else for (int i = 0; i < 5; ++i) p.windowPower[i] &= p.confirmedPower[i];
            p.windowPowerValid = true;
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
        snapshot.powerConfirmed = p.windowPowerValid;
        snapshot.consecutiveZeroFrames = p.zeroFrames;
        for (int i = 0; i < 5; ++i) snapshot.report.subArrayPower[i] = p.windowPowerValid ? p.windowPower[i] : 0;
        p.pending = false;
        p.windowPowerValid = false;
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
        std::array<unsigned char, 5> confirmedPower{}, windowPower{};
        bool hasConfirmedPower = false, windowPowerValid = false, pending = false;
        int zeroFrames = 0; // Carries across 5-second windows, saturates at 1000.
        unsigned char alarms = 0, normalBits = 0x0F, normalPower = 0x03;
    };
    std::array<Panel, 4> m_panels{};
    MonitorParam m_software;
    bool m_softwarePending = false;
};
#pragma pack(pop)
#endif
