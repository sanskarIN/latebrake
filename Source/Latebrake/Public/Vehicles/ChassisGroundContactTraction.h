#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace latebrake
{
// These are four OBSERVED vertical traces around the real Chaos chassis,
// not independently physical wheels or a suspension implementation.
// Distances are from the chassis CENTRE to blocking WorldStatic ground.
struct ChassisGroundProbe final
{
    bool Hit = false;
    double DistanceCm = 0.0;
    double GroundNormalZ = 0.0;
};

struct ChassisGroundContactSample final
{
    double DeltaSeconds = 0.0;
    double ChassisVerticalSpeedCmPerSecond = 0.0;
    bool PhysicsReady = false;
    std::array<ChassisGroundProbe, 4> Probes{};
};

struct ChassisGroundContactOutput final
{
    bool Valid = false;
    int SupportingProbes = 0;
    // Conservative shared force scale for propulsion, service braking,
    // lateral tire-like force and yaw torque. NOT a physical normal load.
    double ContactTractionScale = 0.0;
    bool HasDriveContact = false;
};

inline constexpr double ChassisContactMaxFrameSeconds = 0.10;
inline constexpr double ChassisContactProbeTraceLengthCm = 280.0;
inline constexpr double ChassisContactMaximumSupportedHeightCm = 115.0;
inline constexpr double ChassisContactMinimumGroundNormalZ = 0.68;

// A single wheel/corner touching the road is not enough to drive a
// 1100kg chassis. Restore full physical force only with all four
// qualified downward contacts. This avoids artificial mid-air
// accelerator/yaw control and overly strong grip when leaving road edge.
inline ChassisGroundContactOutput EvaluateChassisGroundContact(
    ChassisGroundContactSample s) noexcept
{
    if (!s.PhysicsReady ||
        !std::isfinite(s.DeltaSeconds) ||
        s.DeltaSeconds <= 0.0 ||
        s.DeltaSeconds > ChassisContactMaxFrameSeconds ||
        !std::isfinite(s.ChassisVerticalSpeedCmPerSecond) ||
        std::abs(s.ChassisVerticalSpeedCmPerSecond) > 15000.0)
    {
        return {};
    }
    int supporting = 0;
    for (const ChassisGroundProbe& probe : s.Probes)
    {
        // A false trace must not use stale hit normal or distance.
        if (!probe.Hit) continue;
        if (!std::isfinite(probe.DistanceCm) ||
            !std::isfinite(probe.GroundNormalZ) ||
            probe.DistanceCm < 0.0 ||
            probe.DistanceCm > ChassisContactProbeTraceLengthCm ||
            probe.GroundNormalZ < -1.0 ||
            probe.GroundNormalZ > 1.0)
        {
            return {};
        }
        if (probe.DistanceCm <= ChassisContactMaximumSupportedHeightCm &&
            probe.GroundNormalZ >= ChassisContactMinimumGroundNormalZ)
        {
            ++supporting;
        }
    }
    // Do NOT apply gravity compensation or synthesize ground contact.
    // Real Chaos ground collision remains entirely in charge.
    const double scale = supporting >= 4 ? 1.0 :
        supporting == 3 ? 0.75 :
        supporting == 2 ? 0.45 : 0.0;
    return {true, supporting, scale, supporting >= 2};
}
} // namespace latebrake
