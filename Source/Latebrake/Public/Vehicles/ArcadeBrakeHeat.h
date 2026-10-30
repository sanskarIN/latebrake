#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// An arcade approximation of brake heating based on chassis speed and
// service-brake demand. This does not simulate wheel temperatures, ABS,
// brake hydraulic pressure, or physically model rotor/pad materials.
struct BrakeHeatSample final
{
    double DeltaSeconds = 0.0;
    double ForwardSpeedCmPerSecond = 0.0;
    bool ServiceBrakeHeld = false;
    bool DrivingAllowed = false;
};

struct BrakeHeatOutput final
{
    bool Valid = false;
    double HeatFraction = 0.20;
    double ServiceBrakeMultiplier = 1.0;
    bool Faded = false;
};

class ArcadeBrakeHeat final
{
public:
    static constexpr double IdleHeat = 0.20;
    static constexpr double MaxFrameSeconds = 0.10;
    static constexpr double MaxSpeedCmPerSecond = 10000.0;
    static constexpr double FadeThreshold = 0.62;
    static constexpr double MinimumBrakeMultiplier = 0.74;

    void Reset() noexcept
    {
        Heat = IdleHeat;
        Valid = false;
    }

    // Pause and focus loss freeze thermal history but never allow a stale
    // reading to affect the next non-driving frame's brake forces.
    void Suspend() noexcept { Valid = false; }

    BrakeHeatOutput Observe(BrakeHeatSample sample) noexcept
    {
        if (!sample.DrivingAllowed || !std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 ||
            sample.DeltaSeconds > MaxFrameSeconds ||
            !std::isfinite(sample.ForwardSpeedCmPerSecond) ||
            std::abs(sample.ForwardSpeedCmPerSecond) > MaxSpeedCmPerSecond)
        {
            Suspend();
            return {false, Heat, 1.0, false};
        }

        const double speed = std::abs(sample.ForwardSpeedCmPerSecond);
        const double speedFraction = std::clamp(speed / 5000.0, 0.0, 1.0);
        // Heating requires a genuinely moving car and a service-brake
        // request; a handbrake-only slide must not heat the service brakes.
        const double heating = sample.ServiceBrakeHeld && speed > 400.0
            ? 0.115 * speedFraction : 0.0;
        // Continuous airflow cools faster than idling, but cooling cannot
        // reduce the synthetic heat state below its fixed ambient floor.
        const double cooling = (0.040 + 0.040 * speedFraction) *
            (Heat - IdleHeat);
        Heat = std::clamp(
            Heat + sample.DeltaSeconds * (heating - cooling),
            IdleHeat, 1.0);
        Valid = true;
        const double multiplier = ComputeServiceBrakeMultiplier();
        return {true, Heat, multiplier, multiplier < 1.0};
    }

    bool HasValidSample() const noexcept { return Valid; }
    double HeatFraction() const noexcept { return Heat; }
    double ServiceBrakeMultiplier() const noexcept
    {
        return Valid ? ComputeServiceBrakeMultiplier() : 1.0;
    }

private:
    double ComputeServiceBrakeMultiplier() const noexcept
    {
        const double fade = std::clamp(
            (Heat - FadeThreshold) / (1.0 - FadeThreshold), 0.0, 1.0);
        return 1.0 - (1.0 - MinimumBrakeMultiplier) * fade;
    }

    double Heat = IdleHeat;
    bool Valid = false;
};
} // namespace latebrake
