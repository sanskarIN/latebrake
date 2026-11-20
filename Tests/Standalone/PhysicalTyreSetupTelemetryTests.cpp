#include "Garage/PhysicalTyreSetupTelemetry.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool v,const char* why){
    if(!v){std::cerr<<"PhysicalTyreSetup FAIL: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
latebrake::SixCarTrackSurfaceEvidence Surface(
    latebrake::WorldScaleVariantId circuit){
    latebrake::SixCarTrackSurfaceEvidence s;
    s.Circuit=circuit;
    s.Session=19u;
    s.SourceFrame=44u;
    s.ClockSeconds=9.0;
    s.MeasuredGripFraction=0.88;
    s.SourceConfidence=1.0;
    s.AuthenticRoadMaterialObserved=true;
    s.ActualTireContactObserved=true;
    s.OriginalRaceRulesPreserved=true;
    return s;
}
std::array<latebrake::TyreSetupWheelWitness,4> Wheels(){
    std::array<latebrake::TyreSetupWheelWitness,4> w{};
    for(unsigned i=0;i<4u;++i){
        w[i].OwnerActor=800u;
        w[i].NativeWheel=1000u+i;
        w[i].SourceFrame=95u;
        w[i].ClockSeconds=10.0;
        w[i].HotPressureKpa=215.0;
        w[i].SurfaceTemperatureC=78.0;
        w[i].CarcassTemperatureC=80.0;
        w[i].TreadDepthMm=6.0;
        w[i].SensorConfidence=1.0;
        w[i].ActualChaosWheelObserved=true;
        w[i].ActualOriginalRoadContact=true;
        w[i].ActualPressureMeasurement=true;
        w[i].ActualTemperatureMeasurement=true;
        w[i].ActualTreadMeasurement=true;
    }
    return w;
}
}
int main() {
    using namespace latebrake;
    auto settings=TyreSetupSettings{};
    Check(ValidTyreSetupSettings(settings),
          "original calibrated wheel pressure policy valid");
    settings.NoNativeWheelModification=false;
    Check(!ValidTyreSetupSettings(settings),
          "physical tire pressure cannot be modified by advisor");
    settings={};
    settings.MaximumSourceClockSkewSeconds=
        std::numeric_limits<double>::quiet_NaN();
    Check(!ValidTyreSetupSettings(settings),
          "nonfinite source clock tolerance rejected");
    settings={};
    Check(ClassifyTyreSetupTemperature(20.0,settings)==
              TyreSetupBand::Cold &&
          ClassifyTyreSetupTemperature(80.0,settings)==
              TyreSetupBand::Nominal &&
          ClassifyTyreSetupTemperature(110.0,settings)==
              TyreSetupBand::Hot &&
          ClassifyTyreSetupTemperature(140.0,settings)==
              TyreSetupBand::Overheated,
          "cold/hot/overheated bands are explicit and finite");
    unsigned verified=0u;
    for(unsigned route=0u;route<WorldScaleVariantCount;++route) {
        const auto circuit=WorldScaleOriginalVariantRecipes[route];
        for(int car=0;car<VehicleCount;++car) {
            GarageSnapshot saved;
            saved.Credits=1000;
            saved.OwnedVehicleMask=KnownVehicleMask;
            saved.SelectedVehicle=car;
            saved.EngineLevel=2;
            saved.GripLevel=1;
            const auto config=BuildSixCarCircuitSetup(
                circuit,saved,Surface(circuit.Id));
            Check(config.Valid,"all 36 actual vehicle/road setup sources exist");
            const auto measured=Wheels();
            const auto review=BuildPhysicalTyreSetupPlan(
                config,800u,95u,10.0,measured);
            Check(review.Valid &&
                  review.OriginalVehicleId==car &&
                  review.Circuit==circuit.Id &&
                  review.DriverActor==800u &&
                  review.SourceSession==19u &&
                  review.SourceFrame==95u &&
                  review.ColdWheelCount==0u &&
                  review.HotWheelCount==0u &&
                  review.OverheatedWheelCount==0u &&
                  review.LowTreadWheelCount==0u &&
                  review.MeanMeasuredTemperatureC==80.0 &&
                  review.MinimumSuggestedPressureKpa>=
                      settings.MinimumRecommendedPressureKpa &&
                  review.MaximumSuggestedPressureKpa<=
                      settings.MaximumRecommendedPressureKpa &&
                  review.SourceOnly &&
                  review.NoRealPressureChanges &&
                  review.NoGarageProgressionChanges,
                  "four actual tire sensor samples create safe source-only review");
            for(const auto& wheel:review.Wheels){
                Check(wheel.Valid &&
                      wheel.Band==TyreSetupBand::Nominal &&
                      wheel.MeasuredPressureKpa==215.0 &&
                      wheel.MeasuredCarcassTemperatureC==80.0 &&
                      wheel.TemperatureSpreadC==2.0 &&
                      wheel.SuggestedColdPressureKpa>=160.0 &&
                      wheel.SuggestedColdPressureKpa<=300.0 &&
                      wheel.RealInflationValveUnchanged,
                      "each tire pressure recommendation is bounded and passive");
            }
            auto hot=measured;
            hot[1].CarcassTemperatureC=140.0;
            hot[1].TreadDepthMm=0.6;
            const auto worn=BuildPhysicalTyreSetupPlan(
                config,800u,95u,10.0,hot);
            Check(worn.Valid &&
                  worn.OverheatedWheelCount==1u &&
                  worn.LowTreadWheelCount==1u &&
                  worn.LargestAxleTemperatureSpreadC>=60.0,
                  "overheated worn tire is flagged from real contact data");
            auto bad=measured;
            bad[0].NativeWheel=bad[1].NativeWheel;
            Check(!BuildPhysicalTyreSetupPlan(
                config,800u,95u,10.0,bad).Valid,
                "duplicated Chaos wheel identity is never considered two tires");
            bad=measured;
            bad[2].ActualPressureMeasurement=false;
            Check(!BuildPhysicalTyreSetupPlan(
                config,800u,95u,10.0,bad).Valid,
                "missing pressure sensor fails entire four-contact review");
            bad=measured;
            bad[3].TreadDepthMm=
                std::numeric_limits<double>::quiet_NaN();
            Check(!BuildPhysicalTyreSetupPlan(
                config,800u,95u,10.0,bad).Valid,
                "nonfinite tread measurement cannot be approximated");
            bad=measured;
            bad[0].ClockSeconds=9.0;
            Check(!BuildPhysicalTyreSetupPlan(
                config,800u,95u,10.0,bad).Valid,
                "stale sensor clock cannot masquerade as current tire data");
            bad=measured;
            bad[3].OwnerActor=999u;
            Check(!BuildPhysicalTyreSetupPlan(
                config,800u,95u,10.0,bad).Valid,
                "wrong physical vehicle must never inherit tire telemetry");
            ++verified;
        }
    }
    Check(verified==SixCarSetupVehicleCount*
          WorldScaleVariantCount,
          "all 36 original car/circuit options validated independently");
    std::cout<<"PhysicalTyreSetupTelemetry: 36 car-track setups passed\n";
    return EXIT_SUCCESS;
}
