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
        PendingNitroExtra = 0.0;
    }

    // Pausing freezes heat; wall-clock time is not qualified driving time.
    void Suspend() noexcept
    {
        // An interrupted tick cannot commit a pending bonus receipt.
        SettleNitroDelivery(false);
        Valid = false;
    }

    // Preserve normal throttle/airflow heating but commit the extra
    // nitro heat only after the real bonus AddForce was submitted.
    // This same-frame receipt can settle exactly once.
    bool SettleNitroDelivery(bool delivered) noexcept
    {
        if (!PendingNitroHeat) return false;
        if (delivered)
        {
            Heat = std::clamp(Heat + PendingNitroExtra, IdleHeat, 1.0);
        }
        // A rejected candidate never entered Heat, so requires no refund.
        PendingNitroHeat = false;
        PendingNitroExtra = 0.0;
        return true;
    }

    EngineHeatOutput Observe(const EngineHeatSample& sample) noexcept
    {
        // Drop any unconfirmed old receipt before computing fresh
        // ordinary throttle/airflow heat; it was never added to Heat.
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
        // Ordinary-load heat is settled now; nitro-specific heat remains
        // only a pending proposal until real positive AddForce dispatch.
        const double cooling = (0.022 + 0.028 * airflow) *
            (Heat - IdleHeat);
        const double baseHeat = std::clamp(
            Heat + sample.DeltaSeconds * (0.035 * load - cooling),
            IdleHeat, 1.0);
        const double nitroExtra = sample.NitroActive && load > 0.01
            ? sample.DeltaSeconds * 0.025 * load : 0.0;
        Heat = baseHeat;
        PendingNitroExtra = nitroExtra;
        PendingNitroHeat = nitroExtra > 0.0;
        Valid = true;
        // Heat, the public observation and motor power are coherent even
        // before force authorization. Only an explicit true settlement
        // increases heat, and only subsequent frames can use that increment.
        const double scale = ComputeMotorMultiplier(Heat);
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
    double PendingNitroExtra = 0.0; // held, not reflected in Heat
};
} // namespace latebrake
