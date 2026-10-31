#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Race-local arcade heat, not a real engine/coolant or battery model.
// Invalid/paused frames never advance history or impose stale penalties.
struct EngineHeatSample final
{
    double DeltaSeconds = 0.0;
    double ForwardSpeedCmPerSecond = 0.0;
    double PropulsionDemand = 0.0;
    bool NitroActive = false;
    bool DrivingAllowed = false;
};

struct EngineHeatOutput final
{
    bool Valid = false;
    double HeatFraction = 0.18;
    double MotorMultiplier = 1.0;
    bool Derated = false;
};

class ArcadeEngineHeat final
{
public:
    static constexpr double IdleHeat = 0.18;
    static constexpr double MaxFrameSeconds = 0.10;
    static constexpr double MaxSpeedCmPerSecond = 10000.0;
    static constexpr double DerateThreshold = 0.67;
    static constexpr double MinimumMotorMultiplier = 0.82;

    void Reset() noexcept
    {
        Heat = IdleHeat;
        Valid = false;
    }

    // Pausing freezes heat; wall-clock time is not qualified driving time.
    void Suspend() noexcept { Valid = false; }

    EngineHeatOutput Observe(const EngineHeatSample& sample) noexcept
    {
        if (!sample.DrivingAllowed ||
            !std::isfinite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 ||
            sample.DeltaSeconds > MaxFrameSeconds ||
            !std::isfinite(sample.ForwardSpeedCmPerSecond) ||
            std::abs(sample.ForwardSpeedCmPerSecond) > MaxSpeedCmPerSecond ||
            !std::isfinite(sample.PropulsionDemand) ||
            std::abs(sample.PropulsionDemand) > 1.0)
        {
            Suspend();
            return {false, Heat, 1.0, false};
        }

        const double load = std::abs(sample.PropulsionDemand);
        const double airflow = std::clamp(
            std::abs(sample.ForwardSpeedCmPerSecond) / 6000.0, 0.0, 1.0);
        // Only eligible propulsion load and active nitro can add heat.
        const double heating = 0.035 * load +
            (sample.NitroActive && load > 0.01 ? 0.025 * load : 0.0);
        const double cooling = (0.022 + 0.028 * airflow) *
            (Heat - IdleHeat);
        Heat = std::clamp(
            Heat + sample.DeltaSeconds * (heating - cooling),
            IdleHeat, 1.0);
        Valid = true;
        const double scale = ComputeMotorMultiplier();
        return {true, Heat, scale, scale < 1.0};
    }

    bool HasValidSample() const noexcept { return Valid; }
    double HeatFraction() const noexcept { return Heat; }
    double MotorMultiplier() const noexcept
    {
        return Valid ? ComputeMotorMultiplier() : 1.0;
    }

private:
    double ComputeMotorMultiplier() const noexcept
    {
        const double excess = std::clamp(
            (Heat - DerateThreshold) / (1.0 - DerateThreshold), 0.0, 1.0);
        return 1.0 - (1.0 - MinimumMotorMultiplier) * excess;
    }

    double Heat = IdleHeat;
    bool Valid = false;
};
} // namespace latebrake
