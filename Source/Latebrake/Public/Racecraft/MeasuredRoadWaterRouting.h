#pragma once
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
inline constexpr int RoadWaterCellCount=16;
enum class WaterRoadCondition : std::uint8_t
{
    Unobserved, Dry, Damp, Wet, DeepWater,
    StandingWater, Flooded, SensorDisagreement
};
struct RoadWaterParameters final
{
    double CircuitLengthCm=250000.0;
    double CellDrainageLengthCm=1600.0;
    double MaximumPhysicalRainMmHour=250.0;
    double MaximumMeasuredWaterDepthMm=80.0;
    double MaximumRainSensorAgeSeconds=5.0;
    double MaximumWaterSensorAgeSeconds=3.0;
    double MaximumTemperatureC=120.0;
    double MinimumTemperatureC=-40.0;
    double MinimumRoadFriction=0.10;
    double MaximumRoadFriction=2.0;
    double MinimumRoadNormalZ=0.60;
    double MaximumRoadGradeRadians=0.65;
    double MaximumBankRadians=0.50;
    double MaximumWindCmS=10000.0;
    double MaximumDrainFlowMmSecond=3.0;
    double MaximumRunoffFlowMmSecond=2.0;
    double MaximumEvaporationMmSecond=0.2;
    double MaximumSplashRemovalMmSecond=0.4;
    double MaximumFilmGrowthMmSecond=2.0;
    double MaximumFrameSeconds=0.10;
    double MinimumSensorConfidence=0.75;
    double MinimumActualWaterSampleSeconds=0.01;
    double MaximumClockSkewSeconds=0.06;
    double MinimumSpatialSeparationCm=300.0;
    double MaximumSpatialSeparationCm=24000.0;
    double MinimumDrainEfficiency=0.0;
    double MaximumDrainEfficiency=1.0;
    double MinimumSurfacePorosity=0.0;
    double MaximumSurfacePorosity=1.0;
    double MinimumCrossFallFraction=0.0;
    double MaximumCrossFallFraction=0.60;
    double MinimumWetDepthMm=0.5;
    double StandingWaterDepthMm=4.0;
    double DeepWaterDepthMm=11.0;
    double FloodedDepthMm=30.0;
    double HydroplaningSpeedCmS=4200.0;
    double HydroplaningDepthMm=3.0;
    double MaximumWaterGripPenalty=0.70;
    double MaximumDryingGripRecoveryPerSecond=0.15;
    double MinimumWetGripFraction=0.25;
    double MaximumFrictionSensorDisagreement=0.40;
    double MaximumModelResidualMm=12.0;
    bool RequireOriginalRoadSensor=true;
    bool RequirePhysicalDrainageGeometry=true;
    bool NeverInventUnmeasuredWeather=true;
};
struct RoadWaterCellProbe final
{
    RaceIdentity Identity{};
    int CellIndex=-1;
    double DistanceAlongOriginalCircuitCm=0.0;
    double PhysicsClockSeconds=0.0;
    double RainSensorClockSeconds=0.0;
    double WaterSensorClockSeconds=0.0;
    double MeasuredRainMmHour=0.0;
    double MeasuredWaterDepthMm=0.0;
    double MeasuredRoadFriction=1.0;
    double MeasuredDryFriction=1.15;
    double MeasuredAirTemperatureC=20.0;
    double MeasuredSurfaceTemperatureC=30.0;
    double WindSpeedCmS=0.0;
    double RoadGradeRadians=0.0;
    double RoadBankRadians=0.0;
    double RoadNormalZ=1.0;
    double DrainEfficiency=0.5;
    double SurfacePorosity=0.2;
    double CrossFallFraction=0.05;
    double ActualDrainageFlowMmSecond=0.0;
    double ActualRoadSprayMmSecond=0.0;
    double SensorConfidence=1.0;
    std::uint64_t OriginalRoadComponent=0;
    std::uint64_t PhysicalDrainComponent=0;
    bool OriginalMapContact=true;
    bool RealRainProbeReady=true;
    bool RealWaterProbeReady=true;
    bool RealDrainageObserved=true;
    bool CollisionRoadQualified=true;
};
struct RoadWaterFrame final
{
    RaceIdentity Identity{};
    std::array<RoadWaterCellProbe,RoadWaterCellCount> Probes{};
    int ProbeCount=0;
    double DeltaSeconds=0.0;
    double RaceClockSeconds=0.0;
    double ActualVehicleSpeedCmS=0.0;
    double ActualWheelTreadFraction=1.0;
    bool EngineRaceRunning=false;
    bool OriginalCircuitVerified=false;
    bool RealEnvironmentSensorReady=false;
};
struct RoadWaterCellResult final
{
    int CellIndex=-1;
    double DistanceAlongCircuitCm=0.0;
    double ObservedWaterMm=0.0;
    double ObservedRainMmHour=0.0;
    double ObservedWetGrip=0.0;
    double ObservedDryGrip=0.0;
    double ModeledRainGrowthMm=0.0;
    double ModeledDrainFlowMm=0.0;
    double ModeledRunoffMm=0.0;
    double ModeledEvaporationMm=0.0;
    double ModeledSplashMm=0.0;
    double ConservativeNextWaterMm=0.0;
    double EffectiveGripFraction=0.0;
    double GripLossFraction=0.0;
    double PhysicalHydroplaningRisk=0.0;
    double SensorModelResidualMm=0.0;
    double Confidence=0.0;
    double WaterSensorAgeSeconds=0.0;
    double RainSensorAgeSeconds=0.0;
    double WetRoadExposureSeconds=0.0;
    WaterRoadCondition Condition=WaterRoadCondition::Unobserved;
    bool DrainagePathVerified=false;
    bool PhysicalStandingWater=false;
    bool HazardousForCurrentTread=false;
    bool ActualRoadFrictionChanged=false;
    bool Valid=false;
};
struct RoadWaterSummary final
{
    RaceIdentity Identity{};
    std::array<RoadWaterCellResult,RoadWaterCellCount> Cells{};
    double MaximumPhysicalWaterMm=0.0;
    double MinimumVerifiedWetGrip=0.0;
    double MeanPhysicalRainMmHour=0.0;
    double TotalObservedWaterMm=0.0;
    double MeanDrainageEfficiency=0.0;
    double MaximumHydroplaningRisk=0.0;
    double MinimumSourceConfidence=0.0;
    double MaximumSensorModelResidualMm=0.0;
    double DistanceCoveredByPhysicalProbesCm=0.0;
    double CurrentVehicleHydroplaningThresholdCmS=0.0;
    int VerifiedProbes=0;
    int WetCells=0;
    int FloodedCells=0;
    int SensorConflictCells=0;
    bool OriginalHydrologyObserved=false;
    bool NoUnrealWeatherModified=true;
    bool NoFrictionBoostFromUnobservedWater=true;
    bool Valid=false;
};
inline bool ValidRoadWaterParameters(
    RoadWaterParameters p) noexcept
{
    return Finite(p.CircuitLengthCm) &&
        p.CircuitLengthCm>5000.0 &&
        Finite(p.CellDrainageLengthCm) &&
        p.CellDrainageLengthCm>0.0 &&
        Finite(p.MaximumPhysicalRainMmHour) &&
        p.MaximumPhysicalRainMmHour>0.0 &&
        Finite(p.MaximumMeasuredWaterDepthMm) &&
        p.MaximumMeasuredWaterDepthMm>0.0 &&
        Finite(p.MaximumRainSensorAgeSeconds) &&
        p.MaximumRainSensorAgeSeconds>0.0 &&
        Finite(p.MaximumWaterSensorAgeSeconds) &&
        p.MaximumWaterSensorAgeSeconds>0.0 &&
        Finite(p.MaximumTemperatureC) &&
        Finite(p.MinimumTemperatureC) &&
        p.MaximumTemperatureC>p.MinimumTemperatureC &&
        Finite(p.MinimumRoadFriction) &&
        p.MinimumRoadFriction>0.0 &&
        Finite(p.MaximumRoadFriction) &&
        p.MaximumRoadFriction>p.MinimumRoadFriction &&
        Finite(p.MinimumRoadNormalZ) &&
        p.MinimumRoadNormalZ>0.0 &&
        Finite(p.MaximumRoadGradeRadians) &&
        p.MaximumRoadGradeRadians>0.0 &&
        Finite(p.MaximumBankRadians) &&
        p.MaximumBankRadians>0.0 &&
        Finite(p.MaximumWindCmS) &&
        p.MaximumWindCmS>0.0 &&
        Finite(p.MaximumDrainFlowMmSecond) &&
        p.MaximumDrainFlowMmSecond>0.0 &&
        Finite(p.MaximumRunoffFlowMmSecond) &&
        p.MaximumRunoffFlowMmSecond>0.0 &&
        Finite(p.MaximumEvaporationMmSecond) &&
        p.MaximumEvaporationMmSecond>0.0 &&
        Finite(p.MaximumSplashRemovalMmSecond) &&
        p.MaximumSplashRemovalMmSecond>0.0 &&
        Finite(p.MaximumFilmGrowthMmSecond) &&
        p.MaximumFilmGrowthMmSecond>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0 &&
        Finite(p.MinimumActualWaterSampleSeconds) &&
        p.MinimumActualWaterSampleSeconds>0.0 &&
        Finite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        Finite(p.MinimumSpatialSeparationCm) &&
        p.MinimumSpatialSeparationCm>0.0 &&
        Finite(p.MaximumSpatialSeparationCm) &&
        p.MaximumSpatialSeparationCm>
            p.MinimumSpatialSeparationCm &&
        Finite(p.MinimumDrainEfficiency) &&
        p.MinimumDrainEfficiency>=0.0 &&
        Finite(p.MaximumDrainEfficiency) &&
        p.MaximumDrainEfficiency<=1.0 &&
        Finite(p.MinimumSurfacePorosity) &&
        p.MinimumSurfacePorosity>=0.0 &&
        Finite(p.MaximumSurfacePorosity) &&
        p.MaximumSurfacePorosity<=1.0 &&
        Finite(p.MinimumCrossFallFraction) &&
        p.MinimumCrossFallFraction>=0.0 &&
        Finite(p.MaximumCrossFallFraction) &&
        p.MaximumCrossFallFraction<=1.0 &&
        Finite(p.MinimumWetDepthMm) &&
        p.MinimumWetDepthMm>=0.0 &&
        Finite(p.StandingWaterDepthMm) &&
        p.StandingWaterDepthMm>p.MinimumWetDepthMm &&
        Finite(p.DeepWaterDepthMm) &&
        p.DeepWaterDepthMm>p.StandingWaterDepthMm &&
        Finite(p.FloodedDepthMm) &&
        p.FloodedDepthMm>p.DeepWaterDepthMm &&
        Finite(p.HydroplaningSpeedCmS) &&
        p.HydroplaningSpeedCmS>0.0 &&
        Finite(p.HydroplaningDepthMm) &&
        p.HydroplaningDepthMm>0.0 &&
        Finite(p.MaximumWaterGripPenalty) &&
        p.MaximumWaterGripPenalty>=0.0 &&
        p.MaximumWaterGripPenalty<1.0 &&
        Finite(p.MaximumDryingGripRecoveryPerSecond) &&
        p.MaximumDryingGripRecoveryPerSecond>0.0 &&
        Finite(p.MinimumWetGripFraction) &&
        p.MinimumWetGripFraction>0.0 &&
        p.MinimumWetGripFraction<=1.0 &&
        Finite(p.MaximumFrictionSensorDisagreement) &&
        p.MaximumFrictionSensorDisagreement>=0.0 &&
        Finite(p.MaximumModelResidualMm) &&
        p.MaximumModelResidualMm>0.0;
}
inline bool ValidRoadWaterProbe(
    RoadWaterCellProbe s,
    RoadWaterParameters p) noexcept
{
    return PhysicalActorIdentityValid(s.Identity) &&
        s.Identity.Frame>0 &&
        s.CellIndex>=0 &&
        s.CellIndex<RoadWaterCellCount &&
        (!p.RequireOriginalRoadSensor ||
         (s.OriginalMapContact &&
          s.CollisionRoadQualified &&
          s.RealRainProbeReady &&
          s.RealWaterProbeReady)) &&
        (!p.RequirePhysicalDrainageGeometry ||
         (s.RealDrainageObserved &&
          s.PhysicalDrainComponent!=0)) &&
        s.OriginalRoadComponent!=0 &&
        Finite(s.DistanceAlongOriginalCircuitCm) &&
        s.DistanceAlongOriginalCircuitCm>=0.0 &&
        s.DistanceAlongOriginalCircuitCm<p.CircuitLengthCm &&
        Finite(s.PhysicsClockSeconds) &&
        s.PhysicsClockSeconds>=0.0 &&
        Finite(s.RainSensorClockSeconds) &&
        s.RainSensorClockSeconds>=0.0 &&
        Finite(s.WaterSensorClockSeconds) &&
        s.WaterSensorClockSeconds>=0.0 &&
        s.RainSensorClockSeconds<=s.PhysicsClockSeconds &&
        s.WaterSensorClockSeconds<=s.PhysicsClockSeconds &&
        s.PhysicsClockSeconds-s.RainSensorClockSeconds<=
            p.MaximumRainSensorAgeSeconds &&
        s.PhysicsClockSeconds-s.WaterSensorClockSeconds<=
            p.MaximumWaterSensorAgeSeconds &&
        Finite(s.MeasuredRainMmHour) &&
        s.MeasuredRainMmHour>=0.0 &&
        s.MeasuredRainMmHour<=p.MaximumPhysicalRainMmHour &&
        Finite(s.MeasuredWaterDepthMm) &&
        s.MeasuredWaterDepthMm>=0.0 &&
        s.MeasuredWaterDepthMm<=p.MaximumMeasuredWaterDepthMm &&
        Finite(s.MeasuredRoadFriction) &&
        s.MeasuredRoadFriction>=p.MinimumRoadFriction &&
        s.MeasuredRoadFriction<=p.MaximumRoadFriction &&
        Finite(s.MeasuredDryFriction) &&
        s.MeasuredDryFriction>=p.MinimumRoadFriction &&
        s.MeasuredDryFriction<=p.MaximumRoadFriction &&
        Finite(s.MeasuredAirTemperatureC) &&
        s.MeasuredAirTemperatureC>=p.MinimumTemperatureC &&
        s.MeasuredAirTemperatureC<=p.MaximumTemperatureC &&
        Finite(s.MeasuredSurfaceTemperatureC) &&
        s.MeasuredSurfaceTemperatureC>=p.MinimumTemperatureC &&
        s.MeasuredSurfaceTemperatureC<=p.MaximumTemperatureC &&
        Finite(s.WindSpeedCmS) &&
        s.WindSpeedCmS>=0.0 &&
        s.WindSpeedCmS<=p.MaximumWindCmS &&
        Finite(s.RoadGradeRadians) &&
        std::abs(s.RoadGradeRadians)<=
            p.MaximumRoadGradeRadians &&
        Finite(s.RoadBankRadians) &&
        std::abs(s.RoadBankRadians)<=p.MaximumBankRadians &&
        Finite(s.RoadNormalZ) &&
        s.RoadNormalZ>=p.MinimumRoadNormalZ &&
        s.RoadNormalZ<=1.0 &&
        Finite(s.DrainEfficiency) &&
        s.DrainEfficiency>=p.MinimumDrainEfficiency &&
        s.DrainEfficiency<=p.MaximumDrainEfficiency &&
        Finite(s.SurfacePorosity) &&
        s.SurfacePorosity>=p.MinimumSurfacePorosity &&
        s.SurfacePorosity<=p.MaximumSurfacePorosity &&
        Finite(s.CrossFallFraction) &&
        s.CrossFallFraction>=p.MinimumCrossFallFraction &&
        s.CrossFallFraction<=p.MaximumCrossFallFraction &&
        Finite(s.ActualDrainageFlowMmSecond) &&
        s.ActualDrainageFlowMmSecond>=0.0 &&
        s.ActualDrainageFlowMmSecond<=
            p.MaximumDrainFlowMmSecond &&
        Finite(s.ActualRoadSprayMmSecond) &&
        s.ActualRoadSprayMmSecond>=0.0 &&
        s.ActualRoadSprayMmSecond<=
            p.MaximumSplashRemovalMmSecond &&
        Finite(s.SensorConfidence) &&
        s.SensorConfidence>=p.MinimumSensorConfidence &&
        s.SensorConfidence<=1.0;
}
inline bool ValidRoadWaterFrame(
    const RoadWaterFrame& s,
    RoadWaterParameters p) noexcept
{
    if (!ValidRoadWaterParameters(p) ||
        !PhysicalActorIdentityValid(s.Identity) ||
        s.Identity.Frame==0 ||
        !s.EngineRaceRunning ||
        !s.OriginalCircuitVerified ||
        !s.RealEnvironmentSensorReady ||
        s.ProbeCount<1 ||
        s.ProbeCount>RoadWaterCellCount ||
        !Finite(s.DeltaSeconds) ||
        s.DeltaSeconds<=0.0 ||
        s.DeltaSeconds>p.MaximumFrameSeconds ||
        !Finite(s.RaceClockSeconds) ||
        s.RaceClockSeconds<0.0 ||
        !Finite(s.ActualVehicleSpeedCmS) ||
        s.ActualVehicleSpeedCmS<0.0 ||
        s.ActualVehicleSpeedCmS>MaximumPhysicalSpeedCmS ||
        !Finite(s.ActualWheelTreadFraction) ||
        s.ActualWheelTreadFraction<0.0 ||
        s.ActualWheelTreadFraction>1.0)
        return false;
    std::array<bool,RoadWaterCellCount> visited{};
    for(int i=0;i<s.ProbeCount;++i)
    {
        const auto& c=s.Probes[static_cast<std::size_t>(i)];
        if (!ValidRoadWaterProbe(c,p) ||
            c.Identity.Session!=s.Identity.Session ||
            c.Identity.SessionRevision!=s.Identity.SessionRevision ||
            c.Identity.Actor!=s.Identity.Actor ||
            c.Identity.Frame!=s.Identity.Frame ||
            std::abs(c.PhysicsClockSeconds-
                     s.RaceClockSeconds)>
                p.MaximumClockSkewSeconds ||
            visited[static_cast<std::size_t>(c.CellIndex)])
            return false;
        visited[static_cast<std::size_t>(c.CellIndex)]=true;
    }
    return true;
}
inline RoadWaterCellResult EvaluateRoadWaterCell(
    RoadWaterCellProbe sample,
    double vehicleSpeedCmS,double treadFraction,
    double dt,RoadWaterParameters p) noexcept
{
    RoadWaterCellResult out;
    if (!ValidRoadWaterProbe(sample,p) ||
        !Finite(vehicleSpeedCmS) ||
        vehicleSpeedCmS<0.0 ||
        vehicleSpeedCmS>MaximumPhysicalSpeedCmS ||
        !Finite(treadFraction) ||
        treadFraction<0.0 ||
        treadFraction>1.0 ||
        !Finite(dt) ||
        dt<=0.0 ||
        dt>p.MaximumFrameSeconds)
        return out;
    out.CellIndex=sample.CellIndex;
    out.DistanceAlongCircuitCm=
        sample.DistanceAlongOriginalCircuitCm;
    out.ObservedWaterMm=sample.MeasuredWaterDepthMm;
    out.ObservedRainMmHour=sample.MeasuredRainMmHour;
    out.ObservedWetGrip=sample.MeasuredRoadFriction;
    out.ObservedDryGrip=sample.MeasuredDryFriction;
    out.RainSensorAgeSeconds=
        sample.PhysicsClockSeconds-
        sample.RainSensorClockSeconds;
    out.WaterSensorAgeSeconds=
        sample.PhysicsClockSeconds-
        sample.WaterSensorClockSeconds;
    out.Confidence=sample.SensorConfidence;
    out.DrainagePathVerified=sample.PhysicalDrainComponent!=0 &&
        sample.RealDrainageObserved;
    out.ModeledRainGrowthMm=
        sample.MeasuredRainMmHour/3600.0*dt;
    out.ModeledDrainFlowMm=std::min(
        p.MaximumDrainFlowMmSecond,
        sample.ActualDrainageFlowMmSecond*
        sample.DrainEfficiency)*dt;
    out.ModeledRunoffMm=std::min(
        p.MaximumRunoffFlowMmSecond,
        std::abs(std::sin(sample.RoadBankRadians))*
            p.MaximumRunoffFlowMmSecond*
            sample.CrossFallFraction+
        std::abs(std::sin(sample.RoadGradeRadians))*
            p.MaximumRunoffFlowMmSecond*0.25)*dt;
    const double temperatureFactor=Clamp01(
        SafeDivision(
            sample.MeasuredSurfaceTemperatureC+10.0,
            p.MaximumTemperatureC+10.0));
    const double windFactor=Clamp01(SafeDivision(
        sample.WindSpeedCmS,p.MaximumWindCmS));
    out.ModeledEvaporationMm=
        p.MaximumEvaporationMmSecond*
        (0.5*temperatureFactor+0.5*windFactor)*dt;
    out.ModeledSplashMm=std::min(
        p.MaximumSplashRemovalMmSecond,
        sample.ActualRoadSprayMmSecond)*dt;
    const double modeled=std::max(0.0,
        sample.MeasuredWaterDepthMm+
        out.ModeledRainGrowthMm-
        out.ModeledDrainFlowMm-
        out.ModeledRunoffMm-
        out.ModeledEvaporationMm-
        out.ModeledSplashMm);
    out.ConservativeNextWaterMm=std::max(
        sample.MeasuredWaterDepthMm,modeled);
    out.SensorModelResidualMm=std::abs(
        modeled-sample.MeasuredWaterDepthMm);
    const double normalizedWater=Clamp01(SafeDivision(
        sample.MeasuredWaterDepthMm,
        p.FloodedDepthMm));
    out.GripLossFraction=std::min(
        p.MaximumWaterGripPenalty,
        p.MaximumWaterGripPenalty*normalizedWater);
    out.EffectiveGripFraction=std::max(
        p.MinimumWetGripFraction,
        SafeDivision(sample.MeasuredRoadFriction,
                     sample.MeasuredDryFriction)*
            (1.0-out.GripLossFraction));
    const double hydroSpeedFactor=Clamp01(SafeDivision(
        vehicleSpeedCmS-p.HydroplaningSpeedCmS,
        p.HydroplaningSpeedCmS));
    const double hydroDepthFactor=Clamp01(SafeDivision(
        sample.MeasuredWaterDepthMm-p.HydroplaningDepthMm,
        p.DeepWaterDepthMm-p.HydroplaningDepthMm));
    out.PhysicalHydroplaningRisk=Clamp01(
        hydroSpeedFactor*hydroDepthFactor*
        (1.0-0.6*treadFraction));
    out.PhysicalStandingWater=
        sample.MeasuredWaterDepthMm>=
            p.StandingWaterDepthMm;
    out.HazardousForCurrentTread=
        out.PhysicalHydroplaningRisk>0.1 ||
        sample.MeasuredWaterDepthMm>=p.DeepWaterDepthMm;
    out.Condition=sample.MeasuredWaterDepthMm>=
        p.FloodedDepthMm ?
        WaterRoadCondition::Flooded:
        sample.MeasuredWaterDepthMm>=p.DeepWaterDepthMm ?
            WaterRoadCondition::DeepWater:
        sample.MeasuredWaterDepthMm>=
            p.StandingWaterDepthMm ?
            WaterRoadCondition::StandingWater:
        sample.MeasuredWaterDepthMm>=p.MinimumWetDepthMm ?
            WaterRoadCondition::Wet:
        sample.MeasuredRainMmHour>0.0 ?
            WaterRoadCondition::Damp:
            WaterRoadCondition::Dry;
    if (out.SensorModelResidualMm>
        p.MaximumModelResidualMm)
        out.Condition=WaterRoadCondition::SensorDisagreement;
    out.ActualRoadFrictionChanged=false;
    out.Valid=Finite(out.ConservativeNextWaterMm) &&
        Finite(out.EffectiveGripFraction) &&
        Finite(out.PhysicalHydroplaningRisk);
    return out;
}
inline RoadWaterSummary EvaluateMeasuredRoadHydrology(
    const RoadWaterFrame& frame,
    RoadWaterParameters p={}) noexcept
{
    RoadWaterSummary out;
    if (!ValidRoadWaterFrame(frame,p))
        return out;
    out.Identity=frame.Identity;
    out.MinimumVerifiedWetGrip=
        std::numeric_limits<double>::infinity();
    out.MinimumSourceConfidence=1.0;
    for(int i=0;i<frame.ProbeCount;++i)
    {
        const auto& sample=frame.Probes[
            static_cast<std::size_t>(i)];
        const RoadWaterCellResult result=
            EvaluateRoadWaterCell(
                sample,frame.ActualVehicleSpeedCmS,
                frame.ActualWheelTreadFraction,
                frame.DeltaSeconds,p);
        if(!result.Valid)return {};
        out.Cells[static_cast<std::size_t>(
            sample.CellIndex)]=result;
        ++out.VerifiedProbes;
        out.MinimumVerifiedWetGrip=std::min(
            out.MinimumVerifiedWetGrip,
            result.ObservedWetGrip);
        out.MinimumSourceConfidence=std::min(
            out.MinimumSourceConfidence,
            result.Confidence);
        out.MaximumPhysicalWaterMm=std::max(
            out.MaximumPhysicalWaterMm,
            result.ObservedWaterMm);
        out.MaximumHydroplaningRisk=std::max(
            out.MaximumHydroplaningRisk,
            result.PhysicalHydroplaningRisk);
        out.MaximumSensorModelResidualMm=std::max(
            out.MaximumSensorModelResidualMm,
            result.SensorModelResidualMm);
        out.MeanPhysicalRainMmHour+=
            result.ObservedRainMmHour;
        out.TotalObservedWaterMm+=
            result.ObservedWaterMm;
        out.MeanDrainageEfficiency+=
            sample.DrainEfficiency;
        out.WetCells+=result.PhysicalStandingWater?1:0;
        out.FloodedCells+=
            result.Condition==WaterRoadCondition::Flooded?1:0;
        out.SensorConflictCells+=
            result.Condition==
                WaterRoadCondition::SensorDisagreement?1:0;
    }
    out.MeanPhysicalRainMmHour/=out.VerifiedProbes;
    out.MeanDrainageEfficiency/=out.VerifiedProbes;
    out.DistanceCoveredByPhysicalProbesCm=
        out.VerifiedProbes*
        p.CircuitLengthCm/RoadWaterCellCount;
    out.CurrentVehicleHydroplaningThresholdCmS=
        p.HydroplaningSpeedCmS*
        (0.4+0.6*frame.ActualWheelTreadFraction);
    out.OriginalHydrologyObserved=
        out.VerifiedProbes==frame.ProbeCount;
    out.NoUnrealWeatherModified=true;
    out.NoFrictionBoostFromUnobservedWater=true;
    out.Valid=out.VerifiedProbes>0 &&
        Finite(out.MinimumVerifiedWetGrip) &&
        Finite(out.MeanPhysicalRainMmHour) &&
        Finite(out.MaximumHydroplaningRisk);
    return out;
}
class RoadHydrologySupervisor final
{
public:
    bool Configure(RoadWaterParameters p) noexcept
    {
        if(!ValidRoadWaterParameters(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Identity={};
        HasFrame=false;
        Accepted=0;
        Rejected=0;
        WetFrames=0;
        FloodFrames=0;
        PeakWaterMm=0.0;
        PeakHydroRisk=0.0;
        AccumulatedWaterExposureMmSeconds=0.0;
    }
    RoadWaterSummary Step(
        const RoadWaterFrame& input) noexcept
    {
        const RoadWaterSummary result=
            EvaluateMeasuredRoadHydrology(input,Settings);
        const bool sameEpoch=HasFrame &&
            Identity.Session==input.Identity.Session &&
            Identity.SessionRevision==
                input.Identity.SessionRevision &&
            Identity.Actor==input.Identity.Actor;
        if(!result.Valid ||
           (sameEpoch &&
            input.Identity.Frame<=Identity.Frame))
        {
            ++Rejected;
            Last={};
            return {};
        }
        Last=result;
        Identity=input.Identity;
        HasFrame=true;
        ++Accepted;
        WetFrames+=result.WetCells>0?1U:0U;
        FloodFrames+=result.FloodedCells>0?1U:0U;
        PeakWaterMm=std::max(
            PeakWaterMm,result.MaximumPhysicalWaterMm);
        PeakHydroRisk=std::max(
            PeakHydroRisk,result.MaximumHydroplaningRisk);
        AccumulatedWaterExposureMmSeconds+=
            result.MaximumPhysicalWaterMm*
            input.DeltaSeconds;
        return Last;
    }
    RoadWaterSummary Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return Accepted;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t WetSamples() const noexcept{return WetFrames;}
    std::uint64_t FloodSamples() const noexcept{return FloodFrames;}
    double MaximumObservedWaterDepthMm() const noexcept
    {return PeakWaterMm;}
    double MaximumObservedHydroplaningRisk() const noexcept
    {return PeakHydroRisk;}
    double WaterExposureMmSeconds() const noexcept
    {return AccumulatedWaterExposureMmSeconds;}
private:
    RoadWaterParameters Settings{};
    RoadWaterSummary Last{};
    RaceIdentity Identity{};
    std::uint64_t Accepted=0;
    std::uint64_t Rejected=0;
    std::uint64_t WetFrames=0;
    std::uint64_t FloodFrames=0;
    double PeakWaterMm=0.0;
    double PeakHydroRisk=0.0;
    double AccumulatedWaterExposureMmSeconds=0.0;
    bool HasFrame=false;
};
} // namespace latebrake::racecraft
