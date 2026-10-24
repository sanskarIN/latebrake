#pragma once

#include "Racing/RaceProgress.h"

namespace latebrake
{
// Explicit focus-loss suspension for racing. A platform may continue
// ticking after the app deactivates, and SetGamePaused can fail. The latch
// therefore blocks timing, gates, recoveries and the rival independently
// until the player EXPLICITLY resumes or restarts. Foreground notification
// alone never authorizes an automatic restart of competitive timing.
class AppFocusRaceGuard final
{
public:
    // Latch even when the engine already paused: some platforms may
    // automatically resume on foreground. Return true only when a NEW
    // world-pause request and checkpoint-epoch invalidation are needed.
    bool OnFocusLost(const RaceProgress& race, bool menuOpen,
                     bool worldPaused) noexcept
    {
        if (Suspended || menuOpen || race.IsFinished())
        {
            return false;
        }
        Suspended = true;
        return !worldPaused;
    }

    bool IsSuspended() const noexcept { return Suspended; }

    // Only an intentional player action that returns to gameplay may
    // clear this latch. OS foreground/background delegates must not.
    void OnExplicitResume() noexcept { Suspended = false; }

private:
    bool Suspended = false;
};
} // namespace latebrake
