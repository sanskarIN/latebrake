#pragma once
#include "Garage/SixCarCircuitSetupResearch.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Source-only garage tire recommendations for the six original cars.
// Pressure, tread, temperatures and four real Chaos wheel contacts must
// be measured, not inferred from the preview setup. No native pressure,
// wheel forces, saved garage state or race/credit authority is changed.
inline constexpr unsigned TyreSetupWheelCount=4u;
enum class TyreSetupBand : std::uint8_t {
    Unverified, Cold, Nominal, Hot, Overheated
};
struct TyreSetupSettings final {
    double MinimumObservedPressureKpa=140.0;
    double MaximumObservedPressureKpa=350.0;
    double MinimumObservedTemperatureC=0.0;
    double MaximumObservedTemperatureC=180.0;
    double MinimumObservedTreadMm=0.4;
    double MaximumObservedTreadMm=12.0;
    double MaximumSourceClockSkewSeconds=0.05;
    double MinimumSensorConfidence=0.85;
    double NominalOperatingTemperatureC=80.0;
    double ColdThresholdC=50.0;
    double HotThresholdC=105.0;
    double OverheatedThresholdC=130.0;
    double PressureTemperatureGainKpaPerC=0.21;
    double MaximumRecommendationShiftKpa=12.0;
    double MinimumRecommendedPressureKpa=160.0;
    double MaximumRecommendedPressureKpa=300.0;
    double MinimumTyreTreadForRecommendationMm=1.5;
    bool NeedRealWheelSource=true;
    bool NoNativeWheelModification=true;
    bool NoAutomaticPitService=true;
    bool NoGarageCreditMutation=true;
};
struct TyreSetupWheelWitness final {
    std::uint64_t OwnerActor=0u;
    std::uint64_t NativeWheel=0u;
    std::uint64_t SourceFrame=0u;
    double ClockSeconds=0.0;
    double HotPressureKpa=0.0;
    double SurfaceTemperatureC=0.0;
    double CarcassTemperatureC=0.0;
    double TreadDepthMm=0.0;
    double SensorConfidence=0.0;
    bool ActualChaosWheelObserved=false;
    bool ActualOriginalRoadContact=false;
    bool ActualPressureMeasurement=false;
    bool ActualTemperatureMeasurement=false;
    bool ActualTreadMeasurement=false;
};
struct TyreSetupWheelAudit final {
    bool Valid=false;
    std::uint64_t NativeWheel=0u;
    unsigned Index=0u;
    TyreSetupBand Band=TyreSetupBand::Unverified;
    double MeasuredPressureKpa=0.0;
    double MeasuredCarcassTemperatureC=0.0;
    double MeasuredSurfaceTemperatureC=0.0;
    double MeasuredTreadMm=0.0;
    double PredictedPressureAtNominalTemperatureKpa=0.0;
    double SuggestedColdPressureAdjustmentKpa=0.0;
    double SuggestedColdPressureKpa=0.0;
    double TemperatureSpreadC=0.0;
    bool RealInflationValveUnchanged=true;
};
struct PhysicalTyreSetupPlan final {
    bool Valid=false;
    int OriginalVehicleId=-1;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t DriverActor=0u;
    std::uint64_t SourceSession=0u;
    std::uint64_t SourceFrame=0u;
    double SourceClockSeconds=0.0;
    std::array<TyreSetupWheelAudit,TyreSetupWheelCount> Wheels{};
    double MinimumSuggestedPressureKpa=0.0;
    double MaximumSuggestedPressureKpa=0.0;
    double MeanMeasuredTemperatureC=0.0;
    double LargestAxleTemperatureSpreadC=0.0;
    unsigned ColdWheelCount=0u;
    unsigned HotWheelCount=0u;
    unsigned OverheatedWheelCount=0u;
    unsigned LowTreadWheelCount=0u;
    bool SourceOnly=true;
    bool NoRealPressureChanges=true;
    bool NoGarageProgressionChanges=true;
};
inline bool ValidTyreSetupSettings(TyreSetupSettings s) noexcept {
    return std::isfinite(s.MinimumObservedPressureKpa) &&
        s.MinimumObservedPressureKpa>=100.0 &&
        s.MinimumObservedPressureKpa<=170.0 &&
        std::isfinite(s.MaximumObservedPressureKpa) &&
        s.MaximumObservedPressureKpa>=300.0 &&
        s.MaximumObservedPressureKpa<=400.0 &&
        s.MaximumObservedPressureKpa>s.MinimumObservedPressureKpa &&
        std::isfinite(s.MinimumObservedTemperatureC) &&
        s.MinimumObservedTemperatureC>=-20.0 &&
        s.MinimumObservedTemperatureC<=10.0 &&
        std::isfinite(s.MaximumObservedTemperatureC) &&
        s.MaximumObservedTemperatureC>=150.0 &&
        s.MaximumObservedTemperatureC<=200.0 &&
        std::isfinite(s.MinimumObservedTreadMm) &&
        s.MinimumObservedTreadMm>=0.2 &&
        s.MinimumObservedTreadMm<=1.0 &&
        std::isfinite(s.MaximumObservedTreadMm) &&
        s.MaximumObservedTreadMm>=8.0 &&
        s.MaximumObservedTreadMm<=16.0 &&
        std::isfinite(s.MaximumSourceClockSkewSeconds) &&
        s.MaximumSourceClockSkewSeconds>=0.0 &&
        s.MaximumSourceClockSkewSeconds<=0.1 &&
        std::isfinite(s.MinimumSensorConfidence) &&
        s.MinimumSensorConfidence>=0.5 &&
        s.MinimumSensorConfidence<=1.0 &&
        std::isfinite(s.NominalOperatingTemperatureC) &&
        s.NominalOperatingTemperatureC>=65.0 &&
        s.NominalOperatingTemperatureC<=105.0 &&
        std::isfinite(s.ColdThresholdC) &&
        s.ColdThresholdC>=30.0 &&
        s.ColdThresholdC<s.NominalOperatingTemperatureC &&
        std::isfinite(s.HotThresholdC) &&
        s.HotThresholdC>s.NominalOperatingTemperatureC &&
        s.HotThresholdC<=125.0 &&
        std::isfinite(s.OverheatedThresholdC) &&
        s.OverheatedThresholdC>s.HotThresholdC &&
        s.OverheatedThresholdC<=160.0 &&
        std::isfinite(s.PressureTemperatureGainKpaPerC) &&
        s.PressureTemperatureGainKpaPerC>=0.05 &&
        s.PressureTemperatureGainKpaPerC<=0.5 &&
        std::isfinite(s.MaximumRecommendationShiftKpa) &&
        s.MaximumRecommendationShiftKpa>=5.0 &&
        s.MaximumRecommendationShiftKpa<=20.0 &&
        std::isfinite(s.MinimumRecommendedPressureKpa) &&
        s.MinimumRecommendedPressureKpa>=140.0 &&
        s.MinimumRecommendedPressureKpa<=200.0 &&
        std::isfinite(s.MaximumRecommendedPressureKpa) &&
        s.MaximumRecommendedPressureKpa>=260.0 &&
        s.MaximumRecommendedPressureKpa<=340.0 &&
        std::isfinite(s.MinimumTyreTreadForRecommendationMm) &&
        s.MinimumTyreTreadForRecommendationMm>=0.5 &&
        s.MinimumTyreTreadForRecommendationMm<=3.0 &&
        s.NeedRealWheelSource &&
        s.NoNativeWheelModification &&
        s.NoAutomaticPitService &&
        s.NoGarageCreditMutation;
}
inline bool ValidTyreSetupWheelWitness(
    TyreSetupWheelWitness w,
    std::uint64_t actor,
    std::uint64_t frame,
    double clock,
    TyreSetupSettings s) noexcept
{
    return w.OwnerActor==actor &&
        w.NativeWheel!=0u &&
        w.SourceFrame==frame &&
        w.ActualChaosWheelObserved &&
        w.ActualOriginalRoadContact &&
        w.ActualPressureMeasurement &&
        w.ActualTemperatureMeasurement &&
        w.ActualTreadMeasurement &&
        std::isfinite(w.ClockSeconds) &&
        std::abs(w.ClockSeconds-clock)<=
            s.MaximumSourceClockSkewSeconds &&
        std::isfinite(w.HotPressureKpa) &&
        w.HotPressureKpa>=s.MinimumObservedPressureKpa &&
        w.HotPressureKpa<=s.MaximumObservedPressureKpa &&
        std::isfinite(w.SurfaceTemperatureC) &&
        w.SurfaceTemperatureC>=s.MinimumObservedTemperatureC &&
        w.SurfaceTemperatureC<=s.MaximumObservedTemperatureC &&
        std::isfinite(w.CarcassTemperatureC) &&
        w.CarcassTemperatureC>=s.MinimumObservedTemperatureC &&
        w.CarcassTemperatureC<=s.MaximumObservedTemperatureC &&
        std::isfinite(w.TreadDepthMm) &&
        w.TreadDepthMm>=s.MinimumObservedTreadMm &&
        w.TreadDepthMm<=s.MaximumObservedTreadMm &&
        std::isfinite(w.SensorConfidence) &&
        w.SensorConfidence>=s.MinimumSensorConfidence &&
        w.SensorConfidence<=1.0;
}
inline TyreSetupBand ClassifyTyreSetupTemperature(
    double temperatureC,TyreSetupSettings s) noexcept
{
    if(!std::isfinite(temperatureC) ||
       temperatureC<s.MinimumObservedTemperatureC ||
       temperatureC>s.MaximumObservedTemperatureC)
        return TyreSetupBand::Unverified;
    if(temperatureC>=s.OverheatedThresholdC)
        return TyreSetupBand::Overheated;
    if(temperatureC>=s.HotThresholdC)
        return TyreSetupBand::Hot;
    if(temperatureC<s.ColdThresholdC)
        return TyreSetupBand::Cold;
    return TyreSetupBand::Nominal;
}
inline TyreSetupWheelAudit AuditPhysicalTyreSetupWheel(
    TyreSetupWheelWitness witness,
    const SixCarSetupPlan& reference,
    unsigned index,
    TyreSetupSettings settings={}) noexcept
{
    TyreSetupWheelAudit out;
    if(!ValidTyreSetupSettings(settings) ||
       !reference.Valid ||
       index>=TyreSetupWheelCount ||
       !ValidTyreSetupWheelWitness(
           witness,witness.OwnerActor,witness.SourceFrame,
           witness.ClockSeconds,settings))
        return out;
    out.NativeWheel=witness.NativeWheel;
    out.Index=index;
    out.Band=ClassifyTyreSetupTemperature(
        witness.CarcassTemperatureC,settings);
    if(out.Band==TyreSetupBand::Unverified)return {};
    const double predicted=witness.HotPressureKpa+
        (settings.NominalOperatingTemperatureC-
         witness.CarcassTemperatureC)*
        settings.PressureTemperatureGainKpaPerC;
    const double correction=std::clamp(
        reference.Values.TyrePressureKpa-predicted,
        -settings.MaximumRecommendationShiftKpa,
        settings.MaximumRecommendationShiftKpa);
    const double suggested=std::clamp(
        witness.HotPressureKpa+correction,
        settings.MinimumRecommendedPressureKpa,
        settings.MaximumRecommendedPressureKpa);
    if(!std::isfinite(predicted) ||
       !std::isfinite(correction) ||
       !std::isfinite(suggested))return {};
    out.MeasuredPressureKpa=witness.HotPressureKpa;
    out.MeasuredCarcassTemperatureC=
        witness.CarcassTemperatureC;
    out.MeasuredSurfaceTemperatureC=
        witness.SurfaceTemperatureC;
    out.MeasuredTreadMm=witness.TreadDepthMm;
    out.PredictedPressureAtNominalTemperatureKpa=predicted;
    out.SuggestedColdPressureAdjustmentKpa=correction;
    out.SuggestedColdPressureKpa=suggested;
    out.TemperatureSpreadC=std::abs(
        witness.CarcassTemperatureC-
        witness.SurfaceTemperatureC);
    out.Valid=true;
    return out;
}
inline PhysicalTyreSetupPlan BuildPhysicalTyreSetupPlan(
    const SixCarSetupPlan& reference,
    std::uint64_t nativeActor,
    std::uint64_t sourceFrame,
    double sourceClockSeconds,
    const std::array<TyreSetupWheelWitness,
        TyreSetupWheelCount>& readings,
    TyreSetupSettings settings={}) noexcept
{
    PhysicalTyreSetupPlan out;
    if(!ValidTyreSetupSettings(settings) ||
       !reference.Valid ||
       reference.VehicleId<0 ||
       !GarageState::IsVehicle(reference.VehicleId) ||
       reference.SourceSession==0u ||
       reference.SourceFrame==0u ||
       nativeActor==0u ||
       sourceFrame==0u ||
       !std::isfinite(sourceClockSeconds) ||
       sourceClockSeconds<=0.0 ||
       sourceClockSeconds>86400.0 ||
       !ValidSixCarMechanicalSetup(reference.Values))
        return out;
    out.OriginalVehicleId=reference.VehicleId;
    out.Circuit=reference.Circuit;
    out.DriverActor=nativeActor;
    out.SourceSession=reference.SourceSession;
    out.SourceFrame=sourceFrame;
    out.SourceClockSeconds=sourceClockSeconds;
    out.MinimumSuggestedPressureKpa=
        std::numeric_limits<double>::infinity();
    for(unsigned i=0u;i<TyreSetupWheelCount;++i){
        const auto& w=readings[i];
        if(!ValidTyreSetupWheelWitness(
            w,nativeActor,sourceFrame,sourceClockSeconds,settings))
            return {};
        for(unsigned j=0u;j<i;++j)
            if(w.NativeWheel==readings[j].NativeWheel)
                return {};
        const auto audit=AuditPhysicalTyreSetupWheel(
            w,reference,i,settings);
        if(!audit.Valid)return {};
        out.Wheels[i]=audit;
        out.MinimumSuggestedPressureKpa=std::min(
            out.MinimumSuggestedPressureKpa,
            audit.SuggestedColdPressureKpa);
        out.MaximumSuggestedPressureKpa=std::max(
            out.MaximumSuggestedPressureKpa,
            audit.SuggestedColdPressureKpa);
        out.MeanMeasuredTemperatureC+=
            audit.MeasuredCarcassTemperatureC;
        if(audit.Band==TyreSetupBand::Cold)
            ++out.ColdWheelCount;
        if(audit.Band==TyreSetupBand::Hot)
            ++out.HotWheelCount;
        if(audit.Band==TyreSetupBand::Overheated)
            ++out.OverheatedWheelCount;
        if(audit.MeasuredTreadMm<
           settings.MinimumTyreTreadForRecommendationMm)
            ++out.LowTreadWheelCount;
    }
    out.MeanMeasuredTemperatureC/=
        static_cast<double>(TyreSetupWheelCount);
    for(unsigned i=0u;i<2u;++i){
        const double spread=std::abs(
            out.Wheels[2u*i].MeasuredCarcassTemperatureC-
            out.Wheels[2u*i+1u].MeasuredCarcassTemperatureC);
        out.LargestAxleTemperatureSpreadC=std::max(
            out.LargestAxleTemperatureSpreadC,spread);
    }
    out.Valid=std::isfinite(out.MinimumSuggestedPressureKpa) &&
        std::isfinite(out.MaximumSuggestedPressureKpa) &&
        std::isfinite(out.MeanMeasuredTemperatureC) &&
        std::isfinite(out.LargestAxleTemperatureSpreadC) &&
        out.MinimumSuggestedPressureKpa>=
            settings.MinimumRecommendedPressureKpa &&
        out.MaximumSuggestedPressureKpa<=
            settings.MaximumRecommendedPressureKpa;
    return out;
}
} // namespace latebrake
