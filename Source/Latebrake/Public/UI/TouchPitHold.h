#pragma once

#include "UI/TouchControlLayout.h"
#include "UI/TouchFingerRange.h"

namespace latebrake
{
// A separate held-action finger, independent of steering/pedal ownership.
// No autonomous re-acquisition on move: a fresh physical press is required
// after leave/cancel/release, viewport change, pause or focus interruption.
// This class does not authorize a service; RacePitService still verifies
// actual chassis position, velocity, throttle, nitro and race state.
class TouchPitHold final
{
public:
    bool Begin(int finger, TouchControl region, bool eligible) noexcept
    {
        if (Owner != -1 || !IsTrackableTouchFinger(finger) ||
            region != TouchControl::PitService || !eligible)
        {
            return false;
        }
        Owner = finger;
        return true;
    }

    void Move(int finger, TouchControl region, bool eligible) noexcept
    {
        if (finger == Owner &&
            (!eligible || region != TouchControl::PitService))
        {
            Owner = -1;
        }
    }

    void Release(int finger) noexcept
    {
        if (finger == Owner)
        {
            Owner = -1;
        }
    }

    void Clear() noexcept { Owner = -1; }
    bool IsHeld() const noexcept { return Owner != -1; }
    int FingerOwner() const noexcept { return Owner; }

private:
    int Owner = -1;
};
} // namespace latebrake
