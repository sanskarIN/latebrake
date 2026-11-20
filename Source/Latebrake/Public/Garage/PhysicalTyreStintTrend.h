#pragma once
#include "Garage/PhysicalTyreSetupTelemetry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Bounded, run-local telemetry trend over authentic ORIGINAL physical
// wheel pressure, tread and thermal observations. Not an engine input,
// replacement tire life model or official maintenance award.
inline constexpr std::size_t PhysicalTyreTrendCapacity=64u;
inline constexpr unsigned PhysicalTyreTrendWheels=TyreSetupWheelCount;
enum class PhysicalTyreTrendFault : std::uint8_t {
    None, BadSettings, InvalidSource, RosterChanged,
    StaleFrame, TimeRollback, UnrealisticSensorRate,
    MissingBaseline
};
struct PhysicalTyreTrendSettings final {
    double MinimumStepSeconds=0.005;
    double MaximumStepSeconds=0.20;
    unsigned MaximumSkippedFrames=8u;
    double MaximumPressureChangeKpaPerSecond=45.0;
    double MaximumCarcassChangeCPerSecond=28.0;
    double MaximumSurfaceChangeCPerSecond=80.0;
    double MaximumTreadWearMmPerSecond=0.5;
    double MinimumTreadRemainingMm=0.4;
    double MaximumSourceClockSkewSeconds=0.05;
    double WarningWearRateMmPerSecond=0.04;
    double WarningHeatRateCPerSecond=5.0;
    double WarningPressureGainKpaPerSecond=8.0;
    bool RequireOriginalFourWheelRoster=true;
    bool NeverApplyPressureCorrection=true;
    bool NeverWriteTyreWear=true;
    bool NeverAwardPitService=true;
};
struct PhysicalTyreTrendSample final {
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t OwnerActor=0u;
    std::uint64_t Frame=0u;
    double ActiveClockSeconds=0.0;
    double StepSeconds=0.0;
    int OriginalVehicleId=-1;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::array<TyreSetupWheelWitness,PhysicalTyreTrendWheels> Wheels{};
    bool OriginalEngineTyresObserved=false;
    bool NoUnrealForceChanges=true;
};
struct PhysicalTyreTrendWheel final {
    bool Valid=false;
    std::uint64_t NativeWheel=0u;
    unsigned WheelIndex=0u;
    double FirstPressureKpa=0.0;
    double LatestPressureKpa=0.0;
    double FirstCarcassTemperatureC=0.0;
    double LatestCarcassTemperatureC=0.0;
    double FirstTreadMm=0.0;
    double LatestTreadMm=0.0;
    double MeanPressureKpa=0.0;
    double MeanCarcassTemperatureC=0.0;
    double MeanSurfaceTemperatureC=0.0;
    double PressureGainKpaPerSecond=0.0;
    double CarcassGainCPerSecond=0.0;
    double TreadWearMmPerSecond=0.0;
    double PeakCarcassTemperatureC=0.0;
    double MinimumTreadMm=0.0;
    bool HeatGainWarning=false;
    bool WearWarning=false;
    bool PressureGainWarning=false;
    bool NoPhysicalTireModified=true;
};
struct PhysicalTyreTrendSummary final {
    bool Valid=false;
    PhysicalTyreTrendFault Fault=PhysicalTyreTrendFault::MissingBaseline;
    std::uint64_t Session=0u;
    std::uint64_t Revision=0u;
    std::uint64_t OwnerActor=0u;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t FirstFrame=0u;
    std::uint64_t LastFrame=0u;
    double FirstClockSeconds=0.0;
    double LastClockSeconds=0.0;
    double ObservedDurationSeconds=0.0;
    unsigned StoredSourceFrames=0u;
    unsigned RejectedFrames=0u;
    unsigned WheelsAtHeatRisk=0u;
    unsigned WheelsAtWearRisk=0u;
    double PeakObservedCarcassC=0.0;
    double MinimumObservedTreadMm=0.0;
    std::array<PhysicalTyreTrendWheel,PhysicalTyreTrendWheels> Wheels{};
    bool NoUnrealTyreCorrectionSubmitted=true;
    bool NoCareerPitCreditsAwarded=true;
};
inline bool ValidPhysicalTyreTrendSettings(
    PhysicalTyreTrendSettings p) noexcept
{
    const auto finite=[](double x) noexcept {
        return std::isfinite(x);
    };
    return finite(p.MinimumStepSeconds) &&
        p.MinimumStepSeconds>=0.001 &&
        p.MinimumStepSeconds<=0.02 &&
        finite(p.MaximumStepSeconds) &&
        p.MaximumStepSeconds>=0.05 &&
        p.MaximumStepSeconds<=0.20 &&
        p.MaximumStepSeconds>p.MinimumStepSeconds &&
        p.MaximumSkippedFrames>=1u &&
        p.MaximumSkippedFrames<=16u &&
        finite(p.MaximumPressureChangeKpaPerSecond) &&
        p.MaximumPressureChangeKpaPerSecond>=10.0 &&
        p.MaximumPressureChangeKpaPerSecond<=100.0 &&
        finite(p.MaximumCarcassChangeCPerSecond) &&
        p.MaximumCarcassChangeCPerSecond>=10.0 &&
        p.MaximumCarcassChangeCPerSecond<=75.0 &&
        finite(p.MaximumSurfaceChangeCPerSecond) &&
        p.MaximumSurfaceChangeCPerSecond>=20.0 &&
        p.MaximumSurfaceChangeCPerSecond<=150.0 &&
        finite(p.MaximumTreadWearMmPerSecond) &&
        p.MaximumTreadWearMmPerSecond>=0.05 &&
        p.MaximumTreadWearMmPerSecond<=1.0 &&
        finite(p.MinimumTreadRemainingMm) &&
        p.MinimumTreadRemainingMm>=0.3 &&
        p.MinimumTreadRemainingMm<=1.0 &&
        finite(p.MaximumSourceClockSkewSeconds) &&
        p.MaximumSourceClockSkewSeconds>=0.0 &&
        p.MaximumSourceClockSkewSeconds<=0.10 &&
        finite(p.WarningWearRateMmPerSecond) &&
        p.WarningWearRateMmPerSecond>=0.01 &&
        p.WarningWearRateMmPerSecond<=0.10 &&
        finite(p.WarningHeatRateCPerSecond) &&
        p.WarningHeatRateCPerSecond>=1.0 &&
        p.WarningHeatRateCPerSecond<=10.0 &&
        finite(p.WarningPressureGainKpaPerSecond) &&
        p.WarningPressureGainKpaPerSecond>=2.0 &&
        p.WarningPressureGainKpaPerSecond<=15.0 &&
        p.RequireOriginalFourWheelRoster &&
        p.NeverApplyPressureCorrection &&
        p.NeverWriteTyreWear &&
        p.NeverAwardPitService;
}
inline bool ValidPhysicalTyreTrendSample(
    PhysicalTyreTrendSample sample,
    PhysicalTyreTrendSettings settings={}) noexcept
{
    if(!ValidPhysicalTyreTrendSettings(settings) ||
       sample.Session==0u || sample.Revision==0u ||
       sample.OwnerActor==0u || sample.Frame==0u ||
       !GarageState::IsVehicle(sample.OriginalVehicleId) ||
       !std::isfinite(sample.ActiveClockSeconds) ||
       sample.ActiveClockSeconds<=0.0 ||
       sample.ActiveClockSeconds>86400.0 ||
       !std::isfinite(sample.StepSeconds) ||
       sample.StepSeconds<settings.MinimumStepSeconds ||
       sample.StepSeconds>settings.MaximumStepSeconds ||
       !sample.OriginalEngineTyresObserved ||
       !sample.NoUnrealForceChanges)
        return false;
    for(unsigned i=0u;i<PhysicalTyreTrendWheels;++i){
        const auto& wheel=sample.Wheels[i];
        if(!ValidTyreSetupWheelWitness(
            wheel,sample.OwnerActor,sample.Frame,
            sample.ActiveClockSeconds,{}))
            return false;
        if(wheel.TreadDepthMm<settings.MinimumTreadRemainingMm)
            return false;
        for(unsigned j=0u;j<i;++j)
            if(wheel.NativeWheel==sample.Wheels[j].NativeWheel)
                return false;
    }
    return true;
}
class PhysicalTyreStintTrend final {
public:
    void Reset() noexcept {
        HavePrior=false;
        Count=0u;
        Head=0u;
        Rejected=0u;
        Previous={};
        Frames={};
        Fault=PhysicalTyreTrendFault::None;
    }
    bool Append(PhysicalTyreTrendSample sample,
                PhysicalTyreTrendSettings settings={}) noexcept
    {
        if(!ValidPhysicalTyreTrendSettings(settings)){
            Fault=PhysicalTyreTrendFault::BadSettings;
            ++Rejected;
            return false;
        }
        if(!ValidPhysicalTyreTrendSample(sample,settings)){
            Fault=PhysicalTyreTrendFault::InvalidSource;
            ++Rejected;
            return false;
        }
        if(HavePrior){
            if(sample.Session!=Previous.Session ||
               sample.Revision!=Previous.Revision ||
               sample.OwnerActor!=Previous.OwnerActor ||
               sample.OriginalVehicleId!=Previous.OriginalVehicleId ||
               sample.Circuit!=Previous.Circuit){
                Fault=PhysicalTyreTrendFault::RosterChanged;
                ++Rejected;
                return false;
            }
            if(sample.Frame<=Previous.Frame ||
               sample.Frame-Previous.Frame>
                   settings.MaximumSkippedFrames){
                Fault=PhysicalTyreTrendFault::StaleFrame;
                ++Rejected;
                return false;
            }
            const double dt=sample.ActiveClockSeconds-
                Previous.ActiveClockSeconds;
            if(!std::isfinite(dt) ||
               dt<=0.0 ||
               dt>settings.MaximumStepSeconds ||
               std::abs(dt-sample.StepSeconds)>
                   settings.MaximumSourceClockSkewSeconds){
                Fault=PhysicalTyreTrendFault::TimeRollback;
                ++Rejected;
                return false;
            }
            for(unsigned wheel=0u;wheel<PhysicalTyreTrendWheels;++wheel){
                const auto& old=Previous.Wheels[wheel];
                const auto& now=sample.Wheels[wheel];
                if(now.NativeWheel!=old.NativeWheel){
                    Fault=PhysicalTyreTrendFault::RosterChanged;
                    ++Rejected;
                    return false;
                }
                const double pressureRate=std::abs(
                    now.HotPressureKpa-old.HotPressureKpa)/dt;
                const double carcassRate=std::abs(
                    now.CarcassTemperatureC-old.CarcassTemperatureC)/dt;
                const double surfaceRate=std::abs(
                    now.SurfaceTemperatureC-old.SurfaceTemperatureC)/dt;
                const double treadWear=old.TreadDepthMm-
                    now.TreadDepthMm;
                if(pressureRate>
                       settings.MaximumPressureChangeKpaPerSecond ||
                   carcassRate>
                       settings.MaximumCarcassChangeCPerSecond ||
                   surfaceRate>
                       settings.MaximumSurfaceChangeCPerSecond ||
                   treadWear< -1e-6 ||
                   treadWear/dt>
                       settings.MaximumTreadWearMmPerSecond){
                    Fault=PhysicalTyreTrendFault::UnrealisticSensorRate;
                    ++Rejected;
                    return false;
                }
            }
        }
        Frames[Head]=sample;
        Head=(Head+1u)%PhysicalTyreTrendCapacity;
        if(Count<PhysicalTyreTrendCapacity)++Count;
        Previous=sample;
        HavePrior=true;
        Fault=PhysicalTyreTrendFault::None;
        return true;
    }
    unsigned Stored() const noexcept {
        return Count;
    }
    unsigned RejectedCount() const noexcept {
        return Rejected;
    }
    PhysicalTyreTrendFault LastFault() const noexcept {
        return Fault;
    }
    bool ReadOldest(unsigned chronological,
                    PhysicalTyreTrendSample& out) const noexcept
    {
        out={};
        if(chronological>=Count)return false;
        const unsigned start=static_cast<unsigned>(
            (Head+PhysicalTyreTrendCapacity-Count)%
            PhysicalTyreTrendCapacity);
        out=Frames[(start+chronological)%
                    PhysicalTyreTrendCapacity];
        return true;
    }
    PhysicalTyreTrendSummary Summarize(
        PhysicalTyreTrendSettings settings={}) const noexcept
    {
        PhysicalTyreTrendSummary result;
        result.Fault=HavePrior?
            PhysicalTyreTrendFault::None:
            PhysicalTyreTrendFault::MissingBaseline;
        result.RejectedFrames=Rejected;
        if(!HavePrior ||
           !ValidPhysicalTyreTrendSettings(settings))
            return result;
        PhysicalTyreTrendSample first;
        if(!ReadOldest(0u,first))return result;
        result.Session=Previous.Session;
        result.Revision=Previous.Revision;
        result.OwnerActor=Previous.OwnerActor;
        result.Circuit=Previous.Circuit;
        result.FirstFrame=first.Frame;
        result.LastFrame=Previous.Frame;
        result.FirstClockSeconds=first.ActiveClockSeconds;
        result.LastClockSeconds=Previous.ActiveClockSeconds;
        result.ObservedDurationSeconds=
            result.LastClockSeconds-result.FirstClockSeconds;
        result.StoredSourceFrames=Count;
        result.MinimumObservedTreadMm=
            std::numeric_limits<double>::infinity();
        for(unsigned wheel=0u;wheel<PhysicalTyreTrendWheels;++wheel){
            auto& w=result.Wheels[wheel];
            const auto& firstWheel=first.Wheels[wheel];
            const auto& lastWheel=Previous.Wheels[wheel];
            w.NativeWheel=lastWheel.NativeWheel;
            w.WheelIndex=wheel;
            w.FirstPressureKpa=firstWheel.HotPressureKpa;
            w.LatestPressureKpa=lastWheel.HotPressureKpa;
            w.FirstCarcassTemperatureC=firstWheel.CarcassTemperatureC;
            w.LatestCarcassTemperatureC=lastWheel.CarcassTemperatureC;
            w.FirstTreadMm=firstWheel.TreadDepthMm;
            w.LatestTreadMm=lastWheel.TreadDepthMm;
            w.MinimumTreadMm=std::numeric_limits<double>::infinity();
            for(unsigned frame=0u;frame<Count;++frame){
                PhysicalTyreTrendSample source;
                if(!ReadOldest(frame,source))
                    return {};
                const auto& sample=source.Wheels[wheel];
                if(sample.NativeWheel!=w.NativeWheel)
                    return {};
                w.MeanPressureKpa+=sample.HotPressureKpa;
                w.MeanCarcassTemperatureC+=sample.CarcassTemperatureC;
                w.MeanSurfaceTemperatureC+=sample.SurfaceTemperatureC;
                w.PeakCarcassTemperatureC=std::max(
                    w.PeakCarcassTemperatureC,sample.CarcassTemperatureC);
                w.MinimumTreadMm=std::min(
                    w.MinimumTreadMm,sample.TreadDepthMm);
            }
            w.MeanPressureKpa/=static_cast<double>(Count);
            w.MeanCarcassTemperatureC/=static_cast<double>(Count);
            w.MeanSurfaceTemperatureC/=static_cast<double>(Count);
            const double dt=result.ObservedDurationSeconds;
            if(dt>0.0){
                w.PressureGainKpaPerSecond=
                    (w.LatestPressureKpa-w.FirstPressureKpa)/dt;
                w.CarcassGainCPerSecond=
                    (w.LatestCarcassTemperatureC-
                     w.FirstCarcassTemperatureC)/dt;
                w.TreadWearMmPerSecond=
                    std::max(0.0,w.FirstTreadMm-w.LatestTreadMm)/dt;
            }
            w.HeatGainWarning=w.CarcassGainCPerSecond>=
                settings.WarningHeatRateCPerSecond;
            w.WearWarning=w.TreadWearMmPerSecond>=
                settings.WarningWearRateMmPerSecond;
            w.PressureGainWarning=w.PressureGainKpaPerSecond>=
                settings.WarningPressureGainKpaPerSecond;
            if(w.HeatGainWarning)++result.WheelsAtHeatRisk;
            if(w.WearWarning)++result.WheelsAtWearRisk;
            result.PeakObservedCarcassC=std::max(
                result.PeakObservedCarcassC,
                w.PeakCarcassTemperatureC);
            result.MinimumObservedTreadMm=std::min(
                result.MinimumObservedTreadMm,w.MinimumTreadMm);
            w.Valid=std::isfinite(w.MeanPressureKpa) &&
                std::isfinite(w.TreadWearMmPerSecond) &&
                std::isfinite(w.MeanCarcassTemperatureC);
            if(!w.Valid)return {};
        }
        result.Valid=true;
        return result;
    }
private:
    std::array<PhysicalTyreTrendSample,
        PhysicalTyreTrendCapacity> Frames{};
    PhysicalTyreTrendSample Previous{};
    unsigned Count=0u;
    unsigned Head=0u;
    unsigned Rejected=0u;
    bool HavePrior=false;
    PhysicalTyreTrendFault Fault=PhysicalTyreTrendFault::None;
};
} // namespace latebrake
