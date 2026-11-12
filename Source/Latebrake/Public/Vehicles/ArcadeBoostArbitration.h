#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Existing ArcadeNitroBoost and SlipstreamDraftBoost remain independent
// admission policies. This engine-free final allocator prevents simultaneous
// valid boosts from exceeding the configured additive motor-force budget.
struct ArcadeBoostAllocation final
{
    double NitroExtra = 0.0;
    double DraftExtra = 0.0;
    double TotalExtra() const noexcept { return NitroExtra + DraftExtra; }
};

inline ArcadeBoostAllocation AllocateArcadeBoost(
    double nitroForceMultiplier, double requestedDraftExtra,
    bool nitroActive) noexcept
{
    constexpr double NitroMaxExtra = 0.38;
    constexpr double DraftMaxExtra = 0.06;
    constexpr double CombinedMaxExtra = 0.40;

    // Malformed or inconsistent gameplay state cannot grant propulsion.
    if (!std::isfinite(nitroForceMultiplier) ||
        !std::isfinite(requestedDraftExtra) ||
        nitroForceMultiplier < 1.0 ||
        nitroForceMultiplier > 1.0 + NitroMaxExtra + 1e-9 ||
        requestedDraftExtra < 0.0 ||
        requestedDraftExtra > DraftMaxExtra + 1e-9 ||
        (!nitroActive && nitroForceMultiplier != 1.0))
    {
        return {};
    }

    const double nitro = nitroActive
        ? std::clamp(nitroForceMultiplier - 1.0, 0.0, NitroMaxExtra)
        : 0.0;
    // Nitro has priority because it spends finite race-local charge. A
    // nearby rival can still supply the remaining share, never >40% total.
    const double draft = std::min(
        std::clamp(requestedDraftExtra, 0.0, DraftMaxExtra),
        CombinedMaxExtra - nitro);
    return {nitro, std::max(0.0, draft)};
}

// The drafting HUD reports the bonus force that can actually reach the
// physical motor, relative to the same nominal base DriveForce * throttle.
// Do not show an allocated +6% when rear load, gearbox taper, impact/heat,
// road surface or the final shared friction circle left zero motor force.
// This is READ-ONLY reporting after the draft AddForce path, never a
// new force multiplier and never a checkpoint/reward authority.
inline double CalculateDeliveredDraftFraction(
    double allocatedDraftExtra, double finalMotorPathScale) noexcept
{
    constexpr double MaximumDraftExtra = 0.06;
    if (!std::isfinite(allocatedDraftExtra) ||
        !std::isfinite(finalMotorPathScale) ||
        allocatedDraftExtra < 0.0 ||
        allocatedDraftExtra > MaximumDraftExtra + 1.0e-9 ||
        finalMotorPathScale < 0.0 ||
        finalMotorPathScale > 1.0 + 1.0e-6)
    {
        return 0.0;
    }
    return std::clamp(
        allocatedDraftExtra * std::min(finalMotorPathScale, 1.0),
        0.0, MaximumDraftExtra);
}
} // namespace latebrake
