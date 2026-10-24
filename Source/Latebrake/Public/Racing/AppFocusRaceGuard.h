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
    // True exactly once per racing interruption: caller should attempt an
    // Unreal world pause and invalidate checkpoint motion baselines.
    bool OnFocusLost(const RaceProgress& race, bool menuOpen,
                     bool worldPaused) noexcept
    {
        if (Suspended || menuOpen || worldPaused || race.IsFinished())
        {
            return false;
        }
        Suspended = true;
        return true;
    }

    bool IsSuspended() const noexcept { return Suspended; }

    // Only an intentional player action that returns to gameplay may
    // clear this latch. OS foreground/background delegates must not.
    void OnExplicitResume() noexcept { Suspended = false; }

private:
    bool Suspended = false;
};
} // namespace latebrake
