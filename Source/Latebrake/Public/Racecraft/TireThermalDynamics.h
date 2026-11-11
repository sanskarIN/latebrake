#pragma once
#include "Racecraft/TireForceModel.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace latebrake::racecraft
{
struct TireThermalParameters final
{
    double OptimalCoreTemperatureC=86.0;
    double OptimalSurfaceTemperatureC=95.0;
    double NominalAmbientC=22.0;
    double CoreHeatCapacityJPerC=21000.0;
    double SurfaceHeatCapacityJPerC=3500.0;
    double InnerAirHeatCapacityJPerC=12000.0;
    double SurfaceToCoreConductanceWPerC=75.0;
    double CoreToAirConductanceWPerC=25.0;
    double SurfaceToRoadConductanceWPerC=70.0;
    double AirConvectionCoefficientWPerCPerMS=0.40;
    double WetSurfaceCoolingMultiplier=1.9;
    double StandingWaterCoolingMultiplier=2.4;
    double LongitudinalSlipHeatFraction=0.80;
    double LateralSlipHeatFraction=0.75;
    double BrakeHeatTransferFraction=0.09;
    double PressureGainKpaPerDegreeC=0.45;
    double MinimumTirePressureKpa=90.0;
    double MaximumTirePressureKpa=420.0;
    double OverheatSurfaceTemperatureC=145.0;
    double OverheatCoreTemperatureC=130.0;
    double MaximumQualifiedTemperatureC=350.0;
    double HeatCycleStartC=90.0;
    double HeatCycleFinishC=45.0;
    double WearPerJoule=0.000000005;
    double WearFromOverheatPerSecond=0.0000010;
    double FlatspotWearFromLockPerSecond=0.00006;
    double TreadDepthNewMm=8.0;
    double TreadDepthEndOfLifeMm=1.4;
    double MaximumAllowedFrameSeconds=0.10;
    bool RequireSourceWheelContact=true;
    bool RequireVerifiedRoadTemperature=true;
};
struct TireThermalInput final
{
    WheelObservation Wheel{};
    TireForceOutput Forces{};
    double PhysicalBrakeTemperatureC=90.0;
    double AmbientC=25.0;
    double ActualRoadTemperatureC=35.0;
    double WetnessFraction=0.0;
    double WaterDepthMm=0.0;
    double SpeedCmS=0.0;
    double MeasuredCoreTemperatureC=80.0;
    double MeasuredAirPressureKpa=210.0;
    double DeltaSeconds=0.0;
    TireCompound Compound=TireCompound::Medium;
    bool PhysicalContactConfirmed=false;
    bool OriginalEngineTireSensorReady=false;
};
struct TireThermalState final
{
    double CoreC=75.0;
    double SurfaceC=75.0;
    double AirC=70.0;
    double PressureKpa=210.0;
    double TreadWearFraction=0.0;
    double FlatspotFraction=0.0;
    double AccumulatedEnergyJ=0.0;
    double HeatToRoadJ=0.0;
    double HeatToAirJ=0.0;
    double AccumulatedSlipHeatJ=0.0;
    double TimeAboveOptimumSeconds=0.0;
    double TimeWithLockedWheelSeconds=0.0;
    double ContactSeconds=0.0;
    double LastUpdateSeconds=0.0;
    int CompletedHeatCycles=0;
    bool HeatCycleArmed=false;
    bool ContactQualified=false;
    bool ActualTemperatureAuthority=false;
    bool Valid=false;
};
struct TireThermalOutput final
{
    TireThermalState Predicted{};
    double LongitudinalSlipHeatWatts=0.0;
    double LateralSlipHeatWatts=0.0;
    double ConductiveRoadHeatWatts=0.0;
    double ConvectionAirHeatWatts=0.0;
    double SurfaceToCoreHeatWatts=0.0;
    double CoreToAirHeatWatts=0.0;
    double BrakeHeatWatts=0.0;
    double NetSurfaceHeatWatts=0.0;
    double NetCoreHeatWatts=0.0;
    double NetAirHeatWatts=0.0;
    double TreadDepthRemainingMm=0.0;
    double OptimalTemperatureDeviationC=0.0;
    double PredictedGripThermalScale=0.0;
    double FlatspotSeverity=0.0;
    bool Overheated=false;
    bool UnderTemperature=false;
    bool WaterCooling=false;
    bool WheelSensorTrusted=false;
    bool AppliedToActualTire=false;
    bool Valid=false;
};
inline bool ValidTireThermalParameters(
    TireThermalParameters p) noexcept
{
    return Finite(p.CoreHeatCapacityJPerC) &&
        p.CoreHeatCapacityJPerC>0.0 &&
        Finite(p.SurfaceHeatCapacityJPerC) &&
        p.SurfaceHeatCapacityJPerC>0.0 &&
        Finite(p.InnerAirHeatCapacityJPerC) &&
        p.InnerAirHeatCapacityJPerC>0.0 &&
        Finite(p.SurfaceToCoreConductanceWPerC) &&
        p.SurfaceToCoreConductanceWPerC>=0.0 &&
        Finite(p.CoreToAirConductanceWPerC) &&
        p.CoreToAirConductanceWPerC>=0.0 &&
        Finite(p.SurfaceToRoadConductanceWPerC) &&
        p.SurfaceToRoadConductanceWPerC>=0.0 &&
        Finite(p.AirConvectionCoefficientWPerCPerMS) &&
        p.AirConvectionCoefficientWPerCPerMS>=0.0 &&
        Finite(p.WetSurfaceCoolingMultiplier) &&
        p.WetSurfaceCoolingMultiplier>=1.0 &&
        Finite(p.StandingWaterCoolingMultiplier) &&
        p.StandingWaterCoolingMultiplier>=1.0 &&
        Finite(p.PressureGainKpaPerDegreeC) &&
        p.PressureGainKpaPerDegreeC>=0.0 &&
        Finite(p.MinimumTirePressureKpa) &&
        p.MinimumTirePressureKpa>0.0 &&
        Finite(p.MaximumTirePressureKpa) &&
        p.MaximumTirePressureKpa>p.MinimumTirePressureKpa &&
        Finite(p.OverheatSurfaceTemperatureC) &&
        Finite(p.OverheatCoreTemperatureC) &&
        Finite(p.HeatCycleStartC) &&
        Finite(p.HeatCycleFinishC) &&
        p.HeatCycleFinishC<p.HeatCycleStartC &&
        Finite(p.WearPerJoule) &&
        p.WearPerJoule>=0.0 &&
        Finite(p.FlatspotWearFromLockPerSecond) &&
        p.FlatspotWearFromLockPerSecond>=0.0 &&
        Finite(p.TreadDepthNewMm) &&
        p.TreadDepthNewMm>0.0 &&
        Finite(p.TreadDepthEndOfLifeMm) &&
        p.TreadDepthEndOfLifeMm>=0.0 &&
        p.TreadDepthEndOfLifeMm<p.TreadDepthNewMm &&
        Finite(p.MaximumAllowedFrameSeconds) &&
        p.MaximumAllowedFrameSeconds>0.0 &&
        p.MaximumAllowedFrameSeconds<=MaxSimulationFrameSeconds;
}
inline bool ValidTireThermalInput(
    TireThermalInput i,TireThermalParameters p) noexcept
{
    return ValidWheelObservation(i.Wheel) &&
        i.Forces.Valid &&
        i.OriginalEngineTireSensorReady &&
        (!p.RequireSourceWheelContact ||
         (i.PhysicalContactConfirmed &&
          i.Wheel.PhysicalContact &&
          i.Wheel.ContactVerified)) &&
        ValidFrameDelta(i.DeltaSeconds) &&
        i.DeltaSeconds<=p.MaximumAllowedFrameSeconds &&
        Finite(i.PhysicalBrakeTemperatureC) &&
        Finite(i.AmbientC) &&
        Finite(i.ActualRoadTemperatureC) &&
        Finite(i.WetnessFraction) &&
        Finite(i.WaterDepthMm) &&
        Finite(i.SpeedCmS) &&
        Finite(i.MeasuredCoreTemperatureC) &&
        Finite(i.MeasuredAirPressureKpa) &&
        i.WetnessFraction>=0.0 &&
        i.WetnessFraction<=1.0 &&
        i.WaterDepthMm>=0.0 &&
        i.WaterDepthMm<=100.0 &&
        i.SpeedCmS>=0.0 &&
        i.SpeedCmS<=MaximumPhysicalSpeedCmS &&
        i.MeasuredCoreTemperatureC>=-80.0 &&
        i.MeasuredCoreTemperatureC<=
            p.MaximumQualifiedTemperatureC &&
        i.MeasuredAirPressureKpa>=
            p.MinimumTirePressureKpa &&
        i.MeasuredAirPressureKpa<=
            p.MaximumTirePressureKpa;
}
inline TireThermalOutput IntegrateTireThermal(
    TireThermalInput input,TireThermalState state,
    TireThermalParameters params={}) noexcept
{
    TireThermalOutput out;
    if (!ValidTireThermalParameters(params) ||
        !ValidTireThermalInput(input,params))
        return out;
    const double dt=input.DeltaSeconds;
    const double speedMS=
        input.SpeedCmS/CentimetersPerMeter;
    const double waterFactor=
        1.0+(params.WetSurfaceCoolingMultiplier-1.0)*
        input.WetnessFraction+
        (params.StandingWaterCoolingMultiplier-1.0)*
        Clamp01(input.WaterDepthMm/8.0);
    out.LongitudinalSlipHeatWatts=
        input.Forces.LongitudinalSlipPowerWatts*
        params.LongitudinalSlipHeatFraction;
    out.LateralSlipHeatWatts=
        input.Forces.LateralSlipPowerWatts*
        params.LateralSlipHeatFraction;
    out.SurfaceToCoreHeatWatts=
        (state.SurfaceC-state.CoreC)*
        params.SurfaceToCoreConductanceWPerC;
    out.CoreToAirHeatWatts=
        (state.CoreC-state.AirC)*
        params.CoreToAirConductanceWPerC;
    out.ConductiveRoadHeatWatts=
        (state.SurfaceC-input.ActualRoadTemperatureC)*
        params.SurfaceToRoadConductanceWPerC*waterFactor;
    out.ConvectionAirHeatWatts=
        (state.SurfaceC-input.AmbientC)*
        params.AirConvectionCoefficientWPerCPerMS*
        speedMS*waterFactor;
    out.BrakeHeatWatts=
        std::max(0.0,input.PhysicalBrakeTemperatureC-
            state.CoreC)*
        params.BrakeHeatTransferFraction*12.0;
    out.NetSurfaceHeatWatts=
        out.LongitudinalSlipHeatWatts+
        out.LateralSlipHeatWatts-
        out.SurfaceToCoreHeatWatts-
        out.ConductiveRoadHeatWatts-
        out.ConvectionAirHeatWatts;
    out.NetCoreHeatWatts=
        out.SurfaceToCoreHeatWatts+
        out.BrakeHeatWatts-
        out.CoreToAirHeatWatts;
    out.NetAirHeatWatts=
        out.CoreToAirHeatWatts-
        (state.AirC-input.AmbientC)*5.0;
    state.SurfaceC+=SafeDivision(
        out.NetSurfaceHeatWatts*dt,
        params.SurfaceHeatCapacityJPerC);
    state.CoreC+=SafeDivision(
        out.NetCoreHeatWatts*dt,
        params.CoreHeatCapacityJPerC);
    state.AirC+=SafeDivision(
        out.NetAirHeatWatts*dt,
        params.InnerAirHeatCapacityJPerC);
    state.SurfaceC=std::clamp(
        state.SurfaceC,-80.0,params.MaximumQualifiedTemperatureC);
    state.CoreC=std::clamp(
        state.CoreC,-80.0,params.MaximumQualifiedTemperatureC);
    state.AirC=std::clamp(
        state.AirC,-80.0,params.MaximumQualifiedTemperatureC);
    state.PressureKpa=std::clamp(
        input.MeasuredAirPressureKpa+
        params.PressureGainKpaPerDegreeC*
        (state.AirC-input.AmbientC),
        params.MinimumTirePressureKpa,
        params.MaximumTirePressureKpa);
    const double slipJ=(
        out.LongitudinalSlipHeatWatts+
        out.LateralSlipHeatWatts)*dt;
    const double overheat=std::max(
        0.0,state.SurfaceC-
            params.OverheatSurfaceTemperatureC);
    const double wearDelta=
        slipJ*params.WearPerJoule+
        overheat*
        params.WearFromOverheatPerSecond*dt;
    state.TreadWearFraction=Clamp01(
        state.TreadWearFraction+wearDelta);
    if (input.Forces.LockedWheel)
    {
        state.TimeWithLockedWheelSeconds+=dt;
        state.FlatspotFraction=Clamp01(
            state.FlatspotFraction+
            params.FlatspotWearFromLockPerSecond*dt);
    }
    if (state.SurfaceC>=params.HeatCycleStartC)
        state.HeatCycleArmed=true;
    if (state.HeatCycleArmed &&
        state.SurfaceC<=params.HeatCycleFinishC)
    {
        ++state.CompletedHeatCycles;
        state.HeatCycleArmed=false;
    }
    if (state.SurfaceC>params.OptimalSurfaceTemperatureC)
        state.TimeAboveOptimumSeconds+=dt;
    state.AccumulatedSlipHeatJ+=slipJ;
    state.AccumulatedEnergyJ+=(
        out.NetSurfaceHeatWatts+
        out.NetCoreHeatWatts)*dt;
    state.HeatToRoadJ+=std::max(
        0.0,out.ConductiveRoadHeatWatts)*dt;
    state.HeatToAirJ+=std::max(
        0.0,out.ConvectionAirHeatWatts)*dt;
    state.ContactSeconds+=dt;
    state.ContactQualified=true;
    state.ActualTemperatureAuthority=false;
    state.Valid=true;
    out.Predicted=state;
    out.TreadDepthRemainingMm=Lerp(
        params.TreadDepthNewMm,
        params.TreadDepthEndOfLifeMm,
        state.TreadWearFraction);
    out.OptimalTemperatureDeviationC=
        state.SurfaceC-
        params.OptimalSurfaceTemperatureC;
    out.PredictedGripThermalScale=
        TemperatureGripFraction(
            state.SurfaceC,input.ActualRoadTemperatureC,
            input.Compound);
    out.FlatspotSeverity=state.FlatspotFraction;
    out.Overheated=state.SurfaceC>=
        params.OverheatSurfaceTemperatureC ||
        state.CoreC>=params.OverheatCoreTemperatureC;
    out.UnderTemperature=state.SurfaceC<
        params.OptimalSurfaceTemperatureC-35.0;
    out.WaterCooling=input.WetnessFraction>0.2;
    out.WheelSensorTrusted=true;
    out.AppliedToActualTire=false;
    out.Valid=Finite(state.SurfaceC) &&
        Finite(state.CoreC) &&
        Finite(state.AirC) &&
        Finite(out.TreadDepthRemainingMm);
    return out;
}
class FourCornerTireThermals final
{
public:
    bool Configure(TireThermalParameters params) noexcept
    {
        if (!ValidTireThermalParameters(params))return false;
        Settings=params;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        States.fill({});
        Last.fill({});
        Frames=0;
        Rejected=0;
        OverheatFrames=0;
        TotalSlipHeatJ=0.0;
        MaximumSurfaceC=-80.0;
    }
    bool Step(const std::array<TireThermalInput,4>& inputs) noexcept
    {
        std::array<TireThermalOutput,4> outputs{};
        for (std::size_t i=0;i<4;++i)
        {
            outputs[i]=IntegrateTireThermal(
                inputs[i],States[i],Settings);
            if (!outputs[i].Valid)
            {
                ++Rejected;
                return false;
            }
        }
        for (std::size_t i=0;i<4;++i)
        {
            States[i]=outputs[i].Predicted;
            Last[i]=outputs[i];
            OverheatFrames+=outputs[i].Overheated?1U:0U;
            TotalSlipHeatJ+=
                outputs[i].Predicted.AccumulatedSlipHeatJ;
            MaximumSurfaceC=std::max(
                MaximumSurfaceC,States[i].SurfaceC);
        }
        ++Frames;
        return true;
    }
    std::array<TireThermalOutput,4> Snapshot() const noexcept
    {return Last;}
    std::uint64_t QualifiedFrames() const noexcept{return Frames;}
    std::uint64_t RejectedFrames() const noexcept{return Rejected;}
    double PeakSurfaceTemperatureC() const noexcept
    {return MaximumSurfaceC;}
private:
    TireThermalParameters Settings{};
    std::array<TireThermalState,4> States{};
    std::array<TireThermalOutput,4> Last{};
    std::uint64_t Frames=0;
    std::uint64_t Rejected=0;
    std::uint64_t OverheatFrames=0;
    double TotalSlipHeatJ=0.0;
    double MaximumSurfaceC=-80.0;
};
} // namespace latebrake::racecraft
