#pragma once
#include "Racecraft/GearboxDynamics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::racecraft
{
enum class DifferentialKind : std::uint8_t
{
    Open, ViscousLimitedSlip, ClutchPlateLimitedSlip,
    ElectronicTorqueVectoring, Locked
};
struct DifferentialParameters final
{
    DifferentialKind Kind = DifferentialKind::ClutchPlateLimitedSlip;
    double BiasRatio = 2.0;
    double PreloadTorqueNm = 80.0;
    double LockingOnPowerFraction = 0.45;
    double LockingOnCoastFraction = 0.25;
    double MaximumLockTorqueNm = 800.0;
    double MaximumWheelTorqueNm = 5000.0;
    double MaximumWheelSpeedDifferenceRadS = 300.0;
    double ViscousDampingNmPerRadS = 12.0;
    double ElectronicVectoringGain = 0.6;
    double TorqueVectoringYawGain = 0.12;
    double ElectronicActuationTimeSeconds = 0.08;
    double MechanicalEfficiency = 0.97;
    double DifferentialCoolingSeconds = 25.0;
    double DifferentialThermalGainCPerJ = 0.00012;
    double OverheatStartC = 145.0;
    double OverheatCriticalC = 210.0;
    double MaximumTemperatureC = 350.0;
    bool ApplyToDrivenAxleOnly = true;
    bool FailClosedOnMissingWheel = true;
};
struct DifferentialInput final
{
    double InputTorqueNm = 0.0;
    double LeftWheelRadS = 0.0;
    double RightWheelRadS = 0.0;
    double LeftNormalLoadN = 0.0;
    double RightNormalLoadN = 0.0;
    double LeftGripFraction = 1.0;
    double RightGripFraction = 1.0;
    double SteeringRequest = 0.0;
    double MeasuredYawRadiansSecond = 0.0;
    double TargetYawRadiansSecond = 0.0;
    double MeasuredDifferentialTempC = 80.0;
    double AmbientC = 25.0;
    double DeltaSeconds = 0.0;
    bool DrivenAxle = true;
    bool PhysicalWheelSpeedVerified = false;
    bool OriginalEngineQualified = false;
};
struct DifferentialOutput final
{
    double LeftTorqueNm = 0.0;
    double RightTorqueNm = 0.0;
    double ClutchLockTorqueNm = 0.0;
    double EffectiveBiasRatio = 1.0;
    double PowerLockFraction = 0.0;
    double CoastLockFraction = 0.0;
    double YawCorrectionDemand = 0.0;
    double HeatGeneratedJ = 0.0;
    double HeatRateWatts = 0.0;
    double DifferentialTemperatureC = 0.0;
    double TorqueTransferEfficiency = 0.0;
    double LimitedByLeftGripNm = 0.0;
    double LimitedByRightGripNm = 0.0;
    double UnusedTorqueNm = 0.0;
    bool WheelSpeedMismatch = false;
    bool IsLocking = false;
    bool Overheated = false;
    bool PhysicalAxleQualified = false;
    bool AuthoritativeWheelForces = false;
    bool Valid = false;
};
inline bool ValidDifferentialParameters(
    DifferentialParameters s) noexcept
{
    return Finite(s.BiasRatio) && s.BiasRatio >= 1.0 &&
        s.BiasRatio <= 20.0 &&
        Finite(s.PreloadTorqueNm) &&
        s.PreloadTorqueNm >= 0.0 &&
        Finite(s.LockingOnPowerFraction) &&
        s.LockingOnPowerFraction >= 0.0 &&
        s.LockingOnPowerFraction <= 1.0 &&
        Finite(s.LockingOnCoastFraction) &&
        s.LockingOnCoastFraction >= 0.0 &&
        s.LockingOnCoastFraction <= 1.0 &&
        Finite(s.MaximumLockTorqueNm) &&
        s.MaximumLockTorqueNm >= 0.0 &&
        Finite(s.MaximumWheelTorqueNm) &&
        s.MaximumWheelTorqueNm > 0.0 &&
        Finite(s.MaximumWheelSpeedDifferenceRadS) &&
        s.MaximumWheelSpeedDifferenceRadS > 0.0 &&
        Finite(s.ViscousDampingNmPerRadS) &&
        s.ViscousDampingNmPerRadS >= 0.0 &&
        Finite(s.ElectronicVectoringGain) &&
        s.ElectronicVectoringGain >= 0.0 &&
        Finite(s.TorqueVectoringYawGain) &&
        s.TorqueVectoringYawGain >= 0.0 &&
        Finite(s.ElectronicActuationTimeSeconds) &&
        s.ElectronicActuationTimeSeconds > 0.0 &&
        Finite(s.MechanicalEfficiency) &&
        s.MechanicalEfficiency > 0.0 &&
        s.MechanicalEfficiency <= 1.0 &&
        Finite(s.DifferentialCoolingSeconds) &&
        s.DifferentialCoolingSeconds > 0.0 &&
        Finite(s.DifferentialThermalGainCPerJ) &&
        s.DifferentialThermalGainCPerJ >= 0.0 &&
        Finite(s.OverheatStartC) &&
        Finite(s.OverheatCriticalC) &&
        s.OverheatCriticalC > s.OverheatStartC &&
        Finite(s.MaximumTemperatureC) &&
        s.MaximumTemperatureC >= s.OverheatCriticalC;
}
inline bool ValidDifferentialInput(
    DifferentialInput input,
    DifferentialParameters params) noexcept
{
    return input.PhysicalWheelSpeedVerified &&
        input.OriginalEngineQualified &&
        input.DrivenAxle &&
        ValidFrameDelta(input.DeltaSeconds) &&
        Finite(input.InputTorqueNm) &&
        std::abs(input.InputTorqueNm) <=
            2.0 * params.MaximumWheelTorqueNm &&
        Finite(input.LeftWheelRadS) &&
        Finite(input.RightWheelRadS) &&
        std::abs(input.LeftWheelRadS) <= 2000.0 &&
        std::abs(input.RightWheelRadS) <= 2000.0 &&
        Finite(input.LeftNormalLoadN) &&
        input.LeftNormalLoadN >= 0.0 &&
        Finite(input.RightNormalLoadN) &&
        input.RightNormalLoadN >= 0.0 &&
        Finite(input.LeftGripFraction) &&
        input.LeftGripFraction >= 0.0 &&
        input.LeftGripFraction <= 4.0 &&
        Finite(input.RightGripFraction) &&
        input.RightGripFraction >= 0.0 &&
        input.RightGripFraction <= 4.0 &&
        Finite(input.SteeringRequest) &&
        std::abs(input.SteeringRequest) <= 1.0 &&
        Finite(input.MeasuredYawRadiansSecond) &&
        Finite(input.TargetYawRadiansSecond) &&
        Finite(input.MeasuredDifferentialTempC) &&
        input.MeasuredDifferentialTempC >= -80.0 &&
        input.MeasuredDifferentialTempC <=
            params.MaximumTemperatureC &&
        Finite(input.AmbientC);
}
inline DifferentialOutput ResolveDifferential(
    DifferentialInput input,
    DifferentialParameters params,
    double previousVectorFraction = 0.0) noexcept
{
    DifferentialOutput out;
    if (!ValidDifferentialParameters(params) ||
        !ValidDifferentialInput(input, params))
        return out;
    const double angularDelta =
        input.LeftWheelRadS - input.RightWheelRadS;
    const double totalNm =
        input.InputTorqueNm * params.MechanicalEfficiency;
    const double leftGrip =
        input.LeftNormalLoadN *
        input.LeftGripFraction * 0.33;
    const double rightGrip =
        input.RightNormalLoadN *
        input.RightGripFraction * 0.33;
    double torqueBias = 0.0;
    double lockDemand = 0.0;
    if (params.Kind == DifferentialKind::Locked)
    {
        lockDemand = params.MaximumLockTorqueNm;
        torqueBias = std::clamp(
            angularDelta * params.ViscousDampingNmPerRadS,
            -params.MaximumLockTorqueNm,
            params.MaximumLockTorqueNm);
    }
    else if (params.Kind ==
             DifferentialKind::ViscousLimitedSlip)
    {
        lockDemand = params.PreloadTorqueNm +
            params.ViscousDampingNmPerRadS *
            std::abs(angularDelta);
        torqueBias = Sign(angularDelta) *
            std::min(params.MaximumLockTorqueNm, lockDemand);
    }
    else if (params.Kind ==
             DifferentialKind::ClutchPlateLimitedSlip)
    {
        const double clutch = totalNm >= 0.0 ?
            params.LockingOnPowerFraction :
            params.LockingOnCoastFraction;
        lockDemand = params.PreloadTorqueNm +
            std::abs(totalNm) * clutch;
        torqueBias = Sign(angularDelta) *
            std::min(params.MaximumLockTorqueNm,
                     lockDemand);
    }
    else if (params.Kind ==
             DifferentialKind::ElectronicTorqueVectoring)
    {
        const double desired = std::clamp(
            (input.TargetYawRadiansSecond -
             input.MeasuredYawRadiansSecond) *
            params.TorqueVectoringYawGain +
            input.SteeringRequest * params.ElectronicVectoringGain,
            -1.0, 1.0);
        const double smoothed = ExponentialResponse(
            previousVectorFraction, desired,
            params.ElectronicActuationTimeSeconds,
            input.DeltaSeconds);
        torqueBias = smoothed * std::min(
            params.MaximumLockTorqueNm,
            0.5 * std::abs(totalNm));
        out.YawCorrectionDemand = smoothed;
        lockDemand = std::abs(torqueBias);
    }
    const double requestedLeft =
        0.5 * totalNm - 0.5 * torqueBias;
    const double requestedRight =
        0.5 * totalNm + 0.5 * torqueBias;
    const double leftLimit = std::min(
        params.MaximumWheelTorqueNm, leftGrip);
    const double rightLimit = std::min(
        params.MaximumWheelTorqueNm, rightGrip);
    out.LeftTorqueNm = std::clamp(
        requestedLeft, -leftLimit, leftLimit);
    out.RightTorqueNm = std::clamp(
        requestedRight, -rightLimit, rightLimit);
    out.ClutchLockTorqueNm =
        std::min(params.MaximumLockTorqueNm,
                 lockDemand);
    out.EffectiveBiasRatio = SafeDivision(
        std::max(std::abs(out.LeftTorqueNm),
                 std::abs(out.RightTorqueNm)) + 1.0,
        std::min(std::abs(out.LeftTorqueNm),
                 std::abs(out.RightTorqueNm)) + 1.0);
    out.EffectiveBiasRatio = std::min(
        out.EffectiveBiasRatio, params.BiasRatio);
    out.PowerLockFraction =
        totalNm >= 0.0 ?
        params.LockingOnPowerFraction : 0.0;
    out.CoastLockFraction =
        totalNm < 0.0 ?
        params.LockingOnCoastFraction : 0.0;
    out.HeatRateWatts =
        std::abs(out.ClutchLockTorqueNm * angularDelta);
    out.HeatGeneratedJ =
        out.HeatRateWatts * input.DeltaSeconds;
    out.DifferentialTemperatureC =
        input.MeasuredDifferentialTempC +
        out.HeatGeneratedJ *
        params.DifferentialThermalGainCPerJ;
    out.TorqueTransferEfficiency =
        params.MechanicalEfficiency;
    out.LimitedByLeftGripNm =
        std::max(0.0, std::abs(requestedLeft) - leftLimit);
    out.LimitedByRightGripNm =
        std::max(0.0, std::abs(requestedRight) - rightLimit);
    out.UnusedTorqueNm =
        totalNm - out.LeftTorqueNm - out.RightTorqueNm;
    out.WheelSpeedMismatch =
        std::abs(angularDelta) >
        params.MaximumWheelSpeedDifferenceRadS;
    out.IsLocking = out.ClutchLockTorqueNm > 0.0;
    out.Overheated =
        input.MeasuredDifferentialTempC >=
            params.OverheatCriticalC;
    out.PhysicalAxleQualified = true;
    out.AuthoritativeWheelForces = false;
    out.Valid = Finite(out.LeftTorqueNm) &&
        Finite(out.RightTorqueNm) &&
        Finite(out.HeatGeneratedJ);
    return out;
}
class DifferentialController final
{
public:
    bool Configure(DifferentialParameters parameters) noexcept
    {
        if (!ValidDifferentialParameters(parameters))
            return false;
        Parameters = parameters;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        PreviousVector = 0.0;
        Last = {};
        Frames = 0;
        Rejected = 0;
        TotalSlipEnergyJ = 0.0;
        PeakBiasRatio = 1.0;
        WheelSpeedMismatchFrames = 0;
    }
    DifferentialOutput Step(DifferentialInput input) noexcept
    {
        const auto calculated = ResolveDifferential(
            input, Parameters, PreviousVector);
        if (!calculated.Valid)
        {
            ++Rejected;
            return {};
        }
        Last = calculated;
        PreviousVector = Last.YawCorrectionDemand;
        TotalSlipEnergyJ += Last.HeatGeneratedJ;
        PeakBiasRatio = std::max(
            PeakBiasRatio, Last.EffectiveBiasRatio);
        WheelSpeedMismatchFrames +=
            Last.WheelSpeedMismatch ? 1U : 0U;
        ++Frames;
        return Last;
    }
    DifferentialOutput Snapshot() const noexcept { return Last; }
    std::uint64_t QualifiedFrames() const noexcept { return Frames; }
    std::uint64_t RejectedFrames() const noexcept { return Rejected; }
    double TotalClutchSlipEnergyJ() const noexcept
    {
        return TotalSlipEnergyJ;
    }
    double MaximumObservedBias() const noexcept { return PeakBiasRatio; }
private:
    DifferentialParameters Parameters{};
    DifferentialOutput Last{};
    double PreviousVector = 0.0;
    double TotalSlipEnergyJ = 0.0;
    double PeakBiasRatio = 1.0;
    std::uint64_t Frames = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t WheelSpeedMismatchFrames = 0;
};
} // namespace latebrake::racecraft
