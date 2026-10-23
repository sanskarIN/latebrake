#pragma once

#include "UI/TouchControlLayout.h"
#include "UI/TouchFingerRange.h"
#include <cmath>

namespace latebrake
{
// Portable state machine for the *held driving fingers* only. Discrete menu
// and race-action taps remain owned by the Unreal pawn. Every touch press,
// move, release, pause and lifecycle interruption routes through this state.
class TouchDriveState final
{
public:
    enum class Pedal { Neutral, Throttle, Brake };

    void Clear() noexcept
    {
        SteeringFinger = -1;
        PedalFinger = -1;
        SteeringAxis = 0.0;
        PedalState = Pedal::Neutral;
    }

    // Presses starting outside their assigned pads cannot later acquire a
    // driving control by dragging into it. Only one steering and one pedal
    // finger can own inputs at a time.
    void Press(const TouchControlLayout& layout, int finger, double x, double y) noexcept
    {
        if (!layout.Valid)
        {
            // Screen rotation/resizing invalidates ALL held controls even
            // if the first event delivered during transition is a press.
            // Never leave an old throttle active just because this new
            // finger could not acquire a control.
            Clear();
            return;
        }
        if (!IsTrackableTouchFinger(finger) ||
            !std::isfinite(x) || !std::isfinite(y) ||
            finger == SteeringFinger || finger == PedalFinger)
        {
            return;
        }
        const TouchControl control = HitTestTouchControl(layout, x, y);
        if (control == TouchControl::Steering && SteeringFinger < 0)
        {
            SteeringFinger = finger;
            SteeringAxis = TouchSteeringValue(layout, x);
        }
        else if ((control == TouchControl::Throttle || control == TouchControl::Brake) &&
                 PedalFinger < 0)
        {
            PedalFinger = finger;
            PedalState = control == TouchControl::Throttle
                ? Pedal::Throttle : Pedal::Brake;
        }
    }

    void Move(const TouchControlLayout& layout, int finger, double x, double y) noexcept
    {
        if (finger < 0) return;
        if (!layout.Valid)
        {
            // A global viewport interruption invalidates BOTH fingers,
            // even if only one generates a move after rotation or resize.
            Clear();
            return;
        }
        // Keep global viewport invalidation before rejecting positive stray
        // ids, but never let an untrackable finger move an active control.
        if (!IsTrackableTouchFinger(finger)) return;
        if (!std::isfinite(x) || !std::isfinite(y))
        {
            Cancel(finger);
            return;
        }
        if (finger == SteeringFinger)
        {
            // Horizontal drags may extend past the edges of the steering pad
            // (clamped to full lock). Leaving its vertical band is neutral,
            // so pulling a finger into a menu area cannot keep turning.
            if (y < layout.Steering.Y || y > layout.Steering.Y + layout.Steering.Height)
            {
                SteeringAxis = 0.0;
            }
            else
            {
                SteeringAxis = TouchSteeringValue(layout, x);
            }
            return;
        }
        if (finger == PedalFinger)
        {
            const TouchControl control = HitTestTouchControl(layout, x, y);
            // The same owned finger can slide gas <-> neutral <-> brake.
            // It cannot trigger a discrete race or menu action on drag.
            PedalState = control == TouchControl::Throttle ? Pedal::Throttle :
                         control == TouchControl::Brake ? Pedal::Brake : Pedal::Neutral;
        }
    }

    void Release(int finger) noexcept { Cancel(finger); }

    void Cancel(int finger) noexcept
    {
        if (finger == SteeringFinger)
        {
            SteeringFinger = -1;
            SteeringAxis = 0.0;
        }
        if (finger == PedalFinger)
        {
            PedalFinger = -1;
            PedalState = Pedal::Neutral;
        }
    }

    bool SteeringActive() const noexcept { return SteeringFinger >= 0; }
    bool ThrottleActive() const noexcept { return PedalState == Pedal::Throttle; }
    bool BrakeActive() const noexcept { return PedalState == Pedal::Brake; }
    double Steering() const noexcept { return SteeringAxis; }
    int SteeringOwner() const noexcept { return SteeringFinger; }
    int PedalOwner() const noexcept { return PedalFinger; }

private:
    int SteeringFinger = -1;
    int PedalFinger = -1;
    double SteeringAxis = 0.0;
    Pedal PedalState = Pedal::Neutral;
};
} // namespace latebrake
