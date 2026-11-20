#include "Racecraft/PhysicalWetPitDecisionGate.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool ok,const char* what){
    if(!ok){std::cerr<<"WetPitDecisionGate FAIL: "<<what<<'\n';std::exit(EXIT_FAILURE);}
}
struct WitnessFixture final {
    latebrake::WetPitStrategyPlan Strategy{};
    latebrake::PitCorridorTrafficFrame Traffic{};
    latebrake::WetPitDecisionFrame Input{};
    WitnessFixture(){
        using namespace latebrake;
        Strategy.Valid=true;
        Strategy.Fault=WetPitStrategyFault::None;
        Strategy.Circuit=WorldScaleVariantId::OriginalLoop;
        Strategy.Session=9u;
        Strategy.Revision=2u;
        Strategy.LastSourceFrame=1u;
        Strategy.SourceClockSeconds=1.0;
        Strategy.RemainingLaps=8u;
        Strategy.Best.Valid=true;
        Strategy.Best.PlannedStopAfterLaps=1u;
        Strategy.FeasibleCandidateCount=3u;
        Strategy.Advisory=WetPitAdvisory::StopSoonForWear;
        Traffic.Valid=true;
        Traffic.Fault=PitCorridorFault::None;
        Traffic.Circuit=WorldScaleVariantId::OriginalLoop;
        Traffic.Session=9u;
        Traffic.Revision=2u;
        Traffic.SourceFrame=1u;
        Traffic.SourceClockSeconds=1.0;
        Traffic.DriverActor=700u;
        Traffic.QualifiedCars=6u;
        Traffic.Traffic=PitCorridorTraffic::Vacant;
        for(unsigned i=0u;i<PitCorridorCarCount;++i){
            auto& c=Traffic.Cars[i];
            c.Valid=true;
            c.Actor=700u+i;
            c.ActualPitEntryNotGranted=true;
            c.CouldEnterService=i==0u;
        }
        Input.Strategy=&Strategy;
        Input.Traffic=&Traffic;
        Input.SourceSession=9u;
        Input.SourceRevision=2u;
        Input.PlayerActor=700u;
        Input.SourceFrame=1u;
        Input.EngineClockSeconds=1.0;
        Input.DeltaSeconds=0.05;
        Input.OfficialRemainingLaps=8u;
        Input.RealPlayerChaosBodyObserved=true;
        Input.NativePitServiceAuthorizationAvailable=true;
        Input.OriginalPitHoldIsNotBypassed=true;
        Input.OriginalOfficialClockVerified=true;
    }
    void Advance(){
        ++Input.SourceFrame;
        Input.EngineClockSeconds+=0.05;
        Strategy.LastSourceFrame=Input.SourceFrame;
        Strategy.SourceClockSeconds=Input.EngineClockSeconds;
        Traffic.SourceFrame=Input.SourceFrame;
        Traffic.SourceClockSeconds=Input.EngineClockSeconds;
    }
};
}
int main(){
    using namespace latebrake;
    auto settings=WetPitDecisionSettings{};
    Check(ValidWetPitDecisionSettings(settings),
        "conservative source-only manual pit decision settings valid");
    settings.NeverIssuePitRequest=false;
    Check(!ValidWetPitDecisionSettings(settings),
        "native service ownership must remain in Unreal pit policy");
    settings={};
    settings.MaximumAcceptedFrameSeconds=
        std::numeric_limits<double>::quiet_NaN();
    Check(!ValidWetPitDecisionSettings(settings),
        "invalid native sample cadence rejected");
    settings={};
    WitnessFixture fixture;
    Check(ValidWetPitDecisionFrame(fixture.Input),
        "six independently identified native driver projections accepted");
    Check(ClassifyWetPitDecision(fixture.Input)==
          WetPitDecision::RequestNativePitSafetyCheck,
          "source-only advisory requests a separate manual real pit safety check");
    PhysicalWetPitDecisionGate gate;
    auto receipt=gate.Observe(fixture.Input);
    Check(receipt.Valid &&
          receipt.Proposed==WetPitDecision::RequestNativePitSafetyCheck &&
          receipt.Confirmed==WetPitDecision::Unqualified &&
          receipt.StablePhysicalFrames==1u &&
          receipt.NoActualServiceIssued &&
          receipt.OriginalPitHoldRequired,
          "one raw sensor frame cannot authorize even advisory confirmation");
    fixture.Advance();
    receipt=gate.Observe(fixture.Input);
    Check(receipt.Valid &&
          receipt.Confirmed==WetPitDecision::RequestNativePitSafetyCheck &&
          receipt.StablePhysicalFrames==2u &&
          receipt.NoRaceOrTyreChange,
          "two continuous real-source frames stabilize advisory only");
    fixture.Advance();
    fixture.Traffic.Traffic=PitCorridorTraffic::Blocked;
    receipt=gate.Observe(fixture.Input);
    Check(receipt.Valid &&
          receipt.Proposed==WetPitDecision::WaitForTraffic &&
          receipt.Confirmed==WetPitDecision::Unqualified &&
          gate.StableFrameCount()==1u,
          "traffic transition invalidates stale pit readiness");
    fixture.Advance();
    receipt=gate.Observe(fixture.Input);
    Check(receipt.Valid &&
          receipt.Confirmed==WetPitDecision::WaitForTraffic,
          "two authentic blocked observations confirm service wait");
    fixture.Advance();
    fixture.Traffic.Traffic=PitCorridorTraffic::Vacant;
    fixture.Input.NativePitServiceAuthorizationAvailable=false;
    receipt=gate.Observe(fixture.Input);
    Check(receipt.Valid &&
          receipt.Proposed==WetPitDecision::ApproachWithCaution &&
          receipt.Confirmed==WetPitDecision::Unqualified,
          "no native pit service authorization means approach only");
    fixture.Advance();
    fixture.Strategy.Best.PlannedStopAfterLaps=4u;
    receipt=gate.Observe(fixture.Input);
    Check(receipt.Valid &&
          receipt.Proposed==WetPitDecision::PlanFutureStop,
          "scheduled future stop cannot become immediate service");
    fixture.Advance();
    fixture.Strategy.Advisory=WetPitAdvisory::RemainOnCurrentTyres;
    receipt=gate.Observe(fixture.Input);
    Check(receipt.Valid &&
          receipt.Proposed==WetPitDecision::StayOnRoad,
          "measured safe no-stop always overrides speculative pit");
    fixture.Advance();
    fixture.Input.OriginalPitHoldIsNotBypassed=false;
    receipt=gate.Observe(fixture.Input);
    Check(!receipt.Valid &&
          receipt.Fault==WetPitDecisionFault::BadSources &&
          gate.StableFrameCount()==0u,
          "bypassed native pit hold invalidates accumulated trust");
    fixture.Input.OriginalPitHoldIsNotBypassed=true;
    fixture.Advance();
    receipt=gate.Observe(fixture.Input);
    Check(receipt.Valid && gate.StableFrameCount()==1u,
          "fresh observations can begin a new epoch after failure");
    fixture.Advance();
    fixture.Input.SourceRevision=3u;
    fixture.Strategy.Revision=3u;
    fixture.Traffic.Revision=3u;
    receipt=gate.Observe(fixture.Input);
    Check(!receipt.Valid &&
          receipt.Fault==WetPitDecisionFault::WrongSession,
          "session restart invalidates all prior decision votes");
    fixture.Input.SourceRevision=2u;
    fixture.Strategy.Revision=2u;
    fixture.Traffic.Revision=2u;
    fixture.Advance();
    fixture.Traffic.Cars[2].Actor=fixture.Traffic.Cars[0].Actor;
    Check(!gate.Observe(fixture.Input).Valid,
          "duplicate player identity cannot claim six physical source cars");
    fixture.Traffic.Cars[2].Actor=702u;
    fixture.Advance();
    fixture.Strategy.NoOfficialTimingAuthority=false;
    Check(!gate.Observe(fixture.Input).Valid,
          "a hidden race-control mutation invalidates whole proposal");
    fixture.Strategy.NoOfficialTimingAuthority=true;
    gate.Reset();
    Check(gate.StableFrameCount()==0u,
          "explicit race restart clears source-only decision latch");
    std::cout<<"PhysicalWetPitDecisionGate: physical-source QA passed\n";
    return EXIT_SUCCESS;
}
