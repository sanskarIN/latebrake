#pragma once
#include "Racecraft/EngineTorqueModel.h"
#include "Racecraft/VehicleAerodynamics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
struct EngineCoolingParameters final
{
    double NominalCoolantCapacityLiters=8.0;
    double NominalCoolantHeatCapacityJPerC=33000.0;
    double EngineBlockHeatCapacityJPerC=160000.0;
    double OilHeatCapacityJPerC=48000.0;
    double RadiatorConductanceWPerC=450.0;
    double EngineCoolantConductanceWPerC=1800.0;
    double EngineOilConductanceWPerC=290.0;
    double OilCoolerConductanceWPerC=90.0;
    double AmbientConvectionWPerC=45.0;
    double RadiatorFanAirflowMultiplier=0.80;
    double MaximumFanFraction=1.0;
    double ThermostatOpenTemperatureC=82.0;
    double ThermostatFullyOpenTemperatureC=96.0;
    double FanOnTemperatureC=98.0;
    double FanOffTemperatureC=89.0;
    double CoolantBoilingTemperatureC=118.0;
    double OilOverheatTemperatureC=148.0;
    double EngineOverheatTemperatureC=145.0;
    double MaximumQualifiedTemperatureC=250.0;
    double MinimumQualifiedTemperatureC=-60.0;
    double NormalizedEngineWasteHeatFraction=0.64;
    double MaximumWasteHeatInputWatts=750000.0;
    double CoolantPumpMinimumRpm=400.0;
    double CoolantPumpMaximumRpm=9000.0;
    double CoolantPumpFlowAtMaxLitersMinute=180.0;
    double PhysicalRadiatorDamageDerateFraction=0.85;
    double PhysicalPumpDamageDerateFraction=0.92;
    double MaximumAllowedCoolantLossLiters=8.0;
    double MaximumFrameSeconds=0.10;
    bool RequirePhysicalCoolantSensor=true;
    bool RequirePhysicalEngineTemperature=true;
    bool RequirePhysicalPumpSensor=true;
    bool NeverChangeRealEngineHeat=true;
};
struct EngineCoolingObservation final
{
    RaceIdentity Identity{};
    double RealEngineBlockC=100.0;
    double RealCoolantC=90.0;
    double RealOilC=98.0;
    double RealAmbientC=25.0;
    double RealEngineRpm=4500.0;
    double RealEngineOutputPowerKw=120.0;
    double RealCoolantVolumeLiters=8.0;
    double RealWaterPumpRpm=4500.0;
    double RealFanFraction=0.0;
    double RealCarSpeedCmS=1500.0;
    double RealRadiatorDamageFraction=0.0;
    double RealWaterPumpDamageFraction=0.0;
    double RealCoolingLeakLitersSecond=0.0;
    double DeltaSeconds=0.0;
    bool PhysicalCoolantTemperatureMeasured=false;
    bool PhysicalOilTemperatureMeasured=false;
    bool PhysicalWaterPumpMeasured=false;
    bool OriginalEngineRunning=false;
    bool OriginalCarPhysicsQualified=false;
};
struct EngineCoolingPrediction final
{
    double PredictedEngineBlockC=0.0;
    double PredictedCoolantC=0.0;
    double PredictedOilC=0.0;
    double PredictedCoolantVolumeLiters=0.0;
    double ThermostatOpenFraction=0.0;
    double RadiatorFanDemandFraction=0.0;
    double RadiatorAirCoolingWatts=0.0;
    double EngineToCoolantWatts=0.0;
    double EngineToOilWatts=0.0;
    double OilToAmbientWatts=0.0;
    double EngineWasteHeatWatts=0.0;
    double PumpFlowLitersMinute=0.0;
    double RadiatorHealthFraction=0.0;
    double PumpHealthFraction=0.0;
    double CoolantThermalReserveC=0.0;
    double ExpectedHeatRiseCPerSecond=0.0;
    double EnginePowerDerateFraction=0.0;
    double HeatRemovedJ=0.0;
    double HeatProducedJ=0.0;
    bool EngineOverheatRisk=false;
    bool CoolantBoilingRisk=false;
    bool OilOverheatRisk=false;
    bool LowCoolantRisk=false;
    bool ActualEngineTemperatureModified=false;
    bool Valid=false;
};
inline bool ValidEngineCoolingParameters(
    EngineCoolingParameters p) noexcept
{
    return Finite(p.NominalCoolantCapacityLiters) &&
        p.NominalCoolantCapacityLiters>0.0 &&
        Finite(p.NominalCoolantHeatCapacityJPerC) &&
        p.NominalCoolantHeatCapacityJPerC>0.0 &&
        Finite(p.EngineBlockHeatCapacityJPerC) &&
        p.EngineBlockHeatCapacityJPerC>0.0 &&
        Finite(p.OilHeatCapacityJPerC) &&
        p.OilHeatCapacityJPerC>0.0 &&
        Finite(p.RadiatorConductanceWPerC) &&
        p.RadiatorConductanceWPerC>0.0 &&
        Finite(p.EngineCoolantConductanceWPerC) &&
        p.EngineCoolantConductanceWPerC>0.0 &&
        Finite(p.EngineOilConductanceWPerC) &&
        p.EngineOilConductanceWPerC>=0.0 &&
        Finite(p.ThermostatOpenTemperatureC) &&
        Finite(p.ThermostatFullyOpenTemperatureC) &&
        p.ThermostatFullyOpenTemperatureC>
            p.ThermostatOpenTemperatureC &&
        Finite(p.CoolantBoilingTemperatureC) &&
        p.CoolantBoilingTemperatureC>
            p.ThermostatFullyOpenTemperatureC &&
        Finite(p.EngineOverheatTemperatureC) &&
        Finite(p.OilOverheatTemperatureC) &&
        Finite(p.MaximumWasteHeatInputWatts) &&
        p.MaximumWasteHeatInputWatts>0.0 &&
        Finite(p.CoolantPumpMaximumRpm) &&
        p.CoolantPumpMaximumRpm>0.0 &&
        Finite(p.CoolantPumpFlowAtMaxLitersMinute) &&
        p.CoolantPumpFlowAtMaxLitersMinute>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaxSimulationFrameSeconds;
}
inline bool ValidEngineCoolingObservation(
    EngineCoolingObservation s,
    EngineCoolingParameters p) noexcept
{
    return PhysicalActorIdentityValid(s.Identity) &&
        s.PhysicalCoolantTemperatureMeasured &&
        s.PhysicalOilTemperatureMeasured &&
        s.PhysicalWaterPumpMeasured &&
        s.OriginalEngineRunning &&
        s.OriginalCarPhysicsQualified &&
        ValidFrameDelta(s.DeltaSeconds) &&
        s.DeltaSeconds<=p.MaximumFrameSeconds &&
        Finite(s.RealEngineBlockC) &&
        Finite(s.RealCoolantC) &&
        Finite(s.RealOilC) &&
        Finite(s.RealAmbientC) &&
        Finite(s.RealEngineRpm) &&
        Finite(s.RealEngineOutputPowerKw) &&
        Finite(s.RealCoolantVolumeLiters) &&
        Finite(s.RealWaterPumpRpm) &&
        Finite(s.RealFanFraction) &&
        Finite(s.RealCarSpeedCmS) &&
        Finite(s.RealRadiatorDamageFraction) &&
        Finite(s.RealWaterPumpDamageFraction) &&
        Finite(s.RealCoolingLeakLitersSecond) &&
        s.RealEngineBlockC>=p.MinimumQualifiedTemperatureC &&
        s.RealEngineBlockC<=p.MaximumQualifiedTemperatureC &&
        s.RealCoolantC>=p.MinimumQualifiedTemperatureC &&
        s.RealCoolantC<=p.MaximumQualifiedTemperatureC &&
        s.RealOilC>=p.MinimumQualifiedTemperatureC &&
        s.RealOilC<=p.MaximumQualifiedTemperatureC &&
        s.RealEngineRpm>=0.0 &&
        s.RealEngineRpm<=30000.0 &&
        s.RealEngineOutputPowerKw>=0.0 &&
        s.RealCoolantVolumeLiters>=0.0 &&
        s.RealCoolantVolumeLiters<=
            p.NominalCoolantCapacityLiters &&
        s.RealWaterPumpRpm>=0.0 &&
        s.RealWaterPumpRpm<=p.CoolantPumpMaximumRpm &&
        s.RealFanFraction>=0.0 &&
        s.RealFanFraction<=1.0 &&
        s.RealCarSpeedCmS>=0.0 &&
        s.RealCarSpeedCmS<=MaximumPhysicalSpeedCmS &&
        s.RealRadiatorDamageFraction>=0.0 &&
        s.RealRadiatorDamageFraction<=1.0 &&
        s.RealWaterPumpDamageFraction>=0.0 &&
        s.RealWaterPumpDamageFraction<=1.0 &&
        s.RealCoolingLeakLitersSecond>=0.0;
}
inline EngineCoolingPrediction EvaluateEngineCooling(
    EngineCoolingObservation s,
    EngineCoolingParameters p={}) noexcept
{
    EngineCoolingPrediction out;
    if (!ValidEngineCoolingParameters(p) ||
        !ValidEngineCoolingObservation(s,p))
        return out;
    const double dt=s.DeltaSeconds;
    const double thermostat=Smoothstep(
        p.ThermostatOpenTemperatureC,
        p.ThermostatFullyOpenTemperatureC,
        s.RealCoolantC);
    const double fanDemand=Smoothstep(
        p.FanOffTemperatureC,
        p.FanOnTemperatureC,
        s.RealCoolantC);
    const double radiatorHealth=1.0-
        p.PhysicalRadiatorDamageDerateFraction*
        s.RealRadiatorDamageFraction;
    const double pumpHealth=1.0-
        p.PhysicalPumpDamageDerateFraction*
        s.RealWaterPumpDamageFraction;
    const double coolantFraction=Clamp01(
        SafeDivision(s.RealCoolantVolumeLiters,
                     p.NominalCoolantCapacityLiters));
    const double pumpFlow=Clamp01(
        SafeDivision(s.RealWaterPumpRpm,
                     p.CoolantPumpMaximumRpm))*
        p.CoolantPumpFlowAtMaxLitersMinute*
        pumpHealth*coolantFraction;
    const double speedMS=
        s.RealCarSpeedCmS/CentimetersPerMeter;
    const double airFactor=1.0+
        speedMS*0.06+
        fanDemand*p.RadiatorFanAirflowMultiplier;
    const double wasteHeat=std::min(
        p.MaximumWasteHeatInputWatts,
        s.RealEngineOutputPowerKw*1000.0*
        p.NormalizedEngineWasteHeatFraction);
    const double transferEngineCoolant=
        (s.RealEngineBlockC-s.RealCoolantC)*
        p.EngineCoolantConductanceWPerC*
        pumpHealth*coolantFraction;
    const double transferEngineOil=
        (s.RealEngineBlockC-s.RealOilC)*
        p.EngineOilConductanceWPerC;
    const double radiatorWatts=std::max(
        0.0,(s.RealCoolantC-s.RealAmbientC)*
        p.RadiatorConductanceWPerC*
        thermostat*airFactor*radiatorHealth);
    const double oilCoolingWatts=std::max(
        0.0,(s.RealOilC-s.RealAmbientC)*
        p.OilCoolerConductanceWPerC*
        airFactor);
    const double engineDelta=SafeDivision(
        (wasteHeat-transferEngineCoolant-
         transferEngineOil)*dt,
        p.EngineBlockHeatCapacityJPerC);
    const double coolantDelta=SafeDivision(
        (transferEngineCoolant-radiatorWatts)*dt,
        p.NominalCoolantHeatCapacityJPerC*
        std::max(0.1,coolantFraction));
    const double oilDelta=SafeDivision(
        (transferEngineOil-oilCoolingWatts)*dt,
        p.OilHeatCapacityJPerC);
    out.PredictedEngineBlockC=
        s.RealEngineBlockC+engineDelta;
    out.PredictedCoolantC=
        s.RealCoolantC+coolantDelta;
    out.PredictedOilC=
        s.RealOilC+oilDelta;
    out.PredictedCoolantVolumeLiters=std::max(
        0.0,s.RealCoolantVolumeLiters-
            s.RealCoolingLeakLitersSecond*dt);
    out.ThermostatOpenFraction=thermostat;
    out.RadiatorFanDemandFraction=fanDemand;
    out.RadiatorAirCoolingWatts=radiatorWatts;
    out.EngineToCoolantWatts=transferEngineCoolant;
    out.EngineToOilWatts=transferEngineOil;
    out.OilToAmbientWatts=oilCoolingWatts;
    out.EngineWasteHeatWatts=wasteHeat;
    out.PumpFlowLitersMinute=pumpFlow;
    out.RadiatorHealthFraction=radiatorHealth;
    out.PumpHealthFraction=pumpHealth;
    out.CoolantThermalReserveC=
        p.CoolantBoilingTemperatureC-
        s.RealCoolantC;
    out.ExpectedHeatRiseCPerSecond=
        SafeDivision(engineDelta,dt);
    out.EnginePowerDerateFraction=1.0-Smoothstep(
        p.ThermostatFullyOpenTemperatureC,
        p.EngineOverheatTemperatureC,
        s.RealEngineBlockC);
    out.HeatRemovedJ=(
        radiatorWatts+oilCoolingWatts)*dt;
    out.HeatProducedJ=wasteHeat*dt;
    out.EngineOverheatRisk=
        out.PredictedEngineBlockC>=
        p.EngineOverheatTemperatureC;
    out.CoolantBoilingRisk=
        out.PredictedCoolantC>=
        p.CoolantBoilingTemperatureC;
    out.OilOverheatRisk=
        out.PredictedOilC>=
        p.OilOverheatTemperatureC;
    out.LowCoolantRisk=
        coolantFraction<0.35;
    out.ActualEngineTemperatureModified=false;
    out.Valid=Finite(out.PredictedEngineBlockC) &&
        Finite(out.PredictedCoolantC) &&
        Finite(out.PredictedOilC);
    return out;
}
struct CoolingHistory final
{
    double PeakEngineBlockC=0.0;
    double PeakCoolantC=0.0;
    double PeakOilC=0.0;
    double LowestCoolantVolumeLiters=0.0;
    double TotalHeatRemovedJ=0.0;
    double TotalWasteHeatJ=0.0;
    double TotalOverheatRiskSeconds=0.0;
    double MeanPowerDerateFraction=0.0;
    std::uint64_t QualifiedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t BoilingRiskFrames=0;
    bool EngineHeatActuallyChanged=false;
};
class CoolingCircuitMonitor final
{
public:
    bool Configure(EngineCoolingParameters p) noexcept
    {
        if (!ValidEngineCoolingParameters(p))return false;
        Params=p;
        Reset();
        return true;
    }
    void Reset() noexcept{Last={};History={};}
    EngineCoolingPrediction Observe(
        EngineCoolingObservation s) noexcept
    {
        const auto result=EvaluateEngineCooling(s,Params);
        if (!result.Valid)
        {
            ++History.RejectedFrames;
            return {};
        }
        Last=result;
        if (History.QualifiedFrames==0)
            History.LowestCoolantVolumeLiters=
                s.RealCoolantVolumeLiters;
        else
            History.LowestCoolantVolumeLiters=std::min(
                History.LowestCoolantVolumeLiters,
                s.RealCoolantVolumeLiters);
        History.PeakEngineBlockC=std::max(
            History.PeakEngineBlockC,
            result.PredictedEngineBlockC);
        History.PeakCoolantC=std::max(
            History.PeakCoolantC,
            result.PredictedCoolantC);
        History.PeakOilC=std::max(
            History.PeakOilC,
            result.PredictedOilC);
        History.TotalHeatRemovedJ+=result.HeatRemovedJ;
        History.TotalWasteHeatJ+=result.HeatProducedJ;
        if (result.EngineOverheatRisk)
            History.TotalOverheatRiskSeconds+=s.DeltaSeconds;
        History.BoilingRiskFrames+=
            result.CoolantBoilingRisk?1U:0U;
        ++History.QualifiedFrames;
        History.MeanPowerDerateFraction+=
            SafeDivision(
                result.EnginePowerDerateFraction-
                History.MeanPowerDerateFraction,
                static_cast<double>(History.QualifiedFrames));
        History.EngineHeatActuallyChanged=false;
        return Last;
    }
    EngineCoolingPrediction Snapshot() const noexcept
    {return Last;}
    CoolingHistory Statistics() const noexcept
    {return History;}
private:
    EngineCoolingParameters Params{};
    EngineCoolingPrediction Last{};
    CoolingHistory History{};
};
} // namespace latebrake::racecraft
