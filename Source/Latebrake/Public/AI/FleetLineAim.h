#pragma once

#include "AI/RivalGateRunThrough.h"
#include "AI/RivalRecoveryOccupancy.h"
#include "Racing/CircuitGateCrossing.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Distinct two-lane AI targets for the original closed virtual circuit.
// This is a cosmetic/gameplay route-planning experiment, not a vehicle
// physics model. No passing maneuvers, gate credit, force, teleport,
// throttle or race classification are supplied by this geometry.
// Each slot retains its original grid-side preference.
enum class FleetLineStatus
{
    Invalid,
    OriginalTarget,
    LaneTarget
};

struct FleetLineSample final
{
    int Slot=-1;
    int NextGate=-1;
    CircuitPose Actual{};
    double SpeedCmS=0.0;
    bool PhysicalReady=false;
};

struct FleetLineAim final
{
    FleetLineStatus Status=FleetLineStatus::Invalid;
    CircuitPoint Target{};
    double CurrentAlongCm=0.0;
    double CurrentLateralCm=0.0;
    double TargetAlongCm=0.0;
    double TargetLateralCm=0.0;
};

struct FleetIncomingStraight final
{
    CircuitPoint Gate{};
    CircuitPoint Forward{};
    CircuitPoint Right{};
    double Heading=0.0;
    double LengthCm=0.0;
    int Slab=-1;
};

inline constexpr double FleetLanePreferredOffsetCm=270.0;
inline constexpr double FleetLaneGateOffsetCm=170.0;
inline constexpr double FleetLaneMaxLateralChangeCm=150.0;
inline constexpr double FleetLaneMaxCurrentLateralCm=500.0;
inline constexpr double FleetLaneMaxHeadingErrorDegrees=30.0;
inline constexpr double FleetLaneLookaheadMinimumCm=950.0;
inline constexpr double FleetLaneLookaheadMaximumCm=2100.0;
inline constexpr double FleetLaneLookaheadSeconds=0.55;
inline constexpr double FleetLaneTaperDistanceCm=1800.0;
inline constexpr double FleetLaneEntryAllowanceCm=700.0;
inline constexpr double FleetLaneMinAheadCm=150.0;
inline constexpr double FleetLaneStaticInsetCm=5.0;
static_assert(FleetLanePreferredOffsetCm+FleetLaneMaxLateralChangeCm<
    GateLateralHalfWidthCm);

inline FleetIncomingStraight DescribeFleetIncomingStraight(int gate) noexcept
{
    if(!IsCircuitGate(gate))return {};
    const auto end=CircuitGate(gate);
    switch(gate)
    {
    case 0:return {end,{1.0,0.0},{0.0,1.0},0.0,EastX-WestX,1};
    case 1:return {end,{0.0,1.0},{-1.0,0.0},90.0,NorthY-SouthY,3};
    case 2:return {end,{-1.0,0.0},{0.0,-1.0},180.0,EastX-WestX,2};
    case 3:return {end,{0.0,-1.0},{1.0,0.0},-90.0,NorthY-SouthY,4};
    default:return {};
    }
}

inline double FleetLaneAlong(const FleetIncomingStraight& straight,CircuitPoint point) noexcept
{
    return (point.X-straight.Gate.X)*straight.Forward.X+
        (point.Y-straight.Gate.Y)*straight.Forward.Y;
}
inline double FleetLaneLateral(const FleetIncomingStraight& straight,CircuitPoint point) noexcept
{
    return (point.X-straight.Gate.X)*straight.Right.X+
        (point.Y-straight.Gate.Y)*straight.Right.Y;
}
inline CircuitPoint FleetLaneWorld(const FleetIncomingStraight& straight,
    double along,double lateral) noexcept
{
    return {straight.Gate.X+straight.Forward.X*along+straight.Right.X*lateral,
        straight.Gate.Y+straight.Forward.Y*along+straight.Right.Y*lateral};
}

// A lane target may be planned only inside one authored asphalt slab.
// Using the same slab for both endpoints and padded oriented rectangles
// makes the translation's convex corridor contained in the actual slab.
// Rotation is not simulated; the actual current body and cardinal target
// footprint both require clearance. The existing corner/brake/traffic
// policies still constrain any subsequently applied pilot command.
inline bool FleetLanePoseFitsSlab(CircuitPose pose,int slab) noexcept
{
    if(!IsValidStartGridPose(pose) || slab<0 || slab>=CircuitCollisionBlockCount)
        return false;
    const auto block=PlannedCollisionBlock(slab);
    if(block.Style!=CollisionBlockStyle::Asphalt || !IsValidCollisionBlock(block))
        return false;
    const auto rectangle=MakeStartGridRect(pose);
    const double halfX=StartGridProjectionRadius(rectangle,1.0,0.0);
    const double halfY=StartGridProjectionRadius(rectangle,0.0,1.0);
    return std::abs(pose.Position.X-block.Center.X)+halfX+FleetLaneStaticInsetCm
                <=block.ScaleX*50.0 &&
        std::abs(pose.Position.Y-block.Center.Y)+halfY+FleetLaneStaticInsetCm
                <=block.ScaleY*50.0 &&
        !StartGridFootprintTouchesBarrier(pose);
}

inline double FleetLanePreference(int slot) noexcept
{
    if(slot<=HumanGridSlot || slot>=StartGridCapacity)return 0.0;
    const double originalOffset=StartGridPoses[static_cast<std::size_t>(slot)].Position.Y-SouthY;
    return originalOffset<0.0?-FleetLanePreferredOffsetCm:FleetLanePreferredOffsetCm;
}

inline FleetLineAim PlanFleetLineAim(const FleetLineSample& sample) noexcept
{
    FleetLineAim result;
    if(sample.Slot<=HumanGridSlot || sample.Slot>=StartGridCapacity ||
        !IsCircuitGate(sample.NextGate))
        return result;
    result.Target=RivalGateRunThroughPoint(sample.NextGate);
    result.Status=FleetLineStatus::OriginalTarget;
    if(!sample.PhysicalReady || !IsValidStartGridPose(sample.Actual) ||
        !std::isfinite(sample.SpeedCmS) || sample.SpeedCmS<0.0 || sample.SpeedCmS>10000.0)
        return result;

    const auto straight=DescribeFleetIncomingStraight(sample.NextGate);
    const double along=FleetLaneAlong(straight,sample.Actual.Position);
    const double lateral=FleetLaneLateral(straight,sample.Actual.Position);
    result.CurrentAlongCm=along;
    result.CurrentLateralCm=lateral;
    const double yawError=std::abs(std::remainder(
        sample.Actual.HeadingDegrees-straight.Heading,360.0));
    const double entryAllowance=sample.NextGate==0?FleetLaneEntryAllowanceCm:0.0;
    if(along < -straight.LengthCm-entryAllowance ||
        along > RivalGateRunThroughDistanceCm-FleetLaneMinAheadCm ||
        std::abs(lateral)>FleetLaneMaxCurrentLateralCm ||
        yawError>FleetLaneMaxHeadingErrorDegrees ||
        !FleetLanePoseFitsSlab(sample.Actual,straight.Slab))
        return result;

    const double lookahead=std::clamp(sample.SpeedCmS*FleetLaneLookaheadSeconds,
        FleetLaneLookaheadMinimumCm,FleetLaneLookaheadMaximumCm);
    const double targetAlong=std::min(RivalGateRunThroughDistanceCm,along+lookahead);
    // Gradually narrow the preferred line near the crossing corridor;
    // remain within the real 500cm gate half-width without changing it.
    const double blend=std::clamp(-targetAlong/FleetLaneTaperDistanceCm,0.0,1.0);
    const double laneMagnitude=FleetLaneGateOffsetCm+
        blend*(FleetLanePreferredOffsetCm-FleetLaneGateOffsetCm);
    const double desired=std::copysign(laneMagnitude,FleetLanePreference(sample.Slot));
    const double targetLateral=std::clamp(lateral+
        std::clamp(desired-lateral,-FleetLaneMaxLateralChangeCm,FleetLaneMaxLateralChangeCm),
        -FleetLanePreferredOffsetCm,FleetLanePreferredOffsetCm);
    // At an extreme current lateral coordinate the final clamp could
    // otherwise exceed the bounded correction. Fall back instead.
    if(std::abs(targetLateral-lateral)>FleetLaneMaxLateralChangeCm ||
        targetAlong-along<FleetLaneMinAheadCm ||
        std::abs(targetLateral)>=GateLateralHalfWidthCm)
        return result;

    const auto target=FleetLaneWorld(straight,targetAlong,targetLateral);
    // Check both actual and cardinal heading at endpoints. During a
    // corner, off-road, invalid pose or geometry failure, preserve the
    // original run-through target and established recovery behavior.
    if(!FleetLanePoseFitsSlab({sample.Actual.Position,straight.Heading},straight.Slab) ||
        !FleetLanePoseFitsSlab({target,straight.Heading},straight.Slab) ||
        !FleetLanePoseFitsSlab({target,sample.Actual.HeadingDegrees},straight.Slab))
        return result;

    result.Status=FleetLineStatus::LaneTarget;
    result.Target=target;
    result.TargetAlongCm=targetAlong;
    result.TargetLateralCm=targetLateral;
    return result;
}
} // namespace latebrake
