#pragma once
#include "Racecraft/EngineTorqueModel.h"
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
struct FuelSupplyParameters final
{
    double TankCapacityLiters=55.0;
    double MinimumReserveLiters=2.0;
    double MaximumPhysicalFlowLitersHour=220.0;
    double MaximumIdleFlowLitersHour=10.0;
    double MaximumRefillRateLitersSecond=4.0;
    double FuelPumpMinimumPressureKpa=200.0;
    double FuelPumpNominalPressureKpa=400.0;
    double FuelPumpMaximumPressureKpa=800.0;
    double PumpPressureResponseSeconds=0.12;
    double InjectorMinimumDutyFraction=0.03;
    double InjectorMaximumDutyFraction=0.96;
    double MinimumEngineRpm=450.0;
    double MaximumEngineRpm=15000.0;
    double FuelSpecificEnergyMJPerKg=43.0;
    double FuelDensityKgPerLiter=0.745;
    double EstimatedThermalEfficiency=0.34;
    double CornerFuelPickupThresholdG=1.4;
    double MinimumSafeFuelFractionForCorner=0.08;
    double LongitudinalPickupCriticalG=1.6;
    double FuelSloshRecoverySeconds=0.8;
    double FuelTemperatureC=25.0;
    double MaximumFuelTemperatureC=80.0;
    double MaximumSensorDeltaLiters=0.7;
    double SensorClockToleranceSeconds=0.02;
    double MaximumFrameSeconds=0.1;
    bool RequireVerifiedEngineMeter=true;
    bool RequirePhysicalFuelTank=true;
    bool RejectUnobservedRefuel=true;
};
struct FuelSupplyObservation final
{
    RaceIdentity Identity{};
    double PhysicalTankFuelLiters=0.0;
    double MeasuredFuelPumpPressureKpa=0.0;
    double MeasuredInjectorDutyFraction=0.0;
    double MeasuredFuelFlowLitersHour=0.0;
    double EngineRpm=0.0;
    double EnginePowerKw=0.0;
    double LateralAccelerationG=0.0;
    double LongitudinalAccelerationG=0.0;
    double FuelTemperatureC=25.0;
    double TrackDistanceCm=0.0;
    double ClockSeconds=0.0;
    double DeltaSeconds=0.0;
    bool ActualFuelTankPresent=false;
    bool MeasuredFuelLevelVerified=false;
    bool PhysicalFuelPumpQualified=false;
    bool PhysicallyRefueling=false;
    bool InOriginalServiceBox=false;
    bool EngineRunning=false;
};
struct FuelSupplyResult final
{
    double MeasuredFuelLiters=0.0;
    double MeasuredFuelFraction=0.0;
    double ExpectedBurnLiters=0.0;
    double PhysicalFlowRateLitersHour=0.0;
    double PredictedPressureKpa=0.0;
    double PressureDeficiencyFraction=0.0;
    double InjectorDutyFraction=0.0;
    double PickupExposureFraction=0.0;
    double EstimatedRangeCm=0.0;
    double EstimatedHoursRemaining=0.0;
    double FuelMassKg=0.0;
    double SpecificFuelConsumptionKgKwh=0.0;
    double InputEnergyKw=0.0;
    double ThermalEfficiencyEstimate=0.0;
    double FlowDiscrepancyLitersHour=0.0;
    bool PickupStarvationRisk=false;
    bool LowFuel=false;
    bool RefuelObserved=false;
    bool RealFuelTankChanged=false;
    bool TrueSensorObservation=false;
    bool Valid=false;
};
inline bool ValidFuelSupplyParameters(
    FuelSupplyParameters p) noexcept
{
    return Finite(p.TankCapacityLiters) &&
        p.TankCapacityLiters>0.0 &&
        Finite(p.MinimumReserveLiters) &&
        p.MinimumReserveLiters>=0.0 &&
        p.MinimumReserveLiters<p.TankCapacityLiters &&
        Finite(p.MaximumPhysicalFlowLitersHour) &&
        p.MaximumPhysicalFlowLitersHour>0.0 &&
        Finite(p.MaximumRefillRateLitersSecond) &&
        p.MaximumRefillRateLitersSecond>0.0 &&
        Finite(p.FuelPumpMinimumPressureKpa) &&
        p.FuelPumpMinimumPressureKpa>0.0 &&
        Finite(p.FuelPumpNominalPressureKpa) &&
        p.FuelPumpNominalPressureKpa>
            p.FuelPumpMinimumPressureKpa &&
        Finite(p.FuelPumpMaximumPressureKpa) &&
        p.FuelPumpMaximumPressureKpa>=
            p.FuelPumpNominalPressureKpa &&
        Finite(p.PumpPressureResponseSeconds) &&
        p.PumpPressureResponseSeconds>0.0 &&
        Finite(p.FuelSpecificEnergyMJPerKg) &&
        p.FuelSpecificEnergyMJPerKg>0.0 &&
        Finite(p.FuelDensityKgPerLiter) &&
        p.FuelDensityKgPerLiter>0.0 &&
        Finite(p.EstimatedThermalEfficiency) &&
        p.EstimatedThermalEfficiency>0.0 &&
        p.EstimatedThermalEfficiency<=1.0 &&
        Finite(p.MinimumSafeFuelFractionForCorner) &&
        p.MinimumSafeFuelFractionForCorner>=0.0 &&
        Finite(p.CornerFuelPickupThresholdG) &&
        p.CornerFuelPickupThresholdG>0.0 &&
        Finite(p.MaximumSensorDeltaLiters) &&
        p.MaximumSensorDeltaLiters>0.0 &&
        Finite(p.MaximumFrameSeconds) &&
        p.MaximumFrameSeconds>0.0 &&
        p.MaximumFrameSeconds<=MaxSimulationFrameSeconds;
}
inline bool ValidFuelSupplyObservation(
    FuelSupplyObservation s,
    FuelSupplyParameters p) noexcept
{
    return PhysicalActorIdentityValid(s.Identity) &&
        s.ActualFuelTankPresent &&
        s.MeasuredFuelLevelVerified &&
        s.PhysicalFuelPumpQualified &&
        ValidFrameDelta(s.DeltaSeconds) &&
        s.DeltaSeconds<=p.MaximumFrameSeconds &&
        Finite(s.PhysicalTankFuelLiters) &&
        s.PhysicalTankFuelLiters>=0.0 &&
        s.PhysicalTankFuelLiters<=p.TankCapacityLiters &&
        Finite(s.MeasuredFuelPumpPressureKpa) &&
        s.MeasuredFuelPumpPressureKpa>=0.0 &&
        s.MeasuredFuelPumpPressureKpa<=
            p.FuelPumpMaximumPressureKpa &&
        Finite(s.MeasuredInjectorDutyFraction) &&
        s.MeasuredInjectorDutyFraction>=0.0 &&
        s.MeasuredInjectorDutyFraction<=1.0 &&
        Finite(s.MeasuredFuelFlowLitersHour) &&
        s.MeasuredFuelFlowLitersHour>=0.0 &&
        s.MeasuredFuelFlowLitersHour<=
            p.MaximumPhysicalFlowLitersHour &&
        Finite(s.EngineRpm) &&
        s.EngineRpm>=0.0 &&
        s.EngineRpm<=p.MaximumEngineRpm &&
        Finite(s.EnginePowerKw) &&
        s.EnginePowerKw>=0.0 &&
        Finite(s.LateralAccelerationG) &&
        Finite(s.LongitudinalAccelerationG) &&
        Finite(s.FuelTemperatureC) &&
        s.FuelTemperatureC<=p.MaximumFuelTemperatureC &&
        Finite(s.TrackDistanceCm) &&
        Finite(s.ClockSeconds) &&
        s.ClockSeconds>=0.0;
}
inline FuelSupplyResult EvaluateFuelSupply(
    FuelSupplyObservation source,
    FuelSupplyParameters params={}) noexcept
{
    FuelSupplyResult out;
    if (!ValidFuelSupplyParameters(params) ||
        !ValidFuelSupplyObservation(source,params))
        return out;
    out.MeasuredFuelLiters=
        source.PhysicalTankFuelLiters;
    out.MeasuredFuelFraction=SafeDivision(
        source.PhysicalTankFuelLiters,
        params.TankCapacityLiters);
    out.PhysicalFlowRateLitersHour=
        source.MeasuredFuelFlowLitersHour;
    out.ExpectedBurnLiters=
        source.MeasuredFuelFlowLitersHour*
        source.DeltaSeconds/3600.0;
    out.PredictedPressureKpa=
        source.MeasuredFuelPumpPressureKpa;
    out.PressureDeficiencyFraction=Clamp01(
        1.0-SafeDivision(
            source.MeasuredFuelPumpPressureKpa,
            params.FuelPumpNominalPressureKpa));
    out.InjectorDutyFraction=
        source.MeasuredInjectorDutyFraction;
    const double lateralRisk=Smoothstep(
        params.CornerFuelPickupThresholdG,
        params.CornerFuelPickupThresholdG+0.7,
        std::abs(source.LateralAccelerationG));
    const double longitudinalRisk=Smoothstep(
        params.LongitudinalPickupCriticalG,
        params.LongitudinalPickupCriticalG+0.6,
        std::abs(source.LongitudinalAccelerationG));
    const double emptyTank=1.0-Smoothstep(
        0.0,params.MinimumSafeFuelFractionForCorner,
        out.MeasuredFuelFraction);
    out.PickupExposureFraction=Clamp01(
        std::max(lateralRisk,longitudinalRisk)*
        emptyTank);
    out.PickupStarvationRisk=
        out.PickupExposureFraction>0.2 ||
        source.MeasuredFuelPumpPressureKpa<
            params.FuelPumpMinimumPressureKpa;
    out.LowFuel=source.PhysicalTankFuelLiters<
        params.MinimumReserveLiters;
    out.RefuelObserved=
        source.PhysicallyRefueling &&
        source.InOriginalServiceBox;
    out.EstimatedHoursRemaining=SafeDivision(
        source.PhysicalTankFuelLiters,
        source.MeasuredFuelFlowLitersHour);
    out.FuelMassKg=
        source.PhysicalTankFuelLiters*
        params.FuelDensityKgPerLiter;
    out.SpecificFuelConsumptionKgKwh=
        SafeDivision(
            source.MeasuredFuelFlowLitersHour*
            params.FuelDensityKgPerLiter,
            source.EnginePowerKw);
    out.InputEnergyKw=
        source.MeasuredFuelFlowLitersHour*
        params.FuelDensityKgPerLiter*
        params.FuelSpecificEnergyMJPerKg/3.6;
    out.ThermalEfficiencyEstimate=SafeDivision(
        source.EnginePowerKw,out.InputEnergyKw);
    out.FlowDiscrepancyLitersHour=
        source.EnginePowerKw>0.0?
        source.MeasuredFuelFlowLitersHour-
        SafeDivision(source.EnginePowerKw,
            params.EstimatedThermalEfficiency*
            params.FuelDensityKgPerLiter*
            params.FuelSpecificEnergyMJPerKg/3.6):0.0;
    out.EstimatedRangeCm=0.0;
    out.RealFuelTankChanged=false;
    out.TrueSensorObservation=true;
    out.Valid=Finite(out.EstimatedHoursRemaining) &&
        Finite(out.FlowDiscrepancyLitersHour);
    return out;
}
struct FuelSupplyHistory final
{
    double FuelConsumedLiters=0.0;
    double FuelRefilledLiters=0.0;
    double LastMeasuredLiters=0.0;
    double HighestMeasuredFlowLitersHour=0.0;
    double LowestMeasuredPressureKpa=0.0;
    double TotalEngineRunningSeconds=0.0;
    double TotalPickupRiskSeconds=0.0;
    double AverageFlowLitersHour=0.0;
    double LargestSensorDiscontinuityLiters=0.0;
    std::uint64_t FuelPickupRiskFrames=0;
    std::uint64_t QualifiedFrames=0;
    std::uint64_t RejectedFrames=0;
    std::uint64_t RefuelEvents=0;
    bool ModifiesActualFuelTank=false;
};
class PhysicalFuelFlowMonitor final
{
public:
    bool Configure(FuelSupplyParameters p) noexcept
    {
        if (!ValidFuelSupplyParameters(p))return false;
        Settings=p;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        History={};
        Last={};
        HasFuel=false;
        Session=0;
        Actor=0;
        LastClockSeconds=0.0;
    }
    FuelSupplyResult Observe(FuelSupplyObservation source) noexcept
    {
        const auto result=EvaluateFuelSupply(
            source,Settings);
        if (!result.Valid)
        {
            ++History.RejectedFrames;
            return {};
        }
        if (HasFuel &&
            (source.Identity.Session!=Session ||
             source.Identity.Actor!=Actor ||
             source.ClockSeconds<=LastClockSeconds))
        {
            ++History.RejectedFrames;
            HasFuel=false;
            return {};
        }
        if (HasFuel)
        {
            const double difference=
                source.PhysicalTankFuelLiters-
                History.LastMeasuredLiters;
            History.LargestSensorDiscontinuityLiters=std::max(
                History.LargestSensorDiscontinuityLiters,
                std::abs(difference));
            if (difference>Settings.MaximumSensorDeltaLiters &&
                !(source.PhysicallyRefueling &&
                  source.InOriginalServiceBox))
            {
                ++History.RejectedFrames;
                HasFuel=false;
                return {};
            }
            if (difference<0.0)
                History.FuelConsumedLiters+=-difference;
            else if (difference>0.0 &&
                    source.PhysicallyRefueling &&
                    source.InOriginalServiceBox)
            {
                History.FuelRefilledLiters+=difference;
                ++History.RefuelEvents;
            }
        }
        Session=source.Identity.Session;
        Actor=source.Identity.Actor;
        LastClockSeconds=source.ClockSeconds;
        HasFuel=true;
        History.LastMeasuredLiters=
            source.PhysicalTankFuelLiters;
        History.HighestMeasuredFlowLitersHour=std::max(
            History.HighestMeasuredFlowLitersHour,
            source.MeasuredFuelFlowLitersHour);
        if (History.QualifiedFrames==0)
            History.LowestMeasuredPressureKpa=
                source.MeasuredFuelPumpPressureKpa;
        else History.LowestMeasuredPressureKpa=std::min(
            History.LowestMeasuredPressureKpa,
            source.MeasuredFuelPumpPressureKpa);
        if (source.EngineRunning)
            History.TotalEngineRunningSeconds+=
                source.DeltaSeconds;
        if (result.PickupStarvationRisk)
        {
            ++History.FuelPickupRiskFrames;
            History.TotalPickupRiskSeconds+=
                source.DeltaSeconds;
        }
        ++History.QualifiedFrames;
        History.AverageFlowLitersHour+=SafeDivision(
            source.MeasuredFuelFlowLitersHour-
                History.AverageFlowLitersHour,
            static_cast<double>(History.QualifiedFrames));
        History.ModifiesActualFuelTank=false;
        Last=result;
        return Last;
    }
    FuelSupplyHistory Snapshot() const noexcept{return History;}
    FuelSupplyResult LastFrame() const noexcept{return Last;}
private:
    FuelSupplyParameters Settings{};
    FuelSupplyHistory History{};
    FuelSupplyResult Last{};
    std::uint64_t Session=0;
    std::uint64_t Actor=0;
    double LastClockSeconds=0.0;
    bool HasFuel=false;
};
} // namespace latebrake::racecraft
