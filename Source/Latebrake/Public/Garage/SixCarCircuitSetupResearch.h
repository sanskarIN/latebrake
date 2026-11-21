#pragma once
#include "Garage/GarageState.h"
#include "Track/WorldScaleBankedRoadPlan.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace latebrake {
// Source-only original six-car / six-circuit setup recommendations. The
// live garage, immutable vehicle IDs, credits, saved engine/grip upgrades,
// actual Chaos wheel setup and current approved race balance are untouched.
// A real native wheel test is required before any recommended numbers
// can become actual driving configuration.
inline constexpr unsigned SixCarSetupVehicleCount=6u;
inline constexpr unsigned SixCarSetupCircuitCount=WorldScaleVariantCount;
static_assert(SixCarSetupVehicleCount==VehicleCount);
struct SixCarSetupBase final {
    double FrontSpringNm=40000.0;
    double RearSpringNm=38000.0;
    double FrontRideCm=9.0;
    double RearRideCm=10.0;
    double FrontDampingRatio=0.55;
    double RearDampingRatio=0.53;
    double FrontAntiRollNm=20000.0;
    double RearAntiRollNm=18000.0;
    double BrakeFrontFraction=0.60;
    double DifferentialLockFraction=0.35;
    double WingFrontDegrees=4.0;
    double WingRearDegrees=8.0;
    double FrontCamberDegrees=-2.2;
    double RearCamberDegrees=-1.7;
    double FrontToeDegrees=0.02;
    double RearToeDegrees=0.10;
    double TyrePressureKpa=210.0;
    double FinalDriveRatio=3.9;
};
inline constexpr std::array<SixCarSetupBase,SixCarSetupVehicleCount>
    SixCarOriginalVehicleSetup{{
        {40000,38000,9.0,10.0,0.54,0.53,20000,18000,
         0.60,0.35,4.0,8.0,-2.2,-1.7,0.02,0.10,210.0,3.9},
        {44000,42000,8.7,9.9,0.58,0.55,23000,19000,
         0.62,0.40,5.5,9.0,-2.4,-1.8,0.02,0.12,215.0,4.0},
        {38000,36500,9.4,10.4,0.51,0.52,19000,17500,
         0.59,0.29,4.5,9.0,-2.0,-1.5,0.01,0.10,208.0,3.7},
        {46000,43500,8.5,9.7,0.60,0.57,24500,21500,
         0.64,0.44,6.0,10.0,-2.5,-1.9,0.03,0.13,219.0,4.1},
        {41000,39000,9.2,10.5,0.56,0.54,23000,20500,
         0.61,0.36,7.0,12.0,-2.6,-1.9,0.02,0.14,212.0,3.9},
        {42500,41000,8.8,9.8,0.57,0.56,23500,21000,
         0.62,0.41,5.5,9.5,-2.4,-1.8,0.02,0.12,216.0,4.0}
    }};
struct SixCarSetupSettings final {
    double MinimumMeasuredGripFraction=0.35;
    double MaximumMeasuredGripFraction=1.0;
    double MinimumSurfaceEvidenceConfidence=0.85;
    double MaximumWeatherClockSkewSeconds=0.05;
    double MinimumSpringNm=26000.0;
    double MaximumSpringNm=65000.0;
    double MinimumRideHeightCm=6.0;
    double MaximumRideHeightCm=16.0;
    double MaximumWingDegrees=20.0;
    double MaximumBrakeFrontFraction=0.75;
    double MinimumBrakeFrontFraction=0.50;
    bool OriginalVehicleIdsImmutable=true;
    bool RequirePurchasedVehicle=true;
    bool KeepOriginalGarageCredits=true;
    bool NeverSetUnrealWheelForces=true;
    bool NeverGrantCareerRewards=true;
};
struct SixCarTrackSurfaceEvidence final {
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    std::uint64_t Session=0u;
    std::uint64_t SourceFrame=0u;
    double ClockSeconds=0.0;
    double MeasuredGripFraction=1.0;
    double SourceConfidence=0.0;
    bool AuthenticRoadMaterialObserved=false;
    bool ActualTireContactObserved=false;
    bool OriginalRaceRulesPreserved=true;
};
struct SixCarSetupPlan final {
    bool Valid=false;
    int VehicleId=-1;
    WorldScaleVariantId Circuit=WorldScaleVariantId::OriginalLoop;
    SixCarSetupBase Values{};
    double CornerSeverity=0.0;
    double TrackCurvatureProxyPerCm=0.0;
    double MeasuredGripFraction=0.0;
    double RideStanceCentimeters=0.0;
    double PitchStanceCentimeters=0.0;
    double PlannedAeroBalanceFront=0.0;
    double EstimatedSpringBalanceFront=0.0;
    double SetupDifferenceNorm=0.0;
    unsigned OriginalUpgradeEngineLevel=0u;
    unsigned OriginalUpgradeGripLevel=0u;
    std::uint64_t SourceSession=0u;
    std::uint64_t SourceFrame=0u;
    bool SourceOnly=true;
    bool NativeChaosConfigurationUnchanged=true;
    bool OriginalGarageStateUnchanged=true;
};
inline bool ValidSixCarSetupSettings(SixCarSetupSettings s) noexcept {
    return std::isfinite(s.MinimumMeasuredGripFraction) &&
        s.MinimumMeasuredGripFraction>=0.2 &&
        s.MinimumMeasuredGripFraction<=0.6 &&
        std::isfinite(s.MaximumMeasuredGripFraction) &&
        s.MaximumMeasuredGripFraction>=0.8 &&
        s.MaximumMeasuredGripFraction<=1.0 &&
        s.MinimumMeasuredGripFraction<
            s.MaximumMeasuredGripFraction &&
        std::isfinite(s.MinimumSurfaceEvidenceConfidence) &&
        s.MinimumSurfaceEvidenceConfidence>=0.5 &&
        s.MinimumSurfaceEvidenceConfidence<=1.0 &&
        std::isfinite(s.MaximumWeatherClockSkewSeconds) &&
        s.MaximumWeatherClockSkewSeconds>=0.0 &&
        s.MaximumWeatherClockSkewSeconds<=0.1 &&
        std::isfinite(s.MinimumSpringNm) &&
        s.MinimumSpringNm>=20000.0 &&
        s.MinimumSpringNm<=30000.0 &&
        std::isfinite(s.MaximumSpringNm) &&
        s.MaximumSpringNm>=60000.0 &&
        s.MaximumSpringNm<=80000.0 &&
        std::isfinite(s.MinimumRideHeightCm) &&
        s.MinimumRideHeightCm>=5.0 &&
        s.MinimumRideHeightCm<=8.0 &&
        std::isfinite(s.MaximumRideHeightCm) &&
        s.MaximumRideHeightCm>=14.0 &&
        s.MaximumRideHeightCm<=20.0 &&
        std::isfinite(s.MaximumWingDegrees) &&
        s.MaximumWingDegrees>=15.0 &&
        s.MaximumWingDegrees<=25.0 &&
        std::isfinite(s.MaximumBrakeFrontFraction) &&
        s.MaximumBrakeFrontFraction>=0.7 &&
        s.MaximumBrakeFrontFraction<=0.8 &&
        std::isfinite(s.MinimumBrakeFrontFraction) &&
        s.MinimumBrakeFrontFraction>=0.45 &&
        s.MinimumBrakeFrontFraction<=0.55 &&
        s.MinimumBrakeFrontFraction<s.MaximumBrakeFrontFraction &&
        s.OriginalVehicleIdsImmutable &&
        s.RequirePurchasedVehicle &&
        s.KeepOriginalGarageCredits &&
        s.NeverSetUnrealWheelForces &&
        s.NeverGrantCareerRewards;
}
inline bool ValidSixCarOriginalGarage(
    GarageSnapshot saved) noexcept
{
    return saved.Credits>=0 &&
        saved.OwnedVehicleMask>=1 &&
        (saved.OwnedVehicleMask&~KnownVehicleMask)==0 &&
        (saved.OwnedVehicleMask&1)==1 &&
        GarageState::IsVehicle(saved.SelectedVehicle) &&
        (saved.OwnedVehicleMask&
         (1<<saved.SelectedVehicle))!=0 &&
        saved.EngineLevel>=0 &&
        saved.EngineLevel<=3 &&
        saved.GripLevel>=0 &&
        saved.GripLevel<=3;
}
inline bool ValidSixCarSurfaceEvidence(
    SixCarTrackSurfaceEvidence e,
    WorldScaleVariantId circuit,
    SixCarSetupSettings settings) noexcept
{
    return e.Circuit==circuit &&
        e.Session!=0u && e.SourceFrame!=0u &&
        e.AuthenticRoadMaterialObserved &&
        e.ActualTireContactObserved &&
        e.OriginalRaceRulesPreserved &&
        std::isfinite(e.ClockSeconds) &&
        e.ClockSeconds>0.0 &&
        e.ClockSeconds<=86400.0 &&
        std::isfinite(e.MeasuredGripFraction) &&
        e.MeasuredGripFraction>=settings.MinimumMeasuredGripFraction &&
        e.MeasuredGripFraction<=settings.MaximumMeasuredGripFraction &&
        std::isfinite(e.SourceConfidence) &&
        e.SourceConfidence>=settings.MinimumSurfaceEvidenceConfidence &&
        e.SourceConfidence<=1.0;
}
inline bool ValidSixCarMechanicalSetup(
    SixCarSetupBase v,SixCarSetupSettings s={}) noexcept
{
    return ValidSixCarSetupSettings(s) &&
        std::isfinite(v.FrontSpringNm) &&
        std::isfinite(v.RearSpringNm) &&
        v.FrontSpringNm>=s.MinimumSpringNm &&
        v.FrontSpringNm<=s.MaximumSpringNm &&
        v.RearSpringNm>=s.MinimumSpringNm &&
        v.RearSpringNm<=s.MaximumSpringNm &&
        std::isfinite(v.FrontRideCm) &&
        std::isfinite(v.RearRideCm) &&
        v.FrontRideCm>=s.MinimumRideHeightCm &&
        v.FrontRideCm<=s.MaximumRideHeightCm &&
        v.RearRideCm>=s.MinimumRideHeightCm &&
        v.RearRideCm<=s.MaximumRideHeightCm &&
        std::isfinite(v.FrontDampingRatio) &&
        std::isfinite(v.RearDampingRatio) &&
        v.FrontDampingRatio>=0.2 &&
        v.FrontDampingRatio<=0.9 &&
        v.RearDampingRatio>=0.2 &&
        v.RearDampingRatio<=0.9 &&
        std::isfinite(v.FrontAntiRollNm) &&
        std::isfinite(v.RearAntiRollNm) &&
        v.FrontAntiRollNm>=10000.0 &&
        v.FrontAntiRollNm<=40000.0 &&
        v.RearAntiRollNm>=10000.0 &&
        v.RearAntiRollNm<=40000.0 &&
        std::isfinite(v.BrakeFrontFraction) &&
        v.BrakeFrontFraction>=s.MinimumBrakeFrontFraction &&
        v.BrakeFrontFraction<=s.MaximumBrakeFrontFraction &&
        std::isfinite(v.DifferentialLockFraction) &&
        v.DifferentialLockFraction>=0.0 &&
        v.DifferentialLockFraction<=0.8 &&
        std::isfinite(v.WingFrontDegrees) &&
        std::isfinite(v.WingRearDegrees) &&
        v.WingFrontDegrees>=0.0 &&
        v.WingFrontDegrees<=s.MaximumWingDegrees &&
        v.WingRearDegrees>=0.0 &&
        v.WingRearDegrees<=s.MaximumWingDegrees &&
        std::isfinite(v.FrontCamberDegrees) &&
        std::isfinite(v.RearCamberDegrees) &&
        v.FrontCamberDegrees>=-5.0 &&
        v.FrontCamberDegrees<=0.0 &&
        v.RearCamberDegrees>=-5.0 &&
        v.RearCamberDegrees<=0.0 &&
        std::isfinite(v.FrontToeDegrees) &&
        std::isfinite(v.RearToeDegrees) &&
        v.FrontToeDegrees>=-0.3 &&
        v.FrontToeDegrees<=0.3 &&
        v.RearToeDegrees>=-0.3 &&
        v.RearToeDegrees<=0.3 &&
        std::isfinite(v.TyrePressureKpa) &&
        v.TyrePressureKpa>=150.0 &&
        v.TyrePressureKpa<=320.0 &&
        std::isfinite(v.FinalDriveRatio) &&
        v.FinalDriveRatio>=2.5 &&
        v.FinalDriveRatio<=5.0;
}
inline SixCarSetupPlan BuildSixCarCircuitSetup(
    const WorldScaleVariantRecipe& circuit,
    GarageSnapshot saved,
    SixCarTrackSurfaceEvidence surface,
    SixCarSetupSettings settings={}) noexcept
{
    SixCarSetupPlan out;
    if(!ValidSixCarSetupSettings(settings) ||
       !ValidWorldScaleVariantRecipe(circuit) ||
       !ValidSixCarOriginalGarage(saved) ||
       !ValidSixCarSurfaceEvidence(
           surface,circuit.Id,settings))
        return out;
    const int car=saved.SelectedVehicle;
    const auto base=SixCarOriginalVehicleSetup[
        static_cast<unsigned>(car)];
    if(!ValidSixCarMechanicalSetup(base,settings))
        return out;
    const double cornerSeverity=std::clamp(
        5000.0/circuit.CornerRadiusCm,0.0,1.0);
    const double gripLoss=std::clamp(
        1.0-surface.MeasuredGripFraction,0.0,0.8);
    const double engineFactor=1.0+
        0.015*saved.EngineLevel;
    SixCarSetupBase v=base;
    v.FrontSpringNm*=1.0-0.07*cornerSeverity;
    v.RearSpringNm*=1.0-0.08*cornerSeverity;
    v.FrontRideCm+=1.1*gripLoss+
        0.30*cornerSeverity;
    v.RearRideCm+=1.3*gripLoss+
        0.35*cornerSeverity;
    v.FrontDampingRatio+=0.02*cornerSeverity;
    v.RearDampingRatio+=0.025*cornerSeverity;
    v.FrontAntiRollNm*=1.0-0.12*gripLoss;
    v.RearAntiRollNm*=1.0-0.15*gripLoss;
    v.BrakeFrontFraction=std::clamp(
        base.BrakeFrontFraction+0.02*gripLoss,
        settings.MinimumBrakeFrontFraction,
        settings.MaximumBrakeFrontFraction);
    v.DifferentialLockFraction=std::clamp(
        base.DifferentialLockFraction-0.10*gripLoss,
        0.1,0.8);
    v.WingFrontDegrees+=2.5*cornerSeverity;
    v.WingRearDegrees+=3.0*cornerSeverity;
    v.FrontCamberDegrees-=0.12*cornerSeverity;
    v.RearCamberDegrees-=0.10*cornerSeverity;
    v.FrontToeDegrees+=0.015*cornerSeverity;
    v.RearToeDegrees+=0.025*cornerSeverity;
    v.TyrePressureKpa+=
        4.0*gripLoss-2.0*cornerSeverity;
    v.FinalDriveRatio*=1.0+
        0.035*cornerSeverity/engineFactor;
    if(!ValidSixCarMechanicalSetup(v,settings))
        return out;
    out.VehicleId=car;
    out.Circuit=circuit.Id;
    out.Values=v;
    out.CornerSeverity=cornerSeverity;
    out.TrackCurvatureProxyPerCm=
        1.0/circuit.CornerRadiusCm;
    out.MeasuredGripFraction=surface.MeasuredGripFraction;
    out.RideStanceCentimeters=
        (v.FrontRideCm+v.RearRideCm)*0.5;
    out.PitchStanceCentimeters=
        v.RearRideCm-v.FrontRideCm;
    out.PlannedAeroBalanceFront=
        (v.WingFrontDegrees+1.0)/
        (v.WingFrontDegrees+v.WingRearDegrees+2.0);
    out.EstimatedSpringBalanceFront=
        v.FrontSpringNm/
        (v.FrontSpringNm+v.RearSpringNm);
    out.SetupDifferenceNorm=std::abs(
        v.FrontSpringNm-base.FrontSpringNm)/
        base.FrontSpringNm+
        std::abs(v.RearSpringNm-base.RearSpringNm)/
        base.RearSpringNm+
        std::abs(v.FrontRideCm-base.FrontRideCm)/
        base.FrontRideCm+
        std::abs(v.RearRideCm-base.RearRideCm)/
        base.RearRideCm;
    out.OriginalUpgradeEngineLevel=
        static_cast<unsigned>(saved.EngineLevel);
    out.OriginalUpgradeGripLevel=
        static_cast<unsigned>(saved.GripLevel);
    out.SourceSession=surface.Session;
    out.SourceFrame=surface.SourceFrame;
    out.Valid=std::isfinite(out.CornerSeverity) &&
        std::isfinite(out.RideStanceCentimeters) &&
        std::isfinite(out.PlannedAeroBalanceFront) &&
        std::isfinite(out.SetupDifferenceNorm) &&
        out.PlannedAeroBalanceFront>0.0 &&
        out.EstimatedSpringBalanceFront>0.0;
    return out;
}
struct SixCarSetupComparison final {
    bool Valid=false;
    int VehicleId=-1;
    double FrontSpringDeltaNm=0.0;
    double RearSpringDeltaNm=0.0;
    double FrontRideDeltaCm=0.0;
    double RearRideDeltaCm=0.0;
    double FrontBrakeFractionDelta=0.0;
    double RearWingDeltaDegrees=0.0;
    double FinalDriveRatioDelta=0.0;
    double TotalAbsoluteChange=0.0;
    bool NoNativeCarModification=true;
};
inline SixCarSetupComparison CompareSixCarSetup(
    const SixCarSetupPlan& first,
    const SixCarSetupPlan& next) noexcept
{
    SixCarSetupComparison out;
    // Comparing settings across two different tracks, garage upgrade
    // configurations, or untrusted physical source records creates a
    // misleading result. Do not turn a forged preview receipt into a
    // seemingly authentic mechanical recommendation. Different frame
    // ordinals within ONE source session remain valid: real grip changes
    // necessarily arrive at later physics frames.
    if(!first.Valid || !next.Valid ||
       first.VehicleId<0 || first.VehicleId>=VehicleCount ||
       first.VehicleId!=next.VehicleId ||
       first.Circuit!=next.Circuit ||
       static_cast<unsigned>(first.Circuit)>=WorldScaleVariantCount ||
       first.SourceSession==0u ||
       first.SourceSession!=next.SourceSession ||
       first.SourceFrame==0u || next.SourceFrame==0u ||
       first.OriginalUpgradeEngineLevel>3u ||
       first.OriginalUpgradeGripLevel>3u ||
       first.OriginalUpgradeEngineLevel!=next.OriginalUpgradeEngineLevel ||
       first.OriginalUpgradeGripLevel!=next.OriginalUpgradeGripLevel ||
       !first.SourceOnly || !next.SourceOnly ||
       !first.NativeChaosConfigurationUnchanged ||
       !next.NativeChaosConfigurationUnchanged ||
       !first.OriginalGarageStateUnchanged ||
       !next.OriginalGarageStateUnchanged ||
       !std::isfinite(first.MeasuredGripFraction) ||
       !std::isfinite(next.MeasuredGripFraction) ||
       first.MeasuredGripFraction<=0.0 ||
       first.MeasuredGripFraction>1.0 ||
       next.MeasuredGripFraction<=0.0 ||
       next.MeasuredGripFraction>1.0 ||
       !ValidSixCarMechanicalSetup(first.Values) ||
       !ValidSixCarMechanicalSetup(next.Values))
        return out;
    out.VehicleId=first.VehicleId;
    out.FrontSpringDeltaNm=
        next.Values.FrontSpringNm-first.Values.FrontSpringNm;
    out.RearSpringDeltaNm=
        next.Values.RearSpringNm-first.Values.RearSpringNm;
    out.FrontRideDeltaCm=
        next.Values.FrontRideCm-first.Values.FrontRideCm;
    out.RearRideDeltaCm=
        next.Values.RearRideCm-first.Values.RearRideCm;
    out.FrontBrakeFractionDelta=
        next.Values.BrakeFrontFraction-first.Values.BrakeFrontFraction;
    out.RearWingDeltaDegrees=
        next.Values.WingRearDegrees-first.Values.WingRearDegrees;
    out.FinalDriveRatioDelta=
        next.Values.FinalDriveRatio-first.Values.FinalDriveRatio;
    out.TotalAbsoluteChange=
        std::abs(out.FrontSpringDeltaNm)/40000.0+
        std::abs(out.RearSpringDeltaNm)/40000.0+
        std::abs(out.FrontRideDeltaCm)/10.0+
        std::abs(out.RearRideDeltaCm)/10.0+
        std::abs(out.FrontBrakeFractionDelta)+
        std::abs(out.RearWingDeltaDegrees)/10.0+
        std::abs(out.FinalDriveRatioDelta)/4.0;
    out.Valid=std::isfinite(out.TotalAbsoluteChange);
    return out;
}
} // namespace latebrake
