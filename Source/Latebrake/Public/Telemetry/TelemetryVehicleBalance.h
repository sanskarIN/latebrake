#pragma once

#include "Telemetry/TelemetryGrip.h"
#include "Telemetry/TelemetryCornering.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class HandlingBalance : std::uint8_t
{
    Unknown,
    Stable,
    FrontLimited,
    RearLimited,
    FourWheelLimited,
    LoadTransferCritical,
    UnstableTransient
};

struct BalanceObservation final
{
    double TimeSeconds = 0.0;
    double SpeedCmS = 0.0;
    double Steering = 0.0;
    double YawRateRadiansSecond = 0.0;
    double LateralAccelerationG = 0.0;
    double LongitudinalAccelerationG = 0.0;
    double ChassisRollRadians = 0.0;
    double ChassisPitchRadians = 0.0;
    double CrossTrackErrorCm = 0.0;
    double GripScale = 1.0;
    double RoadWetness = 0.0;
    std::array<double, 4> TireSlipAnglesRadians{};
    std::array<double, 4> TireSlipRatios{};
    std::array<double, 4> WheelNormalLoadsN{};
    std::array<bool, 4> PhysicalWheelContact{};
    bool OriginalPhysicsReady = false;
    bool OnOriginalRoad = false;
    bool EngineQualified = false;
};

struct BalanceSettings final
{
    double MinimumValidSpeedCmS = 250.0;
    double MinimumSteering = 0.12;
    double MinimumCorneringG = 0.18;
    double UndersteerRatioThreshold = 1.30;
    double OversteerRatioThreshold = 1.35;
    double MaximumAxleLoadDeltaFraction = 0.55;
    double MaximumRollRadians = 0.60;
    double MaximumPitchRadians = 0.50;
    double CriticalSlipRadians = 0.30;
    double CriticalSlipRatio = 0.45;
    double MinimumContactLoadN = 25.0;
    double StableSecondsRequired = 0.25;
    double MaximumSampleGapSeconds = 0.15;
    bool RequireFourWheelContact = true;
    bool IgnoreOffRoadFrames = true;
};

inline bool ValidBalanceSettings(
    BalanceSettings s) noexcept
{
    return Finite(s.MinimumValidSpeedCmS) &&
        s.MinimumValidSpeedCmS >= 0.0 &&
        Finite(s.MinimumSteering) &&
        s.MinimumSteering > 0.0 &&
        s.MinimumSteering <= 1.0 &&
        Finite(s.MinimumCorneringG) &&
        s.MinimumCorneringG > 0.0 &&
        Finite(s.UndersteerRatioThreshold) &&
        s.UndersteerRatioThreshold > 1.0 &&
        Finite(s.OversteerRatioThreshold) &&
        s.OversteerRatioThreshold > 1.0 &&
        Finite(s.MaximumAxleLoadDeltaFraction) &&
        s.MaximumAxleLoadDeltaFraction > 0.0 &&
        Finite(s.MaximumRollRadians) &&
        s.MaximumRollRadians > 0.0 &&
        Finite(s.MaximumPitchRadians) &&
        s.MaximumPitchRadians > 0.0 &&
        Finite(s.CriticalSlipRadians) &&
        s.CriticalSlipRadians > 0.0 &&
        Finite(s.CriticalSlipRatio) &&
        s.CriticalSlipRatio > 0.0 &&
        Finite(s.MinimumContactLoadN) &&
        s.MinimumContactLoadN > 0.0 &&
        Finite(s.StableSecondsRequired) &&
        s.StableSecondsRequired > 0.0 &&
        Finite(s.MaximumSampleGapSeconds) &&
        s.MaximumSampleGapSeconds > 0.0;
}

struct AxleBalanceReview final
{
    double MeanSlipRadians = 0.0;
    double MeanSlipRatio = 0.0;
    double MaximumSlipRadians = 0.0;
    double TotalNormalLoadN = 0.0;
    double LeftRightLoadDifferenceN = 0.0;
    double ContactFraction = 0.0;
    double LoadFraction = 0.0;
    double NormalizedLateralSaturation = 0.0;
    double NormalizedLongitudinalSaturation = 0.0;
    double CombinedSaturation = 0.0;
    bool Saturated = false;
    bool PhysicallySupported = false;
    bool Valid = false;
};

struct HandlingBalanceReview final
{
    AxleBalanceReview Front{};
    AxleBalanceReview Rear{};
    HandlingBalance Classification = HandlingBalance::Unknown;
    double SpeedCmS = 0.0;
    double LateralAccelerationG = 0.0;
    double LongitudinalAccelerationG = 0.0;
    double MeanFrontSlipRadians = 0.0;
    double MeanRearSlipRadians = 0.0;
    double SlipBalanceRatio = 1.0;
    double FrontLoadFraction = 0.0;
    double RearLoadFraction = 0.0;
    double LateralLoadTransferFraction = 0.0;
    double LongitudinalLoadTransferFraction = 0.0;
    double YawResponseToSteering = 0.0;
    double ExpectedYawResponse = 0.0;
    double YawResponseError = 0.0;
    double RollDemandFraction = 0.0;
    double PitchDemandFraction = 0.0;
    double TractionUseEstimate = 0.0;
    double Confidence = 0.0;
    bool FrontLimited = false;
    bool RearLimited = false;
    bool BothAxlesLimited = false;
    bool LoadTransferCritical = false;
    bool EngineObserved = false;
    bool AdvisoryOnly = true;
    bool Valid = false;
};

inline AxleBalanceReview EvaluateAxleBalance(
    BalanceObservation sample,
    int firstWheel,
    BalanceSettings settings) noexcept
{
    AxleBalanceReview review;
    if (firstWheel != 0 && firstWheel != 2)
    {
        return review;
    }
    const std::size_t left =
        static_cast<std::size_t>(firstWheel);
    const std::size_t right = left + 1;
    const bool l = sample.PhysicalWheelContact[left];
    const bool r = sample.PhysicalWheelContact[right];
    const double leftLoad =
        l ? sample.WheelNormalLoadsN[left] : 0.0;
    const double rightLoad =
        r ? sample.WheelNormalLoadsN[right] : 0.0;
    review.TotalNormalLoadN =
        leftLoad + rightLoad;
    review.ContactFraction =
        0.5 * (static_cast<double>(l) +
               static_cast<double>(r));
    review.LeftRightLoadDifferenceN =
        leftLoad - rightLoad;
    review.MeanSlipRadians = 0.5 * (
        std::abs(sample.TireSlipAnglesRadians[left]) +
        std::abs(sample.TireSlipAnglesRadians[right]));
    review.MeanSlipRatio = 0.5 * (
        std::abs(sample.TireSlipRatios[left]) +
        std::abs(sample.TireSlipRatios[right]));
    review.MaximumSlipRadians = std::max(
        std::abs(sample.TireSlipAnglesRadians[left]),
        std::abs(sample.TireSlipAnglesRadians[right]));
    review.NormalizedLateralSaturation = SafeRatio(
        review.MeanSlipRadians,
        settings.CriticalSlipRadians);
    review.NormalizedLongitudinalSaturation = SafeRatio(
        review.MeanSlipRatio,
        settings.CriticalSlipRatio);
    review.CombinedSaturation = std::hypot(
        review.NormalizedLateralSaturation,
        review.NormalizedLongitudinalSaturation);
    review.Saturated =
        review.CombinedSaturation >= 1.0;
    review.PhysicallySupported =
        review.TotalNormalLoadN >=
            settings.MinimumContactLoadN &&
        (!settings.RequireFourWheelContact ||
         review.ContactFraction == 1.0);
    review.Valid = sample.EngineQualified &&
        sample.OriginalPhysicsReady &&
        review.PhysicallySupported;
    return review;
}

inline HandlingBalanceReview EvaluateHandlingBalance(
    BalanceObservation sample,
    BalanceSettings settings = {}) noexcept
{
    HandlingBalanceReview review;
    if (!ValidBalanceSettings(settings) ||
        !sample.EngineQualified ||
        !sample.OriginalPhysicsReady ||
        !Finite(sample.TimeSeconds) ||
        !Finite(sample.SpeedCmS) ||
        !Finite(sample.Steering) ||
        !Finite(sample.YawRateRadiansSecond) ||
        !Finite(sample.LateralAccelerationG) ||
        !Finite(sample.LongitudinalAccelerationG) ||
        !Finite(sample.ChassisRollRadians) ||
        !Finite(sample.ChassisPitchRadians) ||
        !Finite(sample.GripScale) ||
        sample.SpeedCmS < settings.MinimumValidSpeedCmS ||
        sample.SpeedCmS > MaximumPlanarSpeedCmS ||
        std::abs(sample.Steering) > 1.0 ||
        sample.GripScale < 0.0 ||
        sample.GripScale > 4.0 ||
        (settings.IgnoreOffRoadFrames &&
         !sample.OnOriginalRoad))
    {
        return review;
    }
    for (std::size_t i = 0; i < 4; ++i)
    {
        if (!Finite(sample.TireSlipAnglesRadians[i]) ||
            !Finite(sample.TireSlipRatios[i]) ||
            !Finite(sample.WheelNormalLoadsN[i]) ||
            sample.WheelNormalLoadsN[i] < 0.0 ||
            sample.WheelNormalLoadsN[i] > 250000.0)
        {
            return review;
        }
    }
    review.Front =
        EvaluateAxleBalance(sample, 0, settings);
    review.Rear =
        EvaluateAxleBalance(sample, 2, settings);
    if (!review.Front.Valid || !review.Rear.Valid)
    {
        return review;
    }
    const double totalLoad =
        review.Front.TotalNormalLoadN +
        review.Rear.TotalNormalLoadN;
    if (totalLoad <= 0.0)
    {
        return review;
    }
    review.Front.LoadFraction =
        review.Front.TotalNormalLoadN / totalLoad;
    review.Rear.LoadFraction =
        review.Rear.TotalNormalLoadN / totalLoad;
    review.SpeedCmS = sample.SpeedCmS;
    review.LateralAccelerationG =
        sample.LateralAccelerationG;
    review.LongitudinalAccelerationG =
        sample.LongitudinalAccelerationG;
    review.MeanFrontSlipRadians =
        review.Front.MeanSlipRadians;
    review.MeanRearSlipRadians =
        review.Rear.MeanSlipRadians;
    review.SlipBalanceRatio = SafeRatio(
        review.MeanFrontSlipRadians,
        std::max(0.01, review.MeanRearSlipRadians));
    review.FrontLoadFraction =
        review.Front.LoadFraction;
    review.RearLoadFraction =
        review.Rear.LoadFraction;
    review.LateralLoadTransferFraction =
        SafeRatio(
            review.Front.LeftRightLoadDifferenceN +
            review.Rear.LeftRightLoadDifferenceN,
            totalLoad);
    review.LongitudinalLoadTransferFraction =
        review.FrontLoadFraction -
        review.RearLoadFraction;
    review.YawResponseToSteering = SafeRatio(
        sample.YawRateRadiansSecond,
        std::max(settings.MinimumSteering,
                 std::abs(sample.Steering)));
    review.ExpectedYawResponse = SafeRatio(
        sample.LateralAccelerationG * GravityCmS2,
        std::max(settings.MinimumValidSpeedCmS,
                 sample.SpeedCmS));
    review.YawResponseError = std::abs(
        review.YawResponseToSteering -
        review.ExpectedYawResponse);
    review.RollDemandFraction = SafeRatio(
        std::abs(sample.ChassisRollRadians),
        settings.MaximumRollRadians);
    review.PitchDemandFraction = SafeRatio(
        std::abs(sample.ChassisPitchRadians),
        settings.MaximumPitchRadians);
    review.TractionUseEstimate = SafeRatio(
        std::hypot(sample.LateralAccelerationG,
                   sample.LongitudinalAccelerationG),
        std::max(0.1, sample.GripScale));
    review.FrontLimited =
        review.Front.CombinedSaturation >
        review.Rear.CombinedSaturation *
            settings.UndersteerRatioThreshold &&
        review.Front.CombinedSaturation >= 0.7;
    review.RearLimited =
        review.Rear.CombinedSaturation >
        review.Front.CombinedSaturation *
            settings.OversteerRatioThreshold &&
        review.Rear.CombinedSaturation >= 0.7;
    review.BothAxlesLimited =
        review.Front.Saturated &&
        review.Rear.Saturated;
    review.LoadTransferCritical =
        std::abs(review.LateralLoadTransferFraction) >=
            settings.MaximumAxleLoadDeltaFraction ||
        review.RollDemandFraction >= 1.0 ||
        review.PitchDemandFraction >= 1.0;
    review.Confidence =
        Clamp01(0.55 +
            0.15 * review.Front.ContactFraction +
            0.15 * review.Rear.ContactFraction +
            (sample.OnOriginalRoad ? 0.15 : 0.0));
    if (review.LoadTransferCritical)
    {
        review.Classification =
            HandlingBalance::LoadTransferCritical;
    }
    else if (review.BothAxlesLimited)
    {
        review.Classification =
            HandlingBalance::FourWheelLimited;
    }
    else if (review.FrontLimited)
    {
        review.Classification =
            HandlingBalance::FrontLimited;
    }
    else if (review.RearLimited)
    {
        review.Classification =
            HandlingBalance::RearLimited;
    }
    else
    {
        review.Classification =
            HandlingBalance::Stable;
    }
    review.EngineObserved = true;
    review.Valid = true;
    return review;
}

struct HandlingTrendReport final
{
    std::array<std::uint64_t, 7> ConditionFrames{};
    double FrontLimitedSeconds = 0.0;
    double RearLimitedSeconds = 0.0;
    double BothLimitedSeconds = 0.0;
    double LoadTransferCriticalSeconds = 0.0;
    double StableSeconds = 0.0;
    double MeanFrontSlipRadians = 0.0;
    double MeanRearSlipRadians = 0.0;
    double MeanLateralTransfer = 0.0;
    double MeanLongitudinalTransfer = 0.0;
    double MaximumTractionUtilization = 0.0;
    double MaximumRollDemand = 0.0;
    double MaximumPitchDemand = 0.0;
    double TotalSeconds = 0.0;
    std::uint64_t AcceptedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    bool Valid = false;
};

class VehicleBalanceMonitor final
{
public:
    bool Configure(BalanceSettings settings) noexcept
    {
        if (!ValidBalanceSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Last = {};
        Trend = {};
        FrontSlipStats.Reset();
        RearSlipStats.Reset();
        LateralLoadStats.Reset();
        LongitudinalLoadStats.Reset();
        Classified = {};
    }

    bool Observe(
        BalanceObservation observation, double dt) noexcept
    {
        if (!Finite(dt) || dt <= 0.0 ||
            dt > settingsMaximumFrameSeconds)
        {
            ++Trend.RejectedFrames;
            return false;
        }
        const HandlingBalanceReview review =
            EvaluateHandlingBalance(observation, Settings);
        if (!review.Valid)
        {
            ++Trend.RejectedFrames;
            return false;
        }
        Last = review;
        ++Trend.AcceptedFrames;
        Trend.TotalSeconds += dt;
        const std::size_t index =
            static_cast<std::size_t>(review.Classification);
        if (index < Trend.ConditionFrames.size())
        {
            ++Trend.ConditionFrames[index];
        }
        if (review.FrontLimited)
        {
            Trend.FrontLimitedSeconds += dt;
        }
        if (review.RearLimited)
        {
            Trend.RearLimitedSeconds += dt;
        }
        if (review.BothAxlesLimited)
        {
            Trend.BothLimitedSeconds += dt;
        }
        if (review.LoadTransferCritical)
        {
            Trend.LoadTransferCriticalSeconds += dt;
        }
        if (review.Classification ==
            HandlingBalance::Stable)
        {
            Trend.StableSeconds += dt;
        }
        FrontSlipStats.Observe(
            review.MeanFrontSlipRadians);
        RearSlipStats.Observe(
            review.MeanRearSlipRadians);
        LateralLoadStats.Observe(
            review.LateralLoadTransferFraction);
        LongitudinalLoadStats.Observe(
            review.LongitudinalLoadTransferFraction);
        Trend.MeanFrontSlipRadians =
            FrontSlipStats.Mean();
        Trend.MeanRearSlipRadians =
            RearSlipStats.Mean();
        Trend.MeanLateralTransfer =
            LateralLoadStats.Mean();
        Trend.MeanLongitudinalTransfer =
            LongitudinalLoadStats.Mean();
        Trend.MaximumTractionUtilization = std::max(
            Trend.MaximumTractionUtilization,
            review.TractionUseEstimate);
        Trend.MaximumRollDemand = std::max(
            Trend.MaximumRollDemand,
            review.RollDemandFraction);
        Trend.MaximumPitchDemand = std::max(
            Trend.MaximumPitchDemand,
            review.PitchDemandFraction);
        Trend.Valid = true;
        return true;
    }

    HandlingBalanceReview Snapshot() const noexcept
    {
        return Last;
    }

    HandlingTrendReport History() const noexcept
    {
        return Trend;
    }

private:
    static constexpr double settingsMaximumFrameSeconds = 0.15;
    BalanceSettings Settings{};
    HandlingBalanceReview Last{};
    HandlingTrendReport Trend{};
    RunningStatistics FrontSlipStats;
    RunningStatistics RearSlipStats;
    RunningStatistics LateralLoadStats;
    RunningStatistics LongitudinalLoadStats;
    HysteresisDetector Classified;
};
} // namespace latebrake::engineering
