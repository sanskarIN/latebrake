#pragma once
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
struct LoadTransferParameters final
{
    double StaticFrontAxleFraction = 0.53;
    double CenterOfGravityHeightCm = 52.0;
    double WheelbaseCm = 268.0;
    double FrontTrackCm = 158.0;
    double RearTrackCm = 156.0;
    double FrontRollStiffnessFraction = 0.58;
    double MaximumSupportedAccelerationG = 3.5;
    double MaximumBankAngleRadians = 0.45;
    double MaximumSlopeRadians = 0.40;
    double MaximumDownforceNewtons = 60000.0;
    double LiftAxleThresholdN = 100.0;
    double RequiredSupportedFraction = 0.50;
    bool IncludeRoadBanking = true;
    bool IncludeAerodynamicLoads = true;
    bool AllowWheelLift = true;
};
struct LoadTransferInput final
{
    double MassKg = 1250.0;
    double LongitudinalAccelerationCmS2 = 0.0;
    double LateralAccelerationCmS2 = 0.0;
    double BankRadians = 0.0;
    double GradeRadians = 0.0;
    double FrontAerodynamicDownforceN = 0.0;
    double RearAerodynamicDownforceN = 0.0;
    double GravityScale = 1.0;
    double DeltaSeconds = 0.0;
    bool BodySimulating = false;
    bool OriginalRoadContactMeasured = false;
};
struct WheelLoadDistribution final
{
    std::array<double, PhysicalWheelCount> NormalLoadN{};
    double FrontAxleLoadN = 0.0;
    double RearAxleLoadN = 0.0;
    double LeftSideLoadN = 0.0;
    double RightSideLoadN = 0.0;
    double TotalContactLoadN = 0.0;
    double StaticVehicleWeightN = 0.0;
    double AerodynamicDownforceN = 0.0;
    double LongitudinalTransferN = 0.0;
    double FrontLateralTransferN = 0.0;
    double RearLateralTransferN = 0.0;
    double BodyRollTendency = 0.0;
    double BodyPitchTendency = 0.0;
    double SupportFraction = 0.0;
    double MaximumWheelLoadN = 0.0;
    double MinimumWheelLoadN = 0.0;
    int LiftedWheels = 0;
    int SupportingWheels = 0;
    bool TipRisk = false;
    bool LoadConserved = false;
    bool OriginalContactProven = false;
    bool Valid = false;
};
inline bool ValidLoadTransferParameters(
    LoadTransferParameters s) noexcept
{
    return Finite(s.StaticFrontAxleFraction) &&
        s.StaticFrontAxleFraction > 0.2 &&
        s.StaticFrontAxleFraction < 0.8 &&
        Finite(s.CenterOfGravityHeightCm) &&
        s.CenterOfGravityHeightCm > 5.0 &&
        s.CenterOfGravityHeightCm < 250.0 &&
        Finite(s.WheelbaseCm) &&
        s.WheelbaseCm > 100.0 &&
        Finite(s.FrontTrackCm) &&
        s.FrontTrackCm > 75.0 &&
        Finite(s.RearTrackCm) &&
        s.RearTrackCm > 75.0 &&
        Finite(s.FrontRollStiffnessFraction) &&
        s.FrontRollStiffnessFraction >= 0.0 &&
        s.FrontRollStiffnessFraction <= 1.0 &&
        Finite(s.MaximumSupportedAccelerationG) &&
        s.MaximumSupportedAccelerationG > 0.0 &&
        Finite(s.MaximumBankAngleRadians) &&
        s.MaximumBankAngleRadians >= 0.0 &&
        Finite(s.MaximumSlopeRadians) &&
        s.MaximumSlopeRadians >= 0.0 &&
        Finite(s.MaximumDownforceNewtons) &&
        s.MaximumDownforceNewtons >= 0.0 &&
        Finite(s.LiftAxleThresholdN) &&
        s.LiftAxleThresholdN >= 0.0 &&
        Finite(s.RequiredSupportedFraction) &&
        s.RequiredSupportedFraction > 0.0 &&
        s.RequiredSupportedFraction <= 1.0;
}
inline bool ValidLoadTransferInput(
    LoadTransferInput s,
    LoadTransferParameters settings) noexcept
{
    return s.BodySimulating &&
        s.OriginalRoadContactMeasured &&
        ValidFrameDelta(s.DeltaSeconds) &&
        Finite(s.MassKg) && s.MassKg >= 200.0 &&
        s.MassKg <= 12000.0 &&
        Finite(s.LongitudinalAccelerationCmS2) &&
        std::abs(s.LongitudinalAccelerationCmS2) <=
            settings.MaximumSupportedAccelerationG *
            GravityCmS2 &&
        Finite(s.LateralAccelerationCmS2) &&
        std::abs(s.LateralAccelerationCmS2) <=
            settings.MaximumSupportedAccelerationG *
            GravityCmS2 &&
        Finite(s.BankRadians) &&
        std::abs(s.BankRadians) <=
            settings.MaximumBankAngleRadians &&
        Finite(s.GradeRadians) &&
        std::abs(s.GradeRadians) <=
            settings.MaximumSlopeRadians &&
        Finite(s.FrontAerodynamicDownforceN) &&
        s.FrontAerodynamicDownforceN >= 0.0 &&
        s.FrontAerodynamicDownforceN <=
            settings.MaximumDownforceNewtons &&
        Finite(s.RearAerodynamicDownforceN) &&
        s.RearAerodynamicDownforceN >= 0.0 &&
        s.RearAerodynamicDownforceN <=
            settings.MaximumDownforceNewtons &&
        Finite(s.GravityScale) &&
        s.GravityScale >= 0.5 &&
        s.GravityScale <= 1.5;
}
inline WheelLoadDistribution EvaluateWheelLoadTransfer(
    LoadTransferInput input,
    LoadTransferParameters settings = {}) noexcept
{
    WheelLoadDistribution out;
    if (!ValidLoadTransferParameters(settings) ||
        !ValidLoadTransferInput(input, settings)) return out;
    const double effectiveGravity =
        GravityCmS2 * input.GravityScale;
    const double weightN =
        input.MassKg * effectiveGravity /
        CentimetersPerMeter;
    const double frontAero =
        settings.IncludeAerodynamicLoads ?
            input.FrontAerodynamicDownforceN : 0.0;
    const double rearAero =
        settings.IncludeAerodynamicLoads ?
            input.RearAerodynamicDownforceN : 0.0;
    const double totalDownforce = frontAero + rearAero;
    const double bankCorrection =
        settings.IncludeRoadBanking ?
            effectiveGravity * std::sin(input.BankRadians) : 0.0;
    const double effectiveLateral =
        input.LateralAccelerationCmS2 - bankCorrection;
    const double slopeCorrection =
        settings.IncludeRoadBanking ?
            effectiveGravity * std::sin(input.GradeRadians) : 0.0;
    const double effectiveLong =
        input.LongitudinalAccelerationCmS2 +
        slopeCorrection;
    const double transferN =
        input.MassKg * effectiveLong /
        CentimetersPerMeter *
        settings.CenterOfGravityHeightCm /
        settings.WheelbaseCm;
    const double frontLoad = weightN *
        settings.StaticFrontAxleFraction +
        frontAero - transferN;
    const double rearLoad = weightN *
        (1.0 - settings.StaticFrontAxleFraction) +
        rearAero + transferN;
    const double lateralTransferN =
        input.MassKg * effectiveLateral /
        CentimetersPerMeter * settings.CenterOfGravityHeightCm;
    const double frontTransfer =
        lateralTransferN * settings.FrontRollStiffnessFraction /
        settings.FrontTrackCm;
    const double rearTransfer =
        lateralTransferN * (1.0 -
            settings.FrontRollStiffnessFraction) /
        settings.RearTrackCm;
    out.NormalLoadN[0] = 0.5 * frontLoad + frontTransfer;
    out.NormalLoadN[1] = 0.5 * frontLoad - frontTransfer;
    out.NormalLoadN[2] = 0.5 * rearLoad + rearTransfer;
    out.NormalLoadN[3] = 0.5 * rearLoad - rearTransfer;
    out.StaticVehicleWeightN = weightN;
    out.AerodynamicDownforceN = totalDownforce;
    out.LongitudinalTransferN = transferN;
    out.FrontLateralTransferN = frontTransfer;
    out.RearLateralTransferN = rearTransfer;
    out.BodyRollTendency = SafeDivision(
        effectiveLateral, effectiveGravity);
    out.BodyPitchTendency = SafeDivision(
        effectiveLong, effectiveGravity);
    out.MinimumWheelLoadN =
        std::numeric_limits<double>::infinity();
    double rawLoad = 0.0;
    for (double& wheel : out.NormalLoadN)
    {
        rawLoad += wheel;
        if (wheel < settings.LiftAxleThresholdN)
            ++out.LiftedWheels;
        if (wheel < 0.0)
            wheel = 0.0;
        if (wheel >= settings.LiftAxleThresholdN)
            ++out.SupportingWheels;
        out.TotalContactLoadN += wheel;
        out.MaximumWheelLoadN =
            std::max(out.MaximumWheelLoadN, wheel);
        out.MinimumWheelLoadN =
            std::min(out.MinimumWheelLoadN, wheel);
    }
    out.FrontAxleLoadN =
        out.NormalLoadN[0] + out.NormalLoadN[1];
    out.RearAxleLoadN =
        out.NormalLoadN[2] + out.NormalLoadN[3];
    out.LeftSideLoadN =
        out.NormalLoadN[0] + out.NormalLoadN[2];
    out.RightSideLoadN =
        out.NormalLoadN[1] + out.NormalLoadN[3];
    out.SupportFraction =
        out.SupportingWheels / 4.0;
    out.LoadConserved =
        std::abs(rawLoad -
            (weightN + totalDownforce)) <=
            std::max(0.01, weightN * 0.000001);
    out.TipRisk =
        out.LiftedWheels > 0 ||
        out.SupportFraction <
            settings.RequiredSupportedFraction;
    out.OriginalContactProven = true;
    out.Valid = out.LoadConserved &&
        out.SupportFraction >=
            settings.RequiredSupportedFraction &&
        (settings.AllowWheelLift || out.LiftedWheels == 0);
    return out;
}
class LoadTransferMonitor final
{
public:
    bool Configure(LoadTransferParameters settings) noexcept
    {
        if (!ValidLoadTransferParameters(settings)) return false;
        Settings = settings;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Last = {};
        Frames = 0;
        Rejected = 0;
        TipFrames = 0;
        PeakFrontTransferN = 0.0;
        PeakLateralTransferN = 0.0;
        AccumulatedLiftSeconds = 0.0;
        MinSupportedWheels = 4;
        MaximumObservedLoadN = 0.0;
    }
    WheelLoadDistribution Observe(LoadTransferInput input) noexcept
    {
        Last = EvaluateWheelLoadTransfer(input, Settings);
        if (!Last.Valid)
        {
            ++Rejected;
            return Last;
        }
        ++Frames;
        if (Last.TipRisk)
        {
            ++TipFrames;
            AccumulatedLiftSeconds += input.DeltaSeconds;
        }
        PeakFrontTransferN = std::max(
            PeakFrontTransferN,
            std::abs(Last.LongitudinalTransferN));
        PeakLateralTransferN = std::max(
            PeakLateralTransferN,
            std::abs(Last.FrontLateralTransferN) +
            std::abs(Last.RearLateralTransferN));
        MinSupportedWheels =
            std::min(MinSupportedWheels, Last.SupportingWheels);
        MaximumObservedLoadN = std::max(
            MaximumObservedLoadN, Last.MaximumWheelLoadN);
        return Last;
    }
    WheelLoadDistribution Snapshot() const noexcept { return Last; }
    std::uint64_t AcceptedFrames() const noexcept { return Frames; }
    std::uint64_t RejectedFrames() const noexcept { return Rejected; }
    std::uint64_t LiftRiskFrames() const noexcept { return TipFrames; }
    double PeakLongitudinalTransferN() const noexcept
    {
        return PeakFrontTransferN;
    }
    double PeakCombinedLateralTransferN() const noexcept
    {
        return PeakLateralTransferN;
    }
    double SecondsWithWheelLiftRisk() const noexcept
    {
        return AccumulatedLiftSeconds;
    }
    int MinimumSupportedWheels() const noexcept
    {
        return MinSupportedWheels;
    }
private:
    LoadTransferParameters Settings{};
    WheelLoadDistribution Last{};
    std::uint64_t Frames = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t TipFrames = 0;
    double PeakFrontTransferN = 0.0;
    double PeakLateralTransferN = 0.0;
    double AccumulatedLiftSeconds = 0.0;
    double MaximumObservedLoadN = 0.0;
    int MinSupportedWheels = 4;
};
struct StaticCornerWeightReview final
{
    std::array<double, 4> StaticLoadN{};
    double FrontWeightFraction = 0.0;
    double LeftWeightFraction = 0.0;
    double CrossWeightFraction = 0.0;
    double TotalMassKg = 0.0;
    double StaticRollBalance = 0.0;
    bool Valid = false;
};
inline StaticCornerWeightReview EvaluateStaticCornerWeights(
    double vehicleMassKg,
    double frontBias,
    double leftBias = 0.5) noexcept
{
    StaticCornerWeightReview out;
    if (!Finite(vehicleMassKg) || vehicleMassKg < 200.0 ||
        !Finite(frontBias) || frontBias <= 0.0 ||
        frontBias >= 1.0 || !Finite(leftBias) ||
        leftBias <= 0.0 || leftBias >= 1.0) return out;
    const double totalN = vehicleMassKg *
        GravityCmS2 / CentimetersPerMeter;
    out.StaticLoadN = {{
        totalN * frontBias * leftBias,
        totalN * frontBias * (1.0 - leftBias),
        totalN * (1.0 - frontBias) * leftBias,
        totalN * (1.0 - frontBias) * (1.0 - leftBias)
    }};
    out.FrontWeightFraction = frontBias;
    out.LeftWeightFraction = leftBias;
    out.CrossWeightFraction =
        (out.StaticLoadN[0] + out.StaticLoadN[3]) / totalN;
    out.TotalMassKg = vehicleMassKg;
    out.StaticRollBalance = frontBias - 0.5;
    out.Valid = true;
    return out;
}
} // namespace latebrake::racecraft
