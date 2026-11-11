#pragma once
#include "Racecraft/SuspensionForceModel.h"
#include "Racecraft/SteeringKinematics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
struct WheelAlignmentParameters final
{
    double FrontStaticCamberRadians=-0.045;
    double RearStaticCamberRadians=-0.028;
    double FrontStaticToeRadians=0.002;
    double RearStaticToeRadians=0.003;
    double CamberGainRadiansPerCm=-0.0020;
    double ToeGainRadiansPerCm=0.0005;
    double RollCamberGain=0.85;
    double PitchCamberGain=0.16;
    double SteeringCamberGain=-0.10;
    double MaximumAllowedCamberRadians=0.35;
    double MaximumAllowedToeRadians=0.12;
    double MaximumBumpSteerRadians=0.08;
    double MaximumCasterTrailCm=8.0;
    double NominalCasterTrailCm=4.5;
    double MechanicalTrailLeverCm=5.0;
    double ToeTireWearGain=0.02;
    double CamberTireWearGain=0.01;
    double CamberLateralForceGain=0.08;
    double CamberLongitudinalForceLoss=0.05;
    double MaximumCompressionTravelCm=30.0;
    double MaximumVehicleRollRadians=0.5;
    double MaximumVehiclePitchRadians=0.4;
    double MaximumFrameSeconds=0.10;
    bool RequireFourPhysicalSuspensionSensors=true;
    bool RequireSteeringRackSensor=true;
    bool NeverAdjustActualWheelSetup=true;
};
struct WheelAlignmentObservation final
{
    PhysicalVehicleFrame Chassis{};
    SuspensionForceFrame Suspension{};
    SteeringWheelOutput Steering{};
    double DeltaSeconds=0.0;
    bool OriginalWheelAlignmentReadoutReady=false;
    bool ActualRideGeometryVerified=false;
};
struct AlignmentWheelOutput final
{
    double CamberRadians=0.0;
    double ToeRadians=0.0;
    double BumpSteerRadians=0.0;
    double CasterTrailCm=0.0;
    double CamberThrustN=0.0;
    double AligningMomentNm=0.0;
    double EffectiveLateralGripScale=1.0;
    double EffectiveLongitudinalGripScale=1.0;
    double WearRateMultiplier=1.0;
    double SuspensionCompressionCm=0.0;
    double SteerAngleRadians=0.0;
    double NormalLoadN=0.0;
    bool RealWheelContact=false;
    bool Qualified=false;
};
struct WheelAlignmentReview final
{
    std::array<AlignmentWheelOutput,4> Wheels{};
    double FrontToeTotalRadians=0.0;
    double RearToeTotalRadians=0.0;
    double FrontCamberAverageRadians=0.0;
    double RearCamberAverageRadians=0.0;
    double LeftRightCamberDifferenceRadians=0.0;
    double TotalCamberThrustN=0.0;
    double TotalAligningTorqueNm=0.0;
    double PredictedStraightLinePullN=0.0;
    double MeanEffectiveLateralGripScale=0.0;
    double MeanEffectiveLongitudinalGripScale=0.0;
    double MeanWearRateMultiplier=0.0;
    double MaximumBumpSteerRadians=0.0;
    int ContactQualifiedWheels=0;
    bool ExcessiveCamber=false;
    bool ExcessiveToe=false;
    bool HasCompleteActualContact=false;
    bool OriginalWheelAlignmentChanged=false;
    bool Valid=false;
};
inline bool ValidAlignmentParameters(
    WheelAlignmentParameters p) noexcept
{
    return Finite(p.FrontStaticCamberRadians) &&
        Finite(p.RearStaticCamberRadians) &&
        Finite(p.FrontStaticToeRadians) &&
        Finite(p.RearStaticToeRadians) &&
        Finite(p.CamberGainRadiansPerCm) &&
        Finite(p.ToeGainRadiansPerCm) &&
        Finite(p.RollCamberGain) &&
        Finite(p.PitchCamberGain) &&
        Finite(p.SteeringCamberGain) &&
        Finite(p.MaximumAllowedCamberRadians) &&
        p.MaximumAllowedCamberRadians>0.0 &&
        Finite(p.MaximumAllowedToeRadians) &&
        p.MaximumAllowedToeRadians>0.0 &&
        Finite(p.MaximumBumpSteerRadians) &&
        p.MaximumBumpSteerRadians>0.0 &&
        Finite(p.NominalCasterTrailCm) &&
        p.NominalCasterTrailCm>0.0 &&
        Finite(p.ToeTireWearGain) &&
        p.ToeTireWearGain>=0.0 &&
        Finite(p.CamberTireWearGain) &&
        p.CamberTireWearGain>=0.0 &&
        Finite(p.CamberLateralForceGain) &&
        p.CamberLateralForceGain>=0.0 &&
        Finite(p.MaximumCompressionTravelCm) &&
        p.MaximumCompressionTravelCm>0.0 &&
        Finite(p.MaximumVehicleRollRadians) &&
        p.MaximumVehicleRollRadians>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaxSimulationFrameSeconds;
}
inline bool ValidWheelAlignmentObservation(
    const WheelAlignmentObservation& input,
    WheelAlignmentParameters params) noexcept
{
    return ValidateVehicleFrame(input.Chassis,true)==
        SnapshotFault::None &&
        input.Suspension.Valid &&
        input.Steering.Valid &&
        input.OriginalWheelAlignmentReadoutReady &&
        input.ActualRideGeometryVerified &&
        ValidFrameDelta(input.DeltaSeconds) &&
        input.DeltaSeconds<=params.MaximumFrameSeconds &&
        std::abs(input.Chassis.Chassis.RollRadians)<=
            params.MaximumVehicleRollRadians &&
        std::abs(input.Chassis.Chassis.PitchRadians)<=
            params.MaximumVehiclePitchRadians;
}
inline AlignmentWheelOutput EvaluateAlignmentWheel(
    WheelObservation wheel,SuspensionWheelState spring,
    double measuredRollRadians,double measuredPitchRadians,
    double steeringRadians,int wheelSlot,
    WheelAlignmentParameters p) noexcept
{
    AlignmentWheelOutput out;
    if (!ValidWheelObservation(wheel) ||
        !spring.Qualified ||
        !wheel.ContactVerified ||
        !wheel.PhysicalContact ||
        !Finite(measuredRollRadians) ||
        !Finite(measuredPitchRadians) ||
        !Finite(steeringRadians) ||
        wheelSlot<0 ||wheelSlot>=4)return out;
    const bool front=wheelSlot<2;
    const bool left=wheelSlot%2==0;
    const double compression=spring.CompressionCm;
    const double baseCamber=front ?
        p.FrontStaticCamberRadians :
        p.RearStaticCamberRadians;
    const double baseToe=front ?
        p.FrontStaticToeRadians :
        p.RearStaticToeRadians;
    const double side=left?-1.0:1.0;
    const double travel=std::clamp(
        compression-p.MaximumCompressionTravelCm*0.5,
        -p.MaximumCompressionTravelCm,
        p.MaximumCompressionTravelCm);
    const double camber=baseCamber+
        p.CamberGainRadiansPerCm*travel+
        p.RollCamberGain*measuredRollRadians*side+
        p.PitchCamberGain*measuredPitchRadians+
        p.SteeringCamberGain*steeringRadians;
    const double toe=baseToe*side+
        p.ToeGainRadiansPerCm*travel*side;
    out.CamberRadians=std::clamp(
        camber,-p.MaximumAllowedCamberRadians,
        p.MaximumAllowedCamberRadians);
    out.ToeRadians=std::clamp(
        toe,-p.MaximumAllowedToeRadians,
        p.MaximumAllowedToeRadians);
    out.BumpSteerRadians=std::clamp(
        p.ToeGainRadiansPerCm*travel,
        -p.MaximumBumpSteerRadians,
        p.MaximumBumpSteerRadians);
    out.CasterTrailCm=p.NominalCasterTrailCm+
        p.MechanicalTrailLeverCm*
        std::sin(std::abs(steeringRadians));
    out.NormalLoadN=wheel.NormalLoadN;
    out.CamberThrustN=
        out.CamberRadians*
        wheel.NormalLoadN*
        p.CamberLateralForceGain;
    out.AligningMomentNm=
        out.CamberThrustN*out.CasterTrailCm/
        CentimetersPerMeter;
    out.EffectiveLateralGripScale=std::clamp(
        1.0-p.CamberLateralForceGain*
            std::abs(out.CamberRadians),0.50,1.0);
    out.EffectiveLongitudinalGripScale=std::clamp(
        1.0-p.CamberLongitudinalForceLoss*
            std::abs(out.CamberRadians),0.5,1.0);
    out.WearRateMultiplier=
        1.0+p.ToeTireWearGain*
            std::abs(out.ToeRadians)+
        p.CamberTireWearGain*
            std::abs(out.CamberRadians);
    out.SuspensionCompressionCm=compression;
    out.SteerAngleRadians=steeringRadians;
    out.RealWheelContact=true;
    out.Qualified=Finite(out.CamberThrustN) &&
        Finite(out.AligningMomentNm);
    return out;
}
inline WheelAlignmentReview EvaluateWheelAlignment(
    const WheelAlignmentObservation& source,
    WheelAlignmentParameters p={}) noexcept
{
    WheelAlignmentReview result;
    if (!ValidAlignmentParameters(p) ||
        !ValidWheelAlignmentObservation(source,p))
        return result;
    const std::array<double,4> angles{{
        source.Steering.LeftFrontRadians,
        source.Steering.RightFrontRadians,
        source.Steering.LeftRearRadians,
        source.Steering.RightRearRadians
    }};
    for (std::size_t i=0;i<4;++i)
    {
        const auto output=EvaluateAlignmentWheel(
            source.Chassis.Wheels[i],
            source.Suspension.Wheels[i],
            source.Chassis.Chassis.RollRadians,
            source.Chassis.Chassis.PitchRadians,
            angles[i],static_cast<int>(i),p);
        if (!output.Qualified)return {};
        result.Wheels[i]=output;
        ++result.ContactQualifiedWheels;
        result.TotalCamberThrustN+=
            output.CamberThrustN;
        result.TotalAligningTorqueNm+=
            output.AligningMomentNm;
        result.MeanEffectiveLateralGripScale+=
            output.EffectiveLateralGripScale;
        result.MeanEffectiveLongitudinalGripScale+=
            output.EffectiveLongitudinalGripScale;
        result.MeanWearRateMultiplier+=
            output.WearRateMultiplier;
        result.MaximumBumpSteerRadians=std::max(
            result.MaximumBumpSteerRadians,
            std::abs(output.BumpSteerRadians));
        result.ExcessiveCamber=result.ExcessiveCamber ||
            std::abs(output.CamberRadians)>
                p.MaximumAllowedCamberRadians*0.9;
        result.ExcessiveToe=result.ExcessiveToe ||
            std::abs(output.ToeRadians)>
                p.MaximumAllowedToeRadians*0.9;
    }
    result.FrontToeTotalRadians=
        result.Wheels[0].ToeRadians+
        result.Wheels[1].ToeRadians;
    result.RearToeTotalRadians=
        result.Wheels[2].ToeRadians+
        result.Wheels[3].ToeRadians;
    result.FrontCamberAverageRadians=
        (result.Wheels[0].CamberRadians+
         result.Wheels[1].CamberRadians)*0.5;
    result.RearCamberAverageRadians=
        (result.Wheels[2].CamberRadians+
         result.Wheels[3].CamberRadians)*0.5;
    result.LeftRightCamberDifferenceRadians=
        (result.Wheels[0].CamberRadians+
         result.Wheels[2].CamberRadians-
         result.Wheels[1].CamberRadians-
         result.Wheels[3].CamberRadians)*0.5;
    result.MeanEffectiveLateralGripScale*=0.25;
    result.MeanEffectiveLongitudinalGripScale*=0.25;
    result.MeanWearRateMultiplier*=0.25;
    result.PredictedStraightLinePullN=
        result.TotalCamberThrustN;
    result.HasCompleteActualContact=
        result.ContactQualifiedWheels==4;
    result.OriginalWheelAlignmentChanged=false;
    result.Valid=Finite(result.TotalCamberThrustN) &&
        Finite(result.MeanWearRateMultiplier);
    return result;
}
class FourWheelAlignmentMonitor final
{
public:
    bool Configure(WheelAlignmentParameters p) noexcept
    {
        if (!ValidAlignmentParameters(p))return false;
        Params=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        Qualified=0;
        Rejected=0;
        PeakCamber=0.0;
        PeakToe=0.0;
        TotalCamberThrustNSeconds=0.0;
    }
    WheelAlignmentReview Observe(
        WheelAlignmentObservation sample) noexcept
    {
        auto review=EvaluateWheelAlignment(sample,Params);
        if (!review.Valid)
        {
            ++Rejected;
            return {};
        }
        for (const auto& wheel:review.Wheels)
        {
            PeakCamber=std::max(
                PeakCamber,std::abs(wheel.CamberRadians));
            PeakToe=std::max(
                PeakToe,std::abs(wheel.ToeRadians));
        }
        TotalCamberThrustNSeconds+=
            std::abs(review.TotalCamberThrustN)*
            sample.DeltaSeconds;
        ++Qualified;
        Last=review;
        return Last;
    }
    WheelAlignmentReview Snapshot() const noexcept
    {return Last;}
    std::uint64_t Frames() const noexcept{return Qualified;}
    double MaximumCamberRadians() const noexcept{return PeakCamber;}
private:
    WheelAlignmentParameters Params{};
    WheelAlignmentReview Last{};
    std::uint64_t Qualified=0;
    std::uint64_t Rejected=0;
    double PeakCamber=0.0;
    double PeakToe=0.0;
    double TotalCamberThrustNSeconds=0.0;
};
} // namespace latebrake::racecraft
