#pragma once

#include "UI/TouchDriveState.h"
#include <cmath>

namespace latebrake
{
// The pawn applies rate-smoothed commands each physics tick. Removing the
// TouchDrive owner is not enough: stale smoothed throttle still applies
// motor force, and stale steering still applies torque after a release or
// invalid touch coordinate. Cancel only the output owned by THIS finger.
// Physical-down touch tracking is intentionally separate and must only
// be released by an actual OS release or application focus reset.
inline void CancelTouchOwnerAndOutput(
    TouchDriveState& drive, int finger,
    float& smoothedThrottle, float& smoothedSteering) noexcept
{
    const bool wasPedalOwner = drive.PedalOwner() == finger;
    const bool wasSteeringOwner = drive.SteeringOwner() == finger;
    drive.Cancel(finger);
    if (wasPedalOwner) smoothedThrottle = 0.f;
    if (wasSteeringOwner) smoothedSteering = 0.f;
}

// An owned pedal sliding off GAS into the neutral gap must stop applying
// acceleration immediately, not merely decelerate the smoothing filter.
// Moving back into GAS still requires the original physical finger to be
// down and owned by the pedal. Steering can smoothly change INSIDE its
// band; exiting that band is a cancellation of the applied steering torque.
inline void MoveTouchWithResidualSafety(
    TouchDriveState& drive, const TouchControlLayout& layout,
    int finger, double x, double y,
    float& smoothedThrottle, float& smoothedSteering) noexcept
{
    const bool wasThrottleOwner =
        drive.PedalOwner() == finger && drive.ThrottleActive();
    const bool wasSteeringOwner = drive.SteeringOwner() == finger;
    // A caller may pass an invalid viewport. TouchDriveState clears all
    // owners in that case, so zero both residual outputs as well.
    if (!layout.Valid)
    {
        drive.Clear();
        smoothedThrottle = 0.f;
        smoothedSteering = 0.f;
        return;
    }
    // A corrupt coordinate cancels its own owner just like a malformed
    // first-press or release; unowned fingers cannot alter other controls.
    if (!std::isfinite(x) || !std::isfinite(y))
    {
        CancelTouchOwnerAndOutput(
            drive, finger, smoothedThrottle, smoothedSteering);
        return;
    }

    drive.Move(layout, finger, x, y);
    if (wasThrottleOwner && !drive.ThrottleActive())
    {
        smoothedThrottle = 0.f;
    }
    // The new center noise band must remove already-smoothed turn torque
    // immediately. Otherwise a recent full-lock sample could keep turning
    // after the thumb reaches straight-ahead. Preserve gas ownership.
    // Moving back outside the band resumes ordinary steering smoothing.
    if (wasSteeringOwner && drive.Steering() == 0.0)
    {
        smoothedSteering = 0.f;
    }
}
} // namespace latebrake
