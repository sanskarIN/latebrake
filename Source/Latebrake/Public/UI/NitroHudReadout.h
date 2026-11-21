#pragma once

#include "Vehicles/ArcadeNitroBoost.h"

#include <cmath>

namespace latebrake
{
// Presentation-only interpretation of the existing, authoritative nitro
// arcade component. This does not update recharge, heat, cooldown, controls,
// race clock, rewards or vehicle forces. "Stored" does NOT promise that
// throttle, input, ground contact or any other activation gate is satisfied.
enum class NitroHudPhase
{
    Unavailable,
    Active,
    Cooldown,
    Overheated,
    Empty,
    BuildSpeed,
    Stored
};

struct NitroHudSample final
{
    double ChargeFraction = 0.0;
    double HeatFraction = 0.0;
    double CooldownSeconds = 0.0;
    double SignedForwardKmH = 0.0;
    bool Active = false;
    bool DrivingEligible = false;
};

struct NitroHudReadout final
{
    bool Valid = false;
    NitroHudPhase Phase = NitroHudPhase::Unavailable;
    double CooldownSeconds = 0.0;
};

// Read-only status precedence:
// (1) invalid or interrupted evidence -> unavailable,
// (2) genuinely active -> active,
// (3) cooldown -> countdown,
// (4) thermal/charge blocks -> reason,
// (5) too slow to engage -> speed hint,
// (6) stored energy, not a guarantee of immediate activation.
// Keep all ranges bounded so an invalid Unreal physics sample cannot yield
// a misleading "ready" annotation. The charge percentage bar remains the
// existing NitroBoost component's actual, separate meter value.
inline NitroHudReadout ClassifyNitroHud(NitroHudSample sample) noexcept
{
    if (!std::isfinite(sample.ChargeFraction) ||
        sample.ChargeFraction < 0.0 || sample.ChargeFraction > 1.0 ||
        !std::isfinite(sample.HeatFraction) ||
        sample.HeatFraction < 0.0 || sample.HeatFraction > 1.0 ||
        !std::isfinite(sample.CooldownSeconds) ||
        sample.CooldownSeconds < 0.0 ||
        sample.CooldownSeconds > ArcadeNitroBoost::CooldownSeconds)
    {
        return {};
    }

    if (!sample.DrivingEligible)
    {
        // The panel may still be drawn during a guarded start or pause.
        // Never advertise stored charge as immediately usable in that case.
        return {true, NitroHudPhase::Unavailable, 0.0};
    }
    if (!std::isfinite(sample.SignedForwardKmH) ||
        std::abs(sample.SignedForwardKmH) >
            ArcadeNitroBoost::MaxForwardCmPerSecond * 0.036)
    {
        return {};
    }

    if (sample.Active)
    {
        // Nitro Step never reports active after depletion, overheat or
        // while charging cooldown. Inconsistent external snapshots must
        // fail closed instead of showing a false ACTIVE annotation.
        if (sample.ChargeFraction <= 0.0 ||
            sample.HeatFraction >= ArcadeNitroBoost::HeatCutoff ||
            sample.CooldownSeconds > 0.0 ||
            sample.SignedForwardKmH <
                ArcadeNitroBoost::MinForwardCmPerSecond * 0.036)
        {
            return {};
        }
        return {true, NitroHudPhase::Active, 0.0};
    }
    if (sample.CooldownSeconds > 0.0)
    {
        return {true, NitroHudPhase::Cooldown, sample.CooldownSeconds};
    }
    if (sample.HeatFraction >= ArcadeNitroBoost::HeatCutoff)
    {
        return {true, NitroHudPhase::Overheated, 0.0};
    }
    if (sample.ChargeFraction <= 0.0)
    {
        return {true, NitroHudPhase::Empty, 0.0};
    }
    if (sample.SignedForwardKmH <
        ArcadeNitroBoost::MinForwardCmPerSecond * 0.036)
    {
        return {true, NitroHudPhase::BuildSpeed, 0.0};
    }
    return {true, NitroHudPhase::Stored, 0.0};
}

inline NitroHudReadout ReadNitroHud(
    const ArcadeNitroBoost& nitro, double signedForwardKmH,
    bool drivingEligible) noexcept
{
    return ClassifyNitroHud({
        nitro.ChargeFraction(), nitro.HeatFraction(),
        nitro.CooldownRemainingSeconds(), signedForwardKmH,
        nitro.IsActive(), drivingEligible
    });
}
} // namespace latebrake
