#pragma once
#include "Racecraft/RivalLaneOccupancyForecast.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
// Records a bounded sequence of six *live physical* chassis frames for
// diagnosis and replay presentation. This is not an engine override, an
// authoritative timing source or a substitute for live collision evidence.
inline constexpr std::size_t PhysicalTraceCapacity=128;
struct PhysicalTraceSettings final
{
    double MaximumFrameSeconds=0.10;
    double MaximumSourceClockSkewSeconds=0.02;
    double MaximumReplayInterpolationSeconds=0.10;
    double MaximumVehicleTravelCm=1800.0;
    double MaximumRelativeMovementCm=100.0;
    double MinimumSensorConfidence=0.80;
    double NearMissLateralClearanceCm=120.0;
    double NearMissLongitudinalClearanceCm=220.0;
    double MinimumFollowingGapCm=550.0;
    bool RequireContinuousPhysicsFrames=true;
    bool ResetOnRosterChange=true;
    bool NeverImpersonateOfficialRaceResults=true;
};
struct PhysicalTraceActor final
{
    RaceIdentity Identity{};
    Pose2 Pose{};
    Vec2 VelocityCmS{};
    double TrackDistanceCm=0.0;
    double TrackLateralOffsetCm=0.0;
    double HalfLengthCm=0.0;
    double HalfWidthCm=0.0;
    double SensorConfidence=0.0;
    bool OriginalCollisionBody=false;
};
struct PhysicalTracePair final
{
    int FirstSlot=-1;
    int SecondSlot=-1;
    std::uint64_t FirstActor=0;
    std::uint64_t SecondActor=0;
    double ForwardSeparationCm=0.0;
    double LateralSeparationCm=0.0;
    double LongitudinalClearanceCm=0.0;
    double LateralClearanceCm=0.0;
    double RelativeForwardSpeedCmS=0.0;
    double ClosingSpeedCmS=0.0;
    double TimeToContactSeconds=std::numeric_limits<double>::infinity();
    double CenterDistanceCm=0.0;
    bool PredictedContact=false;
    bool MeasuredOverlap=false;
    bool FollowingGapHazard=false;
    bool NearMiss=false;
    bool Valid=false;
};
inline constexpr int PhysicalTracePairCount=
    PhysicalFleetCapacity*(PhysicalFleetCapacity-1)/2;
struct PhysicalTraceFrame final
{
    RaceIdentity Identity{};
    std::array<PhysicalTraceActor,PhysicalFleetCapacity> Actors{};
    std::array<PhysicalTracePair,PhysicalTracePairCount> Pairs{};
    std::array<std::uint64_t,PhysicalFleetCapacity> ActorRoster{};
    double PhysicsClockSeconds=0.0;
    double DeltaSeconds=0.0;
    double MinimumObservedConfidence=1.0;
    double NearestVehicleSeparationCm=
        std::numeric_limits<double>::infinity();
    double MinimumPhysicalLongitudinalClearanceCm=
        std::numeric_limits<double>::infinity();
    double MinimumPhysicalLateralClearanceCm=
        std::numeric_limits<double>::infinity();
    double MinimumPredictedCollisionSeconds=
        std::numeric_limits<double>::infinity();
    double OriginalThrottle=0.0;
    double OriginalBrake=0.0;
    double OriginalSteering=0.0;
    int OverlappingPairs=0;
    int NearMissPairs=0;
    int FollowingGapHazards=0;
    int PredictedContactPairs=0;
    bool RaceGreen=false;
    bool ActualEngineActuationOccurred=false;
    bool OfficialRaceResultModified=false;
    bool Valid=false;
};
struct PhysicalTraceInterval final
{
    PhysicalTraceFrame First{};
    PhysicalTraceFrame Last{};
    double DurationSeconds=0.0;
    double MinimumSeparationCm=std::numeric_limits<double>::infinity();
    int ContactFrames=0;
    int NearMissFrames=0;
    int UnsafeFollowingFrames=0;
    int PredictedContactFrames=0;
    bool RosterContinuous=false;
    bool EngineFramesContinuous=false;
    bool Valid=false;
};
struct PhysicalTraceReplayPose final
{
    std::array<PhysicalTraceActor,PhysicalFleetCapacity> Actors{};
    std::array<std::uint64_t,PhysicalFleetCapacity> ActorRoster{};
    double PhysicsClockSeconds=0.0;
    double SourceAlpha=0.0;
    std::uint64_t EarlierFrame=0;
    std::uint64_t LaterFrame=0;
    bool Interpolated=false;
    bool UnmodifiedEnginePhysics=true;
    bool UnmodifiedOfficialRaceResults=true;
    bool Valid=false;
};
enum class PhysicalTraceFault : std::uint8_t
{
    None, InvalidSettings, InvalidPhysicalInput, ReplayFrame,
    MissingFrame, LostRoster, LostEpoch, ClockMismatch,
    DiscontinuousMovement, UnsupportedReplayRange
};
inline bool ValidPhysicalTraceSettings(PhysicalTraceSettings p) noexcept
{
    return Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumSourceClockSkewSeconds) &&
        p.MaximumSourceClockSkewSeconds>=0.0 &&
        Finite(p.MaximumReplayInterpolationSeconds) &&
        p.MaximumReplayInterpolationSeconds>0.0 &&
        p.MaximumReplayInterpolationSeconds<=p.MaximumFrameSeconds &&
        Finite(p.MaximumVehicleTravelCm) &&
        p.MaximumVehicleTravelCm>0.0 &&
        Finite(p.MaximumRelativeMovementCm) &&
        p.MaximumRelativeMovementCm>=0.0 &&
        Finite(p.MaximumRelativeMovementCm) &&
        p.MaximumRelativeMovementCm>0.0 &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0 &&
        Finite(p.NearMissLateralClearanceCm) &&
        p.NearMissLateralClearanceCm>0.0 &&
        Finite(p.NearMissLongitudinalClearanceCm) &&
        p.NearMissLongitudinalClearanceCm>0.0 &&
        Finite(p.MinimumFollowingGapCm) &&
        p.MinimumFollowingGapCm>0.0;
}
inline PhysicalTraceActor CapturePhysicalTraceActor(
    const RivalLaneActor& original) noexcept
{
    PhysicalTraceActor out;
    out.Identity=original.Identity;
    out.Pose=original.Pose;
    out.VelocityCmS=original.VelocityCmS;
    out.TrackDistanceCm=original.TrackDistanceCm;
    out.TrackLateralOffsetCm=original.TrackLateralOffsetCm;
    out.HalfLengthCm=original.HalfLengthCm;
    out.HalfWidthCm=original.HalfWidthCm;
    out.SensorConfidence=original.SensorConfidence;
    out.OriginalCollisionBody=original.OriginalChassisVerified &&
        original.OriginalPavementVerified &&
        original.LiveChaosVelocityQualified;
    return out;
}
inline PhysicalTracePair ReviewPhysicalTracePair(
    const PhysicalTraceActor& first,
    const PhysicalTraceActor& second,
    PhysicalTraceSettings settings) noexcept
{
    PhysicalTracePair out;
    if (!ValidPhysicalTraceSettings(settings) ||
        !PhysicalActorIdentityValid(first.Identity) ||
        !PhysicalActorIdentityValid(second.Identity) ||
        first.Identity.Actor==second.Identity.Actor ||
        first.Identity.Slot==second.Identity.Slot ||
        first.Identity.Session!=second.Identity.Session ||
        first.Identity.SessionRevision!=
            second.Identity.SessionRevision ||
        first.Identity.Frame!=second.Identity.Frame ||
        !first.OriginalCollisionBody ||
        !second.OriginalCollisionBody ||
        !Finite(first.Pose) || !Finite(second.Pose) ||
        !Finite(first.VelocityCmS) ||
        !Finite(second.VelocityCmS) ||
        first.HalfLengthCm<=0.0 || second.HalfLengthCm<=0.0 ||
        first.HalfWidthCm<=0.0 || second.HalfWidthCm<=0.0 ||
        first.SensorConfidence<settings.MinimumSensorConfidence ||
        second.SensorConfidence<settings.MinimumSensorConfidence)
        return out;
    const Vec2 displacement=second.Pose.Position-
        first.Pose.Position;
    const Vec2 relativeVelocity=second.VelocityCmS-
        first.VelocityCmS;
    const Vec2 heading=HeadingVector(
        first.Pose.HeadingRadians);
    out.FirstActor=first.Identity.Actor;
    out.SecondActor=second.Identity.Actor;
    out.FirstSlot=first.Identity.Slot;
    out.SecondSlot=second.Identity.Slot;
    out.ForwardSeparationCm=Dot(displacement,heading);
    out.LateralSeparationCm=Cross(heading,displacement);
    out.RelativeForwardSpeedCmS=Dot(relativeVelocity,heading);
    out.ClosingSpeedCmS=out.ForwardSeparationCm>0.0 ?
        std::max(0.0,-out.RelativeForwardSpeedCmS) :
        std::max(0.0,out.RelativeForwardSpeedCmS);
    out.LongitudinalClearanceCm=
        std::abs(out.ForwardSeparationCm)-
        first.HalfLengthCm-second.HalfLengthCm;
    out.LateralClearanceCm=
        std::abs(out.LateralSeparationCm)-
        first.HalfWidthCm-second.HalfWidthCm;
    out.CenterDistanceCm=Distance(
        first.Pose.Position,second.Pose.Position);
    out.MeasuredOverlap=
        out.LongitudinalClearanceCm<=0.0 &&
        out.LateralClearanceCm<=0.0;
    out.FollowingGapHazard=
        out.LongitudinalClearanceCm>=0.0 &&
        out.LongitudinalClearanceCm<settings.MinimumFollowingGapCm &&
        out.LateralClearanceCm<=0.0 &&
        out.ClosingSpeedCmS>0.0;
    out.NearMiss=!out.MeasuredOverlap &&
        out.LongitudinalClearanceCm<=
            settings.NearMissLongitudinalClearanceCm &&
        out.LateralClearanceCm<=
            settings.NearMissLateralClearanceCm &&
        out.LongitudinalClearanceCm>=0.0 &&
        out.LateralClearanceCm>=0.0;
    if(out.ClosingSpeedCmS>PhysicsEpsilon &&
       out.LongitudinalClearanceCm>=0.0 &&
       out.LateralClearanceCm<=0.0)
    {
        out.TimeToContactSeconds=out.LongitudinalClearanceCm /
            out.ClosingSpeedCmS;
        out.PredictedContact=
            out.TimeToContactSeconds<=settings.MaximumFrameSeconds;
    }
    out.Valid=Finite(out.ForwardSeparationCm) &&
        Finite(out.LateralSeparationCm) &&
        Finite(out.RelativeForwardSpeedCmS) &&
        Finite(out.LongitudinalClearanceCm) &&
        Finite(out.LateralClearanceCm) &&
        Finite(out.CenterDistanceCm);
    return out;
}
inline PhysicalTraceFrame ReviewPhysicalTraceFrame(
    const RivalLaneTrafficFrame& physics,
    PhysicalTraceSettings settings={}) noexcept
{
    PhysicalTraceFrame out;
    if (!ValidPhysicalTraceSettings(settings) ||
        !ValidRivalLaneFrame(physics,{}) ||
        physics.Self.SensorConfidence<settings.MinimumSensorConfidence)
        return out;
    out.Identity=physics.Self.Identity;
    out.PhysicsClockSeconds=physics.Self.PhysicsClockSeconds;
    out.DeltaSeconds=physics.DeltaSeconds;
    out.OriginalThrottle=physics.CurrentThrottle;
    out.OriginalBrake=physics.CurrentBrake;
    out.OriginalSteering=physics.CurrentSteering;
    out.RaceGreen=physics.RaceGreen;
    out.Actors[static_cast<std::size_t>(
        physics.Self.Identity.Slot)]=
        CapturePhysicalTraceActor(physics.Self);
    out.ActorRoster[static_cast<std::size_t>(
        physics.Self.Identity.Slot)]=physics.Self.Identity.Actor;
    for(int i=0;i<physics.OtherCount;++i)
    {
        const auto& original=physics.Others[
            static_cast<std::size_t>(i)];
        if(original.SensorConfidence<settings.MinimumSensorConfidence)
            return {};
        const auto index=static_cast<std::size_t>(
            original.Identity.Slot);
        out.Actors[index]=CapturePhysicalTraceActor(original);
        out.ActorRoster[index]=original.Identity.Actor;
    }
    if(!std::all_of(out.ActorRoster.begin(),
        out.ActorRoster.end(),
        [](std::uint64_t actor){return actor!=0;}))
        return {};
    int pairIndex=0;
    for(int i=0;i<PhysicalFleetCapacity;++i)
    {
        const auto& first=out.Actors[
            static_cast<std::size_t>(i)];
        out.MinimumObservedConfidence=std::min(
            out.MinimumObservedConfidence,
            first.SensorConfidence);
        for(int j=i+1;j<PhysicalFleetCapacity;++j)
        {
            const auto& second=out.Actors[
                static_cast<std::size_t>(j)];
            auto pair=ReviewPhysicalTracePair(
                first,second,settings);
            if(!pair.Valid)return {};
            out.Pairs[static_cast<std::size_t>(pairIndex++)]=pair;
            out.NearestVehicleSeparationCm=std::min(
                out.NearestVehicleSeparationCm,
                pair.CenterDistanceCm);
            out.MinimumPhysicalLongitudinalClearanceCm=std::min(
                out.MinimumPhysicalLongitudinalClearanceCm,
                pair.LongitudinalClearanceCm);
            out.MinimumPhysicalLateralClearanceCm=std::min(
                out.MinimumPhysicalLateralClearanceCm,
                pair.LateralClearanceCm);
            out.MinimumPredictedCollisionSeconds=std::min(
                out.MinimumPredictedCollisionSeconds,
                pair.TimeToContactSeconds);
            out.OverlappingPairs+=pair.MeasuredOverlap?1:0;
            out.NearMissPairs+=pair.NearMiss?1:0;
            out.FollowingGapHazards+=pair.FollowingGapHazard?1:0;
            out.PredictedContactPairs+=pair.PredictedContact?1:0;
        }
    }
    out.Valid=pairIndex==PhysicalTracePairCount &&
        Finite(out.MinimumObservedConfidence) &&
        Finite(out.NearestVehicleSeparationCm) &&
        Finite(out.MinimumPhysicalLongitudinalClearanceCm) &&
        Finite(out.MinimumPhysicalLateralClearanceCm) &&
        !out.ActualEngineActuationOccurred &&
        !out.OfficialRaceResultModified;
    return out;
}
inline bool PhysicalTraceSameEpoch(
    const PhysicalTraceFrame& older,
    const PhysicalTraceFrame& newer) noexcept
{
    return older.Valid && newer.Valid &&
        older.Identity.Session==newer.Identity.Session &&
        older.Identity.SessionRevision==
            newer.Identity.SessionRevision &&
        older.Identity.Actor==newer.Identity.Actor &&
        older.Identity.Slot==newer.Identity.Slot;
}
inline bool PhysicalTraceSameRoster(
    const PhysicalTraceFrame& older,
    const PhysicalTraceFrame& newer) noexcept
{
    return PhysicalTraceSameEpoch(older,newer) &&
        older.ActorRoster==newer.ActorRoster;
}
inline bool PhysicalTraceContiguous(
    const PhysicalTraceFrame& older,
    const PhysicalTraceFrame& newer,
    PhysicalTraceSettings settings) noexcept
{
    if(!PhysicalTraceSameRoster(older,newer) ||
       newer.Identity.Frame!=older.Identity.Frame+1)
        return false;
    const double interval=newer.PhysicsClockSeconds-
        older.PhysicsClockSeconds;
    if(interval<=0.0 ||
       interval>settings.MaximumFrameSeconds ||
       std::abs(interval-newer.DeltaSeconds)>
           settings.MaximumSourceClockSkewSeconds)
        return false;
    for(int i=0;i<PhysicalFleetCapacity;++i)
    {
        const auto& first=older.Actors[static_cast<std::size_t>(i)];
        const auto& second=newer.Actors[static_cast<std::size_t>(i)];
        // A source frame cannot jump hundreds of centimeters farther
        // than either real chassis velocity could plausibly transport it
        // during this measured engine tick. Retain a bounded allowance
        // for imperfect collision/pose-sensor sampling.
        const double displacement=Distance(
            first.Pose.Position,second.Pose.Position);
        const double physicalTravelCm=std::max(
            Length(first.VelocityCmS),
            Length(second.VelocityCmS))*interval+
                settings.MaximumRelativeMovementCm;
        if(displacement>std::min(
                settings.MaximumVehicleTravelCm,physicalTravelCm))
            return false;
        if(!Finite(second.Pose) ||
           !Finite(second.VelocityCmS) ||
           !Finite(first.Pose) ||
           !Finite(first.VelocityCmS))
            return false;
    }
    return true;
}
inline PhysicalTraceReplayPose InterpolatePhysicalTrace(
    const PhysicalTraceFrame& older,
    const PhysicalTraceFrame& newer,
    double requestedClockSeconds,
    PhysicalTraceSettings settings={}) noexcept
{
    PhysicalTraceReplayPose out;
    if(!ValidPhysicalTraceSettings(settings) ||
       !PhysicalTraceContiguous(older,newer,settings) ||
       !Finite(requestedClockSeconds) ||
       requestedClockSeconds<older.PhysicsClockSeconds ||
       requestedClockSeconds>newer.PhysicsClockSeconds)
        return out;
    const double interval=newer.PhysicsClockSeconds-
        older.PhysicsClockSeconds;
    if(interval>settings.MaximumReplayInterpolationSeconds)
        return out;
    const double alpha=(requestedClockSeconds-
        older.PhysicsClockSeconds)/interval;
    for(int i=0;i<PhysicalFleetCapacity;++i)
    {
        const auto idx=static_cast<std::size_t>(i);
        const auto& from=older.Actors[idx];
        const auto& to=newer.Actors[idx];
        auto actor=from;
        actor.Pose.Position=Lerp(
            from.Pose.Position,to.Pose.Position,alpha);
        actor.Pose.HeadingRadians=BlendAngle(
            from.Pose.HeadingRadians,
            to.Pose.HeadingRadians,alpha);
        actor.VelocityCmS=Lerp(
            from.VelocityCmS,to.VelocityCmS,alpha);
        actor.TrackDistanceCm=Lerp(
            from.TrackDistanceCm,to.TrackDistanceCm,alpha);
        actor.TrackLateralOffsetCm=Lerp(
            from.TrackLateralOffsetCm,
            to.TrackLateralOffsetCm,alpha);
        actor.SensorConfidence=std::min(
            from.SensorConfidence,to.SensorConfidence);
        out.Actors[idx]=actor;
    }
    out.ActorRoster=older.ActorRoster;
    out.PhysicsClockSeconds=requestedClockSeconds;
    out.SourceAlpha=alpha;
    out.EarlierFrame=older.Identity.Frame;
    out.LaterFrame=newer.Identity.Frame;
    out.Interpolated=alpha>0.0 && alpha<1.0;
    out.Valid=true;
    return out;
}
class PhysicalSixCarRaceTrace final
{
public:
    bool Configure(PhysicalTraceSettings settings) noexcept
    {
        if(!ValidPhysicalTraceSettings(settings))return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Count=0;
        Next=0;
        Qualified=0;
        Rejected=0;
        Resets=0;
        LastFault=PhysicalTraceFault::None;
        HasSource=false;
        Source={};
        LastClockSeconds=0.0;
    }
    PhysicalTraceFault Append(
        const RivalLaneTrafficFrame& sample) noexcept
    {
        if(!ValidPhysicalTraceSettings(Settings))
            return Reject(PhysicalTraceFault::InvalidSettings);
        const auto packet=ReviewPhysicalTraceFrame(
            sample,Settings);
        if(!packet.Valid)
            return Reject(PhysicalTraceFault::InvalidPhysicalInput);
        if(HasSource)
        {
            const auto& prior=Latest();
            if(!PhysicalTraceSameEpoch(prior,packet))
            {
                // Never blend race sessions, even when the engine emits a
                // numerically similar chassis frame on a new attempt.
                Count=0;
                Next=0;
                ++Resets;
            }
            else if(packet.Identity.Frame<=Source.Frame)
                return Reject(PhysicalTraceFault::ReplayFrame);
            else if(packet.ActorRoster!=prior.ActorRoster)
            {
                Count=0;
                Next=0;
                ++Resets;
                if(Settings.ResetOnRosterChange)
                {
                    Source={};
                    HasSource=false;
                    return Reject(PhysicalTraceFault::LostRoster);
                }
            }
            else if(Settings.RequireContinuousPhysicsFrames &&
                    packet.Identity.Frame!=Source.Frame+1)
                return Reject(PhysicalTraceFault::MissingFrame);
            else if(packet.PhysicsClockSeconds<=LastClockSeconds ||
                    packet.PhysicsClockSeconds-LastClockSeconds>
                        Settings.MaximumFrameSeconds+
                        Settings.MaximumSourceClockSkewSeconds)
                return Reject(PhysicalTraceFault::ClockMismatch);
            else if(!PhysicalTraceContiguous(prior,packet,Settings))
                return Reject(PhysicalTraceFault::DiscontinuousMovement);
        }
        Buffer[Next]=packet;
        Next=(Next+1)%PhysicalTraceCapacity;
        Count=std::min(Count+1,PhysicalTraceCapacity);
        Source=packet.Identity;
        LastClockSeconds=packet.PhysicsClockSeconds;
        HasSource=true;
        ++Qualified;
        LastFault=PhysicalTraceFault::None;
        return LastFault;
    }
    PhysicalTraceFault LastRejection() const noexcept
    {return LastFault;}
    std::size_t Size() const noexcept{return Count;}
    std::uint64_t AcceptedFrames() const noexcept{return Qualified;}
    std::uint64_t RejectedFrames() const noexcept{return Rejected;}
    std::uint64_t ResetCount() const noexcept{return Resets;}
    const PhysicalTraceFrame& Latest() const noexcept
    {
        return Buffer[(Next+PhysicalTraceCapacity-1)%
            PhysicalTraceCapacity];
    }
    const PhysicalTraceFrame* OldestFirst(
        std::size_t index) const noexcept
    {
        if(index>=Count)return nullptr;
        const std::size_t oldest=(Next+PhysicalTraceCapacity-Count)%
            PhysicalTraceCapacity;
        return &Buffer[(oldest+index)%PhysicalTraceCapacity];
    }
    const PhysicalTraceFrame* FindFrame(
        std::uint64_t frameId) const noexcept
    {
        for(std::size_t i=0;i<Count;++i)
        {
            const auto* frame=OldestFirst(i);
            if(frame && frame->Identity.Frame==frameId)
                return frame;
        }
        return nullptr;
    }
    PhysicalTraceReplayPose SampleReadOnly(
        double physicsClockSeconds) const noexcept
    {
        if(!Finite(physicsClockSeconds))return {};
        for(std::size_t i=1;i<Count;++i)
        {
            const auto* older=OldestFirst(i-1);
            const auto* newer=OldestFirst(i);
            if(older && newer &&
               physicsClockSeconds>=older->PhysicsClockSeconds &&
               physicsClockSeconds<=newer->PhysicsClockSeconds)
                return InterpolatePhysicalTrace(
                    *older,*newer,physicsClockSeconds,Settings);
        }
        return {};
    }
    PhysicalTraceInterval ReviewInterval() const noexcept
    {
        PhysicalTraceInterval out;
        if(Count<2)return out;
        const auto* first=OldestFirst(0);
        const auto* last=OldestFirst(Count-1);
        if(!first || !last)return out;
        out.First=*first;
        out.Last=*last;
        out.DurationSeconds=last->PhysicsClockSeconds-
            first->PhysicsClockSeconds;
        out.RosterContinuous=true;
        out.EngineFramesContinuous=true;
        for(std::size_t i=0;i<Count;++i)
        {
            const auto* current=OldestFirst(i);
            if(!current || !current->Valid)return {};
            out.MinimumSeparationCm=std::min(
                out.MinimumSeparationCm,
                current->NearestVehicleSeparationCm);
            out.ContactFrames+=current->OverlappingPairs>0?1:0;
            out.NearMissFrames+=current->NearMissPairs>0?1:0;
            out.UnsafeFollowingFrames+=
                current->FollowingGapHazards>0?1:0;
            out.PredictedContactFrames+=
                current->PredictedContactPairs>0?1:0;
            if(i>0)
            {
                const auto* older=OldestFirst(i-1);
                out.RosterContinuous=out.RosterContinuous &&
                    PhysicalTraceSameRoster(*older,*current);
                out.EngineFramesContinuous=out.EngineFramesContinuous &&
                    PhysicalTraceContiguous(*older,*current,Settings);
            }
        }
        out.Valid=out.RosterContinuous &&
            out.EngineFramesContinuous &&
            Finite(out.DurationSeconds) &&
            out.DurationSeconds>0.0 &&
            Finite(out.MinimumSeparationCm);
        return out;
    }
private:
    PhysicalTraceFault Reject(PhysicalTraceFault fault) noexcept
    {
        ++Rejected;
        LastFault=fault;
        return fault;
    }
    PhysicalTraceSettings Settings{};
    std::array<PhysicalTraceFrame,PhysicalTraceCapacity> Buffer{};
    RaceIdentity Source{};
    std::size_t Count=0;
    std::size_t Next=0;
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    std::uint64_t Resets=0;
    double LastClockSeconds=0.0;
    PhysicalTraceFault LastFault=PhysicalTraceFault::None;
    bool HasSource=false;
};
} // namespace latebrake::racecraft