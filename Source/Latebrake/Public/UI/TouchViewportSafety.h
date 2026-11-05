#pragma once

#include "UI/TouchDriveState.h"
#include "UI/TouchViewportGuard.h"

namespace latebrake
{
// A viewport resize must cancel the *actual physical drive command*,
// not only the finger capture. Throttle/steering are rate-smoothed for
// normal driving; leaving their previous value intact would keep applying
// motor force / steering torque for multiple frames after the old GAS or
// STEER finger had been invalidated by an orientation change.
//
// Return true when the view was invalidated. The caller may still need
// to suppress a touch being acquired on that particular event.
// The independent TouchPressTracker must NOT be reset: physical fingers
// remain down until a real release event.
// A new physical touch can be the FIRST event to observe a rotation,
// inset change or split-screen resize. In that event even a screen-space
// PAUSE, RESTART, RECOVER or menu rectangle is not trustworthy yet:
// coordinate mapping just changed beneath the finger. Clear drive output
// through NeutralizeTouchOnViewportChange, then discard that *new* action
// without resetting TouchPressTracker. It remains physically down until
// the OS reports release, so a repeated IE_Pressed cannot replay it.
// Later fresh presses on the stable viewport work normally; movement
// alone was never eligible to fire a discrete action.
inline constexpr bool SuppressFreshTouchOnInvalidatedViewport(
    bool freshPress, bool viewportInvalidated) noexcept
{
    return freshPress && viewportInvalidated;
}

inline bool NeutralizeTouchOnViewportChange(TouchViewportGuard& viewport,
                                             TouchDriveState& touch,
                                             float& smoothedThrottle,
                                             float& smoothedSteering,
                                             int width, int height) noexcept
{
    const TouchViewportStatus status = viewport.Observe(width, height);
    if (!TouchViewportNeedsCancellation(status))
    {
        return false;
    }
    touch.Clear();
    smoothedThrottle = 0.f;
    smoothedSteering = 0.f;
    return true;
}
} // namespace latebrake
