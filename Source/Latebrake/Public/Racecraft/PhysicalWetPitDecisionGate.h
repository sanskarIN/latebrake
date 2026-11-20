#pragma once
#include "Racecraft/PhysicalWetPitStrategy.h"
#include "Racecraft/PhysicalSixCarPitCorridor.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace latebrake {
// Cross-source, stateful manual pit recommendation gate. A clear service
// corridor is NOT engine admission: the actual UE pit component retains
// original hold-time, low-speed, grounded and authorization requirements.
// No pit entry, credit, lap finish, motion, tyre or fuel command is emitted.
enum class WetPitDecision : std::uint8_t {
    Unqualified, StayOnRoad, PlanFutureStop, ApproachWithCaution,
    WaitForTraffic, RequestNativePitSafetyCheck
};
enum class WetPitDecisionFault : std::uint8_t {
    None, InvalidSettings, BadSources, WrongSession, StaleFrame,
    UntrustedDriver, MixedClock, NoPhysicalContinuity
};
struct WetPitDecisionSettings final {
    unsigned MinimumStableFrames=2u;
    unsigned MaximumStableFrames=8u;
    double MaximumSourceClockSkewSeconds=0.05;
    double MaximumAcceptedFrameSeconds=0.12;
    double MinimumAcceptedFrameSeconds=0.001;
    double MinimumStintTyreMarginMm=0.0;
    double MaximumConsideredPitETASeconds=4.0;
    bool DemandSixPhysicalActors=true;
    bool DemandTrueOfficialLapClock=true;
    bool RequireAuthenticPitSafetyResult=true;
    bool NeverIssuePitRequest=true;
    bool NeverAwardLapOrCareerResult=true;
    bool NoWheelActuatorChanges=true;
};
struct WetPitDecisionFrame final {
    const WetPitStrategyPlan* Strategy=nullptr;
    const PitCorridorTrafficFrame* Traffic=nullptr;
    std::uint64_t SourceSession=0u;
    std::uint64_t SourceRevision=0u;
    std::uint64_t PlayerActor=0u;
    std::uint64_t SourceFrame=0u;
    double EngineClockSeconds=0.0;
    double DeltaSeconds=0.0;
    unsigned OfficialRemainingLaps=0u;
    bool RealPlayerChaosBodyObserved=false;
    bool NativePitServiceAuthorizationAvailable=false;
    bool OriginalPitHoldIsNotBypassed=true;
    bool OriginalOfficialClockVerified=false;
};
struct WetPitDecisionReceipt final {
    bool Valid=false;
    WetPitDecisionFault Fault=WetPitDecisionFault::BadSources;
    WetPitDecision Proposed=WetPitDecision::Unqualified;
    WetPitDecision Confirmed=WetPitDecision::Unqualified;
    std::uint64_t SourceSession=0u;
    std::uint64_t SourceFrame=0u;
    double SourceClockSeconds=0.0;
    unsigned StablePhysicalFrames=0u;
    bool OriginalPitHoldRequired=true;
    bool NoActualServiceIssued=true;
    bool NoRaceOrTyreChange=true;
};
inline bool ValidWetPitDecisionSettings(
    WetPitDecisionSettings settings) noexcept
{
    return settings.MinimumStableFrames>=2u &&
        settings.MinimumStableFrames<=5u &&
        settings.MaximumStableFrames>=settings.MinimumStableFrames &&
        settings.MaximumStableFrames<=10u &&
        std::isfinite(settings.MaximumSourceClockSkewSeconds) &&
        settings.MaximumSourceClockSkewSeconds>=0.0 &&
        settings.MaximumSourceClockSkewSeconds<=0.1 &&
        std::isfinite(settings.MaximumAcceptedFrameSeconds) &&
        settings.MaximumAcceptedFrameSeconds>=0.02 &&
        settings.MaximumAcceptedFrameSeconds<=0.2 &&
        std::isfinite(settings.MinimumAcceptedFrameSeconds) &&
        settings.MinimumAcceptedFrameSeconds>=0.001 &&
        settings.MinimumAcceptedFrameSeconds<=0.01 &&
        settings.MaximumAcceptedFrameSeconds>
            settings.MinimumAcceptedFrameSeconds &&
        std::isfinite(settings.MinimumStintTyreMarginMm) &&
        settings.MinimumStintTyreMarginMm>=0.0 &&
        settings.MinimumStintTyreMarginMm<=1.0 &&
        std::isfinite(settings.MaximumConsideredPitETASeconds) &&
        settings.MaximumConsideredPitETASeconds>=1.0 &&
        settings.MaximumConsideredPitETASeconds<=5.0 &&
        settings.DemandSixPhysicalActors &&
        settings.DemandTrueOfficialLapClock &&
        settings.RequireAuthenticPitSafetyResult &&
        settings.NeverIssuePitRequest &&
        settings.NeverAwardLapOrCareerResult &&
        settings.NoWheelActuatorChanges;
}
inline bool ValidWetPitDecisionFrame(
    WetPitDecisionFrame frame,
    WetPitDecisionSettings settings={}) noexcept
{
    if(!ValidWetPitDecisionSettings(settings) ||
       frame.Strategy==nullptr ||
       frame.Traffic==nullptr ||
       !frame.Strategy->Valid ||
       !frame.Traffic->Valid ||
       frame.Strategy->Fault!=WetPitStrategyFault::None ||
       frame.Traffic->Fault!=PitCorridorFault::None ||
       frame.Strategy->Circuit!=frame.Traffic->Circuit ||
       frame.SourceSession==0u ||
       frame.SourceRevision==0u ||
       frame.PlayerActor==0u ||
       frame.SourceFrame==0u ||
       frame.SourceSession!=frame.Strategy->Session ||
       frame.SourceSession!=frame.Traffic->Session ||
       frame.SourceRevision!=frame.Strategy->Revision ||
       frame.SourceRevision!=frame.Traffic->Revision ||
       frame.PlayerActor!=frame.Traffic->DriverActor ||
       frame.SourceFrame!=frame.Strategy->LastSourceFrame ||
       frame.SourceFrame!=frame.Traffic->SourceFrame ||
       frame.OfficialRemainingLaps!=frame.Strategy->RemainingLaps ||
       frame.OfficialRemainingLaps==0u ||
       frame.Traffic->QualifiedCars!=PitCorridorCarCount ||
       !frame.RealPlayerChaosBodyObserved ||
       !frame.OriginalPitHoldIsNotBypassed ||
       !frame.OriginalOfficialClockVerified ||
       !frame.Strategy->NoOfficialTimingAuthority ||
       !frame.Strategy->NoOriginalPhysicsChanges ||
       !frame.Traffic->NoNativePitServiceIssued ||
       !frame.Traffic->OriginalRaceTimingUnchanged ||
       !std::isfinite(frame.EngineClockSeconds) ||
       !std::isfinite(frame.DeltaSeconds) ||
       frame.EngineClockSeconds<=0.0 ||
       frame.DeltaSeconds<settings.MinimumAcceptedFrameSeconds ||
       frame.DeltaSeconds>settings.MaximumAcceptedFrameSeconds ||
       std::abs(frame.EngineClockSeconds-
           frame.Strategy->SourceClockSeconds)>
           settings.MaximumSourceClockSkewSeconds ||
       std::abs(frame.EngineClockSeconds-
           frame.Traffic->SourceClockSeconds)>
           settings.MaximumSourceClockSkewSeconds)
        return false;
    unsigned playerMatches=0u;
    for(const auto& car:frame.Traffic->Cars) {
        if(!car.Valid || car.Actor==0u ||
           !car.ActualPitEntryNotGranted)
            return false;
        if(car.Actor==frame.PlayerActor)++playerMatches;
    }
    return playerMatches==1u;
}
inline WetPitDecision ClassifyWetPitDecision(
    WetPitDecisionFrame frame,
    WetPitDecisionSettings settings={}) noexcept
{
    if(!ValidWetPitDecisionFrame(frame,settings))
        return WetPitDecision::Unqualified;
    const auto& plan=*frame.Strategy;
    const auto& traffic=*frame.Traffic;
    const bool planNeedsStop=
        plan.Advisory==WetPitAdvisory::StopSoonForWear ||
        plan.Advisory==WetPitAdvisory::EvaluateAlternativeCompound;
    if(!planNeedsStop ||
       !plan.Best.Valid ||
       plan.FeasibleCandidateCount==0u)
        return WetPitDecision::StayOnRoad;
    if(plan.Best.PlannedStopAfterLaps>1u)
        return WetPitDecision::PlanFutureStop;
    if(traffic.Traffic==PitCorridorTraffic::Blocked ||
       traffic.Traffic==PitCorridorTraffic::YieldToCrossTraffic)
        return WetPitDecision::WaitForTraffic;
    if(traffic.Traffic==PitCorridorTraffic::Unknown)
        return WetPitDecision::Unqualified;
    if(traffic.Traffic==PitCorridorTraffic::WatchApproach &&
       traffic.MinimumPredictedConflictSeconds<=
           settings.MaximumConsideredPitETASeconds)
        return WetPitDecision::ApproachWithCaution;
    bool withinSourcePit=false;
    for(const auto& car:traffic.Cars)
        if(car.Actor==frame.PlayerActor){
            withinSourcePit=car.CouldEnterService;
            break;
        }
    if(!withinSourcePit ||
       !frame.NativePitServiceAuthorizationAvailable)
        return WetPitDecision::ApproachWithCaution;
    return WetPitDecision::RequestNativePitSafetyCheck;
}
class PhysicalWetPitDecisionGate final {
public:
    void Reset() noexcept {
        LastSession=0u;
        LastRevision=0u;
        LastActor=0u;
        LastFrame=0u;
        LastClock=0.0;
        Stable=0u;
        Previous=WetPitDecision::Unqualified;
    }
    unsigned StableFrameCount() const noexcept {
        return Stable;
    }
    WetPitDecisionReceipt Observe(
        WetPitDecisionFrame input,
        WetPitDecisionSettings settings={}) noexcept
    {
        WetPitDecisionReceipt result;
        if(!ValidWetPitDecisionSettings(settings)){
            Reset();
            result.Fault=WetPitDecisionFault::InvalidSettings;
            return result;
        }
        if(!ValidWetPitDecisionFrame(input,settings)){
            Reset();
            result.Fault=WetPitDecisionFault::BadSources;
            return result;
        }
        const auto proposed=ClassifyWetPitDecision(input,settings);
        if(proposed==WetPitDecision::Unqualified){
            Reset();
            result.Fault=WetPitDecisionFault::BadSources;
            return result;
        }
        const bool hadPrior=LastFrame!=0u;
        if(hadPrior){
            if(input.SourceSession!=LastSession ||
               input.SourceRevision!=LastRevision ||
               input.PlayerActor!=LastActor){
                Reset();
                result.Fault=WetPitDecisionFault::WrongSession;
                return result;
            }
            if(input.SourceFrame<=LastFrame ||
               input.SourceFrame-LastFrame>8u){
                Reset();
                result.Fault=WetPitDecisionFault::StaleFrame;
                return result;
            }
            const double elapsed=input.EngineClockSeconds-LastClock;
            if(!std::isfinite(elapsed) ||
               elapsed<=0.0 ||
               elapsed>settings.MaximumAcceptedFrameSeconds ||
               std::abs(elapsed-input.DeltaSeconds)>0.02){
                Reset();
                result.Fault=WetPitDecisionFault::MixedClock;
                return result;
            }
        }
        if(proposed!=Previous)Stable=0u;
        Previous=proposed;
        Stable=std::min(
            settings.MaximumStableFrames,Stable+1u);
        LastSession=input.SourceSession;
        LastRevision=input.SourceRevision;
        LastActor=input.PlayerActor;
        LastFrame=input.SourceFrame;
        LastClock=input.EngineClockSeconds;
        result.Valid=true;
        result.Fault=WetPitDecisionFault::None;
        result.Proposed=proposed;
        result.Confirmed=Stable>=settings.MinimumStableFrames ?
            proposed:WetPitDecision::Unqualified;
        result.SourceSession=LastSession;
        result.SourceFrame=LastFrame;
        result.SourceClockSeconds=LastClock;
        result.StablePhysicalFrames=Stable;
        return result;
    }
private:
    std::uint64_t LastSession=0u;
    std::uint64_t LastRevision=0u;
    std::uint64_t LastActor=0u;
    std::uint64_t LastFrame=0u;
    double LastClock=0.0;
    unsigned Stable=0u;
    WetPitDecision Previous=WetPitDecision::Unqualified;
};
} // namespace latebrake
