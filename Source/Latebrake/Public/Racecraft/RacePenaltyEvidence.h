#pragma once
#include "Racecraft/TrackLimitsEvidence.h"
#include "Racecraft/CollisionImpulseModel.h"
#include "Racecraft/VerifiedGateCrossing.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
enum class ReviewableIncident : std::uint8_t
{
    None, OffRoad, PhysicalCollision, PitSpeed,
    YellowFlagPassing, WrongDirection, MissingCheckpoint,
    UnsafeRejoin, ExcessiveBlocking
};
enum class IncidentConfidence : std::uint8_t
{
    Unqualified, Inferred, PhysicalSource, MultipleIndependentSources
};
struct PenaltyEvidenceParameters final
{
    double MinimumOffroadSeconds=0.25;
    double MinimumOffroadSpeedCmS=150.0;
    double MinimumCollisionEnergyJ=500.0;
    double MinimumCollisionSpeedCmS=180.0;
    double MaximumDuplicateEventSpacingSeconds=0.75;
    double MaximumObservationClockSkewSeconds=0.12;
    double MaximumQualifiedPenaltyClockSeconds=86400.0;
    double MinimumYellowPassingProgressCm=300.0;
    double MinimumSpeedingMarginCmS=100.0;
    double MinimumUnsafeRejoinLateralCmS=250.0;
    double MinimumBlockingMoveCm=250.0;
    double MaximumAllowedSpeedCmS=MaximumPhysicalSpeedCmS;
    double EventRetentionSeconds=1800.0;
    int MaximumEvidenceEvents=128;
    int MaximumIncidentsPerLap=32;
    int MaximumResultClassificationChanges=0;
    bool RequireOriginalPhysicalChassis=true;
    bool RequireIndependentContactProvenance=true;
    bool RequireActualRaceDirectorFlag=true;
    bool OnlyReviewNeverPenalize=true;
};
struct SportingIncidentObservation final
{
    RaceIdentity Identity{};
    ReviewableIncident Kind=ReviewableIncident::None;
    double RaceClockSeconds=0.0;
    double MonotonicSeconds=0.0;
    double TrackDistanceCm=0.0;
    double SpeedCmS=0.0;
    double RelativeSpeedCmS=0.0;
    double ContactEnergyJ=0.0;
    double RoadTimeOutsideSeconds=0.0;
    double MeasuredPitSpeedLimitCmS=0.0;
    double MeasuredProgressGainCm=0.0;
    double LateralVelocityCmS=0.0;
    double LateralBlockingMoveCm=0.0;
    std::uint64_t OtherActor=0;
    int LapIndex=0;
    int SectorIndex=0;
    bool EngineWorldContactVerified=false;
    bool OnOriginalCircuit=false;
    bool RealGameModeActive=false;
    bool PhysicallyObservedRoad=true;
    bool ActualMarshalFlagVerified=false;
    bool ActivePenaltyRulesAvailable=false;
};
struct PenaltyEvidenceEvent final
{
    ReviewableIncident Kind=ReviewableIncident::None;
    IncidentConfidence Confidence=IncidentConfidence::Unqualified;
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    std::uint64_t OtherActor=0;
    std::uint64_t SourceFrame=0;
    double RaceClockSeconds=0.0;
    double TrackDistanceCm=0.0;
    double Magnitude=0.0;
    double VerifiedSeverityFraction=0.0;
    double ConfidenceFraction=0.0;
    double PotentialTimePenaltySeconds=0.0;
    int LapIndex=0;
    int SectorIndex=0;
    bool ReviewRequired=false;
    bool PhysicalSourceVerified=false;
    bool OfficialSanctionIssued=false;
    bool Valid=false;
};
struct PenaltyEvidenceHistory final
{
    std::array<PenaltyEvidenceEvent,128> Events{};
    int Count=0;
    int Head=0;
    std::uint64_t AcceptedEvents=0;
    std::uint64_t RejectedEvents=0;
    std::uint64_t DuplicateEvents=0;
    std::uint64_t EventsDroppedAtCapacity=0;
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    double MaximumEventSeverity=0.0;
    double SumPotentialPenaltySeconds=0.0;
    double LastEventClockSeconds=0.0;
    bool ChangedOfficialRaceTime=false;
    bool ChangedRaceRewards=false;
};
inline bool ValidPenaltyEvidenceParameters(
    PenaltyEvidenceParameters p) noexcept
{
    return Finite(p.MinimumOffroadSeconds)&&
        p.MinimumOffroadSeconds>0.0 &&
        Finite(p.MinimumOffroadSpeedCmS)&&
        p.MinimumOffroadSpeedCmS>=0.0 &&
        Finite(p.MinimumCollisionEnergyJ)&&
        p.MinimumCollisionEnergyJ>=0.0 &&
        Finite(p.MinimumCollisionSpeedCmS)&&
        p.MinimumCollisionSpeedCmS>=0.0 &&
        Finite(p.MaximumDuplicateEventSpacingSeconds)&&
        p.MaximumDuplicateEventSpacingSeconds>0.0 &&
        Finite(p.MaximumObservationClockSkewSeconds)&&
        p.MaximumObservationClockSkewSeconds>=0.0 &&
        Finite(p.MaximumQualifiedPenaltyClockSeconds)&&
        p.MaximumQualifiedPenaltyClockSeconds>0.0 &&
        Finite(p.MinimumYellowPassingProgressCm)&&
        p.MinimumYellowPassingProgressCm>0.0 &&
        Finite(p.MinimumSpeedingMarginCmS)&&
        p.MinimumSpeedingMarginCmS>=0.0 &&
        Finite(p.MaximumAllowedSpeedCmS)&&
        p.MaximumAllowedSpeedCmS>0.0 &&
        Finite(p.EventRetentionSeconds)&&
        p.EventRetentionSeconds>0.0 &&
        p.MaximumEvidenceEvents>=1 &&
        p.MaximumEvidenceEvents<=128 &&
        p.MaximumIncidentsPerLap>=1 &&
        p.MaximumIncidentsPerLap<=128 &&
        p.MaximumResultClassificationChanges==0;
}
inline bool ValidSportingIncident(
    SportingIncidentObservation s,
    PenaltyEvidenceParameters p) noexcept
{
    return PhysicalActorIdentityValid(s.Identity) &&
        s.Kind!=ReviewableIncident::None &&
        s.RealGameModeActive &&
        s.OnOriginalCircuit &&
        s.PhysicallyObservedRoad &&
        Finite(s.RaceClockSeconds) &&
        Finite(s.MonotonicSeconds) &&
        Finite(s.TrackDistanceCm) &&
        Finite(s.SpeedCmS) &&
        Finite(s.RelativeSpeedCmS) &&
        Finite(s.ContactEnergyJ) &&
        Finite(s.RoadTimeOutsideSeconds) &&
        Finite(s.MeasuredPitSpeedLimitCmS) &&
        Finite(s.MeasuredProgressGainCm) &&
        Finite(s.LateralVelocityCmS) &&
        Finite(s.LateralBlockingMoveCm) &&
        s.RaceClockSeconds>=0.0 &&
        s.RaceClockSeconds<=
            p.MaximumQualifiedPenaltyClockSeconds &&
        s.TrackDistanceCm>=0.0 &&
        s.SpeedCmS>=0.0 &&
        s.SpeedCmS<=p.MaximumAllowedSpeedCmS &&
        s.ContactEnergyJ>=0.0 &&
        s.RoadTimeOutsideSeconds>=0.0 &&
        s.MeasuredPitSpeedLimitCmS>=0.0 &&
        s.LapIndex>=0 &&
        s.SectorIndex>=0;
}
inline PenaltyEvidenceEvent EvaluateSportingIncident(
    SportingIncidentObservation s,
    PenaltyEvidenceParameters p={}) noexcept
{
    PenaltyEvidenceEvent out;
    if (!ValidPenaltyEvidenceParameters(p) ||
        !ValidSportingIncident(s,p))
        return out;
    double severity=0.0;
    double magnitude=0.0;
    bool physicallyWitnessed=false;
    switch(s.Kind)
    {
    case ReviewableIncident::OffRoad:
        if (s.RoadTimeOutsideSeconds<
            p.MinimumOffroadSeconds ||
            s.SpeedCmS<p.MinimumOffroadSpeedCmS)
            return out;
        magnitude=s.RoadTimeOutsideSeconds;
        severity=Clamp01(magnitude/4.0);
        physicallyWitnessed=s.PhysicallyObservedRoad;
        break;
    case ReviewableIncident::PhysicalCollision:
        if (!s.EngineWorldContactVerified ||
            s.ContactEnergyJ<p.MinimumCollisionEnergyJ ||
            s.RelativeSpeedCmS<p.MinimumCollisionSpeedCmS)
            return out;
        magnitude=s.ContactEnergyJ;
        severity=Clamp01(SafeDivision(
            magnitude,40000.0));
        physicallyWitnessed=true;
        break;
    case ReviewableIncident::PitSpeed:
        if (s.MeasuredPitSpeedLimitCmS<=0.0 ||
            s.SpeedCmS<
                s.MeasuredPitSpeedLimitCmS+
                p.MinimumSpeedingMarginCmS)
            return out;
        magnitude=s.SpeedCmS-
            s.MeasuredPitSpeedLimitCmS;
        severity=Clamp01(magnitude/1000.0);
        physicallyWitnessed=true;
        break;
    case ReviewableIncident::YellowFlagPassing:
        if (!s.ActualMarshalFlagVerified ||
            s.MeasuredProgressGainCm<
                p.MinimumYellowPassingProgressCm)
            return out;
        magnitude=s.MeasuredProgressGainCm;
        severity=Clamp01(magnitude/1000.0);
        physicallyWitnessed=true;
        break;
    case ReviewableIncident::WrongDirection:
        if (s.MeasuredProgressGainCm>=-100.0)
            return out;
        magnitude=-s.MeasuredProgressGainCm;
        severity=Clamp01(magnitude/1200.0);
        physicallyWitnessed=true;
        break;
    case ReviewableIncident::UnsafeRejoin:
        if (std::abs(s.LateralVelocityCmS)<
            p.MinimumUnsafeRejoinLateralCmS)
            return out;
        magnitude=std::abs(s.LateralVelocityCmS);
        severity=Clamp01(magnitude/1000.0);
        physicallyWitnessed=true;
        break;
    case ReviewableIncident::ExcessiveBlocking:
        if (s.LateralBlockingMoveCm<
            p.MinimumBlockingMoveCm ||
            s.OtherActor==0)
            return out;
        magnitude=s.LateralBlockingMoveCm;
        severity=Clamp01(magnitude/800.0);
        physicallyWitnessed=true;
        break;
    case ReviewableIncident::MissingCheckpoint:
        if (s.MeasuredProgressGainCm<OriginalCircuitLapCm)
            return out;
        magnitude=s.MeasuredProgressGainCm;
        severity=0.5;
        physicallyWitnessed=true;
        break;
    default:return out;
    }
    out.Kind=s.Kind;
    out.Confidence=physicallyWitnessed ?
        IncidentConfidence::PhysicalSource :
        IncidentConfidence::Inferred;
    out.Session=s.Identity.Session;
    out.Actor=s.Identity.Actor;
    out.OtherActor=s.OtherActor;
    out.SourceFrame=s.Identity.Frame;
    out.RaceClockSeconds=s.RaceClockSeconds;
    out.TrackDistanceCm=s.TrackDistanceCm;
    out.Magnitude=magnitude;
    out.VerifiedSeverityFraction=severity;
    out.ConfidenceFraction=physicallyWitnessed?1.0:0.4;
    out.PotentialTimePenaltySeconds=0.0;
    out.LapIndex=s.LapIndex;
    out.SectorIndex=s.SectorIndex;
    out.ReviewRequired=true;
    out.PhysicalSourceVerified=physicallyWitnessed;
    out.OfficialSanctionIssued=false;
    out.Valid=true;
    return out;
}
class SportingIncidentLedger final
{
public:
    bool Configure(PenaltyEvidenceParameters settings) noexcept
    {
        if (!ValidPenaltyEvidenceParameters(settings))
            return false;
        Settings=settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        History={};
        CurrentLap=-1;
        ThisLap=0;
        LastFrame=0;
    }
    bool Observe(SportingIncidentObservation sample) noexcept
    {
        const auto event=EvaluateSportingIncident(
            sample,Settings);
        if (!event.Valid ||
            (History.Session!=0 &&
             (History.Session!=event.Session ||
              History.Actor!=event.Actor)) ||
            (LastFrame!=0 &&
             event.SourceFrame<=LastFrame))
        {
            ++History.RejectedEvents;
            return false;
        }
        if (CurrentLap!=event.LapIndex)
        {
            CurrentLap=event.LapIndex;
            ThisLap=0;
        }
        if (ThisLap>=Settings.MaximumIncidentsPerLap)
        {
            ++History.RejectedEvents;
            return false;
        }
        for (int i=0;i<History.Count;++i)
        {
            const auto& earlier=History.Events[
                static_cast<std::size_t>(
                    (History.Head+i)%128)];
            if (earlier.Kind==event.Kind &&
                earlier.LapIndex==event.LapIndex &&
                std::abs(earlier.RaceClockSeconds-
                    event.RaceClockSeconds)<
                    Settings.MaximumDuplicateEventSpacingSeconds &&
                earlier.OtherActor==event.OtherActor)
            {
                ++History.DuplicateEvents;
                return false;
            }
        }
        if (History.Count>=Settings.MaximumEvidenceEvents)
        {
            History.Head=(History.Head+1)%128;
            --History.Count;
            ++History.EventsDroppedAtCapacity;
        }
        History.Events[static_cast<std::size_t>(
            (History.Head+History.Count)%128)]=event;
        ++History.Count;
        ++ThisLap;
        ++History.AcceptedEvents;
        History.Session=event.Session;
        History.Actor=event.Actor;
        LastFrame=event.SourceFrame;
        History.LastEventClockSeconds=
            event.RaceClockSeconds;
        History.MaximumEventSeverity=std::max(
            History.MaximumEventSeverity,
            event.VerifiedSeverityFraction);
        History.ChangedOfficialRaceTime=false;
        History.ChangedRaceRewards=false;
        return true;
    }
    PenaltyEvidenceHistory Snapshot() const noexcept
    {return History;}
    PenaltyEvidenceEvent MostRecent() const noexcept
    {
        return History.Count>0?
            History.Events[static_cast<std::size_t>(
                (History.Head+History.Count-1)%128)]:
            PenaltyEvidenceEvent{};
    }
    int Stored() const noexcept{return History.Count;}
private:
    PenaltyEvidenceParameters Settings{};
    PenaltyEvidenceHistory History{};
    int CurrentLap=-1;
    int ThisLap=0;
    std::uint64_t LastFrame=0;
};
} // namespace latebrake::racecraft
