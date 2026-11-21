#include "Career/PhysicalCircuitChallengeEvidence.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Require(bool yes,const char* what){
    if(!yes){std::cerr<<"PhysicalCircuitChallenge FAIL: "<<what<<'\n';std::exit(EXIT_FAILURE);}
}
latebrake::PhysicalChallengeSample Sample(
    const latebrake::WorldScaleVariantRecipe& circuit,
    unsigned frame,double speedCmps=8000.0)
{
    using namespace latebrake;
    const double seconds=static_cast<double>(frame)*0.05;
    const double lap=circuit.LapLengthCm/speedCmps;
    const auto pose=WorldScaleVariantCenterlineAt(
        circuit,seconds*speedCmps);
    PhysicalChallengeSample sample;
    sample.Circuit=circuit.Id;
    sample.Session=1234u;
    sample.Revision=9u;
    sample.Actor=1000u;
    sample.Frame=frame;
    sample.ActiveClockSeconds=seconds;
    sample.FrameSeconds=0.05;
    sample.WorldXcm=pose.Pose.Position.X;
    sample.WorldYcm=pose.Pose.Position.Y;
    sample.WorldZcm=80.0;
    const double angle=pose.Pose.HeadingDegrees*
        RoundedPi/180.0;
    sample.VelocityXcmps=std::cos(angle)*speedCmps;
    sample.VelocityYcmps=std::sin(angle)*speedCmps;
    sample.RouteStationCm=std::fmod(
        seconds*speedCmps,circuit.LapLengthCm);
    sample.NativeTargetSpeedCmps=9000.0;
    sample.ActualWaterMm=1.0;
    sample.AssistInput=0.0;
    sample.OfficialCompletedLaps=
        static_cast<unsigned>(seconds/lap);
    sample.OfficialOrderedGateCount=
        4u*sample.OfficialCompletedLaps;
    sample.OfficialLastLapSeconds=
        sample.OfficialCompletedLaps>0u?lap:0.0;
    sample.NativeCollisionCount=0u;
    sample.NativeRecoveryCount=0u;
    sample.NativeChaosChassisObserved=true;
    sample.NativeAsphaltContactObserved=true;
    sample.NativeLapClockVerified=true;
    sample.ActualGateOrderVerified=true;
    sample.NativeWetFilmObserved=true;
    sample.NoOfficialRaceStateModified=true;
    return sample;
}
}
int main() {
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto options=ChoosePhysicalCircuitChallenge(
        WorldScaleVariantId::OriginalLoop,
        PhysicalChallengeKind::CleanStint);
    Require(ValidPhysicalChallengeDefinition(options),
            "original two-lap clean challenge source contract valid");
    options.NeverSelfAwardCareerCredits=false;
    Require(!ValidPhysicalChallengeDefinition(options),
            "source proof cannot mint career credits");
    options=ChoosePhysicalCircuitChallenge(
        WorldScaleVariantId::OriginalLoop,
        PhysicalChallengeKind::SpeedTrap);
    Require(options.RequiredLaps==1u &&
            options.RequiredTrapSpeedCmps>1000.0,
            "speed trap has real physical measured threshold");
    options=ChoosePhysicalCircuitChallenge(
        WorldScaleVariantId::OriginalLoop,
        PhysicalChallengeKind::Endurance);
    Require(options.RequiredLaps==6u &&
            options.MaximumElapsedSeconds>=3600.0,
            "future endurance objective has explicit lap and time budget");
    unsigned approvedRoutes=0u;
    for(unsigned i=0u;i<WorldScaleVariantCount;++i){
        const auto circuit=WorldScaleOriginalVariantRecipes[i];
        auto objective=ChoosePhysicalCircuitChallenge(
            circuit.Id,PhysicalChallengeKind::CleanStint);
        Require(objective.Circuit==circuit.Id &&
                ValidPhysicalChallengeDefinition(objective),
                "every source circuit defines an authentic clean-stint goal");
        PhysicalCircuitChallengeRecorder tracker;
        Require(tracker.Arm(circuit,objective) &&
                tracker.CurrentState()==PhysicalChallengeState::Armed,
                "challenge can only arm with original source circuit");
        const double secondsPerLap=circuit.LapLengthCm/8000.0;
        const unsigned frames=static_cast<unsigned>(
            std::ceil(2.0*secondsPerLap/0.05))+3u;
        PhysicalChallengeReceipt last;
        for(unsigned step=1u;step<=frames;++step){
            const auto snapshot=Sample(circuit,step);
            Require(ValidPhysicalChallengeSample(
                circuit,snapshot,objective),
                "real source-field witness fixture passes input guards");
            last=tracker.Observe(circuit,snapshot);
            Require(last.Accepted &&
                  last.Fault==PhysicalChallengeFault::None &&
                  last.VerifiedFrames==step &&
                  !last.OfficialRaceAwardIssued &&
                  last.NativeGameplayUnchanged,
                  "genuine witness cadence only adds bounded evidence");
        }
        Require(last.LocalObjectiveQualified &&
                last.State==PhysicalChallengeState::SourceQualified &&
                last.VerifiedLaps>=2u &&
                last.PhysicalCollisions==0u &&
                last.Recoveries==0u &&
                last.BestCertifiedLapSeconds>1.0 &&
                last.WorstCertifiedLapSeconds>=
                    last.BestCertifiedLapSeconds,
                "two authoritative clean laps yield local proof, not medal");
        tracker.Reset();
        Require(tracker.CurrentState()==
                  PhysicalChallengeState::Unconfigured,
                "session reset removes previous racing evidence");
        Require(tracker.Arm(circuit,objective),
                "new challenge requires explicit re-arm");
        auto first=Sample(circuit,1u);
        Require(tracker.Observe(circuit,first).Accepted,
                "first frame opens a unique verified session");
        auto jumped=Sample(circuit,2u);
        jumped.WorldXcm+=10000.0;
        last=tracker.Observe(circuit,jumped);
        Require(!last.Accepted &&
                last.Fault==PhysicalChallengeFault::ImpossibleMotion &&
                tracker.CurrentState()==PhysicalChallengeState::Rejected,
                "teleport cannot complete an evidence mission");
        Require(!tracker.Observe(circuit,Sample(circuit,3u)).Accepted,
                "rejected proof cannot silently resume a career mission");
        Require(tracker.Arm(circuit,objective),
                "re-arming clears invalid prior source evidence");
        auto missing=Sample(circuit,1u);
        missing.NativeAsphaltContactObserved=false;
        last=tracker.Observe(circuit,missing);
        Require(!last.Accepted &&
                last.Fault==PhysicalChallengeFault::MissingEngineAuthority,
                "no native road contact means no mission progression");
        Require(tracker.Arm(circuit,objective),
                "fresh explicit arm is required after lost contact");
        first=Sample(circuit,1u);
        Require(tracker.Observe(circuit,first).Accepted,
                "valid initial physical frame opens new source");
        missing=Sample(circuit,2u);
        missing.Revision=10u;
        last=tracker.Observe(circuit,missing);
        Require(!last.Accepted &&
                last.Fault==PhysicalChallengeFault::WrongSession,
                "session revision cannot inherit old challenge progress");
        Require(tracker.Arm(circuit,objective),
                "restarted race needs independent challenge ledger");
        missing=Sample(circuit,1u);
        missing.ActiveClockSeconds=nan;
        Require(!tracker.Observe(circuit,missing).Accepted,
                "nonfinite official race time is never a valid challenge");
        ++approvedRoutes;
    }
    Require(approvedRoutes==WorldScaleVariantCount,
            "six independent source circuits passed career challenge QA");
    std::cout<<"PhysicalCircuitChallengeEvidence: six tracks passed\n";
    return EXIT_SUCCESS;
}
