#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Chassis-slip-based arcade approximation for the existing rigid body;
// NOT real tire compounds, wear sensors, or measured tire temperature.
struct TireConditionSample final
{
    double DeltaSeconds = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double LateralSpeedCmPerSecond = 0.0;
    bool Braking = false;
    bool DrivingAllowed = false;
};

class ArcadeTireCondition final
{
public:
    static constexpr double MaxFrameSeconds = 0.10;
    static constexpr double MaxSpeedCmPerSecond = 10000.0;
    static constexpr double MinimumGripMultiplier = 0.68;

    void Reset() noexcept
    {
        Temperature = 0.42;
        Wear = 0.0;
        Valid = false;
    }
    // An interruption freezes condition, preserving this race's wear.
    void Suspend() noexcept { Valid = false; }

    bool Observe(TireConditionSample sample) noexcept
    {
        if (!sample.DrivingAllowed || !std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 || sample.DeltaSeconds > MaxFrameSeconds ||
            !std::isfinite(sample.PlanarSpeedCmPerSecond) ||
            sample.PlanarSpeedCmPerSecond < 0.0 ||
            sample.PlanarSpeedCmPerSecond > MaxSpeedCmPerSecond ||
            !std::isfinite(sample.LateralSpeedCmPerSecond) ||
            std::abs(sample.LateralSpeedCmPerSecond) >
                sample.PlanarSpeedCmPerSecond + 5.0)
        {
            Suspend();
            return false;
        }
        const double dt = sample.DeltaSeconds;
        const double slip = std::clamp(
            std::abs(sample.LateralSpeedCmPerSecond) /
                std::max(500.0, sample.PlanarSpeedCmPerSecond), 0.0, 1.0);
        const double brakeHeat = sample.Braking &&
            sample.PlanarSpeedCmPerSecond > 1000.0 ? 0.04 : 0.0;
        Temperature = std::clamp(Temperature + dt *
            (0.30 * slip + brakeHeat - 0.10 * (Temperature - 0.42)),
            0.0, 1.0);
        // Sustained slides gradually weaken the existing chassis damping;
        // this has no authority over credits, lap records or pit service.
        const double wearRate = 0.0003 + 0.008 * slip +
            (sample.Braking && sample.PlanarSpeedCmPerSecond > 1000.0
                ? 0.003 : 0.0);
        Wear = std::clamp(Wear + dt * wearRate, 0.0, 1.0);
        Valid = true;
        return true;
    }

    double GripMultiplier() const noexcept
    {
        // Always return a finite, bounded scale (max 32% reduction).
        return std::clamp(1.0 - Wear * 0.22 -
            std::max(0.0, Temperature - 0.70) * 0.35,
            MinimumGripMultiplier, 1.0);
    }
    double TemperatureFraction() const noexcept { return Temperature; }
    double WearFraction() const noexcept { return Wear; }
    bool HasValidSample() const noexcept { return Valid; }

private:
    double Temperature = 0.42;
    double Wear = 0.0;
    bool Valid = false;
};
} // namespace latebrake
