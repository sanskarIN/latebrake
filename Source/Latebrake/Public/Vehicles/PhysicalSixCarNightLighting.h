#pragma once
#include "Racing/PhysicalSixCarReplayTimeline.h"
#include "Track/WorldScaleNightRaceLightingPlan.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Source-only headlight/taillight proposals for the six actual observed
// original cars. Native rendering, beam occlusion, collision, Chaos
// physics, automatic lighting, racing flags and progress are untouched.
// The output is a geometrical light plan, not an installed visual feature.
inline constexpr unsigned PhysicalNightLampCarCount=
    PhysicalReplayCarCount;
inline constexpr unsigned PhysicalNightLampsPerCar=4u;
inline constexpr unsigned PhysicalNightLampCount=
    PhysicalNightLampCarCount*PhysicalNightLampsPerCar;
inline constexpr double PhysicalNightPi=
    3.1415926535897932384626433832795;
enum class PhysicalNightLampKind : std::uint8_t {
    LeftHeadlamp, RightHeadlamp, LeftTail, RightTail
};
struct PhysicalNightLampSettings final {
    double WheelbaseCm=270.0;
    double FrontLampSeparationCm=135.0;
    double RearLampSeparationCm=125.0;
    double FrontLampMountHeightCm=80.0;
    double RearLampMountHeightCm=78.0;
    double FrontLampDownTiltDegrees=2.5;
    double FrontLampBeamHalfAngleDegrees=23.0;
    double RearLampBeamHalfAngleDegrees=65.0;
    double FrontLampMaximumRangeCm=12500.0;
    double RearLampMaximumRangeCm=2300.0;
    double FrontLampNominalLumens=1550.0;
    double RearLampNominalLumens=90.0;
    double BrakeLampAdditionalLumens=350.0;
    double MinimumSourceNightSunElevationDegrees=-30.0;
    double MaximumSourceNightSunElevationDegrees=10.0;
    double MaximumMeasuredFogDensityPerKm=0.2;
    double MaximumSourceClockSkewSeconds=0.05;
    double MinimumRealRoadContactConfidence=0.85;
    double MaximumVisualLightBudgetLumens=25000.0;
    bool RequireActualSixChassis=true;
    bool RequireActualNightWitness=true;
    bool NoNativeLightActorSpawn=true;
    bool NoChaosVehicleModification=true;
    bool NoRacingPenaltyOrScoreAuthority=true;
};
struct PhysicalNightLampSource final {
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t Frame=0u;
    double SourceClockSeconds=0.0;
    double ObservedSunElevationDegrees=-15.0;
    double ObservedFogDensityPerKm=0.0;
    bool OriginalWorldStaticRoadVerified=false;
    bool ActualVehicleLightMountsVerified=false;
    bool ActualNightConditionsObserved=false;
    bool OriginalVehicleControlsUnchanged=true;
};
struct PhysicalNightLamp final {
    bool Valid=false;
    std::uint64_t Actor=0u;
    unsigned SourceSlot=0u;
    PhysicalNightLampKind Kind=PhysicalNightLampKind::LeftHeadlamp;
    double Xcm=0.0;
    double Ycm=0.0;
    double Zcm=0.0;
    double HeadingRadians=0.0;
    double BeamPitchRadians=0.0;
    double BeamHalfAngleRadians=0.0;
    double MaximumRangeCm=0.0;
    double LuminousFluxLumens=0.0;
    double BrakeFraction=0.0;
    bool NativeSourceProvenance=true;
    bool UnrealLightInstalled=false;
    bool RealShadowCastVerified=false;
};
struct PhysicalNightRoadProbe final {
    bool Valid=false;
    double StationCm=0.0;
    double Xcm=0.0;
    double Ycm=0.0;
    double Zcm=0.0;
    double EstimatedLampIlluminanceLux=0.0;
    double MaximumSingleLampContributionLux=0.0;
    unsigned ObservedContributors=0u;
    bool PhysicsCollisionUnchanged=true;
    bool NativeDriverVisibilityUnqualified=true;
};
struct PhysicalNightLightingPlan final {
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t Frame=0u;
    double SourceClockSeconds=0.0;
    double MeasuredSunElevationDegrees=0.0;
    double MeasuredFogDensityPerKm=0.0;
    std::array<PhysicalNightLamp,PhysicalNightLampCount> Lamps{};
    std::array<PhysicalNightRoadProbe,
        NightRaceSurveyStations> Probes{};
    double TotalEmittedLumens=0.0;
    double MaximumRoadIlluminanceLux=0.0;
    double MeanRoadIlluminanceLux=0.0;
    unsigned QualifiedLamps=0u;
    unsigned QualifiedRoadStations=0u;
    unsigned RoadStationsWithinHeadlightReach=0u;
    unsigned ActiveBrakeLamps=0u;
    bool SourceOnly=true;
    bool UnrealLightsUnchanged=true;
    bool OfficialRaceAndChaosUnchanged=true;
};
inline bool ValidPhysicalNightLampSettings(
    PhysicalNightLampSettings s) noexcept
{
    return std::isfinite(s.WheelbaseCm) &&
        s.WheelbaseCm>=200.0 && s.WheelbaseCm<=360.0 &&
        std::isfinite(s.FrontLampSeparationCm) &&
        s.FrontLampSeparationCm>=90.0 &&
        s.FrontLampSeparationCm<=180.0 &&
        std::isfinite(s.RearLampSeparationCm) &&
        s.RearLampSeparationCm>=90.0 &&
        s.RearLampSeparationCm<=180.0 &&
        std::isfinite(s.FrontLampMountHeightCm) &&
        s.FrontLampMountHeightCm>=45.0 &&
        s.FrontLampMountHeightCm<=110.0 &&
        std::isfinite(s.RearLampMountHeightCm) &&
        s.RearLampMountHeightCm>=40.0 &&
        s.RearLampMountHeightCm<=110.0 &&
        std::isfinite(s.FrontLampDownTiltDegrees) &&
        s.FrontLampDownTiltDegrees>=0.5 &&
        s.FrontLampDownTiltDegrees<=6.0 &&
        std::isfinite(s.FrontLampBeamHalfAngleDegrees) &&
        s.FrontLampBeamHalfAngleDegrees>=10.0 &&
        s.FrontLampBeamHalfAngleDegrees<=40.0 &&
        std::isfinite(s.RearLampBeamHalfAngleDegrees) &&
        s.RearLampBeamHalfAngleDegrees>=40.0 &&
        s.RearLampBeamHalfAngleDegrees<=90.0 &&
        std::isfinite(s.FrontLampMaximumRangeCm) &&
        s.FrontLampMaximumRangeCm>=5000.0 &&
        s.FrontLampMaximumRangeCm<=18000.0 &&
        std::isfinite(s.RearLampMaximumRangeCm) &&
        s.RearLampMaximumRangeCm>=1200.0 &&
        s.RearLampMaximumRangeCm<=4000.0 &&
        std::isfinite(s.FrontLampNominalLumens) &&
        s.FrontLampNominalLumens>=800.0 &&
        s.FrontLampNominalLumens<=2600.0 &&
        std::isfinite(s.RearLampNominalLumens) &&
        s.RearLampNominalLumens>=20.0 &&
        s.RearLampNominalLumens<=180.0 &&
        std::isfinite(s.BrakeLampAdditionalLumens) &&
        s.BrakeLampAdditionalLumens>=100.0 &&
        s.BrakeLampAdditionalLumens<=650.0 &&
        std::isfinite(s.MinimumSourceNightSunElevationDegrees) &&
        s.MinimumSourceNightSunElevationDegrees>=-50.0 &&
        s.MinimumSourceNightSunElevationDegrees<=-15.0 &&
        std::isfinite(s.MaximumSourceNightSunElevationDegrees) &&
        s.MaximumSourceNightSunElevationDegrees>=0.0 &&
        s.MaximumSourceNightSunElevationDegrees<=20.0 &&
        std::isfinite(s.MaximumMeasuredFogDensityPerKm) &&
        s.MaximumMeasuredFogDensityPerKm>=0.05 &&
        s.MaximumMeasuredFogDensityPerKm<=0.5 &&
        std::isfinite(s.MaximumSourceClockSkewSeconds) &&
        s.MaximumSourceClockSkewSeconds>=0.0 &&
        s.MaximumSourceClockSkewSeconds<=0.10 &&
        std::isfinite(s.MinimumRealRoadContactConfidence) &&
        s.MinimumRealRoadContactConfidence>=0.8 &&
        s.MinimumRealRoadContactConfidence<=1.0 &&
        std::isfinite(s.MaximumVisualLightBudgetLumens) &&
        s.MaximumVisualLightBudgetLumens>=15000.0 &&
        s.MaximumVisualLightBudgetLumens<=35000.0 &&
        s.RequireActualSixChassis &&
        s.RequireActualNightWitness &&
        s.NoNativeLightActorSpawn &&
        s.NoChaosVehicleModification &&
        s.NoRacingPenaltyOrScoreAuthority;
}
inline bool ValidPhysicalNightLampSource(
    const WorldScaleVariantRecipe& circuit,
    const PhysicalReplayFrame& fleet,
    PhysicalNightLampSource witness,
    PhysicalNightLampSettings settings={}) noexcept
{
    if(!ValidPhysicalNightLampSettings(settings) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       witness.Circuit!=circuit.Id ||
       !witness.OriginalWorldStaticRoadVerified ||
       !witness.ActualVehicleLightMountsVerified ||
       !witness.ActualNightConditionsObserved ||
       !witness.OriginalVehicleControlsUnchanged ||
       ValidatePhysicalReplayFrame(fleet)!=
           PhysicalReplayReject::None ||
       witness.Session!=fleet.Session ||
       witness.Revision!=fleet.SessionRevision ||
       witness.Frame!=fleet.SourceFrame ||
       !std::isfinite(witness.SourceClockSeconds) ||
       std::abs(witness.SourceClockSeconds-
           fleet.ActiveClockSeconds)>
           settings.MaximumSourceClockSkewSeconds ||
       !std::isfinite(witness.ObservedSunElevationDegrees) ||
       witness.ObservedSunElevationDegrees<
           settings.MinimumSourceNightSunElevationDegrees ||
       witness.ObservedSunElevationDegrees>
           settings.MaximumSourceNightSunElevationDegrees ||
       !std::isfinite(witness.ObservedFogDensityPerKm) ||
       witness.ObservedFogDensityPerKm<0.0 ||
       witness.ObservedFogDensityPerKm>
           settings.MaximumMeasuredFogDensityPerKm)
        return false;
    for(const auto& car:fleet.Cars)
        if(car.RoadContactConfidence<
           settings.MinimumRealRoadContactConfidence)
            return false;
    return true;
}
inline PhysicalNightLamp BuildPhysicalNightLamp(
    const PhysicalReplayCarSample& car,
    unsigned carSlot,
    PhysicalNightLampKind kind,
    PhysicalNightLampSettings settings={}) noexcept
{
    PhysicalNightLamp out;
    if(!FinitePhysicalReplayCar(car) ||
       carSlot>=PhysicalNightLampCarCount ||
       !ValidPhysicalNightLampSettings(settings))
        return out;
    const bool front=kind==PhysicalNightLampKind::LeftHeadlamp ||
        kind==PhysicalNightLampKind::RightHeadlamp;
    const bool left=kind==PhysicalNightLampKind::LeftHeadlamp ||
        kind==PhysicalNightLampKind::LeftTail;
    const double forward=front ? settings.WheelbaseCm*0.5 :
        -settings.WheelbaseCm*0.5;
    const double side=(left?1.0:-1.0)*
        (front?settings.FrontLampSeparationCm :
              settings.RearLampSeparationCm)*0.5;
    const double fx=std::cos(car.HeadingRadians);
    const double fy=std::sin(car.HeadingRadians);
    const double rx=-fy;
    const double ry=fx;
    out.Actor=car.Actor;
    out.SourceSlot=carSlot;
    out.Kind=kind;
    out.Xcm=car.WorldXcm+forward*fx+side*rx;
    out.Ycm=car.WorldYcm+forward*fy+side*ry;
    out.Zcm=car.WorldZcm+
        (front?settings.FrontLampMountHeightCm :
               settings.RearLampMountHeightCm);
    out.HeadingRadians=std::remainder(
        car.HeadingRadians+(front?0.0:PhysicalNightPi),
        2.0*PhysicalNightPi);
    out.BeamPitchRadians=front ?
        -settings.FrontLampDownTiltDegrees*PhysicalNightPi/180.0:
        0.0;
    out.BeamHalfAngleRadians=(front?
        settings.FrontLampBeamHalfAngleDegrees:
        settings.RearLampBeamHalfAngleDegrees)*
        PhysicalNightPi/180.0;
    out.MaximumRangeCm=front?
        settings.FrontLampMaximumRangeCm:
        settings.RearLampMaximumRangeCm;
    out.BrakeFraction=front?0.0:car.BrakeNormalized;
    out.LuminousFluxLumens=front?
        settings.FrontLampNominalLumens:
        settings.RearLampNominalLumens+
           settings.BrakeLampAdditionalLumens*
               car.BrakeNormalized;
    out.Valid=std::isfinite(out.Xcm) &&
        std::isfinite(out.Ycm) &&
        std::isfinite(out.Zcm) &&
        std::isfinite(out.LuminousFluxLumens) &&
        out.LuminousFluxLumens>0.0;
    return out;
}
inline double EstimatePhysicalNightLampLux(
    PhysicalNightLamp lamp,
    CircuitPoint ground,
    double groundZcm,
    double fogDensityPerKm) noexcept
{
    if(!lamp.Valid ||
       !std::isfinite(ground.X) ||
       !std::isfinite(ground.Y) ||
       !std::isfinite(groundZcm) ||
       !std::isfinite(fogDensityPerKm) ||
       fogDensityPerKm<0.0 ||
       fogDensityPerKm>0.5)
        return 0.0;
    const double dx=ground.X-lamp.Xcm;
    const double dy=ground.Y-lamp.Ycm;
    const double dz=groundZcm-lamp.Zcm;
    const double distance=std::hypot(
        std::hypot(dx,dy),dz);
    if(!std::isfinite(distance) ||
       distance<50.0 ||
       distance>lamp.MaximumRangeCm)
        return 0.0;
    const double pitch=lamp.BeamPitchRadians;
    const double horizontal=std::cos(pitch);
    const double fx=horizontal*
        std::cos(lamp.HeadingRadians);
    const double fy=horizontal*
        std::sin(lamp.HeadingRadians);
    const double fz=std::sin(pitch);
    const double cosine=(dx*fx+dy*fy+dz*fz)/distance;
    if(!std::isfinite(cosine) ||
       cosine<std::cos(lamp.BeamHalfAngleRadians))
        return 0.0;
    const double solidAngle=2.0*PhysicalNightPi*
        (1.0-std::cos(lamp.BeamHalfAngleRadians));
    const double rangeMeters=std::max(1.0,distance/100.0);
    const double incidence=std::max(0.0,-dz/distance);
    if(!std::isfinite(solidAngle) || solidAngle<=0.0)
        return 0.0;
    const double fog=std::exp(-fogDensityPerKm*
        distance/100000.0);
    const double intensity=lamp.LuminousFluxLumens/
        (solidAngle*rangeMeters*rangeMeters)*
        incidence*std::max(0.0,cosine)*fog;
    return std::isfinite(intensity) ?
        std::clamp(intensity,0.0,800.0) : 0.0;
}
inline PhysicalNightLightingPlan BuildPhysicalNightLighting(
    const WorldScaleVariantRecipe& circuit,
    WorldScaleBankedRoadProfile road,
    const PhysicalReplayFrame& fleet,
    PhysicalNightLampSource source,
    PhysicalNightLampSettings settings={}) noexcept
{
    PhysicalNightLightingPlan out;
    if(!ValidWorldScaleBankedRoadProfile(circuit,road) ||
       !ValidPhysicalNightLampSource(
           circuit,fleet,source,settings))
        return out;
    out.Circuit=circuit.Id;
    out.Session=fleet.Session;
    out.Revision=fleet.SessionRevision;
    out.Frame=fleet.SourceFrame;
    out.SourceClockSeconds=fleet.ActiveClockSeconds;
    out.MeasuredSunElevationDegrees=
        source.ObservedSunElevationDegrees;
    out.MeasuredFogDensityPerKm=
        source.ObservedFogDensityPerKm;
    for(unsigned i=0u;i<PhysicalNightLampCarCount;++i) {
        for(unsigned j=0u;j<PhysicalNightLampsPerCar;++j) {
            const auto kind=static_cast<PhysicalNightLampKind>(j);
            const auto lamp=BuildPhysicalNightLamp(
                fleet.Cars[i],i,kind,settings);
            if(!lamp.Valid)return {};
            const unsigned k=i*PhysicalNightLampsPerCar+j;
            out.Lamps[k]=lamp;
            ++out.QualifiedLamps;
            out.TotalEmittedLumens+=lamp.LuminousFluxLumens;
            if(j>=2u && fleet.Cars[i].BrakeNormalized>0.6)
                ++out.ActiveBrakeLamps;
        }
    }
    if(out.QualifiedLamps!=PhysicalNightLampCount ||
       out.TotalEmittedLumens>
           settings.MaximumVisualLightBudgetLumens)
        return {};
    for(unsigned station=0u;
        station<NightRaceSurveyStations;++station) {
        const double s=circuit.LapLengthCm*station/
            static_cast<double>(NightRaceSurveyStations);
        const auto roadSample=SampleWorldScaleBankedRoad(
            circuit,road,s,0.0);
        if(!roadSample.Valid)return {};
        PhysicalNightRoadProbe probe;
        probe.StationCm=s;
        probe.Xcm=roadSample.WorldXcm;
        probe.Ycm=roadSample.WorldYcm;
        probe.Zcm=roadSample.WorldZcm;
        for(const auto& lamp:out.Lamps) {
            const double lux=EstimatePhysicalNightLampLux(
                lamp,{probe.Xcm,probe.Ycm},probe.Zcm,
                out.MeasuredFogDensityPerKm);
            if(lux>0.0){
                probe.EstimatedLampIlluminanceLux+=lux;
                probe.MaximumSingleLampContributionLux=std::max(
                    probe.MaximumSingleLampContributionLux,lux);
                ++probe.ObservedContributors;
            }
        }
        probe.EstimatedLampIlluminanceLux=std::min(
            probe.EstimatedLampIlluminanceLux,800.0);
        if(!std::isfinite(probe.EstimatedLampIlluminanceLux))
            return {};
        probe.Valid=true;
        out.Probes[station]=probe;
        out.MeanRoadIlluminanceLux+=
            probe.EstimatedLampIlluminanceLux;
        out.MaximumRoadIlluminanceLux=std::max(
            out.MaximumRoadIlluminanceLux,
            probe.EstimatedLampIlluminanceLux);
        if(probe.EstimatedLampIlluminanceLux>0.1)
            ++out.RoadStationsWithinHeadlightReach;
        ++out.QualifiedRoadStations;
    }
    out.MeanRoadIlluminanceLux/=
        static_cast<double>(NightRaceSurveyStations);
    out.Valid=out.QualifiedLamps==PhysicalNightLampCount &&
        out.QualifiedRoadStations==NightRaceSurveyStations &&
        std::isfinite(out.MeanRoadIlluminanceLux);
    return out;
}
} // namespace latebrake
