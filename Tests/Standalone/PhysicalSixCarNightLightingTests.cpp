#include "Vehicles/PhysicalSixCarNightLighting.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Require(bool yes,const char* why) {
    if(!yes){
        std::cerr<<"PhysicalSixCarNightLighting FAIL: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::PhysicalReplayFrame Fleet(
    const latebrake::WorldScaleVariantRecipe& circuit)
{
    using namespace latebrake;
    PhysicalReplayFrame fleet;
    fleet.Session=95u;
    fleet.SessionRevision=4u;
    fleet.SourceFrame=28u;
    fleet.ActiveClockSeconds=6.0;
    fleet.SourceDeltaSeconds=0.05;
    fleet.AllSixEngineActorsObserved=true;
    for(unsigned i=0u;i<PhysicalNightLampCarCount;++i) {
        auto& c=fleet.Cars[i];
        const auto pose=WorldScaleVariantCenterlineAt(
            circuit,circuit.LapLengthCm*i/
                static_cast<double>(PhysicalNightLampCarCount));
        c.Actor=1000u+i;
        c.Session=fleet.Session;
        c.SessionRevision=fleet.SessionRevision;
        c.SourceFrame=fleet.SourceFrame;
        c.WorldXcm=pose.Pose.Position.X;
        c.WorldYcm=pose.Pose.Position.Y;
        c.WorldZcm=100.0;
        c.HeadingRadians=pose.Pose.HeadingDegrees*
            RoundedPi/180.0;
        c.VelocityXcmps=1800.0*
            std::cos(c.HeadingRadians);
        c.VelocityYcmps=1800.0*
            std::sin(c.HeadingRadians);
        c.RoadContactConfidence=1.0;
        c.BrakeNormalized=i==0u ? 0.7:0.0;
        c.ActualChaosBodyObserved=true;
        c.OriginalRoadPhysicalContact=true;
        c.EngineClockRunning=true;
    }
    return fleet;
}
latebrake::PhysicalNightLampSource Source(
    latebrake::WorldScaleVariantId circuit)
{
    using namespace latebrake;
    PhysicalNightLampSource witness;
    witness.Circuit=circuit;
    witness.Session=95u;
    witness.Revision=4u;
    witness.Frame=28u;
    witness.SourceClockSeconds=6.0;
    witness.ObservedSunElevationDegrees=-18.0;
    witness.ObservedFogDensityPerKm=0.04;
    witness.OriginalWorldStaticRoadVerified=true;
    witness.ActualVehicleLightMountsVerified=true;
    witness.ActualNightConditionsObserved=true;
    witness.OriginalVehicleControlsUnchanged=true;
    return witness;
}
}
int main(){
    using namespace latebrake;
    const double nan=std::numeric_limits<double>::quiet_NaN();
    PhysicalNightLampSettings settings;
    Require(ValidPhysicalNightLampSettings(settings),
            "original vehicle exterior lamp settings are finite");
    settings.NoNativeLightActorSpawn=false;
    Require(!ValidPhysicalNightLampSettings(settings),
            "source-only lamp proposal cannot install Unreal lights");
    settings={};
    settings.MaximumVisualLightBudgetLumens=nan;
    Require(!ValidPhysicalNightLampSettings(settings),
            "nonfinite flux budget rejected");
    settings={};
    unsigned sourceRoads=0u;
    for(unsigned route=0u;route<WorldScaleVariantCount;++route) {
        const auto circuit=WorldScaleOriginalVariantRecipes[route];
        WorldScaleBankedRoadProfile road;
        Require(ChooseWorldScaleBankedRoadProfile(
            circuit.Id,road),
            "source six-course road bank/profile exists");
        const auto fleet=Fleet(circuit);
        const auto source=Source(circuit.Id);
        Require(ValidPhysicalNightLampSource(
            circuit,fleet,source),
            "all six physical car and night source sessions match");
        const auto plan=BuildPhysicalNightLighting(
            circuit,road,fleet,source);
        Require(plan.Valid &&
                plan.Circuit==circuit.Id &&
                plan.Session==95u &&
                plan.Revision==4u &&
                plan.Frame==28u &&
                plan.QualifiedLamps==PhysicalNightLampCount &&
                plan.QualifiedRoadStations==NightRaceSurveyStations &&
                plan.TotalEmittedLumens>0.0 &&
                plan.TotalEmittedLumens<=
                    settings.MaximumVisualLightBudgetLumens &&
                plan.ActiveBrakeLamps==2u &&
                plan.SourceOnly &&
                plan.UnrealLightsUnchanged &&
                plan.OfficialRaceAndChaosUnchanged,
                "six observed vehicles produce 24 bounded cosmetic lighting plans");
        for(unsigned i=0u;i<PhysicalNightLampCarCount;++i) {
            const auto& leftFront=plan.Lamps[i*4u];
            const auto& rightFront=plan.Lamps[i*4u+1u];
            const auto& leftRear=plan.Lamps[i*4u+2u];
            const auto& rightRear=plan.Lamps[i*4u+3u];
            Require(leftFront.Valid && rightFront.Valid &&
                    leftRear.Valid && rightRear.Valid &&
                    leftFront.Actor==fleet.Cars[i].Actor &&
                    rightFront.Actor==fleet.Cars[i].Actor &&
                    leftRear.Actor==fleet.Cars[i].Actor &&
                    rightRear.Actor==fleet.Cars[i].Actor,
                    "all four lamps preserve true source car identity");
            Require(leftFront.Kind==
                      PhysicalNightLampKind::LeftHeadlamp &&
                    rightFront.Kind==
                      PhysicalNightLampKind::RightHeadlamp &&
                    leftRear.Kind==
                      PhysicalNightLampKind::LeftTail &&
                    rightRear.Kind==
                      PhysicalNightLampKind::RightTail &&
                    leftFront.BeamPitchRadians<0.0 &&
                    rightFront.BeamPitchRadians<0.0 &&
                    leftRear.BeamPitchRadians==0.0 &&
                    rightRear.BeamPitchRadians==0.0,
                    "front headlights have downward aimed beams, tail lamps are rearward");
            Require(leftFront.LuminousFluxLumens==
                      settings.FrontLampNominalLumens &&
                    rightFront.LuminousFluxLumens==
                      settings.FrontLampNominalLumens &&
                    leftRear.LuminousFluxLumens>=
                      settings.RearLampNominalLumens &&
                    rightRear.LuminousFluxLumens>=
                      settings.RearLampNominalLumens &&
                    !leftFront.UnrealLightInstalled &&
                    !leftRear.RealShadowCastVerified,
                    "planned source intensity never claims real Unreal rendering");
            if(i==0u)
                Require(leftRear.LuminousFluxLumens>
                    plan.Lamps[6u].LuminousFluxLumens,
                    "observed strong actual brake raises only brake-lamp visual flux");
            const double fx=std::cos(fleet.Cars[i].HeadingRadians);
            const double fy=std::sin(fleet.Cars[i].HeadingRadians);
            const CircuitPoint ahead{
                fleet.Cars[i].WorldXcm+fx*1000.0,
                fleet.Cars[i].WorldYcm+fy*1000.0
            };
            const double measuredLux=EstimatePhysicalNightLampLux(
                leftFront,ahead,0.0,source.ObservedFogDensityPerKm);
            const double thickFogLux=EstimatePhysicalNightLampLux(
                leftFront,ahead,0.0,0.20);
            Require(std::isfinite(measuredLux) &&
                    measuredLux>=0.0 &&
                    thickFogLux<=measuredLux+1e-6,
                    "more measured fog never increases proposed headlamp illuminance");
        }
        for(unsigned station=0u;station<NightRaceSurveyStations;++station) {
            const auto& sample=plan.Probes[station];
            Require(sample.Valid &&
                    sample.EstimatedLampIlluminanceLux>=0.0 &&
                    sample.EstimatedLampIlluminanceLux<=800.0 &&
                    sample.MaximumSingleLampContributionLux<=
                        sample.EstimatedLampIlluminanceLux+1e-6 &&
                    sample.PhysicsCollisionUnchanged &&
                    sample.NativeDriverVisibilityUnqualified,
                    "all road light probes are collision-free descriptive evidence");
            Require(IsOnWorldScaleVariantRoad(
                circuit,{sample.Xcm,sample.Ycm}),
                "survey samples are placed on actual original asphalt");
        }
        auto noNight=source;
        noNight.ActualNightConditionsObserved=false;
        Require(!BuildPhysicalNightLighting(
            circuit,road,fleet,noNight).Valid,
            "no native sun witness means no assumed nighttime headlights");
        auto unverified=fleet;
        unverified.Cars[2].ActualChaosBodyObserved=false;
        Require(!BuildPhysicalNightLighting(
            circuit,road,unverified,source).Valid,
            "unauthenticated car cannot create external light actor proposal");
        unverified=fleet;
        unverified.Cars[3].RoadContactConfidence=0.2;
        Require(!BuildPhysicalNightLighting(
            circuit,road,unverified,source).Valid,
            "unverified physical road contact fails entire six-car scene");
        auto mismatched=source;
        mismatched.Frame++;
        Require(!BuildPhysicalNightLighting(
            circuit,road,fleet,mismatched).Valid,
            "stale native night witness cannot be reused across race frames");
        ++sourceRoads;
    }
    Require(sourceRoads==WorldScaleVariantCount,
            "all six original racing routes were independently audited");
    std::cout<<"PhysicalSixCarNightLighting: 24 fixtures / six courses passed\n";
    return EXIT_SUCCESS;
}
