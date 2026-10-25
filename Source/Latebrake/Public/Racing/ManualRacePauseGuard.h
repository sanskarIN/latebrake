#pragma once

namespace latebrake
{
// A standalone keyboard/gamepad/touch PAUSE press is an authoritative
// gameplay transition even when Unreal rejects SetGamePaused(true).
// Unlike UI menu state or OS deactivation, this intent is controlled solely
// by an explicit player PAUSE/RESUME, menu return or accepted restart.
class ManualRacePauseGuard final
{
public:
    // Returns the desired Unreal world pause state. In particular, a FIRST
    // pause press latches an independent gate BEFORE trying SetGamePaused.
    // A second press clears the fallback latch and requests world unpause.
    bool OnToggleRequested(bool worldPaused) noexcept
    {
        if (Requested || worldPaused)
        {
            Requested = false;
            return false;
        }
        Requested = true;
        return true;
    }

    bool IsSuspended() const noexcept { return Requested; }

    // Calling this for an actual user menu return or accepted race restart
    // is okay; never call it automatically on app foreground notifications.
    void OnExplicitResume() noexcept { Requested = false; }

private:
    bool Requested = false;
};
} // namespace latebrake
