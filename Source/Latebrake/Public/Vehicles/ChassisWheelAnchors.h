#pragma once

#include <array>
#include <cstddef>

namespace latebrake
{
// The chassis Cube is scaled in Unreal, but its attached visuals divide
// authored centimeter locations by that scale before attachment. Keep
// ACTUAL world-space centimeters here, independent of mesh scale.
// Order is ALWAYS front-left, front-right, rear-left, rear-right;
// it is the order of Chaos ray samples, spring forces, axle budget,
// decorative spindles and contact-readout QA.
inline constexpr std::size_t ChassisWheelCount = 4;
inline constexpr double ChassisHalfWheelbaseCm = 122.0;
inline constexpr double ChassisHalfTrackCm = 94.0;
inline constexpr double ChassisWheelSpindleHeightCm = -29.0;
inline constexpr double ChassisWheelVisualRadiusCm = 29.0;

struct ChassisWheelAnchor final
{
    double ForwardCm = 0.0;
    double RightCm = 0.0;
    double SpindleHeightCm = 0.0;
};

inline constexpr std::array<ChassisWheelAnchor, ChassisWheelCount>
    ChassisWheelAnchors{{
        { ChassisHalfWheelbaseCm, -ChassisHalfTrackCm,
          ChassisWheelSpindleHeightCm },
        { ChassisHalfWheelbaseCm,  ChassisHalfTrackCm,
          ChassisWheelSpindleHeightCm },
        {-ChassisHalfWheelbaseCm, -ChassisHalfTrackCm,
          ChassisWheelSpindleHeightCm },
        {-ChassisHalfWheelbaseCm,  ChassisHalfTrackCm,
          ChassisWheelSpindleHeightCm }
    }};

// The physics ray begins at the chassis's wheel-aligned XY frame, not
// at the lowered decorative spindle (which moves with spring travel).
// The force remains world +Z at that exact immutable per-corner anchor.
// This avoids a visual wheel travel feedback loop altering physics.
//
// The car's world-forward/right basis rotates all four anchors together;
// visual spindles use the SAME X/Y but add bounded visual-only Z travel.
constexpr ChassisWheelAnchor ChassisProbeOffset(unsigned corner) noexcept
{
    const auto& a = ChassisWheelAnchors[corner];
    return {a.ForwardCm, a.RightCm, 0.0};
}
constexpr ChassisWheelAnchor ChassisVisualOffset(
    unsigned corner, double measuredTravelCm) noexcept
{
    const auto& a = ChassisWheelAnchors[corner];
    return {a.ForwardCm, a.RightCm, a.SpindleHeightCm + measuredTravelCm};
}
static_assert(ChassisWheelAnchors[0].ForwardCm ==
              ChassisWheelAnchors[1].ForwardCm);
static_assert(ChassisWheelAnchors[2].ForwardCm ==
              ChassisWheelAnchors[3].ForwardCm);
static_assert(ChassisWheelAnchors[0].RightCm ==
              ChassisWheelAnchors[2].RightCm);
static_assert(ChassisWheelAnchors[1].RightCm ==
              ChassisWheelAnchors[3].RightCm);
static_assert(ChassisWheelVisualRadiusCm -
              ChassisWheelSpindleHeightCm == 58.0);
} // namespace latebrake
