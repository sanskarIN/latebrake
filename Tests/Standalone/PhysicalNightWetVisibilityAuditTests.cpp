#include "Racecraft/PhysicalNightWetVisibilityAudit.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool ok,const char* msg){
    if(!ok){std::cerr<<"NightWetVisibility FAIL: "<<msg<<'\n';std::exit(EXIT_FAILURE);}
}
latebrake::NightRaceLightingPlan Lighting(
    latebrake::WorldScaleVariantId id)
{
    using namespace latebrake;
    NightRaceLightingPlan scene;
    scene.Valid=true;
    scene.Circuit=id;
    scene.SourceSession=100u;
    scene.SourceFrame=12u;
    scene.SourceClockSeconds=5.0;
    scene.QualifiedSurveyStations=NightRaceSurveyStations;
    scene.UnrealRenderingUnchanged=true;
    scene.OriginalPhysicalRoadUnchanged=true;
    scene.OfficialRaceStateUnchanged=true;
    for(unsigned i=0u;i<NightRaceSurveyStations;++i){
        scene.Probes[i].Valid=true;
        scene.Probes[i].EstimatedIlluminanceLux=20.0;
        scene.Probes[i].EstimatedFogTransmittance=0.90;
        scene.Probes[i].NoDriverVisibilityAuthority=true;
        scene.Probes[i].ActualNativeShadowTested=false;
    }
    return scene;
}
latebrake::ObservedWaterResult Water(
    const latebrake::WorldScaleVariantRecipe& circuit,double film)
{
    using namespace latebrake;
    ObservedWaterResult water;
    water.Valid=true;
    water.Circuit=circuit.Id;
    water.Session=100u;
    water.Revision=4u;
    water.SourceFrame=12u;
    water.ClockSeconds=5.0;
    water.CellLengthCm=circuit.LapLengthCm/
        static_cast<double>(latebrake::ObservedWaterCellCount);
    water.FilmMm.fill(film);
    water.RenderAdvisoryOnly=true;
    water.OriginalRaceUnaffected=true;
    return water;
}
latebrake::PhysicalReplayFrame Fleet(double speed=5000.0){
    using namespace latebrake;
    PhysicalReplayFrame f;
    f.Session=100u;
    f.SessionRevision=4u;
    f.SourceFrame=12u;
    f.ActiveClockSeconds=5.0;
    f.SourceDeltaSeconds=0.05;
    f.AllSixEngineActorsObserved=true;
    for(unsigned i=0u;i<NightWetVehicleCount;++i){
        auto& c=f.Cars[i];
        c.Actor=200u+i;
        c.Session=100u;
        c.SessionRevision=4u;
        c.SourceFrame=12u;
        c.WorldXcm=1000.0+i*2000.0;
        c.WorldYcm=300.0*i;
        c.WorldZcm=120.0;
        c.VelocityXcmps=speed;
        c.HeadingRadians=0.0;
        c.RoadContactConfidence=1.0;
        c.ActualChaosBodyObserved=true;
        c.OriginalRoadPhysicalContact=true;
        c.EngineClockRunning=true;
    }
    return f;
}
std::array<latebrake::NightWetVehicleSource,
    latebrake::NightWetVehicleCount> Witnesses()
{
    using namespace latebrake;
    std::array<NightWetVehicleSource,NightWetVehicleCount> arr{};
    for(unsigned i=0u;i<NightWetVehicleCount;++i){
        auto& o=arr[i];
        o.Actor=200u+i;
        o.NativeLampComponent=500u+i;
        o.SourceFrame=12u;
        o.RoadStationCm=2000.0+i*3000.0;
        o.MeasuredLowBeamLux=80.0;
        o.VerifiedBrakeDecelCmps2=1800.0;
        o.SourceClockSeconds=5.0;
        o.SensorConfidence=0.99;
        o.RealLampObserved=true;
        o.RealRoadProjectionObserved=true;
        o.OriginalChaosBrakeWitnessed=true;
        o.NoRaceAuthorityChange=true;
    }
    return arr;
}
}
int main(){
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    NightWetSettings settings;
    Check(ValidNightWetSettings(settings),
        "night sight and stopping settings are safely bounded");
    settings.NeverClaimRealOcclusion=false;
    Check(!ValidNightWetSettings(settings),
        "a portable lux proxy cannot claim native visibility");
    settings={};
    settings.MaximumSourceTrackLux=nan;
    Check(!ValidNightWetSettings(settings),
        "nonfinite lux source limit rejected");
    settings={};
    unsigned verified=0u;
    for(unsigned i=0u;i<WorldScaleVariantCount;++i){
        const auto circuit=WorldScaleOriginalVariantRecipes[i];
        auto source=Lighting(circuit.Id);
        const auto frame=Fleet();
        auto water=Water(circuit,1.0);
        auto witnesses=Witnesses();
        const auto audit=AuditNightWetSixCarSafety(
            circuit,source,water,frame,witnesses);
        Check(audit.Valid &&
              audit.Fault==NightWetFault::None &&
              audit.Circuit==circuit.Id &&
              audit.Session==100u &&
              audit.Revision==4u &&
              audit.SourceFrame==12u &&
              audit.SourceQualifiedCars==NightWetVehicleCount &&
              audit.MaximumObservedWaterMm==1.0 &&
              audit.CautionCars>0u &&
              audit.OriginalRaceAuthorityUnchanged &&
              audit.OriginalCarCollisionUnchanged &&
              audit.SourceOnly,
              "actual six car sources qualify advisory-only night visibility");
        for(unsigned j=0u;j<NightWetVehicleCount;++j){
            const auto& car=audit.Cars[j];
            Check(car.Valid &&
                  car.Actor==200u+j &&
                  car.WrappedStationCm>=0.0 &&
                  car.RoadWaterFilmMm==1.0 &&
                  car.SourceTrackLux==20.0 &&
                  car.SourceLampLux==80.0 &&
                  car.EstimatedSightDistanceCm>=300.0 &&
                  car.EstimatedSightDistanceCm<=12000.0 &&
                  car.MeasuredStoppingDistanceCm>0.0 &&
                  car.ConservativeDistanceRatio>0.0 &&
                  car.RealOcclusionUnverified &&
                  car.NoPhysicalBrakingApplied,
                  "per-car sight geometry never changes the physical car");
        }
        const auto rain=AuditNightWetSixCarSafety(
            circuit,source,Water(circuit,12.0),frame,witnesses);
        Check(rain.Valid &&
              rain.Cars[0].EstimatedSightDistanceCm<
                  audit.Cars[0].EstimatedSightDistanceCm &&
              rain.Cars[0].BrakingDistanceReserveCm<
                  audit.Cars[0].BrakingDistanceReserveCm,
              "observed wet road reflectance reduces conservative sight proxy");
        const auto slower=AuditNightWetSixCarSafety(
            circuit,source,water,Fleet(1500.0),witnesses);
        Check(slower.Valid &&
              slower.Cars[0].MeasuredStoppingDistanceCm<
                  audit.Cars[0].MeasuredStoppingDistanceCm &&
              slower.Cars[0].ConservativeDistanceRatio<
                  audit.Cars[0].ConservativeDistanceRatio,
              "real lower speed reduces source-measured stopping distance");
        auto brighter=witnesses;
        brighter[0].MeasuredLowBeamLux=150.0;
        const auto stronger=AuditNightWetSixCarSafety(
            circuit,source,water,frame,brighter);
        Check(stronger.Valid &&
              stronger.Cars[0].EstimatedSightDistanceCm>
                  audit.Cars[0].EstimatedSightDistanceCm,
              "native observed brighter lamps improve only estimated sight");
        auto bad=witnesses;
        bad[1].NativeLampComponent=bad[0].NativeLampComponent;
        Check(!AuditNightWetSixCarSafety(
            circuit,source,water,frame,bad).Valid,
              "duplicate source lamp cannot satisfy independent six cars");
        bad=witnesses;
        bad[3].RealLampObserved=false;
        Check(!AuditNightWetSixCarSafety(
            circuit,source,water,frame,bad).Valid,
              "invented headlights cannot appear as physical lamps");
        bad=witnesses;
        bad[4].SourceClockSeconds=0.0;
        Check(!AuditNightWetSixCarSafety(
            circuit,source,water,frame,bad).Valid,
              "stale headlight readings cannot borrow current race clock");
        bad=witnesses;
        bad[4].RoadStationCm=nan;
        Check(!AuditNightWetSixCarSafety(
            circuit,source,water,frame,bad).Valid,
              "unqualified real road station cannot receive lux estimate");
        source.SourceFrame=13u;
        Check(!AuditNightWetSixCarSafety(
            circuit,source,water,frame,witnesses).Valid,
              "night lighting and chassis source frame must match exactly");
        source=Lighting(circuit.Id);
        source.Probes[3].EstimatedIlluminanceLux=nan;
        // Probe 3 may not be sampled by any of the six source actors.
        // The estimator audits the current source station, not all 256
        // unrelated lights; force the sampled car probe invalid instead.
        source.Probes[3].EstimatedIlluminanceLux=20.0;
        source.Probes[static_cast<unsigned>(
            witnesses[0].RoadStationCm/circuit.LapLengthCm*
                NightRaceSurveyStations)].EstimatedIlluminanceLux=nan;
        Check(!AuditNightWetSixCarSafety(
            circuit,source,water,frame,witnesses).Valid,
              "nonfinite measured road illumination at car is rejected");
        ++verified;
    }
    Check(verified==WorldScaleVariantCount,
          "all six independent circuits verified under real source protocol");
    std::cout<<"PhysicalNightWetVisibilityAudit: six cars / six roads passed\n";
    return EXIT_SUCCESS;
}
