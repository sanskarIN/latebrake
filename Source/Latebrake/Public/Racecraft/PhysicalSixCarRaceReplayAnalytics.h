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
// These analyses are observational. They never rank a race, award a lap,
// accuse a driver, or replace a Chaos physics reading. All evidence is
// derived from consecutive, authenticated six-car chassis frames.
inline constexpr std::size_t PhysicalAnalysisCapacity=PhysicalTraceCapacity;
inline constexpr int PhysicalAnalysisPairs=PhysicalTracePairCount;
enum class PhysicalReplayAnalysisFault : std::uint8_t
{
    None, InvalidSettings, NoSourceFrames, InvalidPhysicalFrame,
    ChangedEpoch, ChangedRoster, DiscontinuousFrame,
    UnverifiedActor, UnverifiedPair, InsufficientHistory
};
enum class PhysicalReplayCoachSignal : std::uint8_t
{
    None, CloseFollowing, NearMiss, MeasuredContact,
    RepeatedCloseApproach, SevereRelativeClosing,
    UnstableHeading, HeavySpeedVariation
};
struct PhysicalReplayAnalysisSettings final
{
    double MaximumFrameSeconds=0.10;
    double ClockSkewAllowanceSeconds=0.02;
    double MaximumObservedSpeedCmS=MaximumPhysicalSpeedCmS;
    double MaximumObservedTravelCm=1800.0;
    double MaximumVelocityDisplacementErrorCm=100.0;
    double HighClosingSpeedCmS=900.0;
    double SevereClosingSpeedCmS=1900.0;
    double NearCarDistanceCm=2800.0;
    double CloseFollowGapCm=650.0;
    double MaximumHeadingChangeRateRadiansS=3.4;
    double StrongDecelerationCmS2=750.0;
    double StrongAccelerationCmS2=650.0;
    double MaximumPlausibleAccelerationCmS2=4000.0;
    double LateralOvertakeSeparationCm=210.0;
    double MinimumPassClearanceCm=180.0;
    double MinimumCompletedPassDeltaCm=600.0;
    double MinimumPassingDurationSeconds=0.03;
    double MinimumElapsedWindowSeconds=0.01;
    double MinimumObservedConfidence=0.8;
    int MinimumConsecutiveCloseFrames=2;
    int MinimumConsecutiveContactFrames=1;
    int MinimumAnalysisFrames=2;
    bool RequireAllSixLiveActors=true;
    bool RequireCompletePhysicalSequence=true;
    bool NeverIssueOfficialClassification=true;
    bool NeverWriteVehicleControls=true;
};
struct PhysicalReplayCarStatistics final
{
    std::uint64_t Actor=0;
    int Slot=-1;
    int ObservedFrames=0;
    int HeavyAccelerationFrames=0;
    int HeavyDecelerationFrames=0;
    int SharpHeadingChangeFrames=0;
    int NearTrafficFrames=0;
    int InContactFrames=0;
    int UnsafeFollowingFrames=0;
    int ContactedDistinctRivals=0;
    int PassesObserved=0;
    int PassedByRivals=0;
    double FirstClockSeconds=0.0;
    double LastClockSeconds=0.0;
    double DurationSeconds=0.0;
    double TotalMeasuredTravelCm=0.0;
    double StartSpeedCmS=0.0;
    double FinalSpeedCmS=0.0;
    double MinimumSpeedCmS=std::numeric_limits<double>::infinity();
    double MaximumSpeedCmS=0.0;
    double MeanSpeedCmS=0.0;
    double RmsAccelerationCmS2=0.0;
    double MaximumAccelerationCmS2=0.0;
    double MaximumDecelerationCmS2=0.0;
    double MaximumHeadingRateRadiansS=0.0;
    double HeadingTravelRadians=0.0;
    double ClosestObservedRivalCm=std::numeric_limits<double>::infinity();
    double PeakClosingSpeedCmS=0.0;
    double MinimumObservedSensorConfidence=1.0;
    bool OriginalVehiclePhysicsVerified=false;
    bool SourceOnlyReview=true;
    bool Valid=false;
};
struct PhysicalReplayPairStatistics final
{
    std::uint64_t FirstActor=0;
    std::uint64_t SecondActor=0;
    int FirstSlot=-1;
    int SecondSlot=-1;
    int ObservedFrames=0;
    int ContactFrames=0;
    int NearMissFrames=0;
    int FollowingRiskFrames=0;
    int PredictedContactFrames=0;
    int SignificantClosingFrames=0;
    int PhysicallyCompletedPasses=0;
    int ConfirmedLeaderSwaps=0;
    int ConsecutiveContactPeakFrames=0;
    int ConsecutiveNearPeakFrames=0;
    double MinimumCenterDistanceCm=std::numeric_limits<double>::infinity();
    double MinimumLongitudinalClearanceCm=std::numeric_limits<double>::infinity();
    double MinimumLateralClearanceCm=std::numeric_limits<double>::infinity();
    double MaximumClosingSpeedCmS=0.0;
    double MinimumEstimatedTimeToContactSeconds=std::numeric_limits<double>::infinity();
    double TotalMeasuredContactSeconds=0.0;
    double TotalObservedNearMissSeconds=0.0;
    double FirstObservedForwardCm=0.0;
    double LastObservedForwardCm=0.0;
    bool PairIdentitiesStable=false;
    bool SourceOnlyReview=true;
    bool Valid=false;
};
struct PhysicalReplayPackReview final
{
    std::array<int,PhysicalFleetCapacity> ComponentBySlot{};
    std::array<int,PhysicalFleetCapacity> ComponentSizes{};
    int ConnectedGroups=0;
    int LargestConnectedGroup=0;
    int UnsafeConnectedGroups=0;
    int CloseProximityLinks=0;
    int CollisionRiskLinks=0;
    int IsolatedCars=0;
    double MinimumGroupGapCm=std::numeric_limits<double>::infinity();
    bool SixPhysicalCarsVerified=false;
    bool Valid=false;
};
struct PhysicalReplayCoachingReview final
{
    PhysicalReplayCoachSignal Signal=PhysicalReplayCoachSignal::None;
    int SourceSlot=-1;
    int EvidenceFrameCount=0;
    int LimitingRivalSlot=-1;
    std::uint64_t LimitingRivalActor=0;
    double MeasuredSeverity=0.0;
    double MinimumGapCm=std::numeric_limits<double>::infinity();
    double MaximumClosingSpeedCmS=0.0;
    double StrongestAccelerationCmS2=0.0;
    bool NoSteeringOrBrakeCommandIssued=true;
    bool NoOfficialPenaltyOrLapCredited=true;
    bool Valid=false;
};
struct PhysicalReplayAnalysisReport final
{
    RaceIdentity Identity{};
    std::array<std::uint64_t,PhysicalFleetCapacity> ActorRoster{};
    std::array<PhysicalReplayCarStatistics,PhysicalFleetCapacity> Cars{};
    std::array<PhysicalReplayPairStatistics,PhysicalAnalysisPairs> Pairs{};
    PhysicalReplayPackReview LastFramePack{};
    PhysicalReplayCoachingReview DriverCoach{};
    int SourceFrames=0;
    int PhysicalContacts=0;
    int PhysicalNearMissFrames=0;
    int PhysicalPredictedCollisionFrames=0;
    int PhysicalUnsafeFollowingFrames=0;
    int TotalPhysicallyCompletedPasses=0;
    int ReviewOnlyOvertakeTransitions=0;
    double FirstClockSeconds=0.0;
    double LastClockSeconds=0.0;
    double ObservedDurationSeconds=0.0;
    double LowestActualSensorConfidence=1.0;
    double MinimumObservedTrafficGapCm=std::numeric_limits<double>::infinity();
    PhysicalReplayAnalysisFault Fault=PhysicalReplayAnalysisFault::NoSourceFrames;
    bool ActorIdentitiesContinuous=false;
    bool EngineFramesContinuous=false;
    bool OriginalRaceStateUntouched=true;
    bool RealUnrealVehicleActuationIssued=false;
    bool OfficialRaceClassificationIssued=false;
    bool Valid=false;
};
inline bool ValidPhysicalReplayAnalysisSettings(
    PhysicalReplayAnalysisSettings p) noexcept
{
    return Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.ClockSkewAllowanceSeconds) &&
        p.ClockSkewAllowanceSeconds>=0.0 &&
        p.ClockSkewAllowanceSeconds<=p.MaximumFrameSeconds &&
        Finite(p.MaximumObservedSpeedCmS) &&
        p.MaximumObservedSpeedCmS>0.0 &&
        p.MaximumObservedSpeedCmS<=MaximumPhysicalSpeedCmS &&
        Finite(p.MaximumObservedTravelCm) &&
        p.MaximumObservedTravelCm>0.0 &&
        Finite(p.MaximumVelocityDisplacementErrorCm) &&
        p.MaximumVelocityDisplacementErrorCm>=0.0 &&
        Finite(p.HighClosingSpeedCmS) &&
        p.HighClosingSpeedCmS>=0.0 &&
        Finite(p.SevereClosingSpeedCmS) &&
        p.SevereClosingSpeedCmS>p.HighClosingSpeedCmS &&
        Finite(p.NearCarDistanceCm) &&
        p.NearCarDistanceCm>0.0 &&
        Finite(p.CloseFollowGapCm) &&
        p.CloseFollowGapCm>0.0 &&
        Finite(p.MaximumHeadingChangeRateRadiansS) &&
        p.MaximumHeadingChangeRateRadiansS>0.0 &&
        Finite(p.StrongDecelerationCmS2) &&
        p.StrongDecelerationCmS2>0.0 &&
        Finite(p.StrongAccelerationCmS2) &&
        p.StrongAccelerationCmS2>0.0 &&
        Finite(p.MaximumPlausibleAccelerationCmS2) &&
        p.MaximumPlausibleAccelerationCmS2>
            std::max(p.StrongAccelerationCmS2,p.StrongDecelerationCmS2) &&
        Finite(p.LateralOvertakeSeparationCm) &&
        p.LateralOvertakeSeparationCm>0.0 &&
        Finite(p.MinimumPassClearanceCm) &&
        p.MinimumPassClearanceCm>0.0 &&
        Finite(p.MinimumCompletedPassDeltaCm) &&
        p.MinimumCompletedPassDeltaCm>0.0 &&
        Finite(p.MinimumPassingDurationSeconds) &&
        p.MinimumPassingDurationSeconds>0.0 &&
        Finite(p.MinimumElapsedWindowSeconds) &&
        p.MinimumElapsedWindowSeconds>0.0 &&
        Finite(p.MinimumObservedConfidence) &&
        p.MinimumObservedConfidence>0.0 &&
        p.MinimumObservedConfidence<=1.0 &&
        p.MinimumConsecutiveCloseFrames>=1 &&
        p.MinimumConsecutiveContactFrames>=1 &&
        p.MinimumAnalysisFrames>=2 &&
        p.MinimumAnalysisFrames<=static_cast<int>(PhysicalAnalysisCapacity);
}
inline bool ValidPhysicalReplayAnalysisFrame(
    const PhysicalTraceFrame& frame,
    PhysicalReplayAnalysisSettings settings) noexcept
{
    if(!ValidPhysicalReplayAnalysisSettings(settings) ||
       !frame.Valid ||
       !PhysicalActorIdentityValid(frame.Identity) ||
       !Finite(frame.PhysicsClockSeconds) ||
       !Finite(frame.DeltaSeconds) ||
       frame.DeltaSeconds<=0.0 ||
       frame.DeltaSeconds>settings.MaximumFrameSeconds ||
       !frame.RaceGreen ||
       frame.ActualEngineActuationOccurred ||
       frame.OfficialRaceResultModified ||
       frame.MinimumObservedConfidence<settings.MinimumObservedConfidence)
        return false;
    int checkedPairs=0;
    for(int i=0;i<PhysicalFleetCapacity;++i)
    {
        const auto& actor=frame.Actors[static_cast<std::size_t>(i)];
        if(!PhysicalActorIdentityValid(actor.Identity) ||
           actor.Identity.Slot!=i ||
           actor.Identity.Actor!=frame.ActorRoster[static_cast<std::size_t>(i)] ||
           actor.Identity.Session!=frame.Identity.Session ||
           actor.Identity.SessionRevision!=frame.Identity.SessionRevision ||
           actor.Identity.Frame!=frame.Identity.Frame ||
           !actor.OriginalCollisionBody ||
           !Finite(actor.Pose) ||
           !Finite(actor.VelocityCmS) ||
           !Finite(actor.TrackDistanceCm) ||
           !Finite(actor.TrackLateralOffsetCm) ||
           actor.HalfLengthCm<=0.0 ||
           actor.HalfWidthCm<=0.0 ||
           actor.SensorConfidence<settings.MinimumObservedConfidence ||
           actor.SensorConfidence>1.0 ||
           Length(actor.VelocityCmS)>settings.MaximumObservedSpeedCmS)
            return false;
        for(int j=0;j<i;++j)
        {
            if(frame.ActorRoster[static_cast<std::size_t>(j)]==actor.Identity.Actor)
                return false;
        }
        for(int j=i+1;j<PhysicalFleetCapacity;++j)
        {
            const auto& rival=frame.Actors[static_cast<std::size_t>(j)];
            const auto pair=ReviewPhysicalTracePair(actor,rival,{});
            const auto& stored=frame.Pairs[static_cast<std::size_t>(checkedPairs++)];
            if(!pair.Valid || !stored.Valid ||
               stored.FirstActor!=pair.FirstActor ||
               stored.SecondActor!=pair.SecondActor ||
               stored.FirstSlot!=i ||
               stored.SecondSlot!=j ||
               !Finite(stored.CenterDistanceCm) ||
               std::abs(stored.CenterDistanceCm-pair.CenterDistanceCm)>
                   settings.MaximumVelocityDisplacementErrorCm ||
               std::abs(stored.ForwardSeparationCm-pair.ForwardSeparationCm)>
                   settings.MaximumVelocityDisplacementErrorCm ||
               std::abs(stored.LateralSeparationCm-pair.LateralSeparationCm)>
                   settings.MaximumVelocityDisplacementErrorCm ||
               stored.MeasuredOverlap!=pair.MeasuredOverlap ||
               stored.FollowingGapHazard!=pair.FollowingGapHazard ||
               stored.PredictedContact!=pair.PredictedContact ||
               stored.NearMiss!=pair.NearMiss)
                return false;
        }
    }
    return checkedPairs==PhysicalAnalysisPairs;
}
inline bool PhysicalReplayAnalysisFramePairContiguous(
    const PhysicalTraceFrame& older,
    const PhysicalTraceFrame& newer,
    PhysicalReplayAnalysisSettings settings) noexcept
{
    if(!ValidPhysicalReplayAnalysisFrame(older,settings) ||
       !ValidPhysicalReplayAnalysisFrame(newer,settings) ||
       !PhysicalTraceSameRoster(older,newer) ||
       newer.Identity.Frame!=older.Identity.Frame+1 ||
       newer.PhysicsClockSeconds<=older.PhysicsClockSeconds)
        return false;
    const double dt=newer.PhysicsClockSeconds-older.PhysicsClockSeconds;
    if(dt>settings.MaximumFrameSeconds+
            settings.ClockSkewAllowanceSeconds ||
       std::abs(dt-newer.DeltaSeconds)>settings.ClockSkewAllowanceSeconds)
        return false;
    for(int i=0;i<PhysicalFleetCapacity;++i)
    {
        const auto& a=older.Actors[static_cast<std::size_t>(i)];
        const auto& b=newer.Actors[static_cast<std::size_t>(i)];
        const double observed=Distance(a.Pose.Position,b.Pose.Position);
        const double allowed=std::min(settings.MaximumObservedTravelCm,
            std::max(Length(a.VelocityCmS),Length(b.VelocityCmS))*dt+
                settings.MaximumVelocityDisplacementErrorCm);
        if(observed>allowed)
            return false;
    }
    return true;
}
inline PhysicalReplayPackReview ReviewPhysicalReplayPack(
    const PhysicalTraceFrame& frame,
    PhysicalReplayAnalysisSettings settings={}) noexcept
{
    PhysicalReplayPackReview out;
    if(!ValidPhysicalReplayAnalysisFrame(frame,settings))return out;
    std::array<int,PhysicalFleetCapacity> parent{};
    for(int i=0;i<PhysicalFleetCapacity;++i)
        parent[static_cast<std::size_t>(i)]=i;
    const auto root=[&](int i) -> int
    {
        while(parent[static_cast<std::size_t>(i)]!=i)
            i=parent[static_cast<std::size_t>(i)];
        return i;
    };
    for(const auto& pair:frame.Pairs)
    {
        const bool near=pair.CenterDistanceCm<=settings.NearCarDistanceCm;
        if(!near)continue;
        ++out.CloseProximityLinks;
        out.MinimumGroupGapCm=std::min(out.MinimumGroupGapCm,
            pair.CenterDistanceCm);
        if(pair.MeasuredOverlap || pair.PredictedContact)
            ++out.CollisionRiskLinks;
        const int left=root(pair.FirstSlot), right=root(pair.SecondSlot);
        if(left!=right)parent[static_cast<std::size_t>(right)]=left;
    }
    for(int i=0;i<PhysicalFleetCapacity;++i)
    {
        const int group=root(i);
        out.ComponentBySlot[static_cast<std::size_t>(i)]=group;
        ++out.ComponentSizes[static_cast<std::size_t>(group)];
    }
    for(int i=0;i<PhysicalFleetCapacity;++i)
    {
        const int size=out.ComponentSizes[static_cast<std::size_t>(i)];
        if(size<=0)continue;
        ++out.ConnectedGroups;
        out.LargestConnectedGroup=std::max(out.LargestConnectedGroup,size);
        out.IsolatedCars+=size==1?1:0;
    }
    std::array<bool,PhysicalFleetCapacity> unsafe{};
    for(const auto& pair:frame.Pairs)
    {
        if(pair.MeasuredOverlap || pair.FollowingGapHazard ||
           pair.PredictedContact)
        {
            const auto rootId=out.ComponentBySlot[
                static_cast<std::size_t>(pair.FirstSlot)];
            unsafe[static_cast<std::size_t>(rootId)]=true;
        }
    }
    for(int i=0;i<PhysicalFleetCapacity;++i)
        out.UnsafeConnectedGroups+=unsafe[static_cast<std::size_t>(i)]?1:0;
    out.SixPhysicalCarsVerified=true;
    out.Valid=out.ConnectedGroups>0 &&
        out.LargestConnectedGroup<=PhysicalFleetCapacity;
    return out;
}
inline PhysicalReplayCoachingReview ReviewPhysicalReplayCoach(
    const PhysicalReplayAnalysisReport& report,
    PhysicalReplayAnalysisSettings settings={}) noexcept
{
    PhysicalReplayCoachingReview out;
    if(!report.ActorIdentitiesContinuous ||
       !report.EngineFramesContinuous ||
       report.SourceFrames<settings.MinimumAnalysisFrames ||
       !ValidPhysicalReplayAnalysisSettings(settings))
        return out;
    const auto selfSlot=report.Identity.Slot;
    if(selfSlot<0 || selfSlot>=PhysicalFleetCapacity)return out;
    const auto& own=report.Cars[static_cast<std::size_t>(selfSlot)];
    if(!own.Valid)return out;
    out.SourceSlot=selfSlot;
    out.EvidenceFrameCount=own.ObservedFrames;
    out.StrongestAccelerationCmS2=std::max(
        own.MaximumAccelerationCmS2,own.MaximumDecelerationCmS2);
    PhysicalReplayCoachSignal preferred=PhysicalReplayCoachSignal::None;
    double severity=0.0;
    const auto offer=[&](PhysicalReplayCoachSignal kind,double weight)
    {
        if(weight>severity)
        {
            severity=weight;
            preferred=kind;
        }
    };
    if(own.SharpHeadingChangeFrames>0)
        offer(PhysicalReplayCoachSignal::UnstableHeading,
            static_cast<double>(own.SharpHeadingChangeFrames)/
                static_cast<double>(own.ObservedFrames));
    if(own.HeavyDecelerationFrames>0 &&
       own.HeavyAccelerationFrames>0)
        offer(PhysicalReplayCoachSignal::HeavySpeedVariation,
            0.5*static_cast<double>(own.HeavyDecelerationFrames+
                own.HeavyAccelerationFrames)/own.ObservedFrames);
    for(const auto& pair:report.Pairs)
    {
        if(!pair.Valid ||
           (pair.FirstSlot!=selfSlot && pair.SecondSlot!=selfSlot))
            continue;
        if(pair.ContactFrames>=settings.MinimumConsecutiveContactFrames)
            offer(PhysicalReplayCoachSignal::MeasuredContact,1.0);
        else if(pair.NearMissFrames>=settings.MinimumConsecutiveCloseFrames)
            offer(PhysicalReplayCoachSignal::NearMiss,0.9);
        else if(pair.FollowingRiskFrames>=settings.MinimumConsecutiveCloseFrames)
            offer(PhysicalReplayCoachSignal::CloseFollowing,0.75);
        else if(pair.MaximumClosingSpeedCmS>=settings.SevereClosingSpeedCmS)
            offer(PhysicalReplayCoachSignal::SevereRelativeClosing,0.6);
        if(pair.MinimumCenterDistanceCm<out.MinimumGapCm)
        {
            out.MinimumGapCm=pair.MinimumCenterDistanceCm;
            out.LimitingRivalSlot=pair.FirstSlot==selfSlot?
                pair.SecondSlot:pair.FirstSlot;
            out.LimitingRivalActor=pair.FirstSlot==selfSlot?
                pair.SecondActor:pair.FirstActor;
        }
        out.MaximumClosingSpeedCmS=std::max(
            out.MaximumClosingSpeedCmS,pair.MaximumClosingSpeedCmS);
    }
    out.Signal=preferred;
    out.MeasuredSeverity=std::clamp(severity,0.0,1.0);
    out.Valid=Finite(out.MeasuredSeverity) &&
        out.NoSteeringOrBrakeCommandIssued &&
        out.NoOfficialPenaltyOrLapCredited;
    return out;
}
inline PhysicalReplayAnalysisReport AnalyzePhysicalReplayWindow(
    const PhysicalSixCarRaceTrace& trace,
    PhysicalReplayAnalysisSettings settings={}) noexcept
{
    PhysicalReplayAnalysisReport out;
    if(!ValidPhysicalReplayAnalysisSettings(settings))
    {
        out.Fault=PhysicalReplayAnalysisFault::InvalidSettings;
        return out;
    }
    if(trace.Size()<static_cast<std::size_t>(settings.MinimumAnalysisFrames))
    {
        out.Fault=PhysicalReplayAnalysisFault::InsufficientHistory;
        return out;
    }
    const auto* first=trace.OldestFirst(0);
    const auto* last=trace.OldestFirst(trace.Size()-1);
    if(!first || !last)
    {
        out.Fault=PhysicalReplayAnalysisFault::NoSourceFrames;
        return out;
    }
    out.Identity=last->Identity;
    out.ActorRoster=first->ActorRoster;
    out.FirstClockSeconds=first->PhysicsClockSeconds;
    out.LastClockSeconds=last->PhysicsClockSeconds;
    out.ObservedDurationSeconds=
        out.LastClockSeconds-out.FirstClockSeconds;
    if(out.ObservedDurationSeconds<settings.MinimumElapsedWindowSeconds)
    {
        out.Fault=PhysicalReplayAnalysisFault::InsufficientHistory;
        return out;
    }
    out.ActorIdentitiesContinuous=true;
    out.EngineFramesContinuous=true;
    std::array<double,PhysicalFleetCapacity> speedTimes{};
    std::array<double,PhysicalFleetCapacity> accelerationSquares{};
    std::array<int,PhysicalAnalysisPairs> contactRuns{};
    std::array<int,PhysicalAnalysisPairs> nearRuns{};
    std::array<int,PhysicalAnalysisPairs> signBefore{};
    std::array<bool,PhysicalAnalysisPairs> wasAlongside{};
    std::array<bool,PhysicalAnalysisPairs> armedPass{};
    std::array<double,PhysicalAnalysisPairs> pendingPassStartClock{};
    for(std::size_t index=0;index<trace.Size();++index)
    {
        const auto* current=trace.OldestFirst(index);
        if(!current || !ValidPhysicalReplayAnalysisFrame(*current,settings))
        {
            out.Fault=PhysicalReplayAnalysisFault::InvalidPhysicalFrame;
            return out;
        }
        const auto* previous=index>0?trace.OldestFirst(index-1):nullptr;
        if(previous && !PhysicalReplayAnalysisFramePairContiguous(
                *previous,*current,settings))
        {
            out.Fault=current->ActorRoster!=previous->ActorRoster?
                PhysicalReplayAnalysisFault::ChangedRoster:
                current->Identity.Session!=previous->Identity.Session ||
                current->Identity.SessionRevision!=
                    previous->Identity.SessionRevision?
                    PhysicalReplayAnalysisFault::ChangedEpoch:
                    PhysicalReplayAnalysisFault::DiscontinuousFrame;
            out.ActorIdentitiesContinuous=false;
            out.EngineFramesContinuous=false;
            return out;
        }
        const double dt=previous?
            current->PhysicsClockSeconds-previous->PhysicsClockSeconds:0.0;
        out.LowestActualSensorConfidence=std::min(
            out.LowestActualSensorConfidence,current->MinimumObservedConfidence);
        ++out.SourceFrames;
        out.PhysicalContacts+=current->OverlappingPairs;
        out.PhysicalNearMissFrames+=current->NearMissPairs>0?1:0;
        out.PhysicalPredictedCollisionFrames+=
            current->PredictedContactPairs>0?1:0;
        out.PhysicalUnsafeFollowingFrames+=
            current->FollowingGapHazards>0?1:0;
        out.MinimumObservedTrafficGapCm=std::min(
            out.MinimumObservedTrafficGapCm,
            current->NearestVehicleSeparationCm);
        for(int car=0;car<PhysicalFleetCapacity;++car)
        {
            auto& stats=out.Cars[static_cast<std::size_t>(car)];
            const auto& actor=current->Actors[static_cast<std::size_t>(car)];
            const double speed=Length(actor.VelocityCmS);
            if(index==0)
            {
                stats.Actor=actor.Identity.Actor;
                stats.Slot=actor.Identity.Slot;
                stats.FirstClockSeconds=current->PhysicsClockSeconds;
                stats.StartSpeedCmS=speed;
                stats.MinimumObservedSensorConfidence=actor.SensorConfidence;
                stats.OriginalVehiclePhysicsVerified=true;
            }
            ++stats.ObservedFrames;
            stats.LastClockSeconds=current->PhysicsClockSeconds;
            stats.FinalSpeedCmS=speed;
            stats.MinimumSpeedCmS=std::min(stats.MinimumSpeedCmS,speed);
            stats.MaximumSpeedCmS=std::max(stats.MaximumSpeedCmS,speed);
            stats.MinimumObservedSensorConfidence=std::min(
                stats.MinimumObservedSensorConfidence,actor.SensorConfidence);
            if(!previous)continue;
            const auto& prior=previous->Actors[static_cast<std::size_t>(car)];
            const double lastSpeed=Length(prior.VelocityCmS);
            const double signedAcceleration=(speed-lastSpeed)/dt;
            const double magnitude=std::abs(signedAcceleration);
            stats.MaximumAccelerationCmS2=std::max(
                stats.MaximumAccelerationCmS2,std::max(0.0,signedAcceleration));
            stats.MaximumDecelerationCmS2=std::max(
                stats.MaximumDecelerationCmS2,std::max(0.0,-signedAcceleration));
            stats.HeavyAccelerationFrames+=
                signedAcceleration>settings.StrongAccelerationCmS2?1:0;
            stats.HeavyDecelerationFrames+=
                signedAcceleration<-settings.StrongDecelerationCmS2?1:0;
            const double headingRate=std::abs(AngleDifference(
                prior.Pose.HeadingRadians,actor.Pose.HeadingRadians))/dt;
            stats.MaximumHeadingRateRadiansS=std::max(
                stats.MaximumHeadingRateRadiansS,headingRate);
            stats.HeadingTravelRadians+=headingRate*dt;
            stats.SharpHeadingChangeFrames+=
                headingRate>settings.MaximumHeadingChangeRateRadiansS?1:0;
            stats.TotalMeasuredTravelCm+=Distance(
                prior.Pose.Position,actor.Pose.Position);
            speedTimes[static_cast<std::size_t>(car)]+=
                0.5*(lastSpeed+speed)*dt;
            accelerationSquares[static_cast<std::size_t>(car)]+=
                magnitude*magnitude*dt;
        }
        for(int pairIndex=0;pairIndex<PhysicalAnalysisPairs;++pairIndex)
        {
            const auto& pair=current->Pairs[static_cast<std::size_t>(pairIndex)];
            auto& stats=out.Pairs[static_cast<std::size_t>(pairIndex)];
            if(index==0)
            {
                stats.FirstActor=pair.FirstActor;
                stats.SecondActor=pair.SecondActor;
                stats.FirstSlot=pair.FirstSlot;
                stats.SecondSlot=pair.SecondSlot;
                stats.FirstObservedForwardCm=pair.ForwardSeparationCm;
                stats.PairIdentitiesStable=true;
            }
            ++stats.ObservedFrames;
            stats.LastObservedForwardCm=pair.ForwardSeparationCm;
            stats.MinimumCenterDistanceCm=std::min(
                stats.MinimumCenterDistanceCm,pair.CenterDistanceCm);
            stats.MinimumLongitudinalClearanceCm=std::min(
                stats.MinimumLongitudinalClearanceCm,pair.LongitudinalClearanceCm);
            stats.MinimumLateralClearanceCm=std::min(
                stats.MinimumLateralClearanceCm,pair.LateralClearanceCm);
            stats.MaximumClosingSpeedCmS=std::max(
                stats.MaximumClosingSpeedCmS,pair.ClosingSpeedCmS);
            stats.MinimumEstimatedTimeToContactSeconds=std::min(
                stats.MinimumEstimatedTimeToContactSeconds,pair.TimeToContactSeconds);
            stats.ContactFrames+=pair.MeasuredOverlap?1:0;
            stats.NearMissFrames+=pair.NearMiss?1:0;
            stats.FollowingRiskFrames+=pair.FollowingGapHazard?1:0;
            stats.PredictedContactFrames+=pair.PredictedContact?1:0;
            stats.SignificantClosingFrames+=
                pair.ClosingSpeedCmS>settings.HighClosingSpeedCmS?1:0;
            stats.TotalMeasuredContactSeconds+=pair.MeasuredOverlap?dt:0.0;
            stats.TotalObservedNearMissSeconds+=pair.NearMiss?dt:0.0;
            contactRuns[static_cast<std::size_t>(pairIndex)]=
                pair.MeasuredOverlap?
                    contactRuns[static_cast<std::size_t>(pairIndex)]+1:0;
            nearRuns[static_cast<std::size_t>(pairIndex)]=
                pair.NearMiss?
                    nearRuns[static_cast<std::size_t>(pairIndex)]+1:0;
            stats.ConsecutiveContactPeakFrames=std::max(
                stats.ConsecutiveContactPeakFrames,
                contactRuns[static_cast<std::size_t>(pairIndex)]);
            stats.ConsecutiveNearPeakFrames=std::max(
                stats.ConsecutiveNearPeakFrames,
                nearRuns[static_cast<std::size_t>(pairIndex)]);
            for(int slot:{pair.FirstSlot,pair.SecondSlot})
            {
                auto& car=out.Cars[static_cast<std::size_t>(slot)];
                car.ClosestObservedRivalCm=std::min(
                    car.ClosestObservedRivalCm,pair.CenterDistanceCm);
                car.PeakClosingSpeedCmS=std::max(
                    car.PeakClosingSpeedCmS,pair.ClosingSpeedCmS);
                car.InContactFrames+=pair.MeasuredOverlap?1:0;
                car.UnsafeFollowingFrames+=pair.FollowingGapHazard?1:0;
                car.NearTrafficFrames+=
                    pair.CenterDistanceCm<=settings.NearCarDistanceCm?1:0;
            }
            if(index==0)
            {
                const int sign=pair.ForwardSeparationCm>
                    settings.MinimumPassClearanceCm?1:
                    pair.ForwardSeparationCm<-
                    settings.MinimumPassClearanceCm?-1:0;
                signBefore[static_cast<std::size_t>(pairIndex)]=sign;
                pendingPassStartClock[static_cast<std::size_t>(pairIndex)]=
                    current->PhysicsClockSeconds;
            }
            // This is a physically observed crossing of a relative
            // leading position, not official race position or lap order.
            // Only lateral separation maintained during the transition
            // may qualify a non-contact pass observation.
            const auto& left=current->Actors[
                static_cast<std::size_t>(pair.FirstSlot)];
            const auto& right=current->Actors[
                static_cast<std::size_t>(pair.SecondSlot)];
            const bool physicallySeparate=
                std::abs(left.TrackLateralOffsetCm-
                    right.TrackLateralOffsetCm)>=
                        settings.LateralOvertakeSeparationCm;
            const bool significant=
                std::abs(pair.ForwardSeparationCm)>=
                    settings.MinimumCompletedPassDeltaCm;
            const int side=pair.ForwardSeparationCm>
                    settings.MinimumPassClearanceCm?1:
                pair.ForwardSeparationCm<-
                    settings.MinimumPassClearanceCm?-1:0;
            if(physicallySeparate &&
               std::abs(pair.ForwardSeparationCm)<
                    settings.MinimumCompletedPassDeltaCm)
                wasAlongside[static_cast<std::size_t>(pairIndex)]=true;
            if(signBefore[static_cast<std::size_t>(pairIndex)]!=0 &&
               side==-signBefore[static_cast<std::size_t>(pairIndex)] &&
               physicallySeparate && significant &&
               wasAlongside[static_cast<std::size_t>(pairIndex)] &&
               !armedPass[static_cast<std::size_t>(pairIndex)] &&
               current->PhysicsClockSeconds-
                   pendingPassStartClock[static_cast<std::size_t>(pairIndex)]>=
                       settings.MinimumPassingDurationSeconds)
            {
                ++stats.PhysicallyCompletedPasses;
                ++stats.ConfirmedLeaderSwaps;
                ++out.ReviewOnlyOvertakeTransitions;
                ++out.TotalPhysicallyCompletedPasses;
                const int leader=side>0?pair.SecondSlot:pair.FirstSlot;
                const int trailer=side>0?pair.FirstSlot:pair.SecondSlot;
                ++out.Cars[static_cast<std::size_t>(leader)].PassesObserved;
                ++out.Cars[static_cast<std::size_t>(trailer)].PassedByRivals;
                armedPass[static_cast<std::size_t>(pairIndex)]=true;
            }
            if(side==signBefore[static_cast<std::size_t>(pairIndex)] &&
               side!=0 && significant)
                armedPass[static_cast<std::size_t>(pairIndex)]=false;
        }
    }
    for(int car=0;car<PhysicalFleetCapacity;++car)
    {
        auto& outCar=out.Cars[static_cast<std::size_t>(car)];
        outCar.DurationSeconds=out.ObservedDurationSeconds;
        outCar.MeanSpeedCmS=speedTimes[static_cast<std::size_t>(car)]/
            out.ObservedDurationSeconds;
        outCar.RmsAccelerationCmS2=std::sqrt(
            accelerationSquares[static_cast<std::size_t>(car)]/
                out.ObservedDurationSeconds);
        outCar.Valid=outCar.ObservedFrames==out.SourceFrames &&
            outCar.Actor==out.ActorRoster[static_cast<std::size_t>(car)] &&
            Finite(outCar.MeanSpeedCmS) &&
            Finite(outCar.RmsAccelerationCmS2) &&
            outCar.OriginalVehiclePhysicsVerified;
        if(!outCar.Valid)
        {
            out.Fault=PhysicalReplayAnalysisFault::UnverifiedActor;
            return out;
        }
    }
    for(auto& pair:out.Pairs)
    {
        pair.Valid=pair.PairIdentitiesStable &&
            pair.ObservedFrames==out.SourceFrames &&
            Finite(pair.MinimumCenterDistanceCm) &&
            Finite(pair.MaximumClosingSpeedCmS);
        if(!pair.Valid)
        {
            out.Fault=PhysicalReplayAnalysisFault::UnverifiedPair;
            return out;
        }
    }
    out.LastFramePack=ReviewPhysicalReplayPack(*last,settings);
    out.Valid=out.LastFramePack.Valid &&
        out.ActorIdentitiesContinuous && out.EngineFramesContinuous &&
        out.OriginalRaceStateUntouched &&
        !out.RealUnrealVehicleActuationIssued &&
        !out.OfficialRaceClassificationIssued;
    out.Fault=out.Valid?PhysicalReplayAnalysisFault::None:
        PhysicalReplayAnalysisFault::InvalidPhysicalFrame;
    if(out.Valid)
        out.DriverCoach=ReviewPhysicalReplayCoach(out,settings);
    return out;
}
class PhysicalReplayAnalyticsSupervisor final
{
public:
    bool Configure(PhysicalReplayAnalysisSettings settings) noexcept
    {
        if(!ValidPhysicalReplayAnalysisSettings(settings))return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Trace.Reset();
        Source={};
        Seen=0;
        Rejected=0;
        LastFault=PhysicalReplayAnalysisFault::NoSourceFrames;
        LastReport={};
        HasSource=false;
    }
    PhysicalReplayAnalysisFault Observe(
        const RivalLaneTrafficFrame& sample) noexcept
    {
        const auto fault=Trace.Append(sample);
        if(fault!=PhysicalTraceFault::None)
        {
            ++Rejected;
            LastFault=PhysicalReplayAnalysisFault::InvalidPhysicalFrame;
            return LastFault;
        }
        ++Seen;
        Source=sample.Self.Identity;
        HasSource=true;
        if(Trace.Size()<static_cast<std::size_t>(Settings.MinimumAnalysisFrames))
        {
            LastFault=PhysicalReplayAnalysisFault::InsufficientHistory;
            return LastFault;
        }
        LastReport=AnalyzePhysicalReplayWindow(Trace,Settings);
        LastFault=LastReport.Fault;
        return LastFault;
    }
    PhysicalReplayAnalysisReport Review() const noexcept
    {return LastReport;}
    const PhysicalSixCarRaceTrace& PhysicalSource() const noexcept
    {return Trace;}
    std::uint64_t AcceptedFrames() const noexcept{return Seen;}
    std::uint64_t RejectedFrames() const noexcept{return Rejected;}
    bool HasLiveSource() const noexcept{return HasSource;}
    RaceIdentity LastSourceIdentity() const noexcept{return Source;}
    PhysicalReplayAnalysisFault LastRejection() const noexcept
    {return LastFault;}
private:
    PhysicalReplayAnalysisSettings Settings{};
    PhysicalSixCarRaceTrace Trace{};
    PhysicalReplayAnalysisReport LastReport{};
    RaceIdentity Source{};
    std::uint64_t Seen=0;
    std::uint64_t Rejected=0;
    PhysicalReplayAnalysisFault LastFault=
        PhysicalReplayAnalysisFault::NoSourceFrames;
    bool HasSource=false;
};
} // namespace latebrake::racecraft