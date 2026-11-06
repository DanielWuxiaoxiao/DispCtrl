#ifndef SYSTEMTIMESYNC_H
#define SYSTEMTIMESYNC_H

enum class SystemTimeSyncState {
    Idle,
    Waiting,
    Succeeded,
    Failed,
    TimedOut,
    Unavailable
};

#endif
