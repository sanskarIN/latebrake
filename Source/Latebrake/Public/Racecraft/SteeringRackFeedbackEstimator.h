#pragma once
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

// Distinct measured steering-rack feedback experiment. Its rack parameter
// schema is not the same as the core kinematic steering-rack model.
namespace latebrake::racecraft::rackfeedback
{
// A diagnostic of measured tire alignment and rack loads. This module
// never creates a force-feedback HID effect or manipulates actual Chaos
// steering. All effects require original physical wheel evidence.
enum class SteeringFeedbackCondition : std::uint8_t
{
    Unknown, Centered, NormalLoad, HeavyCornering,
    TireSaturated, LowRoadGrip, AlignmentFault,
    ImpactOrKerb, SteeringRateLimited
};
struct SteeringRackParameters final
{
    double SteeringRatio=14.0;
    double WheelbaseCm=270.0;
    double FrontTrackCm=160.0;
    double MechanicalTrailCm=6.5;
    double PneumaticTrailCm=4.5;
    double CasterAngleRadians=0.12;
    double KingpinInclinationRadians=0.09;
    double ScrubRadiusCm=3.4;
    double NominalRackTravelCm=15.0;
    double MaximumSteeringWheelAngleRadians=7.85;
    double MaximumRoadWheelAngleRadians=0.65;
    double MaximumRackForceN=9500.0;
    double MaximumFeedbackTorqueNm=22.0;
    double SteeringInertiaKgM2=0.11;
    double SteeringViscousDampingNms=0.28;
    double CoulombFrictionNm=0.30;
    double CenterSpringNmPerRad=0.60;
    double CenterDampingGain=0.60;
    double RoadWheelRateLimitRadS=3.0;
    double MaxSteeringWheelRateRadS=20.0;
    double MinimumRoadGrip=0.15;
    double MaximumRoadGrip=2.5;
    double MaximumVehicleSpeedCmS=MaximumPhysicalSpeedCmS;
    double MinimumLoadN=100.0;
    double MaximumLoadN=20000.0;
    double MaximumLateralForceN=25000.0;
    double MaximumAligningMomentNm=2800.0;
    double MaximumCamberRadians=0.40;
    double MaximumToeRadians=0.30;
    double MaximumSlipRadians=1.20;
    double MaximumYawRateRadiansSecond=8.0;
    double MaximumMeasuredRackPositionCm=18.0;
    double MaximumMeasuredColumnTorqueNm=35.0;
    double MaximumReferenceErrorRadians=0.08;
    double MaximumFrameSeconds=0.10;
    double MaximumClockSkewSeconds=0.04;
    double MinimumSensorConfidence=0.75;
    double LowGripFeedbackMultiplier=0.55;
    double LoadedTireFeedbackGain=0.22;
    double KerbKickGain=0.18;
    double SelfCenterGain=0.85;
    double OverspeedSteerReduction=0.30;
    double MaxAllowedDisagreementNm=8.0;
    double MaximumSmoothTorqueChangeNmS=80.0;
    double RecoveryTorqueHoldSeconds=0.10;
    double MinimumAckermannFraction=0.5;
    bool RequirePhysicalSteeringRack=true;
    bool RequireFrontWheelContacts=true;
    bool OriginalMapOnly=true;
};
struct SteeringRackWheelSample final
{
    WheelObservation Wheel{};
    double MeasuredLateralForceN=0.0;
    double MeasuredAligningMomentNm=0.0;
    double MeasuredCamberRadians=0.0;
    double MeasuredToeRadians=0.0;
    double MeasuredSteerAngleRadians=0.0;
    double ActualTrailCm=6.5;
    double MeasuredHubLateralVelocityCmS=0.0;
    double SensorConfidence=1.0;
    bool ActualWheelSteeringComponent=false;
    bool AlignmentSensorReady=false;
    bool OriginalContactPatchQualified=false;
};
struct SteeringRackObservation final
{
    RaceIdentity Identity{};
    std::array<SteeringRackWheelSample,2> FrontWheels{};
    double ActualSteeringColumnAngleRadians=0.0;
    double ActualSteeringColumnSpeedRadS=0.0;
    double DriverSteeringInput=0.0;
    double MeasuredRackTravelCm=0.0;
    double MeasuredColumnTorqueNm=0.0;
    double MeasuredSteeringAssistFraction=0.0;
    double MeasuredVehicleSpeedCmS=0.0;
    double MeasuredYawRateRadiansSecond=0.0;
    double MeasuredRoadGrip=1.0;
    double PhysicsClockSeconds=0.0;
    double DeltaSeconds=0.0;
    bool ActualChaosSteeringReady=false;
    bool OriginalMapVerified=false;
    bool DriverInputAuthoritative=false;
};
struct SteeringWheelFeedback final
{
    double SteeringAngleRadians=0.0;
    double ExpectedAlignmentTorqueNm=0.0;
    double ExpectedScrubTorqueNm=0.0;
    double ExpectedCasterTorqueNm=0.0;
    double TireSaturationFraction=0.0;
    double EffectiveTrailCm=0.0;
    double RoadGrip=0.0;
    double MeasuredLateralForceN=0.0;
    double MeasuredAligningMomentNm=0.0;
    double ContributionTorqueNm=0.0;
    double SlipAngleRadians=0.0;
    bool LateralTireSaturation=false;
    bool ContactQualified=false;
    bool Valid=false;
};
struct SteeringRackFeedback final
{
    RaceIdentity Identity{};
    std::array<SteeringWheelFeedback,2> Wheels{};
    double RequestedRoadWheelAngleRadians=0.0;
    double MeasuredMeanRoadWheelAngleRadians=0.0;
    double EstimatedAckermannErrorRadians=0.0;
    double EstimatedRackForceN=0.0;
    double ExpectedColumnTorqueNm=0.0;
    double MeasuredColumnTorqueNm=0.0;
    double ColumnTorqueResidualNm=0.0;
    double SteeringRateRadiansSecond=0.0;
    double SteeringRateCapRadiansSecond=0.0;
    double SteeringWheelReturnTorqueNm=0.0;
    double SteeringInertiaTorqueNm=0.0;
    double CenterSpringTorqueNm=0.0;
    double FrictionTorqueNm=0.0;
    double AlignmentTorqueNm=0.0;
    double RoadImpactTorqueNm=0.0;
    double GripFeedbackGain=0.0;
    double PowerAssistMultiplier=0.0;
    double RecommendedForceFeedbackTorqueNm=0.0;
    double Confidence=0.0;
    double FrontLateralForceN=0.0;
    double MaximumFrontSlipRadians=0.0;
    double SteerAngleErrorRadians=0.0;
    int ContactWheels=0;
    int SaturatedWheels=0;
    SteeringFeedbackCondition Condition=SteeringFeedbackCondition::Unknown;
    bool ActualHidCommandIssued=false;
    bool ActualChaosSteeringModified=false;
    bool Valid=false;
};
inline bool ValidSteeringRackParameters(
    SteeringRackParameters p) noexcept
{
    return Finite(p.SteeringRatio) && p.SteeringRatio>1.0 &&
        Finite(p.WheelbaseCm) && p.WheelbaseCm>100.0 &&
        Finite(p.FrontTrackCm) && p.FrontTrackCm>80.0 &&
        Finite(p.MechanicalTrailCm) &&
        p.MechanicalTrailCm>=0.0 &&
        Finite(p.PneumaticTrailCm) &&
        p.PneumaticTrailCm>=0.0 &&
        Finite(p.CasterAngleRadians) &&
        std::abs(p.CasterAngleRadians)<Pi/2.0 &&
        Finite(p.KingpinInclinationRadians) &&
        std::abs(p.KingpinInclinationRadians)<Pi/2.0 &&
        Finite(p.ScrubRadiusCm) &&
        std::abs(p.ScrubRadiusCm)<30.0 &&
        Finite(p.NominalRackTravelCm) &&
        p.NominalRackTravelCm>0.0 &&
        Finite(p.MaximumSteeringWheelAngleRadians) &&
        p.MaximumSteeringWheelAngleRadians>0.0 &&
        Finite(p.MaximumRoadWheelAngleRadians) &&
        p.MaximumRoadWheelAngleRadians>0.0 &&
        p.MaximumRoadWheelAngleRadians<Pi/2.0 &&
        Finite(p.MaximumRackForceN) &&
        p.MaximumRackForceN>0.0 &&
        Finite(p.MaximumFeedbackTorqueNm) &&
        p.MaximumFeedbackTorqueNm>0.0 &&
        Finite(p.SteeringInertiaKgM2) &&
        p.SteeringInertiaKgM2>0.0 &&
        Finite(p.SteeringViscousDampingNms) &&
        p.SteeringViscousDampingNms>=0.0 &&
        Finite(p.CoulombFrictionNm) &&
        p.CoulombFrictionNm>=0.0 &&
        Finite(p.CenterSpringNmPerRad) &&
        p.CenterSpringNmPerRad>=0.0 &&
        Finite(p.CenterDampingGain) &&
        p.CenterDampingGain>=0.0 &&
        Finite(p.RoadWheelRateLimitRadS) &&
        p.RoadWheelRateLimitRadS>0.0 &&
        Finite(p.MaxSteeringWheelRateRadS) &&
        p.MaxSteeringWheelRateRadS>0.0 &&
        Finite(p.MinimumRoadGrip) &&
        p.MinimumRoadGrip>0.0 &&
        Finite(p.MaximumRoadGrip) &&
        p.MaximumRoadGrip>p.MinimumRoadGrip &&
        Finite(p.MaximumVehicleSpeedCmS) &&
        p.MaximumVehicleSpeedCmS>0.0 &&
        Finite(p.MinimumLoadN) && p.MinimumLoadN>=0.0 &&
        Finite(p.MaximumLoadN) &&
        p.MaximumLoadN>p.MinimumLoadN &&
        Finite(p.MaximumLateralForceN) &&
        p.MaximumLateralForceN>0.0 &&
        Finite(p.MaximumAligningMomentNm) &&
        p.MaximumAligningMomentNm>0.0 &&
        Finite(p.MaximumCamberRadians) &&
        p.MaximumCamberRadians>0.0 &&
        Finite(p.MaximumToeRadians) &&
        p.MaximumToeRadians>0.0 &&
        Finite(p.MaximumSlipRadians) &&
        p.MaximumSlipRadians>0.0 &&
        Finite(p.MaximumYawRateRadiansSecond) &&
        p.MaximumYawRateRadiansSecond>0.0 &&
        Finite(p.MaximumMeasuredRackPositionCm) &&
        p.MaximumMeasuredRackPositionCm>0.0 &&
        Finite(p.MaximumMeasuredColumnTorqueNm) &&
        p.MaximumMeasuredColumnTorqueNm>0.0 &&
        Finite(p.MaximumReferenceErrorRadians) &&
        p.MaximumReferenceErrorRadians>=0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaximumFrameTimeSeconds &&
        Finite(p.MaximumClockSkewSeconds) &&
        p.MaximumClockSkewSeconds>=0.0 &&
        Finite(p.MinimumSensorConfidence) &&
        p.MinimumSensorConfidence>0.0 &&
        p.MinimumSensorConfidence<=1.0 &&
        Finite(p.LowGripFeedbackMultiplier) &&
        p.LowGripFeedbackMultiplier>=0.0 &&
        Finite(p.LoadedTireFeedbackGain) &&
        p.LoadedTireFeedbackGain>=0.0 &&
        Finite(p.KerbKickGain) && p.KerbKickGain>=0.0 &&
        Finite(p.SelfCenterGain) && p.SelfCenterGain>=0.0 &&
        Finite(p.OverspeedSteerReduction) &&
        p.OverspeedSteerReduction>=0.0 &&
        Finite(p.MaxAllowedDisagreementNm) &&
        p.MaxAllowedDisagreementNm>=0.0 &&
        Finite(p.MaximumSmoothTorqueChangeNmS) &&
        p.MaximumSmoothTorqueChangeNmS>0.0 &&
        Finite(p.RecoveryTorqueHoldSeconds) &&
        p.RecoveryTorqueHoldSeconds>=0.0 &&
        Finite(p.MinimumAckermannFraction) &&
        p.MinimumAckermannFraction>0.0;
}
inline bool ValidSteeringRackWheel(
    SteeringRackWheelSample s,SteeringRackParameters p) noexcept
{
    return ValidWheelObservation(s.Wheel) &&
        (!p.RequireFrontWheelContacts ||
         (s.Wheel.ContactVerified &&
          s.Wheel.PhysicalContact &&
          s.OriginalContactPatchQualified)) &&
        (!p.RequirePhysicalSteeringRack ||
         (s.ActualWheelSteeringComponent &&
          s.AlignmentSensorReady)) &&
        Finite(s.MeasuredLateralForceN) &&
        std::abs(s.MeasuredLateralForceN)<=
            p.MaximumLateralForceN &&
        Finite(s.MeasuredAligningMomentNm) &&
        std::abs(s.MeasuredAligningMomentNm)<=
            p.MaximumAligningMomentNm &&
        Finite(s.MeasuredCamberRadians) &&
        std::abs(s.MeasuredCamberRadians)<=
            p.MaximumCamberRadians &&
        Finite(s.MeasuredToeRadians) &&
        std::abs(s.MeasuredToeRadians)<=
            p.MaximumToeRadians &&
        Finite(s.MeasuredSteerAngleRadians) &&
        std::abs(s.MeasuredSteerAngleRadians)<=
            p.MaximumRoadWheelAngleRadians &&
        Finite(s.ActualTrailCm) &&
        s.ActualTrailCm>=0.0 &&
        s.ActualTrailCm<50.0 &&
        Finite(s.MeasuredHubLateralVelocityCmS) &&
        std::abs(s.MeasuredHubLateralVelocityCmS)<=
            p.MaximumVehicleSpeedCmS &&
        Finite(s.SensorConfidence) &&
        s.SensorConfidence>=p.MinimumSensorConfidence &&
        s.SensorConfidence<=1.0 &&
        s.Wheel.NormalLoadN>=p.MinimumLoadN &&
        s.Wheel.NormalLoadN<=p.MaximumLoadN;
}
inline bool ValidSteeringRackObservation(
    const SteeringRackObservation& s,
    SteeringRackParameters p) noexcept
{
    if (!ValidSteeringRackParameters(p) ||
        !PhysicalActorIdentityValid(s.Identity) ||
        s.Identity.Frame==0 ||
        !s.ActualChaosSteeringReady ||
        !s.OriginalMapVerified ||
        !s.DriverInputAuthoritative ||
        !Finite(s.ActualSteeringColumnAngleRadians) ||
        std::abs(s.ActualSteeringColumnAngleRadians)>
            p.MaximumSteeringWheelAngleRadians ||
        !Finite(s.ActualSteeringColumnSpeedRadS) ||
        std::abs(s.ActualSteeringColumnSpeedRadS)>
            p.MaxSteeringWheelRateRadS ||
        !Finite(s.DriverSteeringInput) ||
        std::abs(s.DriverSteeringInput)>1.0 ||
        !Finite(s.MeasuredRackTravelCm) ||
        std::abs(s.MeasuredRackTravelCm)>
            p.MaximumMeasuredRackPositionCm ||
        !Finite(s.MeasuredColumnTorqueNm) ||
        std::abs(s.MeasuredColumnTorqueNm)>
            p.MaximumMeasuredColumnTorqueNm ||
        !Finite(s.MeasuredSteeringAssistFraction) ||
        s.MeasuredSteeringAssistFraction<0.0 ||
        s.MeasuredSteeringAssistFraction>1.0 ||
        !Finite(s.MeasuredVehicleSpeedCmS) ||
        s.MeasuredVehicleSpeedCmS<0.0 ||
        s.MeasuredVehicleSpeedCmS>
            p.MaximumVehicleSpeedCmS ||
        !Finite(s.MeasuredYawRateRadiansSecond) ||
        std::abs(s.MeasuredYawRateRadiansSecond)>
            p.MaximumYawRateRadiansSecond ||
        !Finite(s.MeasuredRoadGrip) ||
        s.MeasuredRoadGrip<p.MinimumRoadGrip ||
        s.MeasuredRoadGrip>p.MaximumRoadGrip ||
        !Finite(s.PhysicsClockSeconds) ||
        s.PhysicsClockSeconds<0.0 ||
        !Finite(s.DeltaSeconds) ||
        s.DeltaSeconds<=0.0 ||
        s.DeltaSeconds>p.MaximumFrameSeconds)
        return false;
    return ValidSteeringRackWheel(s.FrontWheels[0],p) &&
        ValidSteeringRackWheel(s.FrontWheels[1],p);
}
inline SteeringWheelFeedback EvaluateSteeringWheelFeedback(
    SteeringRackWheelSample s,double roadGrip,
    SteeringRackParameters p) noexcept
{
    SteeringWheelFeedback out;
    if (!ValidSteeringRackWheel(s,p) ||
        !Finite(roadGrip) ||
        roadGrip<p.MinimumRoadGrip ||
        roadGrip>p.MaximumRoadGrip)
        return out;
    out.SteeringAngleRadians=s.MeasuredSteerAngleRadians;
    out.MeasuredLateralForceN=s.MeasuredLateralForceN;
    out.MeasuredAligningMomentNm=
        s.MeasuredAligningMomentNm;
    out.SlipAngleRadians=s.Wheel.SlipAngleRadians;
    out.EffectiveTrailCm=
        s.ActualTrailCm+p.PneumaticTrailCm*
            (1.0-Clamp01(std::abs(
                s.Wheel.SlipAngleRadians)/p.MaximumSlipRadians));
    out.ExpectedAlignmentTorqueNm=
        s.MeasuredLateralForceN*out.EffectiveTrailCm/100.0;
    out.ExpectedCasterTorqueNm=
        s.Wheel.NormalLoadN*
        std::sin(p.CasterAngleRadians)*
        s.MeasuredSteerAngleRadians*
        p.MechanicalTrailCm/100.0;
    out.ExpectedScrubTorqueNm=
        s.MeasuredLateralForceN*p.ScrubRadiusCm/100.0*
        std::sin(p.KingpinInclinationRadians);
    const double limit=std::max(
        1.0,s.Wheel.NormalLoadN*roadGrip);
    out.TireSaturationFraction=std::abs(
        s.MeasuredLateralForceN)/limit;
    out.RoadGrip=roadGrip;
    out.LateralTireSaturation=
        out.TireSaturationFraction>1.0;
    out.ContributionTorqueNm=std::clamp(
        out.ExpectedAlignmentTorqueNm+
        out.ExpectedCasterTorqueNm+
        out.ExpectedScrubTorqueNm+
        s.MeasuredAligningMomentNm,
        -p.MaximumFeedbackTorqueNm,
        p.MaximumFeedbackTorqueNm);
    out.ContactQualified=s.Wheel.ContactVerified &&
        s.Wheel.PhysicalContact;
    out.Valid=Finite(out.ContributionTorqueNm) &&
        Finite(out.TireSaturationFraction) &&
        Finite(out.EffectiveTrailCm);
    return out;
}
inline SteeringRackFeedback EstimateRackFeedback(
    const SteeringRackObservation& s,
    SteeringRackParameters p={}) noexcept
{
    SteeringRackFeedback out;
    if (!ValidSteeringRackObservation(s,p))
        return out;
    out.Identity=s.Identity;
    out.Confidence=1.0;
    for(std::size_t i=0;i<2;++i)
    {
        const auto wheel=EvaluateSteeringWheelFeedback(
            s.FrontWheels[i],s.MeasuredRoadGrip,p);
        if (!wheel.Valid)return {};
        out.Wheels[i]=wheel;
        out.FrontLateralForceN+=
            wheel.MeasuredLateralForceN;
        out.AlignmentTorqueNm+=
            wheel.ContributionTorqueNm;
        out.MaximumFrontSlipRadians=std::max(
            out.MaximumFrontSlipRadians,
            std::abs(wheel.SlipAngleRadians));
        out.ContactWheels+=wheel.ContactQualified?1:0;
        out.SaturatedWheels+=wheel.LateralTireSaturation?1:0;
        out.Confidence=std::min(
            out.Confidence,s.FrontWheels[i].SensorConfidence);
        if (s.FrontWheels[i].Wheel.Surface==
            ContactSurface::PaintedKerb)
            out.RoadImpactTorqueNm+=p.KerbKickGain*
                wheel.ContributionTorqueNm;
    }
    out.RequestedRoadWheelAngleRadians=
        s.DriverSteeringInput*p.MaximumRoadWheelAngleRadians;
    out.MeasuredMeanRoadWheelAngleRadians=0.5*(
        out.Wheels[0].SteeringAngleRadians+
        out.Wheels[1].SteeringAngleRadians);
    out.EstimatedAckermannErrorRadians=
        std::abs(out.Wheels[0].SteeringAngleRadians-
                 out.Wheels[1].SteeringAngleRadians);
    out.SteerAngleErrorRadians=
        out.RequestedRoadWheelAngleRadians-
        out.MeasuredMeanRoadWheelAngleRadians;
    out.SteeringRateRadiansSecond=
        s.ActualSteeringColumnSpeedRadS/p.SteeringRatio;
    out.SteeringRateCapRadiansSecond=
        p.RoadWheelRateLimitRadS*
        (1.0-p.OverspeedSteerReduction*
         Clamp01(SafeDivision(
            s.MeasuredVehicleSpeedCmS,
            p.MaximumVehicleSpeedCmS)));
    out.CenterSpringTorqueNm=
        -s.ActualSteeringColumnAngleRadians*
        p.CenterSpringNmPerRad;
    out.SteeringWheelReturnTorqueNm=
        -out.MeasuredMeanRoadWheelAngleRadians*
        p.SelfCenterGain;
    out.FrictionTorqueNm=
        s.ActualSteeringColumnSpeedRadS>0.0 ?
        -p.CoulombFrictionNm :
        s.ActualSteeringColumnSpeedRadS<0.0 ?
        p.CoulombFrictionNm:0.0;
    out.SteeringInertiaTorqueNm=
        -p.SteeringViscousDampingNms*
        s.ActualSteeringColumnSpeedRadS;
    out.PowerAssistMultiplier=1.0-
        0.75*s.MeasuredSteeringAssistFraction;
    out.GripFeedbackGain=std::clamp(
        s.MeasuredRoadGrip,
        p.LowGripFeedbackMultiplier,1.5);
    const double rackTorque=
        out.AlignmentTorqueNm/
        p.SteeringRatio+
        out.CenterSpringTorqueNm+
        out.FrictionTorqueNm+
        out.SteeringInertiaTorqueNm+
        out.SteeringWheelReturnTorqueNm+
        out.RoadImpactTorqueNm;
    out.ExpectedColumnTorqueNm=std::clamp(
        rackTorque*out.PowerAssistMultiplier*
        out.GripFeedbackGain,
        -p.MaximumFeedbackTorqueNm,
        p.MaximumFeedbackTorqueNm);
    out.MeasuredColumnTorqueNm=
        s.MeasuredColumnTorqueNm;
    out.ColumnTorqueResidualNm=
        out.ExpectedColumnTorqueNm-
        out.MeasuredColumnTorqueNm;
    out.EstimatedRackForceN=std::clamp(
        SafeDivision(out.AlignmentTorqueNm,
                     p.NominalRackTravelCm/100.0),
        -p.MaximumRackForceN,
        p.MaximumRackForceN);
    out.RecommendedForceFeedbackTorqueNm=
        out.ExpectedColumnTorqueNm;
    out.Condition=out.SaturatedWheels>0 ?
        SteeringFeedbackCondition::TireSaturated :
        s.MeasuredRoadGrip<0.6 ?
            SteeringFeedbackCondition::LowRoadGrip :
        std::abs(out.ColumnTorqueResidualNm)>
            p.MaxAllowedDisagreementNm ?
            SteeringFeedbackCondition::AlignmentFault :
        std::abs(out.RoadImpactTorqueNm)>1.0 ?
            SteeringFeedbackCondition::ImpactOrKerb :
        std::abs(out.SteeringRateRadiansSecond)>
            out.SteeringRateCapRadiansSecond ?
            SteeringFeedbackCondition::SteeringRateLimited :
        std::abs(s.DriverSteeringInput)<0.02 ?
            SteeringFeedbackCondition::Centered :
            SteeringFeedbackCondition::NormalLoad;
    out.ActualHidCommandIssued=false;
    out.ActualChaosSteeringModified=false;
    out.Valid=out.ContactWheels==2 &&
        Finite(out.EstimatedRackForceN) &&
        Finite(out.ExpectedColumnTorqueNm) &&
        Finite(out.ColumnTorqueResidualNm) &&
        Finite(out.RecommendedForceFeedbackTorqueNm);
    return out;
}
class PhysicalSteeringFeedbackSupervisor final
{
public:
    bool Configure(SteeringRackParameters p) noexcept
    {
        if (!ValidSteeringRackParameters(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Epoch={};
        SmoothTorqueNm=0.0;
        Qualified=0;
        Rejected=0;
        SaturatedFrames=0;
        SourceFaultFrames=0;
        PeakTorqueNm=0.0;
        MaximumResidualNm=0.0;
        HasEpoch=false;
    }
    SteeringRackFeedback Step(
        const SteeringRackObservation& o) noexcept
    {
        SteeringRackFeedback result=EstimateRackFeedback(o,Settings);
        const bool sameEpoch=HasEpoch &&
            Epoch.Session==o.Identity.Session &&
            Epoch.SessionRevision==o.Identity.SessionRevision &&
            Epoch.Actor==o.Identity.Actor;
        if (!result.Valid ||
            (sameEpoch && o.Identity.Frame<=Epoch.Frame))
        {
            ++Rejected;
            Last={};
            SmoothTorqueNm=0.0;
            return {};
        }
        if (!sameEpoch)
            SmoothTorqueNm=0.0;
        SmoothTorqueNm=RateLimited(
            SmoothTorqueNm,
            result.RecommendedForceFeedbackTorqueNm,
            Settings.MaximumSmoothTorqueChangeNmS,
            Settings.MaximumSmoothTorqueChangeNmS,
            o.DeltaSeconds);
        result.RecommendedForceFeedbackTorqueNm=
            SmoothTorqueNm;
        Last=result;
        Epoch=o.Identity;
        HasEpoch=true;
        ++Qualified;
        SaturatedFrames+=result.SaturatedWheels>0?1U:0U;
        SourceFaultFrames+=
            result.Condition==
                SteeringFeedbackCondition::AlignmentFault?1U:0U;
        PeakTorqueNm=std::max(
            PeakTorqueNm,std::abs(SmoothTorqueNm));
        MaximumResidualNm=std::max(
            MaximumResidualNm,
            std::abs(result.ColumnTorqueResidualNm));
        return Last;
    }
    SteeringRackFeedback Snapshot() const noexcept{return Last;}
    std::uint64_t Frames() const noexcept{return Qualified;}
    std::uint64_t Rejections() const noexcept{return Rejected;}
    std::uint64_t SaturatedTireFrames() const noexcept
    {return SaturatedFrames;}
    std::uint64_t AlignmentWarningFrames() const noexcept
    {return SourceFaultFrames;}
    double PeakFeedbackTorqueNm() const noexcept
    {return PeakTorqueNm;}
    double LargestColumnTorqueResidualNm() const noexcept
    {return MaximumResidualNm;}
private:
    SteeringRackParameters Settings{};
    SteeringRackFeedback Last{};
    RaceIdentity Epoch{};
    double SmoothTorqueNm=0.0;
    double PeakTorqueNm=0.0;
    double MaximumResidualNm=0.0;
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    std::uint64_t SaturatedFrames=0;
    std::uint64_t SourceFaultFrames=0;
    bool HasEpoch=false;
};
} // namespace latebrake::racecraft::rackfeedback
