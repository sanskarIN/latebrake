#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>

namespace latebrake
{
// Decorative-only wheel animation. It is NOT a Chaos wheeled-vehicle
// simulation: the four cylinders remain non-colliding chassis children.
// Coasting derives from actual forward speed and steering; explicitly
// committed brake-model angular deltas can correct individual phases.
// This policy never changes forces, grip, vehicle position or race timing.
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
        CornerRollDegrees = {};
        HasFreshAdvance = false;
    }

    // Preserve valid wheel phase when suspending (avoids a visible jump),
    // but neutralize the steering display. Recovery/restart uses Reset().
    void Suspend() noexcept
    {
        Pose.FrontSteerDegrees = 0.0;
        HasFreshAdvance = false;
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

        std::array<double, 4> nextCorners{};
        for (unsigned i = 0; i < nextCorners.size(); ++i)
            nextCorners[i] = std::remainder(CornerRollDegrees[i] + deltaDegrees, 360.0);
        CornerRollDegrees = nextCorners;
        LastDeltaDegrees = deltaDegrees;
        HasFreshAdvance = true;
        Pose.RollDegrees = wrapped;
        Pose.FrontSteerDegrees = sample.SteeringInput * MaxSteerDegrees;
        return true;
    }

    WheelVisualPose Current() const noexcept { return Pose; }

    WheelVisualPose CurrentCorner(unsigned corner) const noexcept
    {
        if (corner >= CornerRollDegrees.size()) return {};
        return {CornerRollDegrees[corner], corner < 2 ? Pose.FrontSteerDegrees : 0.0};
    }

    // Correct ONLY this frame's coasting increment after the actual wheel
    // model delivery receipt was committed. The angular delta is already
    // integrated over internal rotor steps; last-step RPM is insufficient.
    // No contact, force, timing or save state can be authored by this API.
    bool ApplyCommittedBrakeRotation(std::uint64_t receipt, unsigned mask,
                                    const std::array<double, 4>& radians) noexcept
    {
        if (!HasFreshAdvance || receipt == 0 || receipt <= LastBrakeReceipt ||
            mask == 0 || mask > 15u) return false;
        for (unsigned i = 0; i < radians.size(); ++i)
            if ((mask & (1u << i)) &&
                (!std::isfinite(radians[i]) || std::abs(radians[i]) > 2500.0))
                return false;
        constexpr double Degrees = 180.0 / 3.1415926535897932384626433832795;
        auto next = CornerRollDegrees;
        for (unsigned i = 0; i < radians.size(); ++i)
            if (mask & (1u << i))
                next[i] = std::remainder(next[i] - LastDeltaDegrees + radians[i] * Degrees, 360.0);
        CornerRollDegrees = next;
        LastBrakeReceipt = receipt;
        HasFreshAdvance = false;
        return true;
    }

private:
    WheelVisualPose Pose;
    std::array<double, 4> CornerRollDegrees{};
    double LastDeltaDegrees = 0.0;
    std::uint64_t LastBrakeReceipt = 0;
    bool HasFreshAdvance = false;
};
} // namespace latebrake
