#pragma once

#include "Racing/RaceProgress.h"

namespace latebrake
{
// The race suspension and the need to ask Unreal to pause the world are
// different decisions. Even if Unreal has already paused, a NEW focus-loss
// transition must invalidate checkpoint motion from the prior session.
enum class FocusLossTransition
{
    Ignored,
    SuspendedAlreadyPaused,
    SuspendedNeedsWorldPause
};

// Explicit focus-loss suspension for racing. OS pausing can happen before
// the focus delegate fires, and SetGamePaused can fail entirely. This latch
// independently blocks timing, gates, recoveries and autonomous rivals until
// the player intentionally resumes/restarts; foreground alone never clears it.
class AppFocusRaceGuard final
{
public:
    FocusLossTransition OnFocusLost(const RaceProgress& race, bool menuOpen,
                                   bool worldPaused) noexcept
    {
        if (Suspended || menuOpen || race.IsFinished())
        {
            return FocusLossTransition::Ignored;
        }
        Suspended = true;
        return worldPaused ? FocusLossTransition::SuspendedAlreadyPaused
                           : FocusLossTransition::SuspendedNeedsWorldPause;
    }

    bool IsSuspended() const noexcept { return Suspended; }

    // Only a deliberate player return to gameplay may clear the latch.
    // OS foreground and background delegates must not call this.
    void OnExplicitResume() noexcept { Suspended = false; }

private:
    bool Suspended = false;
};
} // namespace latebrake
