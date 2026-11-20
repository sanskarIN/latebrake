#include "Racecraft/PhysicalWetPitStrategy.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool ok,const char* label){
    if(!ok){std::cerr<<"PhysicalWetPitStrategy FAIL: "<<label<<'\n';std::exit(EXIT_FAILURE);}
}
latebrake::WetPitMeasuredRace Race(
    latebrake::WorldScaleVariantId track,unsigned laps=8u)
{
    using namespace latebrake;
    WetPitMeasuredRace race;
    race.Circuit=track;
    race.Session=21u;
    race.SessionRevision=3u;
    race.ChassisActor=1001u;
    race.LastNativeFrame=500u;
    race.EngineClockSeconds=80.0;
    race.RemainingOfficialLaps=laps;
    race.CurrentCompound=WetPitCompound::Dry;
    race.ActualCurrentTreadMm=6.0;
    race.ActualFuelLitres=45.0;
    race.MeasuredFuelLitresPerLap=2.3;
    race.Confidence=0.99;
    race.MeasuredPitTimeLossSeconds=23.0;
    race.MeasuredPitLossConfidence=0.98;
    race.RealChaosCarMeasured=true;
    race.RealPitLaneTimeLossMeasured=true;
    race.OriginalOfficialLapClockVerified=true;
    race.OriginalRaceAndGarageUnmodified=true;
    for(unsigned i=0u;i<WetPitCompoundCount;++i){
        auto& c=race.Compounds[i];
        c.Compound=static_cast<WetPitCompound>(i);
        c.AvailableInNativeGarage=true;
        c.ActualCarAndRoadLapTiming=true;
        c.InstalledTreadMm=8.0;
        c.VerifiedWearMmPerLap=(i==0u)?0.6:0.30;
        c.Confidence=0.95;
        c.QualifiedSamples=5u;
        for(unsigned j=0u;j<c.QualifiedSamples;++j)
            c.LapSeconds[j]=
                (i==0u?115.0:(i==1u?112.0:118.0))+
                0.2*static_cast<double>(j);
    }
    return race;
}
latebrake::WetPitScenario Scenario(
    unsigned laps,double measuredFutureAssumption)
{
    using namespace latebrake;
    WetPitScenario out;
    out.Valid=true;
    out.ScenarioId=321u;
    out.RemainingLaps=laps;
    out.ExternalScenarioAcknowledged=true;
    out.NotAnEngineWeatherForecast=true;
    for(unsigned i=0u;i<laps;++i)
        out.AssumedFilmMm[i]=measuredFutureAssumption;
    return out;
}
}
int main(){
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    WetPitSettings settings;
    Check(ValidWetPitSettings(settings),
        "default source-only pit timing rules are bounded");
    settings.NeverChangeNativeTyres=false;
    Check(!ValidWetPitSettings(settings),
        "portable advisory cannot mutate live tire compound");
    settings={};
    settings.MaximumLapTimeVariationFraction=nan;
    Check(!ValidWetPitSettings(settings),
        "nonfinite timing uncertainty rejected");
    settings={};
    Check(WetPitCompoundIndex(WetPitCompound::Dry)==0u &&
          WetPitCompoundIndex(WetPitCompound::Intermediate)==1u &&
          WetPitCompoundIndex(WetPitCompound::FullWet)==2u &&
          WetPitCompoundIndex(static_cast<WetPitCompound>(250))==
              WetPitCompoundCount,
          "compound choices preserve explicit stable original identity");
    Check(WetPitCompoundScenarioPenalty(
              WetPitCompound::Dry,0.8)>
          WetPitCompoundScenarioPenalty(
              WetPitCompound::Intermediate,0.8),
          "stated wet scenario allows measured intermediate comparison");
    Check(WetPitCompoundScenarioPenalty(
              WetPitCompound::FullWet,0.0)>
          WetPitCompoundScenarioPenalty(
              WetPitCompound::FullWet,1.0),
          "full wet rubber not assumed fastest on unmeasured dry surface");
    unsigned routes=0u;
    for(unsigned i=0u;i<WorldScaleVariantCount;++i){
        const auto circuit=WorldScaleOriginalVariantRecipes[i];
        auto race=Race(circuit.Id);
        auto scenario=Scenario(8u,14.0);
        Check(ValidWetPitMeasuredRace(circuit,race,settings),
              "all actual chassis, pit, lap and tire measurements present");
        Check(ValidWetPitScenario(scenario,race,settings),
              "caller acknowledges wet future scenario is not a forecast");
        const auto sample=SummarizeWetPitLapEvidence(
            race.Compounds[1],settings);
        Check(sample.Valid &&
              sample.WorstSeconds>=sample.MeanSeconds &&
              sample.ConservativeSeconds>=sample.MeanSeconds &&
              sample.SampleSpreadFraction<0.05,
              "conservative pit timing derives from true certified laps");
        const auto plan=PlanPhysicalWetPitStrategy(
            circuit,race,scenario,settings);
        Check(plan.Valid &&
              plan.Fault==WetPitStrategyFault::None &&
              plan.Circuit==circuit.Id &&
              plan.Session==21u &&
              plan.Revision==3u &&
              plan.LastSourceFrame==500u &&
              plan.RemainingLaps==8u &&
              plan.CandidateCount==(8u-1u)*WetPitCompoundCount &&
              plan.FeasibleCandidateCount>0u &&
              plan.Best.Valid &&
              plan.Best.PlannedStopAfterLaps>=1u &&
              plan.Best.PlannedStopAfterLaps<8u &&
              plan.BestCandidateSeconds>0.0 &&
              plan.Best.TyreMarginMm>=0.0 &&
              plan.Best.FuelMarginLitres>=0.0 &&
              plan.Advisory==WetPitAdvisory::StopSoonForWear &&
              plan.NoOfficialTimingAuthority &&
              plan.NoOriginalPhysicsChanges &&
              plan.SourceOnly,
              "wear requires stop but source-only strategy never submits one");
        double fastest=std::numeric_limits<double>::infinity();
        unsigned goodCandidates=0u;
        for(unsigned j=0u;j<plan.CandidateCount;++j){
            const auto& c=plan.Candidates[j];
            if(!c.Valid)continue;
            ++goodCandidates;
            Check(c.TyreMarginMm>=0.0 &&
                  c.FuelMarginLitres>=0.0 &&
                  c.PredictedTotalSeconds>0.0 &&
                  c.MeanCurrentStintLapSeconds>0.0 &&
                  c.MeanSecondStintLapSeconds>0.0 &&
                  c.RequiresUnverifiedWeatherFuture &&
                  c.NoActualPitIssued &&
                  c.NoGarageStateChanged,
                  "all feasible alternatives explicitly remain hypothetical");
            fastest=std::min(fastest,c.PredictedTotalSeconds);
        }
        Check(goodCandidates==plan.FeasibleCandidateCount &&
              std::abs(fastest-plan.BestCandidateSeconds)<1e-7,
              "global strategy selects actual shortest qualified scenario");
        const auto again=PlanPhysicalWetPitStrategy(
            circuit,race,scenario);
        Check(again.Valid &&
              again.Best.PlannedStopAfterLaps==
                  plan.Best.PlannedStopAfterLaps &&
              again.Best.SelectedCompound==
                  plan.Best.SelectedCompound &&
              again.BestCandidateSeconds==plan.BestCandidateSeconds,
              "repeated source inputs cannot introduce random pit timing");
        race=Race(circuit.Id,3u);
        race.ActualCurrentTreadMm=10.0;
        race.Compounds[0].VerifiedWearMmPerLap=0.1;
        scenario=Scenario(3u,0.0);
        const auto noStop=PlanPhysicalWetPitStrategy(
            circuit,race,scenario);
        Check(noStop.Valid &&
              noStop.Advisory==WetPitAdvisory::RemainOnCurrentTyres &&
              std::isfinite(noStop.ConservativeNoStopSeconds),
              "short safe stint should not invent unnecessary stop");
        race=Race(circuit.Id);
        scenario=Scenario(8u,14.0);
        race.ActualFuelLitres=5.0;
        const auto impossible=PlanPhysicalWetPitStrategy(
            circuit,race,scenario);
        Check(!impossible.Valid &&
              impossible.Fault==WetPitStrategyFault::NoFeasibleScenario,
              "no refuel authority and insufficient fuel fail safely");
        race=Race(circuit.Id);
        race.RealPitLaneTimeLossMeasured=false;
        Check(!PlanPhysicalWetPitStrategy(
            circuit,race,scenario).Valid,
              "unmeasured pit loss cannot qualify strategy");
        race=Race(circuit.Id);
        race.Compounds[1].LapSeconds[0]=nan;
        Check(!PlanPhysicalWetPitStrategy(
            circuit,race,scenario).Valid,
              "nonfinite certified lap sample denied");
        race=Race(circuit.Id);
        race.Compounds[0].QualifiedSamples=2u;
        Check(!PlanPhysicalWetPitStrategy(
            circuit,race,scenario).Valid,
              "fewer than three real laps cannot establish confident pace");
        race=Race(circuit.Id);
        race.Compounds[2].VerifiedWearMmPerLap=10.0;
        Check(!PlanPhysicalWetPitStrategy(
            circuit,race,scenario).Valid,
              "imaginary negative tread wear not trusted");
        race=Race(circuit.Id);
        scenario.ExternalScenarioAcknowledged=false;
        Check(!PlanPhysicalWetPitStrategy(
            circuit,race,scenario).Valid,
              "a supposed magic forecast cannot become real-world evidence");
        ++routes;
    }
    Check(routes==WorldScaleVariantCount,
        "six circuit strategies received independent true-source audits");
    std::cout<<"PhysicalWetPitStrategy: six original circuits passed\n";
    return EXIT_SUCCESS;
}
