#pragma once
#include "Track/WorldScaleCircuitVariants.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Real-source evidence ledger for six prospective physical circuits.
// A successful local objective is NEVER a cash award, career unlock,
// podium result or certified lap. Existing official race/save systems
// remain the only authorities and can consume evidence after UE QA.
inline constexpr unsigned PhysicalChallengeMaximumFrames=512u;
inline constexpr unsigned PhysicalChallengeMaximumLaps=12u;
enum class PhysicalChallengeKind : std::uint8_t {
    LapConsistency, CleanStint, SpeedTrap, CornerDiscipline,
    Endurance, MeasuredWetHandling
};
enum class PhysicalChallengeFault : std::uint8_t {
    None, InvalidRules, BadRoad, MissingEngineAuthority,
    WrongSession, WrongActor, StaleFrame, ClockRollback,
    ImpossibleMotion, InvalidTelemetry, IncompleteEvidence
};
enum class PhysicalChallengeState : std::uint8_t {
    Unconfigured, Armed, Active, SourceQualified, Rejected
};
struct PhysicalChallengeDefinition final {
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    PhysicalChallengeKind Kind=PhysicalChallengeKind::LapConsistency;
    unsigned RequiredLaps=3u;
    double MaximumElapsedSeconds=1200.0;
    double RequiredTrapSpeedCmps=1800.0;
    double MaximumBestWorstGapSeconds=12.0;
    double MaximumCornerOverspeedCmps=300.0;
    double MinimumMeasuredWaterMm=0.2;
    double MaximumAssistInput=0.50;
    unsigned AllowedCollisions=0u;
    unsigned AllowedRecoveries=0u;
    bool RequireActualEngineLapClock=true;
    bool RequireOrderedOriginalGateWitness=true;
    bool RequireRealChassisAndContacts=true;
    bool NeverSelfAwardCareerCredits=true;
    bool NoWorldOrChaosMutation=true;
};
struct PhysicalChallengeSample final {
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t Actor=0u;
    std::uint64_t Frame=0u;
    double ActiveClockSeconds=0.0;
    double FrameSeconds=0.0;
    double WorldXcm=0.0;
    double WorldYcm=0.0;
    double WorldZcm=0.0;
    double VelocityXcmps=0.0;
    double VelocityYcmps=0.0;
    double VelocityZcmps=0.0;
    double RouteStationCm=0.0;
    double NativeTargetSpeedCmps=0.0;
    double ActualWaterMm=0.0;
    double AssistInput=0.0;
    double OfficialLastLapSeconds=0.0;
    unsigned OfficialCompletedLaps=0u;
    unsigned OfficialOrderedGateCount=0u;
    unsigned NativeCollisionCount=0u;
    unsigned NativeRecoveryCount=0u;
    bool NativeChaosChassisObserved=false;
    bool NativeAsphaltContactObserved=false;
    bool NativeLapClockVerified=false;
    bool ActualGateOrderVerified=false;
    bool NativeWetFilmObserved=false;
    bool NoOfficialRaceStateModified=true;
};
struct PhysicalChallengeReceipt final {
    bool Accepted=false;
    PhysicalChallengeFault Fault=PhysicalChallengeFault::MissingEngineAuthority;
    PhysicalChallengeState State=PhysicalChallengeState::Unconfigured;
    std::uint64_t SourceSession=0u;
    std::uint64_t LastSourceFrame=0u;
    double SourceClockSeconds=0.0;
    unsigned VerifiedFrames=0u;
    unsigned VerifiedLaps=0u;
    unsigned MeasuredSpeedTrapHits=0u;
    unsigned VerifiedWetFrames=0u;
    double BestCertifiedLapSeconds=0.0;
    double WorstCertifiedLapSeconds=0.0;
    double MaximumObservedOverspeedCmps=0.0;
    unsigned PhysicalCollisions=0u;
    unsigned Recoveries=0u;
    bool LocalObjectiveQualified=false;
    bool OfficialRaceAwardIssued=false;
    bool NativeGameplayUnchanged=true;
};
inline bool ValidPhysicalChallengeDefinition(
    PhysicalChallengeDefinition c) noexcept
{
    switch(c.Kind){
    case PhysicalChallengeKind::LapConsistency:
    case PhysicalChallengeKind::CleanStint:
    case PhysicalChallengeKind::SpeedTrap:
    case PhysicalChallengeKind::CornerDiscipline:
    case PhysicalChallengeKind::Endurance:
    case PhysicalChallengeKind::MeasuredWetHandling:break;
    default:return false;
    }
    return c.RequiredLaps>=1u &&
        c.RequiredLaps<=PhysicalChallengeMaximumLaps &&
        std::isfinite(c.MaximumElapsedSeconds) &&
        c.MaximumElapsedSeconds>=60.0 &&
        c.MaximumElapsedSeconds<=7200.0 &&
        std::isfinite(c.RequiredTrapSpeedCmps) &&
        c.RequiredTrapSpeedCmps>=600.0 &&
        c.RequiredTrapSpeedCmps<=12000.0 &&
        std::isfinite(c.MaximumBestWorstGapSeconds) &&
        c.MaximumBestWorstGapSeconds>=0.0 &&
        c.MaximumBestWorstGapSeconds<=120.0 &&
        std::isfinite(c.MaximumCornerOverspeedCmps) &&
        c.MaximumCornerOverspeedCmps>=0.0 &&
        c.MaximumCornerOverspeedCmps<=1500.0 &&
        std::isfinite(c.MinimumMeasuredWaterMm) &&
        c.MinimumMeasuredWaterMm>=0.0 &&
        c.MinimumMeasuredWaterMm<=10.0 &&
        std::isfinite(c.MaximumAssistInput) &&
        c.MaximumAssistInput>=0.0 &&
        c.MaximumAssistInput<=1.0 &&
        c.AllowedCollisions<=10u &&
        c.AllowedRecoveries<=10u &&
        c.RequireActualEngineLapClock &&
        c.RequireOrderedOriginalGateWitness &&
        c.RequireRealChassisAndContacts &&
        c.NeverSelfAwardCareerCredits &&
        c.NoWorldOrChaosMutation;
}
inline bool ValidPhysicalChallengeSample(
    const WorldScaleVariantRecipe& road,
    const PhysicalChallengeSample& s,
    const PhysicalChallengeDefinition& rule) noexcept
{
    if(!ValidPhysicalChallengeDefinition(rule) ||
       !ValidWorldScaleVariantRecipe(road) ||
       road.Id!=rule.Circuit ||
       s.Circuit!=road.Id ||
       s.Session==0u || s.Revision==0u ||
       s.Actor==0u || s.Frame==0u ||
       !s.NativeChaosChassisObserved ||
       !s.NativeAsphaltContactObserved ||
       !s.NativeLapClockVerified ||
       !s.ActualGateOrderVerified ||
       !s.NoOfficialRaceStateModified ||
       !std::isfinite(s.ActiveClockSeconds) ||
       !std::isfinite(s.FrameSeconds) ||
       s.ActiveClockSeconds<=0.0 ||
       s.ActiveClockSeconds>86400.0 ||
       s.FrameSeconds<=0.0 ||
       s.FrameSeconds>0.10 ||
       !std::isfinite(s.WorldXcm) ||
       !std::isfinite(s.WorldYcm) ||
       !std::isfinite(s.WorldZcm) ||
       !std::isfinite(s.VelocityXcmps) ||
       !std::isfinite(s.VelocityYcmps) ||
       !std::isfinite(s.VelocityZcmps) ||
       !std::isfinite(s.RouteStationCm) ||
       !std::isfinite(s.NativeTargetSpeedCmps) ||
       !std::isfinite(s.ActualWaterMm) ||
       !std::isfinite(s.AssistInput) ||
       !std::isfinite(s.OfficialLastLapSeconds) ||
       std::abs(s.WorldXcm)>1000000.0 ||
       std::abs(s.WorldYcm)>1000000.0 ||
       std::abs(s.WorldZcm)>1000000.0 ||
       std::abs(s.RouteStationCm)>1000000.0 ||
       s.NativeTargetSpeedCmps<0.0 ||
       s.NativeTargetSpeedCmps>15000.0 ||
       s.ActualWaterMm<0.0 ||
       s.ActualWaterMm>20.0 ||
       s.AssistInput<0.0 ||
       s.AssistInput>1.0 ||
       s.OfficialCompletedLaps>PhysicalChallengeMaximumLaps ||
       s.OfficialOrderedGateCount>s.OfficialCompletedLaps*8u+8u ||
       s.NativeCollisionCount>100u ||
       s.NativeRecoveryCount>100u)
        return false;
    const double speed=std::hypot(
        std::hypot(s.VelocityXcmps,
                   s.VelocityYcmps),
        s.VelocityZcmps);
    return std::isfinite(speed) && speed<=15000.0;
}
inline PhysicalChallengeDefinition ChoosePhysicalCircuitChallenge(
    WorldScaleVariantId circuit,PhysicalChallengeKind kind) noexcept
{
    PhysicalChallengeDefinition out;
    out.Circuit=circuit;
    out.Kind=kind;
    switch(kind){
    case PhysicalChallengeKind::LapConsistency:
        out.RequiredLaps=3u;
        out.MaximumBestWorstGapSeconds=8.0;
        break;
    case PhysicalChallengeKind::CleanStint:
        out.RequiredLaps=2u;
        out.AllowedCollisions=0u;
        out.AllowedRecoveries=0u;
        break;
    case PhysicalChallengeKind::SpeedTrap:
        out.RequiredLaps=1u;
        out.RequiredTrapSpeedCmps=2500.0;
        break;
    case PhysicalChallengeKind::CornerDiscipline:
        out.RequiredLaps=2u;
        out.MaximumCornerOverspeedCmps=100.0;
        break;
    case PhysicalChallengeKind::Endurance:
        out.RequiredLaps=6u;
        out.MaximumElapsedSeconds=3600.0;
        break;
    case PhysicalChallengeKind::MeasuredWetHandling:
        out.RequiredLaps=2u;
        out.MinimumMeasuredWaterMm=0.3;
        break;
    default:
        out.RequireRealChassisAndContacts=false;
        return out;
    }
    return out;
}
class PhysicalCircuitChallengeRecorder final {
public:
    void Reset() noexcept {
        Rule={};
        State=PhysicalChallengeState::Unconfigured;
        Session=0u;
        Revision=0u;
        Actor=0u;
        HavePrevious=false;
        Previous={};
        Count=0u;
        QualifiedLaps=0u;
        BestLap=std::numeric_limits<double>::infinity();
        WorstLap=0.0;
        TrapHits=0u;
        WetFrames=0u;
        MaxOverspeed=0.0;
        MaximumCollisions=0u;
        MaximumRecoveries=0u;
        Rejection=PhysicalChallengeFault::None;
    }
    bool Arm(const WorldScaleVariantRecipe& road,
             PhysicalChallengeDefinition rule) noexcept
    {
        Reset();
        if(!ValidWorldScaleVariantRecipe(road) ||
           rule.Circuit!=road.Id ||
           !ValidPhysicalChallengeDefinition(rule)){
            Rejection=PhysicalChallengeFault::InvalidRules;
            return false;
        }
        Rule=rule;
        State=PhysicalChallengeState::Armed;
        return true;
    }
    PhysicalChallengeState CurrentState() const noexcept {
        return State;
    }
    PhysicalChallengeFault LastFault() const noexcept {
        return Rejection;
    }
    PhysicalChallengeReceipt Observe(
        const WorldScaleVariantRecipe& road,
        const PhysicalChallengeSample& s) noexcept
    {
        if(State!=PhysicalChallengeState::Armed &&
           State!=PhysicalChallengeState::Active &&
           State!=PhysicalChallengeState::SourceQualified){
            return Receipt(false,PhysicalChallengeFault::InvalidRules);
        }
        if(!ValidPhysicalChallengeSample(road,s,Rule))
            return Reject(PhysicalChallengeFault::MissingEngineAuthority);
        if(HavePrevious){
            if(s.Session!=Session || s.Revision!=Revision)
                return Reject(PhysicalChallengeFault::WrongSession);
            if(s.Actor!=Actor)
                return Reject(PhysicalChallengeFault::WrongActor);
            if(s.Frame<=Previous.Frame ||
               s.Frame-Previous.Frame>8u)
                return Reject(PhysicalChallengeFault::StaleFrame);
            const double dt=s.ActiveClockSeconds-
                Previous.ActiveClockSeconds;
            if(dt<=0.0 || dt>0.10 ||
               std::abs(dt-s.FrameSeconds)>0.02)
                return Reject(PhysicalChallengeFault::ClockRollback);
            const double dx=s.WorldXcm-Previous.WorldXcm;
            const double dy=s.WorldYcm-Previous.WorldYcm;
            const double dz=s.WorldZcm-Previous.WorldZcm;
            const double moved=std::hypot(std::hypot(dx,dy),dz);
            const double speedNow=std::hypot(
                std::hypot(s.VelocityXcmps,
                           s.VelocityYcmps),s.VelocityZcmps);
            const double speedBefore=std::hypot(
                std::hypot(Previous.VelocityXcmps,
                           Previous.VelocityYcmps),
                Previous.VelocityZcmps);
            const double maximum=dt*std::max(
                speedNow,speedBefore)+175.0;
            if(!std::isfinite(moved) || moved>maximum)
                return Reject(PhysicalChallengeFault::ImpossibleMotion);
            if(s.OfficialCompletedLaps<Previous.OfficialCompletedLaps ||
               s.OfficialCompletedLaps>
                   Previous.OfficialCompletedLaps+1u ||
               s.NativeCollisionCount<Previous.NativeCollisionCount ||
               s.NativeRecoveryCount<Previous.NativeRecoveryCount ||
               s.OfficialOrderedGateCount<
                   Previous.OfficialOrderedGateCount)
                return Reject(PhysicalChallengeFault::InvalidTelemetry);
            if(s.OfficialCompletedLaps>
               Previous.OfficialCompletedLaps){
                if(s.OfficialLastLapSeconds<=0.0 ||
                   s.OfficialLastLapSeconds>
                       Rule.MaximumElapsedSeconds)
                    return Reject(PhysicalChallengeFault::InvalidTelemetry);
                BestLap=std::min(BestLap,s.OfficialLastLapSeconds);
                WorstLap=std::max(WorstLap,s.OfficialLastLapSeconds);
                ++QualifiedLaps;
            }
        }else{
            Session=s.Session;
            Revision=s.Revision;
            Actor=s.Actor;
        }
        if(s.OfficialCompletedLaps>QualifiedLaps+1u)
            return Reject(PhysicalChallengeFault::InvalidTelemetry);
        if(!s.NoOfficialRaceStateModified)
            return Reject(PhysicalChallengeFault::MissingEngineAuthority);
        const double speed=std::hypot(
            std::hypot(s.VelocityXcmps,s.VelocityYcmps),
            s.VelocityZcmps);
        if(speed>=Rule.RequiredTrapSpeedCmps)
            ++TrapHits;
        if(s.NativeWetFilmObserved &&
           s.ActualWaterMm>=Rule.MinimumMeasuredWaterMm)
            ++WetFrames;
        const double overspeed=std::max(
            0.0,speed-s.NativeTargetSpeedCmps);
        MaxOverspeed=std::max(MaxOverspeed,overspeed);
        MaximumCollisions=std::max(
            MaximumCollisions,s.NativeCollisionCount);
        MaximumRecoveries=std::max(
            MaximumRecoveries,s.NativeRecoveryCount);
        Previous=s;
        HavePrevious=true;
        ++Count;
        State=PhysicalChallengeState::Active;
        if(QualifiedLaps>=Rule.RequiredLaps &&
           s.ActiveClockSeconds<=Rule.MaximumElapsedSeconds &&
           MaximumCollisions<=Rule.AllowedCollisions &&
           MaximumRecoveries<=Rule.AllowedRecoveries)
        {
            bool objective=false;
            switch(Rule.Kind){
            case PhysicalChallengeKind::LapConsistency:
                objective=std::isfinite(BestLap) &&
                    WorstLap-BestLap<=
                        Rule.MaximumBestWorstGapSeconds;
                break;
            case PhysicalChallengeKind::CleanStint:
            case PhysicalChallengeKind::Endurance:
                objective=true;
                break;
            case PhysicalChallengeKind::SpeedTrap:
                objective=TrapHits>0u;
                break;
            case PhysicalChallengeKind::CornerDiscipline:
                objective=MaxOverspeed<=
                    Rule.MaximumCornerOverspeedCmps;
                break;
            case PhysicalChallengeKind::MeasuredWetHandling:
                objective=WetFrames>0u;
                break;
            }
            if(objective)
                State=PhysicalChallengeState::SourceQualified;
        }
        return Receipt(true,PhysicalChallengeFault::None);
    }
private:
    PhysicalChallengeReceipt Receipt(
        bool accepted,PhysicalChallengeFault fault) const noexcept
    {
        PhysicalChallengeReceipt out;
        out.Accepted=accepted;
        out.Fault=fault;
        out.State=State;
        out.SourceSession=Session;
        out.LastSourceFrame=HavePrevious?Previous.Frame:0u;
        out.SourceClockSeconds=
            HavePrevious?Previous.ActiveClockSeconds:0.0;
        out.VerifiedFrames=Count;
        out.VerifiedLaps=QualifiedLaps;
        out.MeasuredSpeedTrapHits=TrapHits;
        out.VerifiedWetFrames=WetFrames;
        out.BestCertifiedLapSeconds=
            std::isfinite(BestLap)?BestLap:0.0;
        out.WorstCertifiedLapSeconds=WorstLap;
        out.MaximumObservedOverspeedCmps=MaxOverspeed;
        out.PhysicalCollisions=MaximumCollisions;
        out.Recoveries=MaximumRecoveries;
        out.LocalObjectiveQualified=
            State==PhysicalChallengeState::SourceQualified;
        return out;
    }
    PhysicalChallengeReceipt Reject(
        PhysicalChallengeFault fault) noexcept
    {
        State=PhysicalChallengeState::Rejected;
        Rejection=fault;
        return Receipt(false,fault);
    }
    PhysicalChallengeDefinition Rule{};
    PhysicalChallengeState State=PhysicalChallengeState::Unconfigured;
    PhysicalChallengeFault Rejection=PhysicalChallengeFault::None;
    PhysicalChallengeSample Previous{};
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t Actor=0u;
    unsigned Count=0u;
    unsigned QualifiedLaps=0u;
    unsigned TrapHits=0u;
    unsigned WetFrames=0u;
    unsigned MaximumCollisions=0u;
    unsigned MaximumRecoveries=0u;
    double BestLap=std::numeric_limits<double>::infinity();
    double WorstLap=0.0;
    double MaxOverspeed=0.0;
    bool HavePrevious=false;
};
} // namespace latebrake
