#pragma once
#include "Racecraft/HybridEnergyRecovery.h"
#include "Racecraft/BrakeHydraulics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
struct BrakeBlendParameters final
{
    double FrontMechanicalBias=0.62;
    double MaximumRequestedTotalTorqueNm=20000.0;
    double MaximumRegenShareFraction=0.55;
    double MaximumRegenAxleFraction=0.70;
    double MinimumRegenSpeedCmS=400.0;
    double MaximumBlendSpeedCmS=10000.0;
    double FrontAxleTorqueCapacityNm=9000.0;
    double RearAxleTorqueCapacityNm=6000.0;
    double MaximumWheelTorqueNm=5500.0;
    double MaximumRegenRateNmPerSecond=8000.0;
    double MaximumHydraulicRiseNmPerSecond=25000.0;
    double MaximumHydraulicFallNmPerSecond=40000.0;
    double MaximumBrakePedal=1.0;
    double MaximumFrameSeconds=0.1;
    double MinimumVerifiedWheelGripG=0.2;
    double MaximumVerifiedWheelGripG=2.5;
    double ContactLossRegenFraction=0.0;
    double AbsInterventionRegenFraction=0.0;
    double MaximumBlendErrorNm=250.0;
    double MinimumBatteryStateOfCharge=0.05;
    double MaximumBatteryStateOfCharge=0.99;
    bool RequireRealHybridMotor=true;
    bool RequireActualHydraulicWheelSensors=true;
    bool AbortRegenUnderAbs=true;
    bool BrakeOnlyNeverApplyTorque=true;
};
struct BrakeBlendInput final
{
    PhysicalVehicleFrame Vehicle{};
    BrakeHydraulicOutput Hydraulics{};
    HybridEnergyOutput Hybrid{};
    double DriverBrakeDemand=0.0;
    double MeasuredSpeedCmS=0.0;
    double MeasuredBatterySoc=0.5;
    double FourWheelGripG=1.0;
    double DeltaSeconds=0.0;
    bool PhysicalBrakePedalQualified=false;
    bool RealHybridMotorPresent=false;
    bool NativeWheelContactsReady=false;
};
struct BrakeBlendOutput final
{
    std::array<double,4> RequestedFrictionBrakeNm{};
    std::array<double,4> RequestedRegenBrakeNm{};
    double TotalRequestedBrakeNm=0.0;
    double TotalRequestedRegenNm=0.0;
    double TotalRequestedHydraulicNm=0.0;
    double RequestedRegenShareFraction=0.0;
    double FrontAxleBrakeNm=0.0;
    double RearAxleBrakeNm=0.0;
    double EffectiveFrontBias=0.0;
    double EffectiveRearBias=0.0;
    double MissingRequestedBrakeTorqueNm=0.0;
    double ExcessAvailableBrakeTorqueNm=0.0;
    double PredictedRegenEnergyJ=0.0;
    double FrictionHeatPowerWatts=0.0;
    double VehicleDecelerationEstimateCmS2=0.0;
    int AbsProtectedWheels=0;
    int PhysicsQualifiedWheels=0;
    bool RegenBlockedByAbs=false;
    bool RegenBlockedByBattery=false;
    bool BrakeRequestConserved=false;
    bool ActualHydraulicsAdjusted=false;
    bool OriginalVehicleForcesChanged=false;
    bool Valid=false;
};
inline bool ValidBrakeBlendParameters(
    BrakeBlendParameters p) noexcept
{
    return Finite(p.FrontMechanicalBias)&&
        p.FrontMechanicalBias>0.0 &&
        p.FrontMechanicalBias<1.0 &&
        Finite(p.MaximumRequestedTotalTorqueNm)&&
        p.MaximumRequestedTotalTorqueNm>0.0 &&
        Finite(p.MaximumRegenShareFraction)&&
        p.MaximumRegenShareFraction>=0.0 &&
        p.MaximumRegenShareFraction<=1.0 &&
        Finite(p.MaximumRegenAxleFraction)&&
        p.MaximumRegenAxleFraction>=0.0 &&
        p.MaximumRegenAxleFraction<=1.0 &&
        Finite(p.MinimumRegenSpeedCmS)&&
        p.MinimumRegenSpeedCmS>=0.0 &&
        Finite(p.MaximumBlendSpeedCmS)&&
        p.MaximumBlendSpeedCmS>
            p.MinimumRegenSpeedCmS &&
        Finite(p.FrontAxleTorqueCapacityNm)&&
        p.FrontAxleTorqueCapacityNm>0.0 &&
        Finite(p.RearAxleTorqueCapacityNm)&&
        p.RearAxleTorqueCapacityNm>0.0 &&
        Finite(p.MaximumWheelTorqueNm)&&
        p.MaximumWheelTorqueNm>0.0 &&
        Finite(p.MaximumRegenRateNmPerSecond)&&
        p.MaximumRegenRateNmPerSecond>0.0 &&
        Finite(p.MaximumFrameSeconds)&&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaxSimulationFrameSeconds &&
        Finite(p.MinimumVerifiedWheelGripG)&&
        p.MinimumVerifiedWheelGripG>0.0 &&
        Finite(p.MaximumVerifiedWheelGripG)&&
        p.MaximumVerifiedWheelGripG>=
            p.MinimumVerifiedWheelGripG &&
        Finite(p.MaximumBlendErrorNm)&&
        p.MaximumBlendErrorNm>=0.0;
}
inline bool ValidBrakeBlendInput(
    const BrakeBlendInput& s,
    BrakeBlendParameters p) noexcept
{
    return ValidateVehicleFrame(s.Vehicle,true)==
        SnapshotFault::None &&
        s.Hydraulics.Valid &&
        s.Hybrid.Valid &&
        s.PhysicalBrakePedalQualified &&
        s.RealHybridMotorPresent &&
        s.NativeWheelContactsReady &&
        ValidFrameDelta(s.DeltaSeconds) &&
        s.DeltaSeconds<=p.MaximumFrameSeconds &&
        Finite(s.DriverBrakeDemand)&&
        Finite(s.MeasuredSpeedCmS)&&
        Finite(s.MeasuredBatterySoc)&&
        Finite(s.FourWheelGripG)&&
        s.DriverBrakeDemand>=0.0 &&
        s.DriverBrakeDemand<=1.0 &&
        s.MeasuredSpeedCmS>=0.0 &&
        s.MeasuredSpeedCmS<=MaximumPhysicalSpeedCmS &&
        s.MeasuredBatterySoc>=0.0 &&
        s.MeasuredBatterySoc<=1.0 &&
        s.FourWheelGripG>=p.MinimumVerifiedWheelGripG &&
        s.FourWheelGripG<=p.MaximumVerifiedWheelGripG;
}
inline BrakeBlendOutput EvaluateBrakeTorqueBlend(
    BrakeBlendInput source,
    BrakeBlendParameters p={}) noexcept
{
    BrakeBlendOutput out;
    if (!ValidBrakeBlendParameters(p) ||
        !ValidBrakeBlendInput(source,p))
        return out;
    const double demand=source.DriverBrakeDemand*
        p.MaximumRequestedTotalTorqueNm;
    double availableRegen=source.Hybrid.RequestedRegenTorqueNm;
    out.RegenBlockedByAbs=
        source.Hydraulics.AbsActiveWheels>0 ||
        source.Hydraulics.LockedWheels>0;
    out.RegenBlockedByBattery=
        source.MeasuredBatterySoc<=
            p.MinimumBatteryStateOfCharge ||
        source.MeasuredBatterySoc>=
            p.MaximumBatteryStateOfCharge;
    if (source.MeasuredSpeedCmS<
            p.MinimumRegenSpeedCmS ||
        source.MeasuredSpeedCmS>
            p.MaximumBlendSpeedCmS ||
        (out.RegenBlockedByAbs &&
         p.AbortRegenUnderAbs) ||
        out.RegenBlockedByBattery)
        availableRegen=0.0;
    const double regen=std::clamp(
        availableRegen,0.0,
        std::min({
            demand*p.MaximumRegenShareFraction,
            p.MaximumRequestedTotalTorqueNm*
                p.MaximumRegenAxleFraction,
            source.Hybrid.RequestedRegenTorqueNm
        }));
    const double friction=std::max(
        0.0,demand-regen);
    const double front=friction*p.FrontMechanicalBias;
    const double rear=friction*(1.0-p.FrontMechanicalBias);
    const double perFront=std::min(
        front*0.5,p.MaximumWheelTorqueNm);
    const double perRear=std::min(
        rear*0.5,p.MaximumWheelTorqueNm);
    out.RequestedFrictionBrakeNm={{
        perFront,perFront,perRear,perRear
    }};
    out.RequestedRegenBrakeNm={{
        0.0,0.0,regen*0.5,regen*0.5
    }};
    for (std::size_t i=0;i<4;++i)
    {
        if (!source.Vehicle.Wheels[i].PhysicalContact ||
            !source.Vehicle.Wheels[i].ContactVerified)
        {
            out.RequestedRegenBrakeNm[i]=0.0;
            out.RequestedFrictionBrakeNm[i]=0.0;
        }
        else ++out.PhysicsQualifiedWheels;
        out.TotalRequestedBrakeNm+=
            out.RequestedFrictionBrakeNm[i]+
            out.RequestedRegenBrakeNm[i];
        out.TotalRequestedRegenNm+=
            out.RequestedRegenBrakeNm[i];
        out.TotalRequestedHydraulicNm+=
            out.RequestedFrictionBrakeNm[i];
        out.AbsProtectedWheels+=
            source.Hydraulics.Wheels[i].AbsCycling?1:0;
    }
    out.FrontAxleBrakeNm=
        out.RequestedFrictionBrakeNm[0]+
        out.RequestedFrictionBrakeNm[1];
    out.RearAxleBrakeNm=
        out.RequestedFrictionBrakeNm[2]+
        out.RequestedFrictionBrakeNm[3]+
        out.TotalRequestedRegenNm;
    out.EffectiveFrontBias=SafeDivision(
        out.FrontAxleBrakeNm,out.TotalRequestedBrakeNm);
    out.EffectiveRearBias=SafeDivision(
        out.RearAxleBrakeNm,out.TotalRequestedBrakeNm);
    out.RequestedRegenShareFraction=SafeDivision(
        out.TotalRequestedRegenNm,out.TotalRequestedBrakeNm);
    out.MissingRequestedBrakeTorqueNm=std::max(
        0.0,demand-out.TotalRequestedBrakeNm);
    out.ExcessAvailableBrakeTorqueNm=std::max(
        0.0,out.TotalRequestedBrakeNm-demand);
    out.BrakeRequestConserved=
        out.MissingRequestedBrakeTorqueNm<=
            p.MaximumBlendErrorNm;
    out.PredictedRegenEnergyJ=
        source.Hybrid.MechanicalEnergyRecoveredJ*
        out.RequestedRegenShareFraction;
    out.FrictionHeatPowerWatts=
        out.TotalRequestedHydraulicNm*
        source.MeasuredSpeedCmS/100.0;
    out.VehicleDecelerationEstimateCmS2=SafeDivision(
        out.TotalRequestedBrakeNm*100.0,
        source.Vehicle.Chassis.MassKg*
            0.34)*0.50;
    out.ActualHydraulicsAdjusted=false;
    out.OriginalVehicleForcesChanged=false;
    out.Valid=Finite(out.TotalRequestedBrakeNm) &&
        Finite(out.VehicleDecelerationEstimateCmS2) &&
        Finite(out.FrictionHeatPowerWatts);
    return out;
}
struct BrakeBlendHistory final
{
    double AccumulatedRegenEnergyJ=0.0;
    double AccumulatedFrictionHeatJ=0.0;
    double PeakRegenShare=0.0;
    double MaximumBrakeShortfallNm=0.0;
    double MeanFrontBias=0.0;
    std::uint64_t Frames=0;
    std::uint64_t Rejected=0;
    std::uint64_t AbsBlockedFrames=0;
    std::uint64_t BatteryBlockedFrames=0;
    std::uint64_t InsufficientBrakeFrames=0;
    bool ActualVehicleBrakeTorquesChanged=false;
};
class FourWheelBrakeBlendMonitor final
{
public:
    bool Configure(BrakeBlendParameters p) noexcept
    {
        if (!ValidBrakeBlendParameters(p))return false;
        Params=p;
        Reset();
        return true;
    }
    void Reset() noexcept{Last={};History={};}
    BrakeBlendOutput Observe(BrakeBlendInput source) noexcept
    {
        const auto output=EvaluateBrakeTorqueBlend(
            source,Params);
        if (!output.Valid)
        {
            ++History.Rejected;
            return {};
        }
        Last=output;
        ++History.Frames;
        History.AccumulatedRegenEnergyJ+=
            output.PredictedRegenEnergyJ;
        History.AccumulatedFrictionHeatJ+=
            output.FrictionHeatPowerWatts*
            source.DeltaSeconds;
        History.PeakRegenShare=std::max(
            History.PeakRegenShare,
            output.RequestedRegenShareFraction);
        History.MaximumBrakeShortfallNm=std::max(
            History.MaximumBrakeShortfallNm,
            output.MissingRequestedBrakeTorqueNm);
        History.MeanFrontBias+=SafeDivision(
            output.EffectiveFrontBias-
                History.MeanFrontBias,
            static_cast<double>(History.Frames));
        History.AbsBlockedFrames+=
            output.RegenBlockedByAbs?1U:0U;
        History.BatteryBlockedFrames+=
            output.RegenBlockedByBattery?1U:0U;
        History.InsufficientBrakeFrames+=
            output.BrakeRequestConserved?0U:1U;
        History.ActualVehicleBrakeTorquesChanged=false;
        return Last;
    }
    BrakeBlendOutput Snapshot() const noexcept{return Last;}
    BrakeBlendHistory Statistics() const noexcept{return History;}
private:
    BrakeBlendParameters Params{};
    BrakeBlendOutput Last{};
    BrakeBlendHistory History{};
};
} // namespace latebrake::racecraft
