#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// This is a race-local arcade *impact response*, not a damage/repair
// economy, mesh deformation, crash physics or a real structural model.
// The engine must supply the actual chassis collision impulse, actual
// body mass, horizontal contact normal and measured vehicle speed.
struct ArcadeImpactSample final
{
    double NormalImpulseKgCmPerSecond = 0.0;
    double ChassisMassKg = 0.0;
    double PlanarSpeedCmPerSecond = 0.0;
    double ContactNormalZ = 0.0;
    bool DrivingAllowed = false;
};

class ArcadeCollisionDamage final
{
public:
    static constexpr double MaxFrameSeconds = 0.10;
    static constexpr double MinChassisMassKg = 50.0;
    static constexpr double MaxChassisMassKg = 10000.0;
    static constexpr double MaxPlanarSpeedCmPerSecond = 10000.0;
    static constexpr double MaxImpulseKgCmPerSecond = 1.0e9;
    static constexpr double MinImpactSpeedCmPerSecond = 1100.0;
    static constexpr double MinImpulsePerMassCmPerSecond = 360.0;
    static constexpr double MaxHorizontalContactNormalZ = 0.55;
    static constexpr double ContactCooldownSeconds = 0.40;
    static constexpr double MaxDamageFraction = 0.60;
    static constexpr double MinMotorMultiplier = 0.79;
    static constexpr double MinLateralGripMultiplier = 0.856;

    void Reset() noexcept
    {
        Damage = 0.0;
        Cooldown = 0.0;
        ImpactCount = 0;
        FrameEligible = false;
    }

    // Pause, focus loss, recovery, invalid physics or a long frame cannot
    // certify a new hit. Keep already-earned race-local damage and cooldown.
    void Suspend() noexcept { FrameEligible = false; }

    bool Advance(double deltaSeconds, bool drivingAllowed) noexcept
    {
        if (!drivingAllowed || !std::isfinite(deltaSeconds) ||
            deltaSeconds <= 0.0 || deltaSeconds > MaxFrameSeconds)
        {
            Suspend();
            return false;
        }
        FrameEligible = true;
        Cooldown = std::max(0.0, Cooldown - deltaSeconds);
        // Collapse binary floating-point residue at the exact contact
        // boundary, so eight 50-ms frames reliably expire a 400-ms hold.
        if (Cooldown < 1.0e-9)
        {
            Cooldown = 0.0;
        }
        return true;
    }

    bool ObserveImpact(ArcadeImpactSample impact) noexcept
    {
        if (!FrameEligible || !impact.DrivingAllowed ||
            !std::isfinite(impact.NormalImpulseKgCmPerSecond) ||
            impact.NormalImpulseKgCmPerSecond < 0.0 ||
            impact.NormalImpulseKgCmPerSecond > MaxImpulseKgCmPerSecond ||
            !std::isfinite(impact.ChassisMassKg) ||
            impact.ChassisMassKg < MinChassisMassKg ||
            impact.ChassisMassKg > MaxChassisMassKg ||
            !std::isfinite(impact.PlanarSpeedCmPerSecond) ||
            impact.PlanarSpeedCmPerSecond < MinImpactSpeedCmPerSecond ||
            impact.PlanarSpeedCmPerSecond > MaxPlanarSpeedCmPerSecond ||
            !std::isfinite(impact.ContactNormalZ) ||
            std::abs(impact.ContactNormalZ) > MaxHorizontalContactNormalZ ||
            Cooldown > 0.0 || Damage >= MaxDamageFraction)
        {
            return false;
        }

        const double impulsePerMass =
            impact.NormalImpulseKgCmPerSecond / impact.ChassisMassKg;
        if (!std::isfinite(impulsePerMass) ||
            impulsePerMass < MinImpulsePerMassCmPerSecond)
        {
            return false;
        }

        // A real high-speed, predominantly side-on hard collision may
        // weaken subsequent engine drive and lateral arcade stability.
        // One physical contact manifold can emit repeated callbacks;
        // the cooldown grants at most one bounded hit per 0.4 seconds.
        const double severity = std::clamp(
            (impulsePerMass - MinImpulsePerMassCmPerSecond) / 2500.0,
            0.0, 1.0);
        const double increment = 0.03 + 0.10 * severity;
        Damage = std::min(MaxDamageFraction, Damage + increment);
        Cooldown = ContactCooldownSeconds;
        if (ImpactCount < 65535u)
        {
            ++ImpactCount;
        }
        return true;
    }

    double DamageFraction() const noexcept { return Damage; }
    unsigned ImpactEventCount() const noexcept { return ImpactCount; }
    bool CanRecordImpact() const noexcept
    {
        return FrameEligible && Cooldown == 0.0 &&
            Damage < MaxDamageFraction;
    }
    bool HasEligibleFrame() const noexcept { return FrameEligible; }
    double MotorMultiplier() const noexcept
    {
        return std::clamp(1.0 - 0.35 * Damage, MinMotorMultiplier, 1.0);
    }
    double LateralGripMultiplier() const noexcept
    {
        return std::clamp(
            1.0 - 0.24 * Damage, MinLateralGripMultiplier, 1.0);
    }

private:
    double Damage = 0.0;
    double Cooldown = 0.0;
    unsigned ImpactCount = 0;
    bool FrameEligible = false;
};
} // namespace latebrake
