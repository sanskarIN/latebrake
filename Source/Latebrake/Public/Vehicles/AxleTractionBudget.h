#pragma once

#include "Vehicles/SprungChassisDynamics.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// A first, explicit REAR-DRIVE/FRONT-STEER contract for the source-stage
// LateBrake box-rigid-body prototype. This selects which real, measured
// axle loads may authorize engine thrust and yaw steering. It does not
// create any new tire contact, drivetrain differential, lateral tire solver,
// torque distribution, reward, or a claimed production Chaos Vehicles rig.
//
// Four corners are always FL, FR, RL, RR in the shared sprung response.
inline constexpr double OneDrivenWheelGripFraction = 0.40;
inline constexpr double OneSteeringWheelGripFraction = 0.45;
inline constexpr double AxleGripLoadEpsilon = 1.0e-6;

struct AxleTractionBudget final
{
    bool Valid = false;
    double FrontAxleLoadRatio = 0.0; // measured load / nominal HALF weight
    double RearAxleLoadRatio = 0.0;
    double RearDriveTractionScale = 0.0;  // bounded [0, existing whole-body grip]
    double FrontSteeringTractionScale = 0.0;
    int FrontWheelContacts = 0;
    int RearWheelContacts = 0;
    bool RearDriveReady = false;
    bool FrontSteeringReady = false;
};

// The original contact scaling remains an overall ceiling: this policy
// only reduces the motor and yaw-controller portions by *actual axle load*.
// A fully supported flat car retains identical force to the baseline;
// front-only contact cannot power an ungrounded rear differential, and
// rear-only contact cannot produce the usual front-wheel steering torque.
inline AxleTractionBudget CalculateAxleTractionBudget(
    const ChassisGroundContactOutput& ground,
    const SuspensionResponse& springs) noexcept
{
    if (!ground.Valid || !springs.Valid ||
        ground.SupportingProbes < 0 || ground.SupportingProbes > 4 ||
        springs.ContactCount < 0 || springs.ContactCount > 4 ||
        ground.SupportingProbes != springs.ContactCount ||
        !std::isfinite(ground.ContactTractionScale) ||
        ground.ContactTractionScale < 0.0 ||
        ground.ContactTractionScale > 1.0 ||
        ground.HasDriveContact != (ground.SupportingProbes >= 2 &&
            ground.ContactTractionScale > AxleGripLoadEpsilon) ||
        !std::isfinite(springs.WeightForce) ||
        springs.WeightForce <= 0.0 ||
        !std::isfinite(springs.TotalUpForce) ||
        springs.TotalUpForce < 0.0)
    {
        return {};
    }

    double frontLoad = 0.0;
    double rearLoad = 0.0;
    int frontCount = 0;
    int rearCount = 0;
    double sum = 0.0;
    for (int i = 0; i < 4; ++i)
    {
        const auto& corner = springs.Corners[static_cast<unsigned>(i)];
        if (!std::isfinite(corner.UpForce) ||
            corner.UpForce < 0.0 ||
            corner.UpForce > springs.WeightForce *
                SuspensionMaxPerCornerWeightFactor + 0.01 ||
            (!corner.Contact && corner.UpForce > 0.0))
        {
            return {};
        }
        if (corner.Contact)
        {
            if (i < 2) ++frontCount;
            else ++rearCount;
        }
        if (i < 2) frontLoad += corner.UpForce;
        else rearLoad += corner.UpForce;
        sum += corner.UpForce;
    }
    if (frontCount + rearCount != springs.ContactCount ||
        std::abs(sum - springs.TotalUpForce) >
            0.01 + springs.WeightForce * 1.0e-10 ||
        sum > springs.WeightForce *
            SuspensionMaxPerCornerWeightFactor * 4.0 + 0.01)
    {
        return {};
    }

    AxleTractionBudget result;
    result.Valid = true;
    result.FrontWheelContacts = frontCount;
    result.RearWheelContacts = rearCount;
    // Saturate so spring spikes / bumps never create more grip than the
    // previous correctly qualified whole-vehicle traction ceiling.
    result.FrontAxleLoadRatio =
        std::clamp(frontLoad * 2.0 / springs.WeightForce, 0.0, 1.0);
    result.RearAxleLoadRatio =
        std::clamp(rearLoad * 2.0 / springs.WeightForce, 0.0, 1.0);
    const double rearContactFactor = rearCount == 2 ? 1.0 :
        rearCount == 1 ? OneDrivenWheelGripFraction : 0.0;
    const double frontContactFactor = frontCount == 2 ? 1.0 :
        frontCount == 1 ? OneSteeringWheelGripFraction : 0.0;

    result.RearDriveTractionScale = std::min(
        ground.ContactTractionScale,
        result.RearAxleLoadRatio * rearContactFactor);
    result.FrontSteeringTractionScale = std::min(
        ground.ContactTractionScale,
        result.FrontAxleLoadRatio * frontContactFactor);

    // Two total qualified sprung contacts are still required; on a
    // single corner neither drivetrain nor steering may steer/propel.
    result.RearDriveReady = ground.HasDriveContact &&
        rearCount > 0 &&
        result.RearDriveTractionScale > AxleGripLoadEpsilon;
    result.FrontSteeringReady = ground.HasDriveContact &&
        frontCount > 0 &&
        result.FrontSteeringTractionScale > AxleGripLoadEpsilon;
    return result;
}
} // namespace latebrake
