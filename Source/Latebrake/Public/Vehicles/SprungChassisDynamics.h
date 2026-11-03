#pragma once

#include "Vehicles/ChassisGroundContactTraction.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace latebrake
{
// Engine-independent contact-to-force policy for the REAL Chaos rigid body.
// The four observations are blocking WorldStatic ray hits. They are not
// virtual tires, road-mask guesses, or authoritative race checkpoints.
// Unreal applies the returned vertical forces AT the four probe positions.
inline constexpr double SuspensionGravityCmPerSecondSquared = 980.0;
inline constexpr double SuspensionMaximumFrameSeconds = 0.10;
inline constexpr double SuspensionRestDistanceCm = 82.0;
inline constexpr double SuspensionTargetRideDistanceCm = 59.0;
inline constexpr double SuspensionMinNormalZ = 0.68;
inline constexpr double SuspensionMaxTraceDistanceCm = 280.0;
inline constexpr double SuspensionMaxPerCornerWeightFactor = 0.70;
inline constexpr double SuspensionDampingRatio = 0.78;
inline constexpr double SuspensionVisualMinTravelCm = -23.0;
inline constexpr double SuspensionVisualMaxTravelCm = 25.0;

struct SuspensionCornerObservation final
{
    bool Hit = false;
    double DistanceCm = 0.0; // Origin (at chassis wheel anchor) to WorldStatic
    double NormalZ = 0.0;
    double PointVerticalSpeedCmPerSecond = 0.0; // true Chaos velocity AT corner
};

struct SuspensionFrame final
{
    double DeltaSeconds = 0.0;
    double MassKg = 0.0;
    // Actual UWorld gravity magnitude, not an assumed universal fixed g.
    double GravityCmPerSecondSquared = SuspensionGravityCmPerSecondSquared;
    bool PhysicsReady = false;
    std::array<SuspensionCornerObservation, 4> Corners{};
};

struct SuspensionCornerResponse final
{
    bool Contact = false;
    double UpForce = 0.0; // kg*cm/s^2, world +Z at the same measured corner
    // Visual-only spindle position; zero at the designated static ride height.
    double WheelTravelCm = SuspensionVisualMinTravelCm;
};

struct SuspensionResponse final
{
    bool Valid = false;
    int ContactCount = 0;
    double TotalUpForce = 0.0;
    // Authoritative weight for this *sampled* mass/gravity pair. Used only
    // to cap arcade traction by actual spring loading, never to add force.
    double WeightForce = 0.0;
    std::array<SuspensionCornerResponse, 4> Corners{};
};

// Fail closed before any per-corner force is submitted. Forces scale with the
// genuine Chaos mass so changing car mass will not change equilibrium height.
// A one-sided road edge still has independent wheel forces, causing natural
// pitch/roll in the now-unlocked rigid body without invented contact.
inline SuspensionResponse CalculateSprungChassis(
    const SuspensionFrame& frame) noexcept
{
    if (!frame.PhysicsReady ||
        !std::isfinite(frame.DeltaSeconds) ||
        frame.DeltaSeconds <= 0.0 ||
        frame.DeltaSeconds > SuspensionMaximumFrameSeconds ||
        !std::isfinite(frame.MassKg) ||
        frame.MassKg < 100.0 || frame.MassKg > 15000.0 ||
        !std::isfinite(frame.GravityCmPerSecondSquared) ||
        frame.GravityCmPerSecondSquared < 100.0 ||
        frame.GravityCmPerSecondSquared > 2000.0)
    {
        return {};
    }

    for (const auto& c : frame.Corners)
    {
        if (!c.Hit) continue;
        if (!std::isfinite(c.DistanceCm) ||
            !std::isfinite(c.NormalZ) ||
            !std::isfinite(c.PointVerticalSpeedCmPerSecond) ||
            c.DistanceCm < 0.0 ||
            c.DistanceCm > SuspensionMaxTraceDistanceCm ||
            c.NormalZ < -1.0 || c.NormalZ > 1.0 ||
            std::abs(c.PointVerticalSpeedCmPerSecond) > 15000.0)
        {
            return {};
        }
    }

    const double quarterMass = frame.MassKg * 0.25;
    const double spring = quarterMass * frame.GravityCmPerSecondSquared /
        (SuspensionRestDistanceCm - SuspensionTargetRideDistanceCm);
    const double damping = 2.0 * SuspensionDampingRatio *
        std::sqrt(spring * quarterMass);
    const double maxCornerForce = frame.MassKg *
        frame.GravityCmPerSecondSquared *
        SuspensionMaxPerCornerWeightFactor;

    SuspensionResponse result;
    result.Valid = true;
    result.WeightForce = frame.MassKg * frame.GravityCmPerSecondSquared;
    for (unsigned i = 0; i < result.Corners.size(); ++i)
    {
        const auto& observed = frame.Corners[i];
        auto& output = result.Corners[i];
        // Missing/steep/far contact is not a wheel ground support. Visually,
        // wheels droop to their bounded extension until the ray sees road.
        if (!observed.Hit || observed.NormalZ < SuspensionMinNormalZ ||
            observed.DistanceCm > SuspensionRestDistanceCm)
        {
            continue;
        }
        output.Contact = true;
        ++result.ContactCount;
        const double penetration =
            SuspensionRestDistanceCm - observed.DistanceCm;
        // Damping acts against actual point motion, not only COM velocity:
        // compression on one side can calm roll without braking yaw.
        const double requested = spring * penetration -
            damping * observed.PointVerticalSpeedCmPerSecond;
        output.UpForce = std::clamp(requested, 0.0, maxCornerForce);
        output.WheelTravelCm = std::clamp(
            SuspensionTargetRideDistanceCm - observed.DistanceCm,
            SuspensionVisualMinTravelCm, SuspensionVisualMaxTravelCm);
        result.TotalUpForce += output.UpForce;
    }
    return result;
}
// The legacy road query allowed center heights up to 115 cm for coarse
// advisory traction. A sprung body has REAL road support only when its
// individual 82 cm spring contacts engage. Keep drive, brake, yaw, wear,
// pit service and the HUD on the identical constrained support estimate.
inline ChassisGroundContactOutput ConstrainGroundContactToSuspension(
    ChassisGroundContactOutput coarse,
    const SuspensionResponse& sprung) noexcept
{
    if (!coarse.Valid || !sprung.Valid ||
        sprung.ContactCount < 0 || sprung.ContactCount > 4 ||
        sprung.ContactCount > coarse.SupportingProbes ||
        !std::isfinite(sprung.TotalUpForce) ||
        !std::isfinite(sprung.WeightForce) ||
        sprung.TotalUpForce < 0.0 ||
        sprung.WeightForce <= 0.0 ||
        sprung.TotalUpForce > sprung.WeightForce *
            SuspensionMaxPerCornerWeightFactor * 4.0)
    {
        return {};
    }
    coarse.SupportingProbes = sprung.ContactCount;
    const double countBasedGrip = sprung.ContactCount == 4 ? 1.0 :
        sprung.ContactCount == 3 ? 0.75 :
        sprung.ContactCount == 2 ? 0.45 : 0.0;
    // A ray geometrically within 82 cm is not sufficient for tyre grip:
    // near full extension, or on upward rebound, the damped spring can
    // carry no weight. Use its *actual bounded positive load* to cap
    // the legacy traction budget. At flat equilibrium the old 4/4,
    // 3/4 and 2/4 behavior is preserved; unsupported wheels cannot
    // still launch nitro, steer, brake or accumulate fictitious wear.
    const double supportedLoadFraction = std::clamp(
        sprung.TotalUpForce / sprung.WeightForce, 0.0, 1.0);
    coarse.ContactTractionScale =
        std::min(countBasedGrip, supportedLoadFraction);
    coarse.HasDriveContact = sprung.ContactCount >= 2 &&
        coarse.ContactTractionScale > 1.0e-6;
    return coarse;
}

} // namespace latebrake
