#pragma once

#include "UI/TouchDriveState.h"
#include "UI/TouchPressTracker.h"

#include <cmath>

namespace latebrake
{
// The engine-facing Pressed callback must not return on a duplicate before
// checking for corrupt coordinates. Otherwise TouchDriveState::Press()
// correctly handles NaN/Inf in isolation, but is never reached by real input.
//
// A *new* press is forwarded to ApplyTouch, which observes viewport state,
// rejects malformed coordinates, and handles menu/gameplay action routing.
// Duplicate presses never fire discrete actions or reacquire driving pads.
enum class TouchPressAdmission
{
    NewPress,
    Duplicate,
    MalformedDuplicate,
    InvalidFinger
};

inline TouchPressAdmission AdmitTouchPress(
    TouchPressTracker& presses, TouchDriveState& drive,
    int finger, double x, double y) noexcept
{
    if (!IsTrackableTouchFinger(finger))
    {
        return TouchPressAdmission::InvalidFinger;
    }

    if (presses.Press(finger))
    {
        // Preserve the original viewport inspection on a first press,
        // including a malformed first press. ApplyTouch cancels it.
        return TouchPressAdmission::NewPress;
    }

    if (!std::isfinite(x) || !std::isfinite(y))
    {
        // This is an already physically held finger. A corrupt repeated
        // notification must neutralize only its prior driving ownership.
        // Keep it physically down until a genuine Release or OS reset.
        drive.Cancel(finger);
        return TouchPressAdmission::MalformedDuplicate;
    }

    return TouchPressAdmission::Duplicate;
}
} // namespace latebrake
