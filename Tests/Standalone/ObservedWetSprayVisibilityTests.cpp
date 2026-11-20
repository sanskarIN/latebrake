#include "Racing/ObservedWetSprayVisibility.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool value,const char* label) {
    if(!value){std::cerr<<"WetSprayVisibility FAIL: "<<label<<'\n';std::exit(EXIT_FAILURE);}
}
latebrake::PhysicalReplayFrame Fleet() {
    using namespace latebrake;
    PhysicalReplayFrame frame;
    frame.Session=5u;
    frame.SessionRevision=7u;
    frame.SourceFrame=13u;
    frame.ActiveClockSeconds=6.0;
    frame.SourceDeltaSeconds=0.05;
    frame.AllSixEngineActorsObserved=true;
    for(unsigned i=0;i<WetSprayFleetSize;++i) {
        auto& c=frame.Cars[i];
        c.Actor=100u+i;
        c.Session=5u;
        c.SessionRevision=7u;
        c.SourceFrame=13u;
        c.WorldXcm=5000.0+2500.0*i;
        c.WorldYcm=500.0*i;
        c.WorldZcm=100.0;
        c.VelocityXcmps=6000.0;
        c.VelocityYcmps=0.0;
        c.VelocityZcmps=0.0;
        c.HeadingRadians=0.0;
        c.SteeringNormalized=0.0;
        c.ThrottleNormalized=0.5;
        c.BrakeNormalized=0.0;
        c.RoadContactConfidence=1.0;
        c.ActualChaosBodyObserved=true;
        c.OriginalRoadPhysicalContact=true;
        c.EngineClockRunning=true;
    }
    frame.Cars[1].WorldXcm=4200.0;
    frame.Cars[1].WorldYcm=0.0;
    return frame;
}
latebrake::ObservedWaterResult Film(
    const latebrake::WorldScaleVariantRecipe& circuit,
    double measuredFilm=9.0)
{
    using namespace latebrake;
    ObservedWaterResult result;
    result.Valid=true;
    result.Fault=ObservedWaterFault::None;
    result.Circuit=circuit.Id;
    result.Session=5u;
    result.Revision=7u;
    result.SourceFrame=13u;
    result.ClockSeconds=6.0;
    result.StepSeconds=0.05;
    result.CellLengthCm=circuit.LapLengthCm/
        static_cast<double>(ObservedWaterCellCount);
    result.FilmMm.fill(measuredFilm);
    result.RenderAdvisoryOnly=true;
    result.OriginalRaceUnaffected=true;
    return result;
}
}
int main() {
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    auto settings=WetSpraySettings{};
    Check(ValidWetSpraySettings(settings),
          "bounded source-only water spray policy valid");
    settings.NeverClaimRealCameraOcclusion=false;
    Check(!ValidWetSpraySettings(settings),
          "source hint cannot claim actual native visibility");
    settings={};
    settings.MaximumTotalParticles=0u;
    Check(!ValidWetSpraySettings(settings),
          "global particle capacity cannot be zero");
    settings={};
    settings.SprayConeHalfAngleDegrees=nan;
    Check(!ValidWetSpraySettings(settings),
          "nonfinite spray cone rejected");
    unsigned routes=0;
    for(unsigned track=0;track<WorldScaleVariantCount;++track) {
        const auto circuit=WorldScaleOriginalVariantRecipes[track];
        const auto frame=Fleet();
        const auto water=Film(circuit);
        std::array<WetSprayVehicleObservation,WetSprayFleetSize> observed{};
        for(unsigned i=0;i<WetSprayFleetSize;++i) {
            observed[i].Actor=frame.Cars[i].Actor;
            observed[i].MeasuredStationCm=
                5000.0+static_cast<double>(i)*2500.0;
            observed[i].ClockSeconds=6.0;
            observed[i].NativeRoadProjectionObserved=true;
            observed[i].PhysicalTireContactObserved=true;
            observed[i].OriginalRaceClockUnchanged=true;
        }
        Check(ValidateWetSprayProvenance(
            circuit,frame,water,observed,{}),
            "physical chassis epochs and native measured rain sources align");
        const auto plan=BuildObservedWetSpray(
            circuit,frame,water,observed);
        Check(plan.Valid &&
              plan.Circuit==circuit.Id &&
              plan.Session==5u &&
              plan.Frame==13u &&
              plan.VerifiedEmitterCount==WetSprayFleetSize &&
              plan.ActiveEmitterCount==WetSprayFleetSize &&
              plan.ProposedParticleCount<=settings.MaximumTotalParticles &&
              plan.ActiveExposureCount>=1u &&
              plan.PeakExposure>0.0 &&
              plan.TotalIntensity>0.0 &&
              plan.ActualOcclusionUnverified &&
              plan.NoPhysicsOrRaceChanges,
              "all six real cars emit bounded cosmetic wet mist");
        for(const auto& spray:plan.Emitters) {
            Check(spray.Valid && spray.Actor!=0u &&
                spray.ActualSpeedCmps==6000.0 &&
                spray.SourceFilmMm==9.0 &&
                spray.Intensity>0.0 &&
                spray.RecommendedRangeCm>0.0 &&
                spray.ProposedParticleBudget<=settings.MaximumParticlesPerVehicle &&
                spray.SourceOnly &&
                !spray.UnrealParticleSystemCreated,
                "each source mist emitter has no actual scene installation");
        }
        unsigned validPairs=0u;
        bool sawFollower=false;
        for(const auto& pair:plan.Exposures) {
            Check(pair.Valid &&
                  pair.EmitterActor!=pair.FollowingActor &&
                  pair.NoVisibilityGameplayAuthority &&
                  pair.ObservedIntensity>=0.0 &&
                  pair.OcclusionAdvisoryFraction<=1.0,
                  "all ordered 30 unique pairs are bounded visual hints");
            ++validPairs;
            if(pair.EmitterActor==100u &&
               pair.FollowingActor==101u) {
                sawFollower=true;
                Check(pair.AlongSprayCm>0.0 &&
                      std::abs(pair.LateralSprayCm)<1e-7 &&
                      pair.ObservedIntensity>0.0,
                      "measured car behind spray emitter receives exposure");
            }
            if(pair.EmitterActor==101u &&
               pair.FollowingActor==100u)
                Check(pair.ObservedIntensity==0.0,
                      "car in front never receives rear-facing mist");
        }
        Check(validPairs==WetSprayPairCount &&
              sawFollower,"six-vehicle directed pairs complete");

        auto dry=Film(circuit,0.0);
        const auto noSpray=BuildObservedWetSpray(
            circuit,frame,dry,observed);
        Check(noSpray.Valid &&
              noSpray.ActiveEmitterCount==0u &&
              noSpray.ActiveExposureCount==0u &&
              noSpray.ProposedParticleCount==0u,
              "dry verified asphalt generates no fake rain mist");
        auto changed=water;
        changed.SourceFrame++;
        Check(!BuildObservedWetSpray(
            circuit,frame,changed,observed).Valid,
            "stale source film epoch rejects replay-derived mist");
        changed=water;
        changed.ClockSeconds+=1.0;
        Check(!BuildObservedWetSpray(
            circuit,frame,changed,observed).Valid,
            "source weather and chassis clock skew rejects frame");
        auto obsBad=observed;
        obsBad[2].PhysicalTireContactObserved=false;
        Check(!BuildObservedWetSpray(
            circuit,frame,water,obsBad).Valid,
            "unverified wheel contact cannot render synthetic spray");
        obsBad=observed;
        obsBad[1].Actor=0u;
        Check(!BuildObservedWetSpray(
            circuit,frame,water,obsBad).Valid,
            "wrong actor identity cannot borrow real vehicle spray");
        auto fleetBad=frame;
        fleetBad.Cars[2].WorldXcm=nan;
        Check(!BuildObservedWetSpray(
            circuit,fleetBad,water,observed).Valid,
            "nonfinite observed chassis fails the entire visual frame");
        ++routes;
    }
    Check(routes==WorldScaleVariantCount,
        "all six planned circuits exercise genuine six-car wet conditions");
    std::cout<<"ObservedWetSprayVisibility: six-course, six-car QA passed\n";
    return EXIT_SUCCESS;
}
