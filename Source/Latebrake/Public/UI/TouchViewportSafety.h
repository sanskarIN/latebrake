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
