#pragma once

#include "AI/RivalFleetTraffic.h"
#include "AI/RivalRouteProgress.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// Only the opt-in fleet can defer stall recovery for a legitimate queue.
// A stopped car and every potential blocker must be physically observed.
// No current overlap, reversing, unknown body, unrelated rear vehicle or
// moving-oncoming opponent qualifies as a queued vehicle ahead.
inline constexpr double FleetQueueBudgetSeconds=12.0;
inline constexpr double FleetQueueMaximumSelfSpeedCmS=80.0;
inline constexpr double FleetQueueMaximumOtherSpeedCmS=250.0;
inline constexpr double FleetQueueMaximumAheadCm=1150.0;
inline constexpr double FleetQueueMaximumLateralCm=290.0;
inline constexpr double FleetQueueMaximumHeadingErrorDegrees=35.0;
inline constexpr double FleetQueueMinimumAheadCm=300.0;
inline constexpr double FleetQueueTimeTolerance=1.0e-9;

enum class FleetQueueStatus
{
    Inactive,
    NoVerifiedQueue,
    Waiting,
    BudgetExhausted,
    InvalidRoute,
    InvalidFrame
};

struct FleetQueueEvidence final
{
    bool ValidRoster=false;
    int BlockerSlot=-1;
    double ForwardDistanceCm=0.0;
    int SelfSlot=-1;
    std::array<std::uint64_t,StartGridCapacity> Identity{};
    bool HasQueue() const noexcept {return ValidRoster && BlockerSlot>=0;}
};

inline FleetQueueEvidence InspectFleetQueue(const RivalFleetTrafficSample& sample) noexcept
{
    FleetQueueEvidence result;
    if(!sample.SessionQualified || sample.BlockerCount!=MaxRecoveryObstacles ||
        !ValidFleetPhysicalObservation(sample.Self) ||
        sample.Self.Slot==HumanGridSlot)
        return result;
    std::array<std::uint64_t,StartGridCapacity> tokens{};
    tokens[static_cast<std::size_t>(sample.Self.Slot)]=sample.Self.ActorToken;
    for(int i=0;i<sample.BlockerCount;++i)
    {
        const auto& other=sample.Blockers[static_cast<std::size_t>(i)];
        if(!ValidFleetPhysicalObservation(other) ||
            tokens[static_cast<std::size_t>(other.Slot)]!=0)
            return result;
        for(auto token:tokens)
            if(token==other.ActorToken)return result;
        tokens[static_cast<std::size_t>(other.Slot)]=other.ActorToken;
    }
    for(auto token:tokens)if(token==0)return result;
    result.ValidRoster=true;
    result.SelfSlot=sample.Self.Slot;
    result.Identity=tokens;
    if(std::hypot(sample.Self.VelocityX,sample.Self.VelocityY)>=FleetQueueMaximumSelfSpeedCmS)
        return result;
    const auto body=MakeStartGridRect(sample.Self.Pose);
    if(!RecoveryChassisFitsPhysicalAsphalt(sample.Self.Pose) ||
        StartGridFootprintTouchesBarrier(sample.Self.Pose))return result;
    for(int i=0;i<sample.BlockerCount;++i)
    {
        const auto& other=sample.Blockers[static_cast<std::size_t>(i)];
        if(std::hypot(other.VelocityX,other.VelocityY)>FleetQueueMaximumOtherSpeedCmS ||
            !RecoveryChassisFitsPhysicalAsphalt(other.Pose) ||
            StartGridFootprintTouchesBarrier(other.Pose) ||
            StartGridFootprintsOverlap(sample.Self.Pose,other.Pose))
            continue;
        const double yawError=std::abs(std::remainder(
            other.Pose.HeadingDegrees-sample.Self.Pose.HeadingDegrees,360.0));
        if(yawError>FleetQueueMaximumHeadingErrorDegrees)continue;
        const double dx=other.Pose.Position.X-sample.Self.Pose.Position.X;
        const double dy=other.Pose.Position.Y-sample.Self.Pose.Position.Y;
        const double ahead=dx*body.ForwardX+dy*body.ForwardY;
        const double lateral=std::abs(dx*body.RightX+dy*body.RightY);
        const double forwardSpeed=other.VelocityX*body.ForwardX+
            other.VelocityY*body.ForwardY;
        if(ahead<FleetQueueMinimumAheadCm || ahead>FleetQueueMaximumAheadCm ||
            lateral>FleetQueueMaximumLateralCm || forwardSpeed<-10.0)
            continue;
        if(result.BlockerSlot<0 || ahead<result.ForwardDistanceCm ||
            (ahead==result.ForwardDistanceCm && other.Slot<result.BlockerSlot))
        {
            result.BlockerSlot=other.Slot;
            result.ForwardDistanceCm=ahead;
        }
    }
    return result;
}

struct FleetQueueReadout final
{
    FleetQueueStatus Status=FleetQueueStatus::Inactive;
    int GateOrdinal=-1;
    int BlockerSlot=-1;
    double DeferredSeconds=0.0;
    double RemainingSeconds=FleetQueueBudgetSeconds;
    bool DeferStallRecovery=false;
};

struct FleetQueueFrameStamp final
{
    std::uint64_t Attempt=0;
    std::uint64_t Frame=0;
    std::uint64_t EngineFrame=0;
    std::uint64_t SessionEpoch=0;
    std::uint64_t TeleportRevision=0;

    bool Valid() const noexcept
    {
        return Attempt!=0 && Frame!=0 && EngineFrame!=0;
    }

    bool FreshAfter(const FleetQueueFrameStamp& previous) const noexcept
    {
        return Valid() && previous.Valid() && Attempt==previous.Attempt &&
            Frame>previous.Frame && EngineFrame>previous.EngineFrame &&
            SessionEpoch>=previous.SessionEpoch && TeleportRevision>=previous.TeleportRevision;
    }
};

class FleetQueueDeferral final
{
public:
    static constexpr int MaxGateOrdinal=25600;

    void Reset() noexcept {State={};LastStamp={};Identity={};SelfSlot=-1;}
    // Pauses, invalid frames and missing observations clear current
    // permission, not the per-gate budget. The same route cannot acquire
    // a new wait allowance by repeatedly losing and regaining evidence.
    void Suspend() noexcept
    {
        State.Status=FleetQueueStatus::Inactive;
        State.BlockerSlot=-1;
        State.DeferStallRecovery=false;
    }

    FleetQueueReadout Readout() const noexcept {return State;}

    FleetQueueReadout Observe(int acceptedGateOrdinal,double deltaSeconds,
        const RivalFleetTrafficSample& physical,const FleetQueueFrameStamp& stamp) noexcept
    {
        Suspend();
        if(!stamp.Valid() || (LastStamp.Valid() && !stamp.FreshAfter(LastStamp)))
        {
            State.Status=FleetQueueStatus::InvalidFrame;
            return State;
        }
        // A real frame is consumed once, even when no queue is present.
        // Invalid snapshots cannot be repaired and replayed in that frame.
        LastStamp=stamp;
        if(acceptedGateOrdinal<0 || acceptedGateOrdinal>=MaxGateOrdinal ||
            acceptedGateOrdinal<State.GateOrdinal)
        {
            State.Status=FleetQueueStatus::InvalidRoute;
            return State;
        }
        if(!RivalRouteProgress::HasQualifiedFrameTime(deltaSeconds))
            return State;
        const auto evidence=InspectFleetQueue(physical);
        if(!evidence.ValidRoster || (SelfSlot>=0 &&
            (evidence.SelfSlot!=SelfSlot || evidence.Identity!=Identity)))
        {
            State.Status=FleetQueueStatus::NoVerifiedQueue;
            return State;
        }
        Identity=evidence.Identity;
        SelfSlot=evidence.SelfSlot;
        if(acceptedGateOrdinal>State.GateOrdinal)
        {
            // A valid physical roster at an advanced route can earn
            // a new allowance. A malformed roster cannot reset it.
            State={};
            State.GateOrdinal=acceptedGateOrdinal;
        }
        if(State.RemainingSeconds<=FleetQueueTimeTolerance)
        {
            State.RemainingSeconds=0.0;
            State.Status=FleetQueueStatus::BudgetExhausted;
            return State;
        }

        if(!evidence.HasQueue())
        {
            State.Status=FleetQueueStatus::NoVerifiedQueue;
            return State;
        }
        // No partial-frame accounting: the route observation consumes
        // the whole frame. Refuse rather than exceed the budget.
        if(deltaSeconds>State.RemainingSeconds+FleetQueueTimeTolerance)
        {
            State.Status=FleetQueueStatus::BudgetExhausted;
            State.RemainingSeconds=0.0;
            return State;
        }
        State.DeferredSeconds=std::min(FleetQueueBudgetSeconds,
            State.DeferredSeconds+deltaSeconds);
        State.RemainingSeconds=std::max(0.0,FleetQueueBudgetSeconds-State.DeferredSeconds);
        State.Status=FleetQueueStatus::Waiting;
        State.BlockerSlot=evidence.BlockerSlot;
        State.DeferStallRecovery=true;
        return State;
    }
private:
    FleetQueueReadout State{};
    FleetQueueFrameStamp LastStamp{};
    std::array<std::uint64_t,StartGridCapacity> Identity{};
    int SelfSlot=-1;
};
} // namespace latebrake
