#pragma once
#include "Racecraft/RacecraftFrames.h"
#include "Racecraft/BrakeHydraulics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
enum class HybridControlMode : std::uint8_t
{
    Off, Balanced, Recovery, Performance, Qualifying
};
struct HybridEnergyParameters final
{
    double BatteryNominalCapacityKwh=2.5;
    double MaximumBatteryCapacityKwh=6.0;
    double MinimumBatteryStateOfCharge=0.08;
    double MaximumBatteryStateOfCharge=0.98;
    double MaximumRegenPowerKw=120.0;
    double MaximumDeployPowerKw=140.0;
    double MinimumRegenSpeedCmS=350.0;
    double MaximumRegenSpeedCmS=8500.0;
    double MinimumDeploySpeedCmS=150.0;
    double MaximumDeploySpeedCmS=9000.0;
    double MaximumRegenTorqueNm=900.0;
    double MaximumDeployTorqueNm=1100.0;
    double RegenMechanicalEfficiency=0.82;
    double DeployMechanicalEfficiency=0.88;
    double BatteryChargeEfficiency=0.93;
    double BatteryDischargeEfficiency=0.94;
    double BatteryHeatGainCPerKwh=35.0;
    double BatteryCoolingSeconds=65.0;
    double BatteryOverheatStartC=55.0;
    double BatteryOverheatCriticalC=82.0;
    double BatteryColdDerateStartC=5.0;
    double BatteryOptimumTemperatureC=35.0;
    double MaximumBatteryCurrentAmps=650.0;
    double NominalBatteryVoltage=400.0;
    double MaximumHybridDecelerationCmS2=420.0;
    double MaximumHybridAccelerationCmS2=450.0;
    double MinimumPhysicalWheelGripG=0.2;
    double MaximumBatterySensorJumpFraction=0.03;
    double MaximumFrameSeconds=0.10;
    bool RequirePhysicalBatterySensor=true;
    bool RequirePhysicalMotorObservation=true;
    bool PermitRegenWhenAbsEngaged=false;
    bool RestrictionOnly=true;
};
struct HybridEnergyObservation final
{
    PhysicalVehicleFrame Vehicle{};
    BrakeHydraulicOutput Brakes{};
    double MotorRotationRpm=0.0;
    double MotorAvailableRegenPowerKw=0.0;
    double MotorAvailableDeployPowerKw=0.0;
    double MotorTorqueNm=0.0;
    double MeasuredBatteryEnergyKwh=0.0;
    double MeasuredBatteryTemperatureC=35.0;
    double MeasuredBatteryVoltage=400.0;
    double PhysicalWheelGripG=1.0;
    double DriverRegenDemand=0.0;
    double DriverDeployDemand=0.0;
    double DeltaSeconds=0.0;
    bool PhysicalBatteryReadoutVerified=false;
    bool ActualHybridMotorPresent=false;
    bool DriveAxleContactsVerified=false;
    bool OriginalEnergyEpoch=true;
};
struct HybridEnergyOutput final
{
    double RequestedRegenPowerKw=0.0;
    double RequestedDeployPowerKw=0.0;
    double RequestedRegenTorqueNm=0.0;
    double RequestedDeployTorqueNm=0.0;
    double ExpectedBatteryDeltaKwh=0.0;
    double ExpectedChargeFraction=0.0;
    double ExpectedBatteryTemperatureC=0.0;
    double ChargeEfficiency=0.0;
    double DischargeEfficiency=0.0;
    double MechanicalEnergyRecoveredJ=0.0;
    double MechanicalEnergyDeliveredJ=0.0;
    double ExpectedBatteryHeatJ=0.0;
    double EngineBrakingOffsetNm=0.0;
    double FrictionBrakeResidualNm=0.0;
    double RecoveryFraction=0.0;
    double DeploymentFraction=0.0;
    double BatteryThermalDerate=0.0;
    double BatteryPowerLimitKw=0.0;
    bool RegenActive=false;
    bool DeployActive=false;
    bool RejectedForAbs=false;
    bool ContactGripLimited=false;
    bool ThermalLimited=false;
    bool PowerDeliveryToRealMotor=false;
    bool BatteryChargeChangedInEngine=false;
    bool Valid=false;
};
inline bool ValidHybridParameters(HybridEnergyParameters s) noexcept
{
    return Finite(s.BatteryNominalCapacityKwh) &&
        s.BatteryNominalCapacityKwh>0.0 &&
        Finite(s.MaximumBatteryCapacityKwh) &&
        s.MaximumBatteryCapacityKwh>=s.BatteryNominalCapacityKwh &&
        Finite(s.MinimumBatteryStateOfCharge) &&
        s.MinimumBatteryStateOfCharge>=0.0 &&
        Finite(s.MaximumBatteryStateOfCharge) &&
        s.MaximumBatteryStateOfCharge>
            s.MinimumBatteryStateOfCharge &&
        s.MaximumBatteryStateOfCharge<=1.0 &&
        Finite(s.MaximumRegenPowerKw) &&
        s.MaximumRegenPowerKw>=0.0 &&
        Finite(s.MaximumDeployPowerKw) &&
        s.MaximumDeployPowerKw>=0.0 &&
        Finite(s.MinimumRegenSpeedCmS) &&
        s.MinimumRegenSpeedCmS>=0.0 &&
        Finite(s.MaximumRegenSpeedCmS) &&
        s.MaximumRegenSpeedCmS>s.MinimumRegenSpeedCmS &&
        Finite(s.MaximumRegenTorqueNm) &&
        s.MaximumRegenTorqueNm>=0.0 &&
        Finite(s.MaximumDeployTorqueNm) &&
        s.MaximumDeployTorqueNm>=0.0 &&
        Finite(s.RegenMechanicalEfficiency) &&
        s.RegenMechanicalEfficiency>0.0 &&
        s.RegenMechanicalEfficiency<=1.0 &&
        Finite(s.DeployMechanicalEfficiency) &&
        s.DeployMechanicalEfficiency>0.0 &&
        s.DeployMechanicalEfficiency<=1.0 &&
        Finite(s.BatteryChargeEfficiency) &&
        s.BatteryChargeEfficiency>0.0 &&
        s.BatteryChargeEfficiency<=1.0 &&
        Finite(s.BatteryDischargeEfficiency) &&
        s.BatteryDischargeEfficiency>0.0 &&
        s.BatteryDischargeEfficiency<=1.0 &&
        Finite(s.MaximumBatterySensorJumpFraction) &&
        s.MaximumBatterySensorJumpFraction>0.0 &&
        Finite(s.MinimumPhysicalWheelGripG) &&
        s.MinimumPhysicalWheelGripG>0.0 &&
        Finite(s.MaximumFrameSeconds) &&
        s.MaximumFrameSeconds>0.0 &&
        s.MaximumFrameSeconds<=MaxSimulationFrameSeconds;
}
inline bool ValidHybridObservation(
    HybridEnergyObservation s,
    HybridEnergyParameters p) noexcept
{
    return ValidateVehicleFrame(s.Vehicle,true)==
        SnapshotFault::None &&
        s.PhysicalBatteryReadoutVerified &&
        s.ActualHybridMotorPresent &&
        s.DriveAxleContactsVerified &&
        s.OriginalEnergyEpoch &&
        ValidFrameDelta(s.DeltaSeconds) &&
        s.DeltaSeconds<=p.MaximumFrameSeconds &&
        Finite(s.MotorRotationRpm) &&
        Finite(s.MotorAvailableRegenPowerKw) &&
        Finite(s.MotorAvailableDeployPowerKw) &&
        Finite(s.MotorTorqueNm) &&
        Finite(s.MeasuredBatteryEnergyKwh) &&
        Finite(s.MeasuredBatteryTemperatureC) &&
        Finite(s.MeasuredBatteryVoltage) &&
        Finite(s.PhysicalWheelGripG) &&
        Finite(s.DriverRegenDemand) &&
        Finite(s.DriverDeployDemand) &&
        s.MotorRotationRpm>=0.0 &&
        s.MotorAvailableRegenPowerKw>=0.0 &&
        s.MotorAvailableDeployPowerKw>=0.0 &&
        s.MeasuredBatteryEnergyKwh>=0.0 &&
        s.MeasuredBatteryEnergyKwh<=
            p.MaximumBatteryCapacityKwh &&
        s.MeasuredBatteryVoltage>0.0 &&
        s.PhysicalWheelGripG>=p.MinimumPhysicalWheelGripG &&
        s.DriverRegenDemand>=0.0 &&
        s.DriverRegenDemand<=1.0 &&
        s.DriverDeployDemand>=0.0 &&
        s.DriverDeployDemand<=1.0;
}
inline double HybridBatteryThermalFraction(
    double c,HybridEnergyParameters p) noexcept
{
    if (!Finite(c))return 0.0;
    const double hot=Smoothstep(
        p.BatteryOverheatStartC,
        p.BatteryOverheatCriticalC,c);
    const double cold=1.0-Smoothstep(
        p.BatteryColdDerateStartC,
        p.BatteryOptimumTemperatureC,c);
    return std::clamp(
        1.0-0.85*hot-0.55*cold,0.0,1.0);
}
inline HybridEnergyOutput EvaluateHybridEnergy(
    HybridEnergyObservation input,
    HybridEnergyParameters p={},
    HybridControlMode mode=HybridControlMode::Balanced) noexcept
{
    HybridEnergyOutput out;
    if (!ValidHybridParameters(p) ||
        !ValidHybridObservation(input,p))
        return out;
    const double speed=Length(
        input.Vehicle.Chassis.VelocityCmS);
    const double stateOfCharge=SafeDivision(
        input.MeasuredBatteryEnergyKwh,
        p.BatteryNominalCapacityKwh);
    const double thermal=HybridBatteryThermalFraction(
        input.MeasuredBatteryTemperatureC,p);
    const bool absIntervention=
        input.Brakes.Valid &&
        input.Brakes.AbsActiveWheels>0;
    const bool regenAllowed=
        mode!=HybridControlMode::Off &&
        !input.DriverDeployDemand &&
        (p.PermitRegenWhenAbsEngaged || !absIntervention) &&
        speed>=p.MinimumRegenSpeedCmS &&
        speed<=p.MaximumRegenSpeedCmS &&
        stateOfCharge<p.MaximumBatteryStateOfCharge;
    const bool deployAllowed=
        mode!=HybridControlMode::Off &&
        !input.DriverRegenDemand &&
        speed>=p.MinimumDeploySpeedCmS &&
        speed<=p.MaximumDeploySpeedCmS &&
        stateOfCharge>p.MinimumBatteryStateOfCharge;
    const double modeRegen=
        mode==HybridControlMode::Recovery ? 1.0 :
        mode==HybridControlMode::Qualifying ? 0.5 : 0.85;
    const double modeDeploy=
        mode==HybridControlMode::Performance ? 1.0 :
        mode==HybridControlMode::Recovery ? 0.25 :
        mode==HybridControlMode::Qualifying ? 1.0 : 0.60;
    out.RequestedRegenPowerKw=regenAllowed ?
        std::min(p.MaximumRegenPowerKw,
                 input.MotorAvailableRegenPowerKw)*
        input.DriverRegenDemand*modeRegen*thermal :0.0;
    out.RequestedDeployPowerKw=deployAllowed ?
        std::min(p.MaximumDeployPowerKw,
                 input.MotorAvailableDeployPowerKw)*
        input.DriverDeployDemand*modeDeploy*thermal :0.0;
    const double motorOmega=std::max(
        1.0,input.MotorRotationRpm*TwoPi/60.0);
    out.RequestedRegenTorqueNm=std::min(
        p.MaximumRegenTorqueNm,
        out.RequestedRegenPowerKw*1000.0/motorOmega);
    out.RequestedDeployTorqueNm=std::min(
        p.MaximumDeployTorqueNm,
        out.RequestedDeployPowerKw*1000.0/motorOmega);
    const double seconds=input.DeltaSeconds;
    const double chargeEnergyKwh=
        out.RequestedRegenPowerKw*
        p.RegenMechanicalEfficiency*
        p.BatteryChargeEfficiency*seconds/3600.0;
    const double dischargeEnergyKwh=
        SafeDivision(out.RequestedDeployPowerKw,
                     p.DeployMechanicalEfficiency*
                     p.BatteryDischargeEfficiency)*
        seconds/3600.0;
    out.ExpectedBatteryDeltaKwh=
        chargeEnergyKwh-dischargeEnergyKwh;
    out.ExpectedChargeFraction=Clamp01(
        SafeDivision(input.MeasuredBatteryEnergyKwh+
                     out.ExpectedBatteryDeltaKwh,
                     p.BatteryNominalCapacityKwh));
    out.MechanicalEnergyRecoveredJ=
        out.RequestedRegenPowerKw*1000.0*seconds;
    out.MechanicalEnergyDeliveredJ=
        out.RequestedDeployPowerKw*1000.0*seconds;
    out.ExpectedBatteryHeatJ=
        (out.MechanicalEnergyRecoveredJ*
            (1.0-p.BatteryChargeEfficiency)+
         out.MechanicalEnergyDeliveredJ*
            (1.0-p.BatteryDischargeEfficiency));
    out.ExpectedBatteryTemperatureC=
        input.MeasuredBatteryTemperatureC+
        out.ExpectedBatteryHeatJ/
            (p.BatteryNominalCapacityKwh*3.6e6)*
            p.BatteryHeatGainCPerKwh;
    out.ChargeEfficiency=p.BatteryChargeEfficiency;
    out.DischargeEfficiency=p.BatteryDischargeEfficiency;
    out.BatteryPowerLimitKw=std::min(
        p.MaximumRegenPowerKw,p.MaximumDeployPowerKw)*thermal;
    out.EngineBrakingOffsetNm=out.RequestedRegenTorqueNm;
    out.FrictionBrakeResidualNm=input.Brakes.Valid ?
        std::max(0.0,input.Brakes.TotalCommandedTorqueNm-
                    out.RequestedRegenTorqueNm):0.0;
    out.RecoveryFraction=SafeDivision(
        out.RequestedRegenPowerKw,p.MaximumRegenPowerKw);
    out.DeploymentFraction=SafeDivision(
        out.RequestedDeployPowerKw,p.MaximumDeployPowerKw);
    out.BatteryThermalDerate=1.0-thermal;
    out.RegenActive=out.RequestedRegenPowerKw>0.0;
    out.DeployActive=out.RequestedDeployPowerKw>0.0;
    out.RejectedForAbs=absIntervention &&
        input.DriverRegenDemand>0.0 &&
        !p.PermitRegenWhenAbsEngaged;
    out.ThermalLimited=thermal<0.99;
    out.PowerDeliveryToRealMotor=false;
    out.BatteryChargeChangedInEngine=false;
    out.Valid=Finite(out.ExpectedBatteryDeltaKwh) &&
        Finite(out.ExpectedBatteryTemperatureC) &&
        Finite(out.FrictionBrakeResidualNm);
    return out;
}
struct HybridEnergyHistory final
{
    double TotalRecoveredKwh=0.0;
    double TotalDeployedKwh=0.0;
    double MaximumRecoveredPowerKw=0.0;
    double MaximumDeployPowerKw=0.0;
    double TotalRegenSeconds=0.0;
    double TotalDeploySeconds=0.0;
    double MaximumBatteryTemperatureC=0.0;
    double LastMeasuredBatteryEnergyKwh=0.0;
    double BatterySensorContinuityErrorKwh=0.0;
    std::uint64_t QualifiedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t AbsRegenVetoes=0;
    std::uint64_t ThermalLimitFrames=0;
    bool AnyPhysicalBatteryUpdated=false;
};
class HybridEnergySupervisor final
{
public:
    bool Configure(HybridEnergyParameters p,
                   HybridControlMode m) noexcept
    {
        if (!ValidHybridParameters(p))return false;
        Settings=p;
        Mode=m;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last={};
        History={};
        PreviousMeasured=0.0;
        HasBattery=false;
    }
    HybridEnergyOutput Observe(HybridEnergyObservation input) noexcept
    {
        const auto review=EvaluateHybridEnergy(
            input,Settings,Mode);
        if (!review.Valid)
        {
            ++History.RejectedFrames;
            HasBattery=false;
            return {};
        }
        if (HasBattery)
        {
            History.BatterySensorContinuityErrorKwh=std::abs(
                input.MeasuredBatteryEnergyKwh-
                PreviousMeasured);
            if (History.BatterySensorContinuityErrorKwh>
                Settings.MaximumBatterySensorJumpFraction*
                Settings.BatteryNominalCapacityKwh)
            {
                ++History.RejectedFrames;
                HasBattery=false;
                return {};
            }
        }
        PreviousMeasured=input.MeasuredBatteryEnergyKwh;
        HasBattery=true;
        History.LastMeasuredBatteryEnergyKwh=
            PreviousMeasured;
        History.TotalRecoveredKwh+=
            review.MechanicalEnergyRecoveredJ/3.6e6;
        History.TotalDeployedKwh+=
            review.MechanicalEnergyDeliveredJ/3.6e6;
        History.MaximumRecoveredPowerKw=std::max(
            History.MaximumRecoveredPowerKw,
            review.RequestedRegenPowerKw);
        History.MaximumDeployPowerKw=std::max(
            History.MaximumDeployPowerKw,
            review.RequestedDeployPowerKw);
        if (review.RegenActive)
            History.TotalRegenSeconds+=input.DeltaSeconds;
        if (review.DeployActive)
            History.TotalDeploySeconds+=input.DeltaSeconds;
        History.MaximumBatteryTemperatureC=std::max(
            History.MaximumBatteryTemperatureC,
            input.MeasuredBatteryTemperatureC);
        History.AbsRegenVetoes+=review.RejectedForAbs?1U:0U;
        History.ThermalLimitFrames+=review.ThermalLimited?1U:0U;
        ++History.QualifiedFrames;
        History.AnyPhysicalBatteryUpdated=false;
        Last=review;
        return Last;
    }
    HybridEnergyOutput Snapshot() const noexcept{return Last;}
    HybridEnergyHistory Statistics() const noexcept{return History;}
private:
    HybridEnergyParameters Settings{};
    HybridControlMode Mode=HybridControlMode::Balanced;
    HybridEnergyOutput Last{};
    HybridEnergyHistory History{};
    double PreviousMeasured=0.0;
    bool HasBattery=false;
};
} // namespace latebrake::racecraft
