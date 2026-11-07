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
        PendingNitroHeat = false;
        PendingHeatWithoutNitro = IdleHeat;
    }

    // Pausing freezes heat; wall-clock time is not qualified driving time.
    void Suspend() noexcept
    {
        // An interrupted tick is not evidence of delivered bonus force.
        SettleNitroDelivery(false);
        Valid = false;
        PendingNitroHeat = false;
    }

    // Preserve normal throttle/airflow heating but commit the extra
    // nitro heat only after the real bonus AddForce was submitted.
    // This same-frame receipt can settle exactly once.
    bool SettleNitroDelivery(bool delivered) noexcept
    {
        if (!PendingNitroHeat) return false;
        if (!delivered) Heat = PendingHeatWithoutNitro;
        PendingNitroHeat = false;
        return true;
    }

    EngineHeatOutput Observe(const EngineHeatSample& sample) noexcept
    {
        // A forgotten prior receipt never rolls phantom nitro heat into
        // the next frame's ordinary throttle/airflow calculation.
        SettleNitroDelivery(false);
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
        // Base heat is still sampled before motor derating, while
        // nitro-only heat is a provisional extra until force dispatch.
        const double cooling = (0.022 + 0.028 * airflow) *
            (Heat - IdleHeat);
        const double baseHeat = std::clamp(
            Heat + sample.DeltaSeconds * (0.035 * load - cooling),
            IdleHeat, 1.0);
        const double nitroExtra = sample.NitroActive && load > 0.01
            ? sample.DeltaSeconds * 0.025 * load : 0.0;
        Heat = std::clamp(baseHeat + nitroExtra, IdleHeat, 1.0);
        PendingNitroHeat = nitroExtra > 0.0;
        PendingHeatWithoutNitro = baseHeat;
        Valid = true;
        // The motor dispatch occurs BEFORE its real nitro force verdict.
        // Use the settled ordinary-load heat for this frame's motor limit;
        // provisional bonus heat can only affect later frames if delivered.
        const double scale = ComputeMotorMultiplier(baseHeat);
        return {true, Heat, scale, scale < 1.0};
    }

    bool HasValidSample() const noexcept { return Valid; }
    double HeatFraction() const noexcept { return Heat; }
    double MotorMultiplier() const noexcept
    {
        return Valid ? ComputeMotorMultiplier(Heat) : 1.0;
    }

private:
    double ComputeMotorMultiplier(double heatFraction) const noexcept
    {
        const double excess = std::clamp(
            (heatFraction - DerateThreshold) / (1.0 - DerateThreshold), 0.0, 1.0);
        return 1.0 - (1.0 - MinimumMotorMultiplier) * excess;
    }

    double Heat = IdleHeat;
    bool Valid = false;
    bool PendingNitroHeat = false;
    double PendingHeatWithoutNitro = IdleHeat;
};
} // namespace latebrake
