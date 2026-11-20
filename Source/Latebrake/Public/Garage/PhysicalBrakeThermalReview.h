#pragma once
#include "Garage/SixCarCircuitSetupResearch.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Genuine-source original six-car BRAKE THERMAL REVIEW. Computations are
// diagnostic evidence only and never change native Chaos brake torque,
// braking ABS, grip, original garage credits or an official race result.
inline constexpr unsigned PhysicalBrakeCornerCount=4u;
enum class PhysicalBrakeThermalBand : std::uint8_t {
    Unverified, Cold, Nominal, Hot, FadeRisk, EmergencyReview
};
struct PhysicalBrakeThermalSettings final {
    double MinimumRotorTemperatureC=-20.0;
    double MaximumRotorTemperatureC=1200.0;
    double ColdTemperatureC=120.0;
    double IdealMinimumTemperatureC=220.0;
    double IdealMaximumTemperatureC=600.0;
    double HotTemperatureC=650.0;
    double FadeRiskTemperatureC=780.0;
    double EmergencyTemperatureC=950.0;
    double MaximumDiscThicknessMm=45.0;
    double MinimumDiscThicknessMm=18.0;
    double MinimumWearReserveMm=2.0;
    double MaximumBrakePressureKpa=15000.0;
    double MaximumMeasuredBrakeTorqueNm=10000.0;
    double MaximumSourceClockSkewSeconds=0.05;
    double MinimumSensorConfidence=0.85;
    double MaximumTemperatureSideDifferenceC=200.0;
    double MaximumAxleTorqueImbalanceFraction=0.35;
    double CoolingTargetTemperatureC=400.0;
    double MinimumCoolingSeconds=5.0;
    double MaximumCoolingSeconds=300.0;
    double ConservativeCoolingRateCPerSecond=3.5;
    bool RequireRealPhysicalRotorSource=true;
    bool RequireRealHydraulicPressureSensor=true;
    bool RequireRealWheelAndContactProof=true;
    bool NeverApplyBrakeForce=true;
    bool NoOfficialRepairOrCredits=true;
};
struct PhysicalBrakeRotorWitness final {
    std::uint64_t DriverActor=0u;
    std::uint64_t NativeRotor=0u;
    std::uint64_t NativeWheel=0u;
    std::uint64_t SourceFrame=0u;
    double SourceClockSeconds=0.0;
    double RotorTemperatureC=0.0;
    double RotorThicknessMm=0.0;
    double ManufacturerMinimumThicknessMm=0.0;
    double HydraulicPressureKpa=0.0;
    double MeasuredBrakeTorqueNm=0.0;
    double AngularVelocityRadiansPerSecond=0.0;
    double MeasuredContactConfidence=0.0;
    bool NativeRotorTemperatureObserved=false;
    bool NativeHydraulicPressureObserved=false;
    bool OriginalPhysicalWheelObserved=false;
    bool NativeRoadContactVerified=false;
    bool ActualRotorWearObserved=false;
};
struct PhysicalBrakeThermalCorner final {
    bool Valid=false;
    unsigned CornerIndex=0u;
    std::uint64_t NativeRotor=0u;
    std::uint64_t NativeWheel=0u;
    PhysicalBrakeThermalBand Band=
        PhysicalBrakeThermalBand::Unverified;
    double ActualRotorTemperatureC=0.0;
    double ActualThicknessMm=0.0;
    double MeasuredThicknessMarginMm=0.0;
    double ActualPressureKpa=0.0;
    double ActualBrakeTorqueNm=0.0;
    double TorquePerKpaNm=0.0;
    double EstimatedThermalHeadroomC=0.0;
    double SuggestedCoolingReviewSeconds=0.0;
    double WearReserveFraction=0.0;
    bool PhysicalRotorUnchanged=true;
};
struct PhysicalBrakeThermalPlan final {
    bool Valid=false;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    int OriginalGarageVehicleId=-1;
    std::uint64_t DriverActor=0u;
    std::uint64_t SourceSession=0u;
    std::uint64_t SourceFrame=0u;
    double SourceClockSeconds=0.0;
    std::array<PhysicalBrakeThermalCorner,
        PhysicalBrakeCornerCount> Corners{};
    double MeanRotorTemperatureC=0.0;
    double MaximumRotorTemperatureC=0.0;
    double MinimumRotorThicknessMarginMm=0.0;
    double MaximumAxleTemperatureSplitC=0.0;
    double FrontTorqueFraction=0.0;
    double OriginalGarageFrontBrakeBias=0.0;
    double FrontBiasObservedDifference=0.0;
    double MaximumSuggestedCoolingReviewSeconds=0.0;
    unsigned FadeRiskCorners=0u;
    unsigned EmergencyReviewCorners=0u;
    unsigned LowThicknessCorners=0u;
    bool SourceOnly=true;
    bool NoBrakeCommandsIssued=true;
    bool NoCareerPenaltyOrRaceResultAuthority=true;
};
inline bool ValidPhysicalBrakeThermalSettings(
    PhysicalBrakeThermalSettings s) noexcept
{
    return std::isfinite(s.MinimumRotorTemperatureC) &&
        s.MinimumRotorTemperatureC>=-50.0 &&
        s.MinimumRotorTemperatureC<=0.0 &&
        std::isfinite(s.MaximumRotorTemperatureC) &&
        s.MaximumRotorTemperatureC>=1000.0 &&
        s.MaximumRotorTemperatureC<=1400.0 &&
        std::isfinite(s.ColdTemperatureC) &&
        s.ColdTemperatureC>=50.0 &&
        s.ColdTemperatureC<=150.0 &&
        std::isfinite(s.IdealMinimumTemperatureC) &&
        s.IdealMinimumTemperatureC>s.ColdTemperatureC &&
        s.IdealMinimumTemperatureC<=350.0 &&
        std::isfinite(s.IdealMaximumTemperatureC) &&
        s.IdealMaximumTemperatureC>=500.0 &&
        s.IdealMaximumTemperatureC<=650.0 &&
        std::isfinite(s.HotTemperatureC) &&
        s.HotTemperatureC>=s.IdealMaximumTemperatureC &&
        s.HotTemperatureC<=700.0 &&
        std::isfinite(s.FadeRiskTemperatureC) &&
        s.FadeRiskTemperatureC>s.HotTemperatureC &&
        s.FadeRiskTemperatureC<=850.0 &&
        std::isfinite(s.EmergencyTemperatureC) &&
        s.EmergencyTemperatureC>s.FadeRiskTemperatureC &&
        s.EmergencyTemperatureC<=1100.0 &&
        std::isfinite(s.MaximumDiscThicknessMm) &&
        s.MaximumDiscThicknessMm>=35.0 &&
        s.MaximumDiscThicknessMm<=50.0 &&
        std::isfinite(s.MinimumDiscThicknessMm) &&
        s.MinimumDiscThicknessMm>=12.0 &&
        s.MinimumDiscThicknessMm<=22.0 &&
        std::isfinite(s.MinimumWearReserveMm) &&
        s.MinimumWearReserveMm>=1.0 &&
        s.MinimumWearReserveMm<=4.0 &&
        std::isfinite(s.MaximumBrakePressureKpa) &&
        s.MaximumBrakePressureKpa>=10000.0 &&
        s.MaximumBrakePressureKpa<=20000.0 &&
        std::isfinite(s.MaximumMeasuredBrakeTorqueNm) &&
        s.MaximumMeasuredBrakeTorqueNm>=5000.0 &&
        s.MaximumMeasuredBrakeTorqueNm<=15000.0 &&
        std::isfinite(s.MaximumSourceClockSkewSeconds) &&
        s.MaximumSourceClockSkewSeconds>=0.0 &&
        s.MaximumSourceClockSkewSeconds<=0.1 &&
        std::isfinite(s.MinimumSensorConfidence) &&
        s.MinimumSensorConfidence>=0.5 &&
        s.MinimumSensorConfidence<=1.0 &&
        std::isfinite(s.MaximumTemperatureSideDifferenceC) &&
        s.MaximumTemperatureSideDifferenceC>=100.0 &&
        s.MaximumTemperatureSideDifferenceC<=350.0 &&
        std::isfinite(s.MaximumAxleTorqueImbalanceFraction) &&
        s.MaximumAxleTorqueImbalanceFraction>=0.2 &&
        s.MaximumAxleTorqueImbalanceFraction<=0.5 &&
        std::isfinite(s.CoolingTargetTemperatureC) &&
        s.CoolingTargetTemperatureC>=250.0 &&
        s.CoolingTargetTemperatureC<=500.0 &&
        std::isfinite(s.MinimumCoolingSeconds) &&
        s.MinimumCoolingSeconds>=3.0 &&
        s.MinimumCoolingSeconds<=15.0 &&
        std::isfinite(s.MaximumCoolingSeconds) &&
        s.MaximumCoolingSeconds>=120.0 &&
        s.MaximumCoolingSeconds<=600.0 &&
        std::isfinite(s.ConservativeCoolingRateCPerSecond) &&
        s.ConservativeCoolingRateCPerSecond>=1.0 &&
        s.ConservativeCoolingRateCPerSecond<=7.0 &&
        s.RequireRealPhysicalRotorSource &&
        s.RequireRealHydraulicPressureSensor &&
        s.RequireRealWheelAndContactProof &&
        s.NeverApplyBrakeForce &&
        s.NoOfficialRepairOrCredits;
}
inline bool ValidPhysicalBrakeRotorWitness(
    PhysicalBrakeRotorWitness w,
    std::uint64_t actor,
    std::uint64_t sourceFrame,
    double sourceClock,
    PhysicalBrakeThermalSettings s={}) noexcept
{
    return ValidPhysicalBrakeThermalSettings(s) &&
        actor!=0u &&
        w.DriverActor==actor &&
        w.NativeRotor!=0u &&
        w.NativeWheel!=0u &&
        w.SourceFrame==sourceFrame &&
        w.NativeRotorTemperatureObserved &&
        w.NativeHydraulicPressureObserved &&
        w.OriginalPhysicalWheelObserved &&
        w.NativeRoadContactVerified &&
        w.ActualRotorWearObserved &&
        std::isfinite(w.SourceClockSeconds) &&
        std::abs(w.SourceClockSeconds-sourceClock)<=
            s.MaximumSourceClockSkewSeconds &&
        std::isfinite(w.RotorTemperatureC) &&
        w.RotorTemperatureC>=s.MinimumRotorTemperatureC &&
        w.RotorTemperatureC<=s.MaximumRotorTemperatureC &&
        std::isfinite(w.RotorThicknessMm) &&
        w.RotorThicknessMm>=s.MinimumDiscThicknessMm &&
        w.RotorThicknessMm<=s.MaximumDiscThicknessMm &&
        std::isfinite(w.ManufacturerMinimumThicknessMm) &&
        w.ManufacturerMinimumThicknessMm>=
            s.MinimumDiscThicknessMm &&
        w.ManufacturerMinimumThicknessMm<
            w.RotorThicknessMm &&
        std::isfinite(w.HydraulicPressureKpa) &&
        w.HydraulicPressureKpa>=0.0 &&
        w.HydraulicPressureKpa<=s.MaximumBrakePressureKpa &&
        std::isfinite(w.MeasuredBrakeTorqueNm) &&
        w.MeasuredBrakeTorqueNm>=0.0 &&
        w.MeasuredBrakeTorqueNm<=s.MaximumMeasuredBrakeTorqueNm &&
        std::isfinite(w.AngularVelocityRadiansPerSecond) &&
        std::abs(w.AngularVelocityRadiansPerSecond)<=700.0 &&
        std::isfinite(w.MeasuredContactConfidence) &&
        w.MeasuredContactConfidence>=s.MinimumSensorConfidence &&
        w.MeasuredContactConfidence<=1.0;
}
inline PhysicalBrakeThermalBand ClassifyPhysicalBrakeTemperature(
    double temperature,
    PhysicalBrakeThermalSettings settings={}) noexcept
{
    if(!std::isfinite(temperature) ||
       temperature<settings.MinimumRotorTemperatureC ||
       temperature>settings.MaximumRotorTemperatureC)
        return PhysicalBrakeThermalBand::Unverified;
    if(temperature>=settings.EmergencyTemperatureC)
        return PhysicalBrakeThermalBand::EmergencyReview;
    if(temperature>=settings.FadeRiskTemperatureC)
        return PhysicalBrakeThermalBand::FadeRisk;
    if(temperature>=settings.HotTemperatureC)
        return PhysicalBrakeThermalBand::Hot;
    if(temperature<settings.ColdTemperatureC)
        return PhysicalBrakeThermalBand::Cold;
    return PhysicalBrakeThermalBand::Nominal;
}
inline PhysicalBrakeThermalCorner ReviewPhysicalBrakeCorner(
    PhysicalBrakeRotorWitness witness,
    unsigned index,
    PhysicalBrakeThermalSettings settings={}) noexcept
{
    PhysicalBrakeThermalCorner out;
    if(index>=PhysicalBrakeCornerCount ||
       !ValidPhysicalBrakeRotorWitness(
           witness,witness.DriverActor,witness.SourceFrame,
           witness.SourceClockSeconds,settings))
        return out;
    const double margin=witness.RotorThicknessMm-
        witness.ManufacturerMinimumThicknessMm;
    const double pressure=witness.HydraulicPressureKpa;
    const double torque=witness.MeasuredBrakeTorqueNm;
    const double nominal=std::max(
        1.0,settings.IdealMaximumTemperatureC);
    const double headroom=std::max(
        0.0,settings.FadeRiskTemperatureC-
            witness.RotorTemperatureC);
    const double cooling=std::clamp(
        (witness.RotorTemperatureC-
         settings.CoolingTargetTemperatureC)/
        settings.ConservativeCoolingRateCPerSecond,
        0.0,settings.MaximumCoolingSeconds);
    out.CornerIndex=index;
    out.NativeRotor=witness.NativeRotor;
    out.NativeWheel=witness.NativeWheel;
    out.Band=ClassifyPhysicalBrakeTemperature(
        witness.RotorTemperatureC,settings);
    out.ActualRotorTemperatureC=witness.RotorTemperatureC;
    out.ActualThicknessMm=witness.RotorThicknessMm;
    out.MeasuredThicknessMarginMm=margin;
    out.ActualPressureKpa=pressure;
    out.ActualBrakeTorqueNm=torque;
    out.TorquePerKpaNm=pressure>1e-9?
        torque/pressure:0.0;
    out.EstimatedThermalHeadroomC=headroom;
    out.SuggestedCoolingReviewSeconds=cooling;
    out.WearReserveFraction=std::clamp(
        margin/nominal,0.0,1.0);
    out.Valid=std::isfinite(out.TorquePerKpaNm) &&
        std::isfinite(out.WearReserveFraction) &&
        std::isfinite(out.SuggestedCoolingReviewSeconds) &&
        out.Band!=PhysicalBrakeThermalBand::Unverified;
    return out;
}
inline PhysicalBrakeThermalPlan BuildPhysicalBrakeThermalReview(
    const SixCarSetupPlan& garage,
    std::uint64_t sourceActor,
    std::uint64_t sourceFrame,
    double sourceClock,
    const std::array<PhysicalBrakeRotorWitness,
        PhysicalBrakeCornerCount>& witnesses,
    PhysicalBrakeThermalSettings settings={}) noexcept
{
    PhysicalBrakeThermalPlan out;
    if(!garage.Valid ||
       !GarageState::IsVehicle(garage.VehicleId) ||
       !ValidSixCarMechanicalSetup(garage.Values) ||
       garage.SourceSession==0u ||
       sourceActor==0u ||
       sourceFrame==0u ||
       !std::isfinite(sourceClock) ||
       sourceClock<=0.0 ||
       sourceClock>86400.0 ||
       !ValidPhysicalBrakeThermalSettings(settings))
        return out;
    out.Circuit=garage.Circuit;
    out.OriginalGarageVehicleId=garage.VehicleId;
    out.DriverActor=sourceActor;
    out.SourceSession=garage.SourceSession;
    out.SourceFrame=sourceFrame;
    out.SourceClockSeconds=sourceClock;
    out.OriginalGarageFrontBrakeBias=
        garage.Values.BrakeFrontFraction;
    out.MinimumRotorThicknessMarginMm=
        std::numeric_limits<double>::infinity();
    double frontTorque=0.0;
    double rearTorque=0.0;
    for(unsigned wheel=0u;wheel<PhysicalBrakeCornerCount;++wheel){
        const auto& witness=witnesses[wheel];
        if(!ValidPhysicalBrakeRotorWitness(
            witness,sourceActor,sourceFrame,
            sourceClock,settings))
            return {};
        for(unsigned previous=0u;previous<wheel;++previous)
            if(witness.NativeRotor==
                    witnesses[previous].NativeRotor ||
               witness.NativeWheel==
                    witnesses[previous].NativeWheel)
                return {};
        const auto review=ReviewPhysicalBrakeCorner(
            witness,wheel,settings);
        if(!review.Valid)return {};
        out.Corners[wheel]=review;
        out.MaximumRotorTemperatureC=std::max(
            out.MaximumRotorTemperatureC,
            review.ActualRotorTemperatureC);
        out.MeanRotorTemperatureC+=review.ActualRotorTemperatureC;
        out.MinimumRotorThicknessMarginMm=std::min(
            out.MinimumRotorThicknessMarginMm,
            review.MeasuredThicknessMarginMm);
        out.MaximumSuggestedCoolingReviewSeconds=std::max(
            out.MaximumSuggestedCoolingReviewSeconds,
            review.SuggestedCoolingReviewSeconds);
        if(review.Band>=PhysicalBrakeThermalBand::FadeRisk)
            ++out.FadeRiskCorners;
        if(review.Band==PhysicalBrakeThermalBand::EmergencyReview)
            ++out.EmergencyReviewCorners;
        if(review.MeasuredThicknessMarginMm<
           settings.MinimumWearReserveMm)
            ++out.LowThicknessCorners;
        if(wheel<2u)frontTorque+=review.ActualBrakeTorqueNm;
        else rearTorque+=review.ActualBrakeTorqueNm;
    }
    out.MeanRotorTemperatureC/=
        static_cast<double>(PhysicalBrakeCornerCount);
    const double allTorque=frontTorque+rearTorque;
    out.FrontTorqueFraction=allTorque>1.0?
        frontTorque/allTorque:out.OriginalGarageFrontBrakeBias;
    out.FrontBiasObservedDifference=
        out.FrontTorqueFraction-out.OriginalGarageFrontBrakeBias;
    for(unsigned axle=0u;axle<2u;++axle){
        const double split=std::abs(
            out.Corners[2u*axle].ActualRotorTemperatureC-
            out.Corners[2u*axle+1u].ActualRotorTemperatureC);
        out.MaximumAxleTemperatureSplitC=std::max(
            out.MaximumAxleTemperatureSplitC,split);
    }
    out.Valid=std::isfinite(out.MeanRotorTemperatureC) &&
        std::isfinite(out.FrontTorqueFraction) &&
        std::isfinite(out.FrontBiasObservedDifference) &&
        out.FrontTorqueFraction>=0.0 &&
        out.FrontTorqueFraction<=1.0 &&
        out.MinimumRotorThicknessMarginMm>0.0;
    return out;
}
} // namespace latebrake
