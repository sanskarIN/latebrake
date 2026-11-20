#include "Racing/ObservedTyreWaterContactRisk.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool yes,const char* why){
    if(!yes){std::cerr<<"TyreWaterRisk FAIL: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
latebrake::PhysicalReplayFrame Fleet(){
    using namespace latebrake;
    PhysicalReplayFrame f;
    f.Session=51u;
    f.SessionRevision=2u;
    f.SourceFrame=81u;
    f.ActiveClockSeconds=10.0;
    f.SourceDeltaSeconds=0.05;
    f.AllSixEngineActorsObserved=true;
    for(unsigned i=0u;i<WaterRiskCarCount;++i){
        auto& car=f.Cars[i];
        car.Actor=800u+i;
        car.Session=51u;
        car.SessionRevision=2u;
        car.SourceFrame=81u;
        car.WorldXcm=1000.0+2000.0*i;
        car.WorldYcm=350.0*i;
        car.WorldZcm=80.0;
        car.VelocityXcmps=4200.0;
        car.HeadingRadians=0.0;
        car.ThrottleNormalized=0.4;
        car.RoadContactConfidence=1.0;
        car.ActualChaosBodyObserved=true;
        car.OriginalRoadPhysicalContact=true;
        car.EngineClockRunning=true;
    }
    return f;
}
latebrake::ObservedWaterResult Water(
    const latebrake::WorldScaleVariantRecipe& circuit,
    double depth){
    using namespace latebrake;
    ObservedWaterResult w;
    w.Valid=true;
    w.Circuit=circuit.Id;
    w.Session=51u;
    w.Revision=2u;
    w.SourceFrame=81u;
    w.ClockSeconds=10.0;
    w.CellLengthCm=circuit.LapLengthCm/
        static_cast<double>(ObservedWaterCellCount);
    w.FilmMm.fill(depth);
    w.RenderAdvisoryOnly=true;
    w.OriginalRaceUnaffected=true;
    return w;
}
std::array<latebrake::WaterRiskWheelObservation,
    latebrake::WaterRiskSampleCount> Wheels(){
    using namespace latebrake;
    std::array<WaterRiskWheelObservation,WaterRiskSampleCount> result{};
    for(unsigned i=0u;i<WaterRiskSampleCount;++i){
        auto& w=result[i];
        w.OwnerActor=800u+i/WaterRiskWheelCount;
        w.NativeWheelComponent=i+1u;
        w.SourceFrame=81u;
        w.PressureKpa=210.0;
        w.TreadDepthMm=6.0;
        w.MeasuredLoadN=3500.0;
        w.ContactStationCm=2000.0+1000.0*i;
        w.SourceClockSeconds=10.0;
        w.Confidence=1.0;
        w.OriginalChaosWheelObserved=true;
        w.NativeRoadContactObserved=true;
        w.ActualPressureSensorObserved=true;
        w.ActualTreadReadingObserved=true;
    }
    return result;
}
}
int main(){
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto settings=WaterRiskSettings{};
    Check(ValidWaterRiskSettings(settings),
          "strict four-wheel physical source evidence defaults");
    settings.NoActualTyreForceAuthority=false;
    Check(!ValidWaterRiskSettings(settings),
          "risk hint cannot become a native tire-force command");
    settings={};
    settings.MinimumObservationConfidence=nan;
    Check(!ValidWaterRiskSettings(settings),
          "nonfinite sensor confidence denied");
    settings={};
    unsigned qualified=0u;
    for(unsigned index=0u;index<WorldScaleVariantCount;++index){
        const auto circuit=WorldScaleOriginalVariantRecipes[index];
        const auto fleet=Fleet();
        const auto probes=Wheels();
        const auto dry=Water(circuit,0.0);
        const auto wet=Water(circuit,7.0);
        const auto dryResult=BuildObservedWaterRiskFleet(
            circuit,fleet,dry,probes);
        const auto wetResult=BuildObservedWaterRiskFleet(
            circuit,fleet,wet,probes);
        Check(dryResult.Valid && wetResult.Valid &&
              dryResult.Circuit==circuit.Id &&
              wetResult.Session==51u &&
              wetResult.VerifiedWheelSources==WaterRiskSampleCount &&
              wetResult.MaximumFleetRisk>=dryResult.MaximumFleetRisk &&
              wetResult.MeanFleetRisk>=dryResult.MeanFleetRisk &&
              wetResult.NoRealPhysicsForceChanged &&
              wetResult.NoRaceAuthorityChanged,
              "24 actual wheel probes form source-only wet risk estimate");
        Check(dryResult.MaximumFleetRisk==0.0 &&
              dryResult.MeanFleetRisk==0.0 &&
              wetResult.MaximumFleetRisk>0.0,
              "dry tires have zero water risk, deep water raises advisory");
        for(unsigned i=0u;i<WaterRiskCarCount;++i){
            const auto& car=wetResult.Cars[i];
            Check(car.Valid && car.Actor==800u+i &&
                  car.Wheels.size()==4u &&
                  car.MeanWheelRisk>=0.0 &&
                  car.MaximumWheelRisk>=car.MeanWheelRisk &&
                  car.NoNativeDriveInputChanged,
                  "every real car retains its four original wheel identities");
            for(unsigned j=0u;j<WaterRiskWheelCount;++j){
                const auto& w=car.Wheels[j];
                Check(w.Valid && w.OwnerActor==car.Actor &&
                      w.WheelIndex==j &&
                      w.WaterFilmMm==7.0 &&
                      w.RiskFraction>=0.0 &&
                      w.RiskFraction<=1.0 &&
                      w.NominalHydroThresholdCmps>
                          w.WetSurfaceThresholdCmps &&
                      w.ActualWheelForceUnchanged &&
                      w.SourceOnly,
                      "measured pressure and deep water constrain advisory only");
            }
        }
        const auto baseline=EstimateWaterWheelRisk(
            circuit,fleet.Cars[0],wet,probes[0],0u);
        auto sensor=probes[0];
        sensor.PressureKpa=300.0;
        const auto higherPressure=EstimateWaterWheelRisk(
            circuit,fleet.Cars[0],wet,sensor,0u);
        Check(baseline.Valid && higherPressure.Valid &&
              higherPressure.NominalHydroThresholdCmps>
                  baseline.NominalHydroThresholdCmps &&
              higherPressure.RiskFraction<=baseline.RiskFraction,
              "observed higher inflation raises hydroplaning threshold");
        sensor=probes[0];
        sensor.TreadDepthMm=0.5;
        const auto worn=EstimateWaterWheelRisk(
            circuit,fleet.Cars[0],wet,sensor,0u);
        Check(worn.Valid &&
              worn.NominalHydroThresholdCmps<
                  baseline.NominalHydroThresholdCmps &&
              worn.RiskFraction>=baseline.RiskFraction,
              "worn measured tread worsens wet road risk");
        sensor=probes[0];
        sensor.ActualPressureSensorObserved=false;
        Check(!EstimateWaterWheelRisk(
            circuit,fleet.Cars[0],wet,sensor,0u).Valid,
            "unverified pressure probe cannot invent native hydro threshold");
        sensor=probes[0];
        sensor.PressureKpa=nan;
        Check(!EstimateWaterWheelRisk(
            circuit,fleet.Cars[0],wet,sensor,0u).Valid,
            "nonfinite pressure fails closed");
        auto forged=probes;
        forged[5].NativeWheelComponent=forged[0].NativeWheelComponent;
        Check(!BuildObservedWaterRiskFleet(
            circuit,fleet,wet,forged).Valid,
            "duplicate native wheel identity rejected across two cars");
        forged=probes;
        forged[15].NativeRoadContactObserved=false;
        Check(!BuildObservedWaterRiskFleet(
            circuit,fleet,wet,forged).Valid,
            "missing physical tire contact denies entire fleet evidence");
        auto changed=wet;
        changed.Revision=3u;
        Check(!BuildObservedWaterRiskFleet(
            circuit,fleet,changed,probes).Valid,
            "different race session cannot borrow old tire contacts");
        ++qualified;
    }
    Check(qualified==WorldScaleVariantCount,
          "all six independent track layouts tested with true source probes");
    std::cout<<"ObservedTyreWaterContactRisk: 24 wheels / six tracks passed\n";
    return EXIT_SUCCESS;
}
