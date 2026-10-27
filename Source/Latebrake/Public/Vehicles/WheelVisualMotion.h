#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Decorative-only wheel animation. It is NOT a Chaos wheeled-vehicle
// simulation: the four cylinders remain non-colliding chassis children.
// Derived only from actual rigid-body forward speed and steering command;
// this policy never changes forces, grip, vehicle position or race timing.
struct WheelVisualSample final
{
    double DeltaSeconds = 0.0;
    double ForwardSpeedCmPerSecond = 0.0; // signed, reverse spins backward
    double SteeringInput = 0.0;          // already filtered, [-1,+1]
    double WheelRadiusCm = 29.0;        // prototype decorative cylinder radius
};

struct WheelVisualPose final
{
    double RollDegrees = 0.0;
    double FrontSteerDegrees = 0.0;
};

// Fixed-size, allocation-free and engine-independent. No position
// integration across pause, teleports, hitch or invalid physics samples.
class WheelVisualMotion final
{
public:
    static constexpr double MaxDeltaSeconds = 0.25;
    static constexpr double MaxForwardSpeedCmPerSecond = 15000.0;
    static constexpr double MaxSteerDegrees = 30.0;
    static constexpr double MinRadiusCm = 5.0;
    static constexpr double MaxRadiusCm = 100.0;

    void Reset() noexcept
    {
        Pose = {};
    }

    // Preserve valid wheel phase when suspending (avoids a visible jump),
    // but neutralize the steering display. Recovery/restart uses Reset().
    void Suspend() noexcept
    {
        Pose.FrontSteerDegrees = 0.0;
    }

    bool Advance(WheelVisualSample sample) noexcept
    {
        if (!std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 ||
            sample.DeltaSeconds > MaxDeltaSeconds ||
            !std::isfinite(sample.ForwardSpeedCmPerSecond) ||
            std::abs(sample.ForwardSpeedCmPerSecond) >
                MaxForwardSpeedCmPerSecond ||
            !std::isfinite(sample.SteeringInput) ||
            sample.SteeringInput < -1.0 ||
            sample.SteeringInput > 1.0 ||
            !std::isfinite(sample.WheelRadiusCm) ||
            sample.WheelRadiusCm < MinRadiusCm ||
            sample.WheelRadiusCm > MaxRadiusCm)
        {
            Suspend();
            return false;
        }

        constexpr double Pi = 3.1415926535897932384626433832795;
        const double deltaDegrees =
            (sample.ForwardSpeedCmPerSecond * sample.DeltaSeconds) /
            sample.WheelRadiusCm * (180.0 / Pi);
        if (!std::isfinite(deltaDegrees))
        {
            Suspend();
            return false;
        }

        // Keep the angle tightly bounded over indefinitely long sessions.
        // Cyclic [-180, +180] wrapping changes no visible wheel pose.
        const double wrapped = std::remainder(
            Pose.RollDegrees + deltaDegrees, 360.0);
        if (!std::isfinite(wrapped))
        {
            Suspend();
            return false;
        }

        Pose.RollDegrees = wrapped;
        Pose.FrontSteerDegrees = sample.SteeringInput * MaxSteerDegrees;
        return true;
    }

    WheelVisualPose Current() const noexcept { return Pose; }

private:
    WheelVisualPose Pose;
};
} // namespace latebrake
