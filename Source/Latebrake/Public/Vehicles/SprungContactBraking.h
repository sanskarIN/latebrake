#pragma once

#include "Vehicles/AxleTractionBudget.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace latebrake
{
struct SprungContactBrakeSample final
{
    double MassKg = 0.0;
    double ForwardCmPerSecond = 0.0;
    // Already limited by pedal, grip, heat, frame time and whole-body
    // contact. This policy may REDUCE that force; never multiply grip again.
    double RequestedForwardForce = 0.0;
    bool RearOnly = false; // handbrake alone; service brake uses all corners
    bool ContactSamplesValid = false;
    // Actual Chaos velocity AT each accepted road impact point, projected
    // onto each measured unit road tangent. FL, FR, RL, RR.
    std::array<double, 4> ContactForwardCmPerSecond{};
    // Unit tangent's forward projection; default preserves level-road API.
    std::array<double, 4> PlanarForwardScales{1.0, 1.0, 1.0, 1.0};
};

struct SprungContactBrakeOutput final
{
    bool Valid = false;
    // Signed magnitudes ALONG each unit road tangent, not world XY forces.
    std::array<double, 4> TangentForces{};
    double TotalBrakeForceMagnitude = 0.0;
    double TotalForwardForce = 0.0;
    // Same pre-contact units as LateralGripSample's longitudinal budget.
    // Dispatch already includes contact; this is never a force multiplier.
    double WholeBodyBudgetDeceleration = 0.0;
};

// Redistribute the existing chassis brake request over genuine positively
// loaded spring contacts. Applying these forces at their real road points
// lets Chaos produce pitch/roll/yaw moments instead of braking at the COM.
// This is a rigid-body approximation, NOT wheel torque, ABS or tire slip.
inline SprungContactBrakeOutput CalculateSprungContactBraking(
    const SprungContactBrakeSample& sample,
    const ChassisGroundContactOutput& ground,
    const SuspensionResponse& springs) noexcept
{
    // Reuse the four-corner consistency/finite/load/ground validation.
    // A malformed corner must reject the whole plan before ANY dispatch.
    const auto axles = CalculateAxleTractionBudget(ground, springs);
    if (!axles.Valid || !sample.ContactSamplesValid ||
        !std::isfinite(sample.MassKg) || sample.MassKg < 100.0 ||
        sample.MassKg > 15000.0 ||
        !std::isfinite(sample.ForwardCmPerSecond) ||
        std::abs(sample.ForwardCmPerSecond) > 15000.0 ||
        !std::isfinite(sample.RequestedForwardForce) ||
        std::abs(sample.RequestedForwardForce) > sample.MassKg * 5000.0 ||
        sample.RequestedForwardForce * sample.ForwardCmPerSecond > 0.0 ||
        (std::abs(sample.ForwardCmPerSecond) <= 1.0e-9 &&
         sample.RequestedForwardForce != 0.0))
    {
        return {};
    }
    for (unsigned i = 0; i < springs.Corners.size(); ++i)
    {
        // Unhit probes have no physical point to sample. Never consume
        // arbitrary stale sensor data for an absent corner.
        if (springs.Corners[i].Contact &&
            (!std::isfinite(sample.ContactForwardCmPerSecond[i]) ||
             std::abs(sample.ContactForwardCmPerSecond[i]) > 100000.0 ||
             !std::isfinite(sample.PlanarForwardScales[i]) ||
             sample.PlanarForwardScales[i] < SuspensionMinNormalZ - 1.0e-3 ||
             sample.PlanarForwardScales[i] > 1.0))
        {
            return {};
        }
    }

    SprungContactBrakeOutput out;
    out.Valid = true;
    if (!ground.HasDriveContact || springs.TotalUpForce <= 0.0 ||
        sample.RequestedForwardForce == 0.0)
    {
        return out;
    }
    if (std::abs(sample.RequestedForwardForce) >
        sample.MassKg * 5000.0 * ground.ContactTractionScale)
    {
        return {}; // caller did not respect the shared contact ceiling
    }
    for (unsigned i = 0; i < springs.Corners.size(); ++i)
    {
        const auto& corner = springs.Corners[i];
        if (!corner.Contact || corner.UpForce <= 0.0 ||
            (sample.RearOnly && i < 2))
        {
            continue;
        }
        const double localSpeed = sample.ContactForwardCmPerSecond[i];
        // A spinning chassis can have a road point travelling OPPOSITE
        // its COM. A COM-signed brake there would ADD rotational energy.
        // Suppress that share and fade toward zero local motion. Do not
        // reallocate rejected shares to other tires or create extra force.
        if (localSpeed * sample.ForwardCmPerSecond <= 0.0)
        {
            continue;
        }
        const double speedScale = std::min(1.0,
            std::abs(localSpeed / sample.ForwardCmPerSecond));
        out.TangentForces[i] = sample.RequestedForwardForce *
            (corner.UpForce / springs.TotalUpForce) * speedScale;
        out.TotalForwardForce += out.TangentForces[i] * sample.PlanarForwardScales[i];
        out.TotalBrakeForceMagnitude += std::abs(out.TangentForces[i]);
    }
    // Whole-body contact was applied exactly once before allocation. Feed
    // the full 3D magnitude sum back into the SAME lateral/yaw grip budget.
    // Spending only the XY projection would overgrant grip on a grade.
    out.WholeBodyBudgetDeceleration =
        out.TotalBrakeForceMagnitude / sample.MassKg /
        ground.ContactTractionScale;
    return out;
}
} // namespace latebrake
