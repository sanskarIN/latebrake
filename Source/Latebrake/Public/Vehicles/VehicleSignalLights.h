#pragma once

#include <cmath>

namespace latebrake
{
// Engine-independent *cosmetic* lighting only, not real road lighting,
// human driving advice, braking forces, AI state or race authority.
struct VehicleSignalSample final
{
    bool RaceDrivingAllowed = false;
    bool EffectiveBrakeHeld = false;
    double SignedForwardSpeedCmPerSecond = 0.0;
};

struct VehicleSignalLights final
{
    double RedRearIntensity = 0.0;
    double WhiteReverseIntensity = 0.0;
    bool BrakeIlluminated = false;
    bool ReverseIlluminated = false;
};

inline VehicleSignalLights SelectVehicleSignalLights(
    VehicleSignalSample sample) noexcept
{
    constexpr double MaxObservedSpeedCmPerSecond = 15000.0;
    constexpr double ReverseThresholdCmPerSecond = -130.0;
    constexpr double TailIntensity = 16.0;
    constexpr double BrakeIntensity = 95.0;
    constexpr double ReverseIntensity = 70.0;

    // No light state may be derived from an absent/malformed rigid body
    // or while gameplay is blocked by pause, focus loss or race readiness.
    if (!sample.RaceDrivingAllowed ||
        !std::isfinite(sample.SignedForwardSpeedCmPerSecond) ||
        std::abs(sample.SignedForwardSpeedCmPerSecond) >
            MaxObservedSpeedCmPerSecond)
    {
        return {};
    }

    const bool reverse =
        sample.SignedForwardSpeedCmPerSecond <= ReverseThresholdCmPerSecond;
    return {
        sample.EffectiveBrakeHeld ? BrakeIntensity : TailIntensity,
        reverse ? ReverseIntensity : 0.0,
        sample.EffectiveBrakeHeld,
        reverse
    };
}
} // namespace latebrake
