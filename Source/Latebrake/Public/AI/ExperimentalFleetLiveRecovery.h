#pragma once

#include "AI/ExperimentalFleetAttempt.h"
#include "AI/PlayerFleetRecoveryClearance.h"
#include "AI/SingleRivalRecoverySelection.h"
#include "Racing/PlayerRecoveryClearance.h"

namespace latebrake
{
// Every physical blocker must be represented before either a player
// fee transaction or an autonomous recovery may attempt a teleport.
// This validates the original bound roster and the current live one.
inline bool MatchLiveFleetIdentity(const FleetPhysicalFrame& live,
    const SixCarIdentity& bound) noexcept
{
    const auto inspected=InspectFleetRoster(live);
    return inspected.Valid() && inspected.Tokens==bound;
}

inline bool BuildLiveFleetBlockers(const FleetPhysicalFrame& live,
    const SixCarIdentity& bound,int subject,std::uint64_t token,
    std::array<RecoveryObstacle,MaxRecoveryObstacles>& blockers,int& count) noexcept
{
    blockers={};
    count=0;
    if (subject<HumanGridSlot || subject>=StartGridCapacity ||
        token==0 || bound[static_cast<std::size_t>(subject)]!=token ||
        live.Inactive || !MatchLiveFleetIdentity(live,bound))
        return false;
    for (int slot=0;slot<StartGridCapacity;++slot)
    {
        if (slot!=subject)
            blockers[static_cast<std::size_t>(count++)]=live.Cars[static_cast<std::size_t>(slot)];
    }
    return count==MaxRecoveryObstacles;
}

inline PlayerRecoveryClearance SelectFleetPlayerRecovery(int gate,
    const FleetPhysicalFrame& live,const SixCarIdentity& bound,
    std::uint64_t playerToken) noexcept
{
    PlayerRecoveryClearance result;
    if (!IsCircuitGate(gate)) return result;
    result.Gate=gate;
    result.Status=PlayerRecoveryClearanceStatus::InvalidLanding;
    const auto pose=PlayerRecovery(gate);
    if (!RecoveryChassisFitsPhysicalAsphalt(pose) ||
        StartGridFootprintTouchesBarrier(pose) ||
        EvaluateCircuitSpawnPose(pose)!=CircuitSpawnDecision::Safe)
        return result;
    std::array<RecoveryObstacle,MaxRecoveryObstacles> blockers{};
    int count=0;
    result.Status=PlayerRecoveryClearanceStatus::UnknownTraffic;
    if (!BuildLiveFleetBlockers(live,bound,HumanGridSlot,playerToken,blockers,count))
        return result;
    const auto clearance=EvaluatePlayerFleetRecoveryClearance(pose,blockers,count);
    if (!clearance.Safe())
    {
        if (clearance.Reason==RecoveryClearanceReason::TrafficCollision)
            result.Status=PlayerRecoveryClearanceStatus::Occupied;
        return result;
    }
    result.Status=PlayerRecoveryClearanceStatus::Clear;
    result.Pose=pose;
    return result;
}

struct FleetRivalRecovery final
{
    bool Clear=false;
    CircuitPose Pose{};
    RecoveryClearanceReason Reason=RecoveryClearanceReason::UnknownOccupancy;
    int Evaluated=0;
};

inline FleetRivalRecovery SelectFleetRivalRecovery(int gate,int subject,
    std::uint64_t token,const FleetPhysicalFrame& live,const SixCarIdentity& bound) noexcept
{
    FleetRivalRecovery result;
    if (!IsCircuitGate(gate) || subject<=HumanGridSlot || subject>=StartGridCapacity)
    {
        result.Reason=RecoveryClearanceReason::InvalidTarget;
        return result;
    }
    std::array<RecoveryObstacle,MaxRecoveryObstacles> blockers{};
    int count=0;
    if (!BuildLiveFleetBlockers(live,bound,subject,token,blockers,count))
        return result;
    // Preserve longitudinal progress. Rotate preference across rival slots
    // without granting any new waypoint or modifying a player landing.
    const bool rightFirst=(subject%2)==0;
    const std::array<double,5> offsets=rightFirst
        ? std::array<double,5>{400.0,-400.0,0.0,600.0,-600.0}
        : std::array<double,5>{-400.0,400.0,0.0,-600.0,600.0};
    for (double offset:offsets)
    {
        CircuitPose pose{};
        if (!TrySingleRivalRecoveryPose(gate,offset,pose)) continue;
        ++result.Evaluated;
        const auto clearance=EvaluateRivalRecoveryClearance(pose,blockers,count);
        result.Reason=clearance.Reason;
        if (clearance.Safe())
        {
            result.Clear=true;
            result.Pose=pose;
            return result;
        }
        if (clearance.Reason==RecoveryClearanceReason::UnknownOccupancy)
            return result;
    }
    return result;
}

inline bool MakeFreshFleetTraffic(const FleetPhysicalFrame& live,
    const SixCarIdentity& bound,int subject,std::uint64_t token,
    bool brakeQualified,double brakeScale,RivalFleetTrafficSample& out) noexcept
{
    out={};
    if (subject<=HumanGridSlot || subject>=StartGridCapacity ||
        (brakeQualified && (!std::isfinite(brakeScale) || brakeScale<=0.0 || brakeScale>1.0)))
        return false;
    if (!BuildLiveFleetBlockers(live,bound,subject,token,out.Blockers,out.BlockerCount))
        return false;
    out.Self=live.Cars[static_cast<std::size_t>(subject)];
    out.SessionQualified=true;
    out.BrakeObservationQualified=brakeQualified;
    out.ConservativeBrakeScale=brakeQualified?brakeScale:0.0;
    return true;
}
} // namespace latebrake
