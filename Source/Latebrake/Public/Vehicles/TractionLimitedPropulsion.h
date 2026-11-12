#pragma once

#include "Vehicles/LateralGripBudget.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Final motor-force allowance for the simplified rear-drive rigid body.
// Existing throttle, gear, heat, damage, surface, boost and rear-load
// policies run first. This can only reduce their SUM; it cannot grant
// input authority or change lateral/brake/spring forces. Not a tire solver.
struct TractionLimitedPropulsionSample final
{
    double DeltaSeconds = 0.0;
    double MassKg = 0.0;
    double RequestedForwardForce = 0.0; // signed SUM of base + nitro + draft
    double AppliedRightForce = 0.0; // same-frame lateral force AFTER contact scale
    double GripScale = 1.0; // same surface/tire/garage/damage product as lateral grip
    double ContactTractionScale = 0.0;
    double RearTractionScale = 0.0; // accepted reattached rear-axle ceiling
    bool ArcadeHandling = true;
};

struct TractionLimitedPropulsionOutput final
{
    bool Valid = false;
    double ForceScale = 0.0; // one common [0,1] factor for all three motor paths
    double ForwardForce = 0.0; // diagnostic/reference result, not extra force
    double AvailableForwardForce = 0.0;
    bool GripLimited = false;
};

inline TractionLimitedPropulsionOutput CalculateTractionLimitedPropulsion(
    const TractionLimitedPropulsionSample& s) noexcept
{
    if (!std::isfinite(s.DeltaSeconds) || s.DeltaSeconds <= 0.0 ||
        s.DeltaSeconds > MaximumLateralFrameSeconds ||
        !std::isfinite(s.MassKg) || s.MassKg < 100.0 || s.MassKg > 15000.0 ||
        !std::isfinite(s.RequestedForwardForce) ||
        std::abs(s.RequestedForwardForce) > 1.0e8 ||
        !std::isfinite(s.AppliedRightForce) ||
        std::abs(s.AppliedRightForce) > 1.0e8 ||
        !std::isfinite(s.GripScale) || s.GripScale < 0.05 || s.GripScale > 4.0 ||
        !std::isfinite(s.ContactTractionScale) ||
        s.ContactTractionScale < 0.0 || s.ContactTractionScale > 1.0 ||
        !std::isfinite(s.RearTractionScale) || s.RearTractionScale < 0.0 ||
        s.RearTractionScale > s.ContactTractionScale)
    {
        return {};
    }

    const double nominalBudget = (s.ArcadeHandling
        ? ArcadeCombinedGripBudget : SimulationCombinedGripBudget) * s.GripScale;
    const double wholeBodyBudget = nominalBudget * s.ContactTractionScale;
    const double usedLateral = std::abs(s.AppliedRightForce) / s.MassKg;
    const double remaining = std::sqrt(std::max(0.0,
        wholeBodyBudget * wholeBodyBudget - usedLateral * usedLateral));
    // Rear grip already scales the requested motor force. Use it here as
    // a ceiling, not another multiplier: do not square the axle load.
    const double available = s.MassKg * std::min(
        remaining, nominalBudget * s.RearTractionScale);
    const double request = std::abs(s.RequestedForwardForce);
    const double scale = request > 0.0 ? std::min(1.0, available / request) : 1.0;
    return {true, scale, s.RequestedForwardForce * scale, available,
        request > available + 1.0e-7};
}
} // namespace latebrake
