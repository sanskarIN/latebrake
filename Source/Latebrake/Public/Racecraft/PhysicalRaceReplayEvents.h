#pragma once
#include "Racecraft/PhysicalSixCarRaceTrace.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr std::size_t PhysicalReplayEventCapacity=256;
enum class PhysicalReplayEventKind : std::uint8_t
{
    ContactBegan, ContactEnded, ClosePass, FollowGap,
    CollisionRiskBegan, CollisionRiskEnded
};
enum class PhysicalReplayEventFault : std::uint8_t
{
    None, InvalidSource, InconsistentRoster, StaleFrame,
    LostPhysicsContinuity
};
struct PhysicalReplayEventSettings final
{
    double MinimumContactPersistenceSeconds=0.0;
    double MinimumClosePassSpeedCmS=70.0;
    double NearMissLongitudinalCm=220.0;
    double NearMissLateralCm=120.0;
    double FollowingGapCm=500.0;
    double MinimumFollowingClosingCmS=120.0;
    double MaximumInterFrameSeconds=MaximumFrameTimeSeconds;
    double MaximumClockSkewSeconds=0.02;
    double MaximumRelativeDisplacementCm=1900.0;
    double PositionUncertaintyCm=80.0;
    double MinimumConfidence=0.8;
    bool RequireFullPhysicalRoster=true;
    bool RequireMonotonicFrames=true;
    bool NeverWriteOfficialIncidentOrPenalty=true;
};
struct PhysicalReplayEvent final
{
    PhysicalReplayEventKind Kind=PhysicalReplayEventKind::ContactBegan;
    RaceIdentity Source{};
    std::uint64_t FirstActor=0;
    std::uint64_t SecondActor=0;
    int FirstSlot=-1;
    int SecondSlot=-1;
    double PhysicsClockSeconds=0.0;
    double MeasuredLongitudinalClearanceCm=0.0;
    double MeasuredLateralClearanceCm=0.0;
    double MeasuredClosingSpeedCmS=0.0;
    double MeasuredCenterDistanceCm=0.0;
    bool EngineCollisionEventFired=false;
    bool OfficialPenaltyIssued=false;
    bool ActualVehicleControlModified=false;
    bool Valid=false;
};
struct PhysicalReplayEventStatistics final
{
    std::size_t StoredEvents=0;
    std::uint64_t AcceptedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t RosterResets=0;
    std::uint64_t ContactsBegan=0;
    std::uint64_t ContactsEnded=0;
    std::uint64_t ClosePassEvents=0;
    std::uint64_t FollowGapEvents=0;
    std::uint64_t CollisionRiskEvents=0;
    double MaximumMeasuredClosingSpeedCmS=0.0;
    double MinimumMeasuredClearanceCm=
        std::numeric_limits<double>::infinity();
    bool OfficialRaceControlUntouched=true;
};
inline bool ValidPhysicalReplayEventSettings(
    PhysicalReplayEventSettings p) noexcept
{
    return Finite(p.MinimumContactPersistenceSeconds) &&
        p.MinimumContactPersistenceSeconds>=0.0 &&
        Finite(p.MinimumClosePassSpeedCmS) &&
        p.MinimumClosePassSpeedCmS>=0.0 &&
        Finite(p.NearMissLongitudinalCm) &&
        p.NearMissLongitudinalCm>0.0 &&
        Finite(p.NearMissLateralCm) &&
        p.NearMissLateralCm>0.0 &&
        Finite(p.FollowingGapCm) &&
        p.FollowingGapCm>0.0 &&
        Finite(p.MinimumFollowingClosingCmS) &&
        p.MinimumFollowingClosingCmS>0.0 &&
        Finite(p.MaximumInterFrameSeconds) &&
        p.MaximumInterFrameSeconds>0.0 &&
        p.MaximumInterFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        Finite(p.MaximumRelativeDisplacementCm) &&
        p.MaximumRelativeDisplacementCm>0.0 &&
        Finite(p.PositionUncertaintyCm) &&
        p.PositionUncertaintyCm>=0.0 &&
        Finite(p.MinimumConfidence) &&
        p.MinimumConfidence>0.0 &&
        p.MinimumConfidence<=1.0;
}
inline bool QualifyReplayEventPair(
    const PhysicalTracePair& pair,
    PhysicalReplayEventSettings p) noexcept
{
    return pair.Valid &&
        pair.FirstActor!=0 && pair.SecondActor!=0 &&
        pair.FirstActor!=pair.SecondActor &&
        pair.FirstSlot>=0 && pair.FirstSlot<PhysicalFleetCapacity &&
        pair.SecondSlot>=0 && pair.SecondSlot<PhysicalFleetCapacity &&
        pair.FirstSlot!=pair.SecondSlot &&
        Finite(pair.ForwardSeparationCm) &&
        Finite(pair.LateralSeparationCm) &&
        Finite(pair.LongitudinalClearanceCm) &&
        Finite(pair.LateralClearanceCm) &&
        Finite(pair.ClosingSpeedCmS) &&
        Finite(pair.CenterDistanceCm) &&
        p.MinimumConfidence>0.0;
}
inline PhysicalReplayEvent MakeReplayEvent(
    const PhysicalTraceFrame& source,
    const PhysicalTracePair& pair,
    PhysicalReplayEventKind kind) noexcept
{
    PhysicalReplayEvent event;
    if(!source.Valid || !pair.Valid ||
       !Finite(source.PhysicsClockSeconds))
        return event;
    event.Kind=kind;
    event.Source=source.Identity;
    event.FirstActor=pair.FirstActor;
    event.SecondActor=pair.SecondActor;
    event.FirstSlot=pair.FirstSlot;
    event.SecondSlot=pair.SecondSlot;
    event.PhysicsClockSeconds=source.PhysicsClockSeconds;
    event.MeasuredLongitudinalClearanceCm=
        pair.LongitudinalClearanceCm;
    event.MeasuredLateralClearanceCm=pair.LateralClearanceCm;
    event.MeasuredClosingSpeedCmS=pair.ClosingSpeedCmS;
    event.MeasuredCenterDistanceCm=pair.CenterDistanceCm;
    event.Valid=true;
    return event;
}
inline bool PhysicalEventSourceContinuous(
    const PhysicalTraceFrame& previous,
    const PhysicalTraceFrame& current,
    PhysicalReplayEventSettings p) noexcept
{
    if(!PhysicalTraceSameRoster(previous,current) ||
       current.Identity.Frame<=previous.Identity.Frame ||
       (p.RequireMonotonicFrames &&
        current.Identity.Frame!=previous.Identity.Frame+1))
        return false;
    const double dt=current.PhysicsClockSeconds-
        previous.PhysicsClockSeconds;
    if(!Finite(dt) || dt<=0.0 ||
       dt>p.MaximumInterFrameSeconds ||
       std::abs(dt-current.DeltaSeconds)>
           p.MaximumClockSkewSeconds)
        return false;
    for(int i=0;i<PhysicalFleetCapacity;++i)
    {
        const auto& before=previous.Actors[
            static_cast<std::size_t>(i)];
        const auto& after=current.Actors[
            static_cast<std::size_t>(i)];
        const double displacement=Distance(
            before.Pose.Position,after.Pose.Position);
        const double physicalUpperBound=std::max(
            Length(before.VelocityCmS),
            Length(after.VelocityCmS))*dt+
                p.PositionUncertaintyCm;
        if(!Finite(displacement) ||
           displacement>p.MaximumRelativeDisplacementCm ||
           displacement>physicalUpperBound)
            return false;
    }
    return true;
}
class PhysicalReplayEventRecorder final
{
public:
    bool Configure(PhysicalReplayEventSettings p) noexcept
    {
        if(!ValidPhysicalReplayEventSettings(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Count=0;
        Next=0;
        HasPrevious=false;
        Previous={};
        PairStates.fill({});
        Statistics={};
        LastFault=PhysicalReplayEventFault::None;
    }
    PhysicalReplayEventFault Append(
        const PhysicalTraceFrame& sample) noexcept
    {
        if(!ValidPhysicalReplayEventSettings(Settings) ||
           !sample.Valid ||
           sample.MinimumObservedConfidence<
               Settings.MinimumConfidence ||
           sample.ActorRoster[0]==0 ||
           sample.Identity.Session==0 ||
           sample.Identity.Frame==0 ||
           !Finite(sample.PhysicsClockSeconds))
            return Reject(PhysicalReplayEventFault::InvalidSource);
        if(HasPrevious)
        {
            if(!PhysicalTraceSameEpoch(Previous,sample))
            {
                ResetPairs();
                ++Statistics.RosterResets;
            }
            else if(!PhysicalTraceSameRoster(Previous,sample))
            {
                ResetPairs();
                ++Statistics.RosterResets;
                if(Settings.RequireFullPhysicalRoster)
                {
                    Previous={};
                    HasPrevious=false;
                    return Reject(PhysicalReplayEventFault::InconsistentRoster);
                }
            }
            else if(sample.Identity.Frame<=Previous.Identity.Frame)
                return Reject(PhysicalReplayEventFault::StaleFrame);
            else if(!PhysicalEventSourceContinuous(
                    Previous,sample,Settings))
                return Reject(
                    PhysicalReplayEventFault::LostPhysicsContinuity);
        }
        const double dt=HasPrevious &&
            PhysicalTraceSameRoster(Previous,sample) ?
            sample.PhysicsClockSeconds-Previous.PhysicsClockSeconds:0.0;
        for(int i=0;i<PhysicalTracePairCount;++i)
        {
            const auto index=static_cast<std::size_t>(i);
            const auto& pair=sample.Pairs[index];
            if(!QualifyReplayEventPair(pair,Settings) ||
               sample.ActorRoster[static_cast<std::size_t>(
                   pair.FirstSlot)]!=pair.FirstActor ||
               sample.ActorRoster[static_cast<std::size_t>(
                   pair.SecondSlot)]!=pair.SecondActor)
                return Reject(PhysicalReplayEventFault::InvalidSource);
            auto& prior=PairStates[index];
            const bool continuous=HasPrevious &&
                PhysicalTraceSameRoster(Previous,sample);
            const bool contact=pair.MeasuredOverlap;
            if(contact)
            {
                prior.ContactSeconds+=dt;
                if(!prior.ContactStarted &&
                   prior.ContactSeconds>=
                       Settings.MinimumContactPersistenceSeconds)
                {
                    Add(MakeReplayEvent(sample,pair,
                        PhysicalReplayEventKind::ContactBegan));
                    prior.ContactStarted=true;
                    ++Statistics.ContactsBegan;
                }
            }
            else
            {
                if(prior.ContactStarted && continuous)
                {
                    Add(MakeReplayEvent(sample,pair,
                        PhysicalReplayEventKind::ContactEnded));
                    ++Statistics.ContactsEnded;
                }
                prior.ContactSeconds=0.0;
                prior.ContactStarted=false;
            }
            const bool nearMiss=!contact &&
                pair.LongitudinalClearanceCm>=0.0 &&
                pair.LongitudinalClearanceCm<=
                    Settings.NearMissLongitudinalCm &&
                pair.LateralClearanceCm>=0.0 &&
                pair.LateralClearanceCm<=
                    Settings.NearMissLateralCm &&
                pair.ClosingSpeedCmS>=
                    Settings.MinimumClosePassSpeedCmS;
            if(nearMiss && !prior.PreviousNearMiss)
            {
                Add(MakeReplayEvent(sample,pair,
                    PhysicalReplayEventKind::ClosePass));
                ++Statistics.ClosePassEvents;
            }
            prior.PreviousNearMiss=nearMiss;
            const bool following=!contact &&
                pair.LateralClearanceCm<=0.0 &&
                pair.LongitudinalClearanceCm>=0.0 &&
                pair.LongitudinalClearanceCm<=
                    Settings.FollowingGapCm &&
                pair.ClosingSpeedCmS>=
                    Settings.MinimumFollowingClosingCmS;
            if(following && !prior.PreviousFollowing)
            {
                Add(MakeReplayEvent(sample,pair,
                    PhysicalReplayEventKind::FollowGap));
                ++Statistics.FollowGapEvents;
            }
            prior.PreviousFollowing=following;
            const bool predicted=!contact && pair.PredictedContact;
            if(predicted!=prior.PredictedCollision)
            {
                Add(MakeReplayEvent(sample,pair,predicted?
                    PhysicalReplayEventKind::CollisionRiskBegan:
                    PhysicalReplayEventKind::CollisionRiskEnded));
                ++Statistics.CollisionRiskEvents;
            }
            prior.PredictedCollision=predicted;
            Statistics.MaximumMeasuredClosingSpeedCmS=std::max(
                Statistics.MaximumMeasuredClosingSpeedCmS,
                pair.ClosingSpeedCmS);
            Statistics.MinimumMeasuredClearanceCm=std::min(
                Statistics.MinimumMeasuredClearanceCm,
                std::max(pair.LongitudinalClearanceCm,
                         pair.LateralClearanceCm));
        }
        Previous=sample;
        HasPrevious=true;
        ++Statistics.AcceptedFrames;
        LastFault=PhysicalReplayEventFault::None;
        Statistics.StoredEvents=Count;
        return LastFault;
    }
    std::size_t Size() const noexcept{return Count;}
    const PhysicalReplayEvent* OldestFirst(
        std::size_t index) const noexcept
    {
        if(index>=Count)return nullptr;
        const auto oldest=(Next+PhysicalReplayEventCapacity-
            Count)%PhysicalReplayEventCapacity;
        return &Events[(oldest+index)%PhysicalReplayEventCapacity];
    }
    PhysicalReplayEventFault LastRejection() const noexcept
    {return LastFault;}
    PhysicalReplayEventStatistics Snapshot() const noexcept
    {return Statistics;}
    std::uint64_t Accepted() const noexcept
    {return Statistics.AcceptedFrames;}
    std::uint64_t Rejected() const noexcept
    {return Statistics.RejectedFrames;}
    bool HasActivePhysicalContact(int firstSlot,int secondSlot) const noexcept
    {
        if(!HasPrevious || firstSlot<0 || secondSlot<0 ||
           firstSlot>=PhysicalFleetCapacity ||
           secondSlot>=PhysicalFleetCapacity ||
           firstSlot==secondSlot)
            return false;
        for(int i=0;i<PhysicalTracePairCount;++i)
        {
            const auto& pair=Previous.Pairs[static_cast<std::size_t>(i)];
            if(((pair.FirstSlot==firstSlot &&
                 pair.SecondSlot==secondSlot) ||
                (pair.FirstSlot==secondSlot &&
                 pair.SecondSlot==firstSlot)) &&
               pair.MeasuredOverlap)
                return true;
        }
        return false;
    }
private:
    struct PairState final
    {
        double ContactSeconds=0.0;
        bool ContactStarted=false;
        bool PreviousNearMiss=false;
        bool PreviousFollowing=false;
        bool PredictedCollision=false;
    };
    void ResetPairs() noexcept
    {
        PairStates.fill({});
        Count=0;
        Next=0;
        HasPrevious=false;
    }
    void Add(PhysicalReplayEvent event) noexcept
    {
        if(!event.Valid || event.EngineCollisionEventFired ||
           event.OfficialPenaltyIssued ||
           event.ActualVehicleControlModified)
            return;
        Events[Next]=event;
        Next=(Next+1)%PhysicalReplayEventCapacity;
        Count=std::min(Count+1,PhysicalReplayEventCapacity);
    }
    PhysicalReplayEventFault Reject(
        PhysicalReplayEventFault fault) noexcept
    {
        ++Statistics.RejectedFrames;
        LastFault=fault;
        return fault;
    }
    PhysicalReplayEventSettings Settings{};
    PhysicalTraceFrame Previous{};
    std::array<PairState,PhysicalTracePairCount> PairStates{};
    std::array<PhysicalReplayEvent,PhysicalReplayEventCapacity> Events{};
    PhysicalReplayEventStatistics Statistics{};
    std::size_t Count=0;
    std::size_t Next=0;
    PhysicalReplayEventFault LastFault=PhysicalReplayEventFault::None;
    bool HasPrevious=false;
};
} // namespace latebrake::racecraft