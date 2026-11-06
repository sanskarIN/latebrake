#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Bounded arcade ability on the existing single rigid-body prototype.
// This is NOT a combustion/tire/traction simulation or an official race score.
struct NitroDriveOutput final
{
    double ForceMultiplier = 1.0;
    double SpeedCapMultiplier = 1.0;
    bool Active = false;
};

class ArcadeNitroBoost final
{
public:
    static constexpr double MaxSampleSeconds = 0.10;
    static constexpr double MinForwardCmPerSecond = 700.0;
    static constexpr double MaxForwardCmPerSecond = 10000.0;
    static constexpr double MinimumThrottle = 0.5;
    static constexpr double ForceMultiplier = 1.38;
    static constexpr double SpeedCapMultiplier = 1.12;
    static constexpr double DrainPerSecond = 0.25;
    static constexpr double RechargePerSecond = 0.085;
    static constexpr double HeatPerSecond = 0.36;
    static constexpr double CoolingPerSecond = 0.24;
    static constexpr double HeatCutoff = 0.95;
    static constexpr double CooldownSeconds = 2.5;

    void Reset() noexcept
    {
        Charge = 1.0;
        Heat = 0.0;
        Cooldown = 0.0;
        Held = false;
        Active = false;
        PendingChargeDebit = 0.0;
        PendingHeatIncrease = 0.0;
        PendingCutoff = false;
    }

    // Input interruption stops boost without replenishing run-local energy.
    void Suspend() noexcept
    {
        Held = false;
        Active = false;
        PendingChargeDebit = 0.0;
        PendingHeatIncrease = 0.0;
        PendingCutoff = false;
    }
    void Press(bool eligibleHumanDriver) noexcept
    {
        Held = eligibleHumanDriver;
        Active = false;
        PendingChargeDebit = 0.0;
        PendingHeatIncrease = 0.0;
        PendingCutoff = false;
    }
    void Release() noexcept { Suspend(); }

    // A qualifying key press produces a CANDIDATE nitro bonus. The real
    // Unreal motor may still be fully blocked by gears, engine heat or
    // the final axle/contact friction budget. Undo just this ACTIVE
    // frame's provisional charge/heat when no bonus force was delivered.
    // Keep Held so the actual player can retry on the next racing frame.
    // Idempotent: an invalid/overheated/depleted step cannot be refunded.
    bool RejectUndeliveredFrame() noexcept
    {
        if (!Active) return false;
        Charge = std::clamp(Charge + PendingChargeDebit, 0.0, 1.0);
        Heat = std::clamp(Heat - PendingHeatIncrease, 0.0, 1.0);
        Active = false;
        PendingChargeDebit = 0.0;
        PendingHeatIncrease = 0.0;
        PendingCutoff = false;
        return true;
    }

    // Only an actual positive Chaos nitro AddForce may finalize the
    // depletion/heat-cutoff cooldown. A terminal candidate is payable
    // once if delivered, or refundable if friction/gear blocked it.
    bool CommitDeliveredFrame() noexcept
    {
        if (!Active) return false;
        const bool cutoff = PendingCutoff;
        PendingChargeDebit = 0.0;
        PendingHeatIncrease = 0.0;
        PendingCutoff = false;
        if (cutoff)
        {
            Cooldown = CooldownSeconds;
            Suspend();
        }
        return true;
    }

    NitroDriveOutput Step(double seconds, bool eligibleHumanDriver,
                          bool serviceOrHandBraking, double throttle,
                          double signedForwardCmPerSecond) noexcept
    {
        Active = false;
        PendingChargeDebit = 0.0;
        PendingHeatIncrease = 0.0;
        PendingCutoff = false;
        if (!eligibleHumanDriver || !std::isfinite(seconds) ||
            seconds <= 0.0 || seconds > MaxSampleSeconds ||
            !std::isfinite(throttle) || throttle < -1.0 || throttle > 1.0 ||
            !std::isfinite(signedForwardCmPerSecond) ||
            std::abs(signedForwardCmPerSecond) > MaxForwardCmPerSecond)
        {
            Suspend();
            return {};
        }

        // Race time must advance before energy, cooldown or heat do.
        Cooldown = std::max(0.0, Cooldown - seconds);
        const bool canFire = Held && !serviceOrHandBraking &&
            throttle >= MinimumThrottle &&
            signedForwardCmPerSecond >= MinForwardCmPerSecond &&
            Cooldown <= 0.0 && Charge > 0.0 && Heat < HeatCutoff;
        if (canFire)
        {
            const double previousCharge = Charge;
            const double previousHeat = Heat;
            Charge = std::max(0.0, Charge - seconds * DrainPerSecond);
            Heat = std::min(1.0, Heat + seconds * HeatPerSecond);
            // This terminal sample is PROVISIONAL until the same frame
            // submits an actual motor bonus, not only a qualifying key.
            PendingChargeDebit = previousCharge - Charge;
            PendingHeatIncrease = Heat - previousHeat;
            PendingCutoff = Charge <= 0.0 || Heat >= HeatCutoff;
            Active = true;
            return {ForceMultiplier, SpeedCapMultiplier, true};
        }

        // Regeneration only during an eligible, advancing race frame.
        Charge = std::min(1.0, Charge + seconds * RechargePerSecond);
        Heat = std::max(0.0, Heat - seconds * CoolingPerSecond);
        return {};
    }

    // One completed pit visit restores at most 40% charge and cools
    // boost hardware. Held input cannot remain armed after service.
    void RefillAtPit() noexcept
    {
        Suspend();
        Charge = std::min(1.0, Charge + 0.40);
        Heat = std::max(0.0, Heat - 0.40);
        Cooldown = 0.0;
    }

    double ChargeFraction() const noexcept { return Charge; }
    double HeatFraction() const noexcept { return Heat; }
    double CooldownRemainingSeconds() const noexcept { return Cooldown; }
    bool IsActive() const noexcept { return Active; }
    bool IsHeld() const noexcept { return Held; }

private:
    double Charge = 1.0;
    double Heat = 0.0;
    double Cooldown = 0.0;
    bool Held = false;
    bool Active = false;
    // Valid only for the latest ACTIVE Step; never a deferred frame timer.
    double PendingChargeDebit = 0.0;
    double PendingHeatIncrease = 0.0;
    bool PendingCutoff = false; // settle only on delivered motor force
};
} // namespace latebrake
