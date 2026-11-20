#pragma once
#include "Track/WorldScaleBankedPacePlan.h"
#include "Track/WorldScaleObservedWaterFilm.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// A six-circuit source-only pace proposal combining genuinely measured
// friction with real road-water observations. Film DEPTH is never used
// as a substitute for a physical tire/road friction measurement.
// This routine never writes Chaos traction, AI throttle or official times.
inline constexpr std::size_t WetGripProbeCount=ObservedWaterCellCount;
inline constexpr std::size_t WetGripPaceNodeCount=BankedPaceCapacity;
enum class WetGripPaceFault : std::uint8_t {
    None, BadBaseline, MissingFrictionWitness,
    InvalidSettings, SourceEpochChanged,
    UntrustedWater, InfeasibleBraking
};
struct WetGripPaceSettings final {
    double MinimumGrip=0.30;
    double MaximumGrip=1.00;
    double MinimumConfidence=0.85;
    double MaximumMeasurementAgeSeconds=2.0;
    double MaximumClockSkewSeconds=0.05;
    double MinimumAcceptedSpeedCmps=200.0;
    double MaximumAdvisoryLapSeconds=3600.0;
    bool RequireActualMeasuredFriction=true;
    bool RequireActualRoadContact=true;
    bool NeverApplyNativeControls=true;
    bool NeverAwardOfficialRaceTimes=true;
};
struct WetGripProbe final {
    double StationCm=0.0;
    double MeasuredGrip=1.0;
    double Confidence=0.0;
    double ClockSeconds=0.0;
    std::uint64_t RoadBody=0u;
    bool NativeTireContactObserved=false;
    bool TrueFrictionMeasurement=false;
};
struct WetGripPaceInput final {
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t Frame=0u;
    double ClockSeconds=0.0;
    bool All256StationsMeasured=false;
    bool AuthenticRoadBodyVerified=false;
    std::array<WetGripProbe,WetGripProbeCount> Probes{};
};
struct WetGripPaceNode final {
    double StationCm=0.0;
    double Grip=0.0;
    double ObservedFilmMm=0.0;
    double OriginalDryTargetCmps=0.0;
    double LocalCapCmps=0.0;
    double TargetCmps=0.0;
    double BrakingCmps2=0.0;
    double DrivingCmps2=0.0;
    bool NoNativeActuation=true;
};
struct WetGripPacePlan final {
    bool Valid=false;
    WetGripPaceFault Fault=WetGripPaceFault::MissingFrictionWitness;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t SourceFrame=0u;
    double ClockSeconds=0.0;
    double NodeLengthCm=0.0;
    double MinimumGrip=1.0;
    double MaximumGrip=0.0;
    double MaximumObservedFilmMm=0.0;
    double MinimumTargetCmps=0.0;
    double MaximumTargetCmps=0.0;
    double EstimatedLapSeconds=0.0;
    unsigned ReducedNodes=0u;
    unsigned RelaxationPasses=0u;
    std::array<WetGripPaceNode,WetGripPaceNodeCount> Nodes{};
    bool OriginalDryPaceUntouched=true;
    bool NativePhysicsUntouched=true;
    bool RaceAuthorityUntouched=true;
};
inline bool ValidWetGripPaceSettings(WetGripPaceSettings s) noexcept {
    return std::isfinite(s.MinimumGrip) &&
        s.MinimumGrip>=0.2 && s.MinimumGrip<=0.7 &&
        std::isfinite(s.MaximumGrip) &&
        s.MaximumGrip>=0.8 && s.MaximumGrip<=1.0 &&
        s.MinimumGrip<s.MaximumGrip &&
        std::isfinite(s.MinimumConfidence) &&
        s.MinimumConfidence>=0.5 && s.MinimumConfidence<=1.0 &&
        std::isfinite(s.MaximumMeasurementAgeSeconds) &&
        s.MaximumMeasurementAgeSeconds>0.0 &&
        s.MaximumMeasurementAgeSeconds<=10.0 &&
        std::isfinite(s.MaximumClockSkewSeconds) &&
        s.MaximumClockSkewSeconds>=0.0 &&
        s.MaximumClockSkewSeconds<=0.1 &&
        std::isfinite(s.MinimumAcceptedSpeedCmps) &&
        s.MinimumAcceptedSpeedCmps>=100.0 &&
        s.MinimumAcceptedSpeedCmps<=400.0 &&
        std::isfinite(s.MaximumAdvisoryLapSeconds) &&
        s.MaximumAdvisoryLapSeconds>=300.0 &&
        s.MaximumAdvisoryLapSeconds<=3600.0 &&
        s.RequireActualMeasuredFriction &&
        s.RequireActualRoadContact &&
        s.NeverApplyNativeControls &&
        s.NeverAwardOfficialRaceTimes;
}
inline bool ValidWetGripPaceSource(
    const WorldScaleVariantRecipe& circuit,
    const BankedPacePlan& dry,
    const ObservedWaterResult& water,
    const WetGripPaceInput& input,
    WetGripPaceSettings settings={}) noexcept
{
    if(!ValidWetGripPaceSettings(settings) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       !dry.Valid || dry.Circuit!=circuit.Id ||
       !dry.SourceOnly || !dry.OfficialTimingUnchanged ||
       !water.Valid || water.Circuit!=circuit.Id ||
       !water.RenderAdvisoryOnly || !water.OriginalRaceUnaffected ||
       !std::isfinite(water.ClockSeconds) ||
       input.Circuit!=circuit.Id ||
       input.Session==0u || input.Revision==0u ||
       input.Frame==0u ||
       input.Session!=water.Session ||
       input.Revision!=water.Revision ||
       input.Frame!=water.SourceFrame ||
       !input.All256StationsMeasured ||
       !input.AuthenticRoadBodyVerified ||
       !std::isfinite(input.ClockSeconds) ||
       std::abs(input.ClockSeconds-water.ClockSeconds)>
           settings.MaximumClockSkewSeconds)
        return false;
    const double spacing=circuit.LapLengthCm/
        static_cast<double>(WetGripProbeCount);
    for(std::size_t i=0u;i<WetGripProbeCount;++i) {
        const auto& p=input.Probes[i];
        const double age=input.ClockSeconds-p.ClockSeconds;
        if(!std::isfinite(p.StationCm) ||
           !std::isfinite(p.MeasuredGrip) ||
           !std::isfinite(p.Confidence) ||
           !std::isfinite(p.ClockSeconds) ||
           !std::isfinite(age) ||
           std::abs(p.StationCm-i*spacing)>0.1 ||
           p.MeasuredGrip<settings.MinimumGrip ||
           p.MeasuredGrip>settings.MaximumGrip ||
           p.Confidence<settings.MinimumConfidence ||
           p.Confidence>1.0 ||
           age<0.0 ||
           age>settings.MaximumMeasurementAgeSeconds ||
           p.RoadBody==0u ||
           !p.NativeTireContactObserved ||
           !p.TrueFrictionMeasurement)
            return false;
    }
    return true;
}
// The minimum of adjacent real probes is used rather than a wetness
// extrapolation, so an unmeasured high-grip island is never invented.
inline double ConservativeWetGripAt(
    const WetGripPaceInput& observed,
    std::size_t paceNode) noexcept
{
    const std::size_t first=
        (paceNode*WetGripProbeCount)/WetGripPaceNodeCount;
    const auto next=(first+1u)%WetGripProbeCount;
    return std::min(observed.Probes[first].MeasuredGrip,
                    observed.Probes[next].MeasuredGrip);
}
inline WetGripPacePlan BuildMeasuredWetGripPace(
    const WorldScaleVariantRecipe& circuit,
    const BankedPacePlan& dry,
    const ObservedWaterResult& water,
    const WetGripPaceInput& observed,
    WetGripPaceSettings settings={}) noexcept
{
    WetGripPacePlan out;
    if(!ValidWetGripPaceSettings(settings)){
        out.Fault=WetGripPaceFault::InvalidSettings;
        return out;
    }
    if(!dry.Valid || dry.Circuit!=circuit.Id){
        out.Fault=WetGripPaceFault::BadBaseline;
        return out;
    }
    if(!water.Valid || water.Circuit!=circuit.Id ||
       !water.RenderAdvisoryOnly){
        out.Fault=WetGripPaceFault::UntrustedWater;
        return out;
    }
    if(observed.Session!=water.Session ||
       observed.Revision!=water.Revision ||
       observed.Frame!=water.SourceFrame){
        out.Fault=WetGripPaceFault::SourceEpochChanged;
        return out;
    }
    if(!ValidWetGripPaceSource(
        circuit,dry,water,observed,settings))return out;
    out.Circuit=circuit.Id;
    out.Session=observed.Session;
    out.Revision=observed.Revision;
    out.SourceFrame=observed.Frame;
    out.ClockSeconds=observed.ClockSeconds;
    out.NodeLengthCm=circuit.LapLengthCm/
        static_cast<double>(WetGripPaceNodeCount);
    std::array<double,WetGripPaceNodeCount> squared{};
    for(std::size_t i=0u;i<WetGripPaceNodeCount;++i){
        const auto& baseline=dry.Nodes[i];
        if(!std::isfinite(baseline.TargetSpeedCmps) ||
           !std::isfinite(baseline.LocalSpeedCapCmps) ||
           !std::isfinite(baseline.GuaranteedBrakingCmps2) ||
           !std::isfinite(baseline.GuaranteedDriveCmps2) ||
           baseline.TargetSpeedCmps<
               settings.MinimumAcceptedSpeedCmps){
            out.Fault=WetGripPaceFault::BadBaseline;
            return out;
        }
        const double grip=ConservativeWetGripAt(observed,i);
        const double station=i*out.NodeLengthCm;
        const auto wet=SampleObservedWaterFilm(
            circuit,water,station);
        if(!wet.Valid){
            out.Fault=WetGripPaceFault::UntrustedWater;
            return out;
        }
        const double cap=std::min(
            baseline.TargetSpeedCmps,
            baseline.LocalSpeedCapCmps*std::sqrt(grip));
        const double braking=baseline.GuaranteedBrakingCmps2*grip;
        const double driving=baseline.GuaranteedDriveCmps2*grip;
        if(!std::isfinite(cap) ||
           !std::isfinite(braking) ||
           !std::isfinite(driving) ||
           cap<settings.MinimumAcceptedSpeedCmps ||
           braking<=0.0 || driving<=0.0){
            out.Fault=WetGripPaceFault::InfeasibleBraking;
            return out;
        }
        out.Nodes[i]={station,grip,wet.FilmMm,
            baseline.TargetSpeedCmps,cap,cap,braking,driving,true};
        squared[i]=cap*cap;
        out.MinimumGrip=std::min(out.MinimumGrip,grip);
        out.MaximumGrip=std::max(out.MaximumGrip,grip);
        out.MaximumObservedFilmMm=std::max(
            out.MaximumObservedFilmMm,wet.FilmMm);
    }
    bool settled=false;
    for(unsigned pass=0u;pass<WetGripPaceNodeCount;++pass){
        bool changed=false;
        for(std::size_t i=WetGripPaceNodeCount;i-- >0u;){
            const auto next=(i+1u)%WetGripPaceNodeCount;
            const double brake=std::min(
                out.Nodes[i].BrakingCmps2,
                out.Nodes[next].BrakingCmps2);
            const double allowed=squared[next]+
                2.0*out.NodeLengthCm*brake;
            if(squared[i]>allowed){
                squared[i]=allowed;
                changed=true;
            }
        }
        for(std::size_t i=0u;i<WetGripPaceNodeCount;++i){
            const auto next=(i+1u)%WetGripPaceNodeCount;
            const double drive=std::min(
                out.Nodes[i].DrivingCmps2,
                out.Nodes[next].DrivingCmps2);
            const double allowed=squared[i]+
                2.0*out.NodeLengthCm*drive;
            if(squared[next]>allowed){
                squared[next]=allowed;
                changed=true;
            }
        }
        out.RelaxationPasses=pass+1u;
        if(!changed){settled=true;break;}
    }
    if(!settled){
        out.Fault=WetGripPaceFault::InfeasibleBraking;
        return out;
    }
    out.MinimumTargetCmps=std::numeric_limits<double>::infinity();
    double lap=0.0;
    for(std::size_t i=0u;i<WetGripPaceNodeCount;++i){
        const auto next=(i+1u)%WetGripPaceNodeCount;
        const double v=std::sqrt(squared[i]);
        const double ahead=std::sqrt(squared[next]);
        const double brake=std::min(
            out.Nodes[i].BrakingCmps2,
            out.Nodes[next].BrakingCmps2);
        const double drive=std::min(
            out.Nodes[i].DrivingCmps2,
            out.Nodes[next].DrivingCmps2);
        if(!std::isfinite(v) ||
           v<settings.MinimumAcceptedSpeedCmps ||
           v>out.Nodes[i].LocalCapCmps+1e-6 ||
           squared[i]>squared[next]+
               2.0*out.NodeLengthCm*brake+0.01 ||
           squared[next]>squared[i]+
               2.0*out.NodeLengthCm*drive+0.01){
            out.Fault=WetGripPaceFault::InfeasibleBraking;
            return out;
        }
        out.Nodes[i].TargetCmps=v;
        out.MinimumTargetCmps=std::min(out.MinimumTargetCmps,v);
        out.MaximumTargetCmps=std::max(out.MaximumTargetCmps,v);
        if(v<out.Nodes[i].OriginalDryTargetCmps-1e-6)
            ++out.ReducedNodes;
        lap+=2.0*out.NodeLengthCm/(v+ahead);
    }
    if(!std::isfinite(lap) ||
       lap<dry.EstimatedGeometricLapSeconds-1e-5 ||
       lap>settings.MaximumAdvisoryLapSeconds){
        out.Fault=WetGripPaceFault::InfeasibleBraking;
        return out;
    }
    out.EstimatedLapSeconds=lap;
    out.Fault=WetGripPaceFault::None;
    out.Valid=true;
    return out;
}
struct WetGripPaceReference final {
    bool Valid=false;
    double WrappedStationCm=0.0;
    double TargetSpeedCmps=0.0;
    double MinimumMeasuredGrip=0.0;
    double ObservedFilmMm=0.0;
    bool SourceOnly=true;
    bool NoRaceResultAuthority=true;
};
inline WetGripPaceReference SampleMeasuredWetGripPace(
    const WorldScaleVariantRecipe& circuit,
    const WetGripPacePlan& plan,double station) noexcept
{
    if(!plan.Valid || plan.Circuit!=circuit.Id ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       !std::isfinite(station) || std::abs(station)>1.0e9 ||
       !std::isfinite(plan.NodeLengthCm) ||
       plan.NodeLengthCm<=0.0)return {};
    const double wrapped=WorldScaleVariantWrapDistance(
        circuit,station);
    if(!std::isfinite(wrapped))return {};
    const auto i=std::min(
        static_cast<std::size_t>(wrapped/plan.NodeLengthCm),
        WetGripPaceNodeCount-1u);
    const auto next=(i+1u)%WetGripPaceNodeCount;
    const double fraction=std::clamp(
        (wrapped-i*plan.NodeLengthCm)/plan.NodeLengthCm,
        0.0,1.0);
    const auto& a=plan.Nodes[i];
    const auto& b=plan.Nodes[next];
    const double v2=(1.0-fraction)*a.TargetCmps*a.TargetCmps+
        fraction*b.TargetCmps*b.TargetCmps;
    const double v=std::sqrt(v2);
    if(!std::isfinite(v) || v<=0.0)return {};
    return {true,wrapped,v,std::min(a.Grip,b.Grip),
        (1.0-fraction)*a.ObservedFilmMm+
        fraction*b.ObservedFilmMm,true,true};
}
} // namespace latebrake
