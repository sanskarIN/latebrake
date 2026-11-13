#pragma once

#include "Telemetry/TelemetryKinematics.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace latebrake::engineering
{
enum class TireLoadCondition : std::uint8_t
{
    Unknown,
    Unloaded,
    Light,
    Balanced,
    Heavy,
    Overloaded
};

enum class GripState : std::uint8_t
{
    Unobserved,
    Stable,
    ApproachingLimit,
    Saturated,
    Sliding,
    AquaplaningSuspected
};

struct GripAnalysisSettings final
{
    double EnterLateralG = 0.58;
    double EnterSlipRatio = 0.18;
    double ExitSlipRatio = 0.09;
    double EnterSlipAngleRad = 0.18;
    double ExitSlipAngleRad = 0.09;
    double CriticalSlipRatio = 0.80;
    double CriticalSlipAngleRad = 0.45;
    double MinimumSpeedCmS = 250.0;
    double MinimumContactLoadN = 30.0;
    double NominalCorneringG = 0.92;
    double MinimumSensorConfidence = 0.55;
    double TireColdThresholdC = 35.0;
    double TireWarmThresholdC = 65.0;
    double TireHotThresholdC = 115.0;
    double TireCriticalThresholdC = 150.0;
    double MaximumWetness = 1.0;
    double MaximumWear = 1.0;
    double EventCooldownSeconds = 0.12;
};

inline bool ValidGripSettings(const GripAnalysisSettings& s) noexcept
{
    return Finite(s.EnterLateralG) && s.EnterLateralG >= 0.0 &&
           Finite(s.EnterSlipRatio) &&
           s.EnterSlipRatio > s.ExitSlipRatio &&
           Finite(s.ExitSlipRatio) && s.ExitSlipRatio >= 0.0 &&
           Finite(s.EnterSlipAngleRad) &&
           s.EnterSlipAngleRad > s.ExitSlipAngleRad &&
           Finite(s.ExitSlipAngleRad) && s.ExitSlipAngleRad >= 0.0 &&
           Finite(s.CriticalSlipRatio) &&
           s.CriticalSlipRatio > s.EnterSlipRatio &&
           Finite(s.CriticalSlipAngleRad) &&
           s.CriticalSlipAngleRad > s.EnterSlipAngleRad &&
           Finite(s.MinimumSpeedCmS) && s.MinimumSpeedCmS >= 0.0 &&
           Finite(s.MinimumContactLoadN) &&
           s.MinimumContactLoadN >= 0.0 &&
           Finite(s.NominalCorneringG) && s.NominalCorneringG > 0.0 &&
           Finite(s.MinimumSensorConfidence) &&
           s.MinimumSensorConfidence >= 0.0 &&
           s.MinimumSensorConfidence <= 1.0 &&
           Finite(s.TireColdThresholdC) &&
           Finite(s.TireWarmThresholdC) &&
           Finite(s.TireHotThresholdC) &&
           Finite(s.TireCriticalThresholdC) &&
           s.TireWarmThresholdC > s.TireColdThresholdC &&
           s.TireHotThresholdC > s.TireWarmThresholdC &&
           s.TireCriticalThresholdC > s.TireHotThresholdC &&
           Finite(s.MaximumWetness) &&
           s.MaximumWetness > 0.0 &&
           Finite(s.MaximumWear) &&
           s.MaximumWear > 0.0 &&
           Finite(s.EventCooldownSeconds) &&
           s.EventCooldownSeconds >= 0.0;
}

struct TireGripObservation final
{
    double AngularSpeedRadS = 0.0;
    double NormalLoadNewtons = 0.0;
    double LoadFraction = 0.0;
    double SlipRatio = 0.0;
    double SlipAngleRadians = 0.0;
    double TireTemperatureC = 0.0;
    double TireWear = 0.0;
    double TemperatureGripFactor = 0.0;
    double WearGripFactor = 0.0;
    double WetnessGripFactor = 0.0;
    double CompositeGripFactor = 0.0;
    double LongitudinalSaturation = 0.0;
    double LateralSaturation = 0.0;
    double CombinedSaturation = 0.0;
    TireLoadCondition LoadCondition = TireLoadCondition::Unknown;
    bool Contact = false;
    bool Locking = false;
    bool Wheelspin = false;
    bool Hot = false;
    bool Critical = false;
    bool Qualified = false;
};

struct GripTelemetry final
{
    GripState State = GripState::Unobserved;
    std::array<TireGripObservation, 4> Tires{};
    double SpeedCmS = 0.0;
    double SmoothedLateralG = 0.0;
    double ObservedLongitudinalG = 0.0;
    double ObservedLateralG = 0.0;
    double EstimatedRequiredFriction = 0.0;
    double LoadWeightedGripFactor = 0.0;
    double MaximumCombinedSaturation = 0.0;
    double MeanCombinedSaturation = 0.0;
    double MeanTireTemperatureC = 0.0;
    double MeanWearFraction = 0.0;
    double TotalContactLoadNewtons = 0.0;
    double ContactFraction = 0.0;
    double FrontLoadFraction = 0.0;
    double RearLoadFraction = 0.0;
    double LateralLoadDifferenceFraction = 0.0;
    double TractionUtilizationFraction = 0.0;
    double EstimatedGripG = 0.0;
    double WetnessPenalty = 0.0;
    double HeatPenalty = 0.0;
    double WearPenalty = 0.0;
    double Confidence = 0.0;
    bool RearAxleSaturated = false;
    bool FrontAxleSaturated = false;
    bool SuspectedUndersteer = false;
    bool SuspectedOversteer = false;
    bool InTractionLimit = false;
    bool Valid = false;
};

inline double GripTemperatureFactor(
    double temperatureC,
    const GripAnalysisSettings& settings) noexcept
{
    if (!Finite(temperatureC))
    {
        return 0.0;
    }
    if (temperatureC <= settings.TireColdThresholdC)
    {
        return std::clamp(0.55 +
            (temperatureC + 20.0) * 0.004, 0.2, 0.78);
    }
    if (temperatureC < settings.TireWarmThresholdC)
    {
        return Lerp(0.78, 1.0,
            SafeRatio(temperatureC - settings.TireColdThresholdC,
                      settings.TireWarmThresholdC -
                      settings.TireColdThresholdC));
    }
    if (temperatureC <= settings.TireHotThresholdC)
    {
        return 1.0;
    }
    return std::clamp(
        1.0 - SafeRatio(
            temperatureC - settings.TireHotThresholdC,
            settings.TireCriticalThresholdC -
            settings.TireHotThresholdC) * 0.45, 0.3, 1.0);
}

inline double GripWearFactor(
    double wearFraction, const GripAnalysisSettings& settings) noexcept
{
    if (!Finite(wearFraction))
    {
        return 0.0;
    }
    const double wear = Clamp01(
        wearFraction / settings.MaximumWear);
    return std::clamp(1.0 - 0.52 * wear * wear -
        0.12 * wear, 0.25, 1.0);
}

inline double GripWetnessFactor(
    double wetness, const GripAnalysisSettings& settings) noexcept
{
    if (!Finite(wetness))
    {
        return 0.0;
    }
    const double fraction = Clamp01(
        wetness / settings.MaximumWetness);
    return std::clamp(
        1.0 - 0.56 * fraction * fraction - 0.18 * fraction,
        0.20, 1.0);
}

inline TireLoadCondition ClassifyTireLoad(
    double fraction, bool contact) noexcept
{
    if (!contact || !Finite(fraction))
    {
        return TireLoadCondition::Unloaded;
    }
    if (fraction < 0.08)
    {
        return TireLoadCondition::Light;
    }
    if (fraction < 0.17)
    {
        return TireLoadCondition::Balanced;
    }
    if (fraction <= 0.33)
    {
        return TireLoadCondition::Heavy;
    }
    return TireLoadCondition::Overloaded;
}

inline GripTelemetry EvaluateGripTelemetry(
    const TelemetryFrame& frame,
    const TelemetryKinematics& motion,
    const GripAnalysisSettings& settings) noexcept
{
    GripTelemetry result;
    if (!ValidGripSettings(settings) || !motion.Valid ||
        !ValidTelemetryWheels(frame.Wheels) ||
        !Finite(frame.Environment.AmbientWetness) ||
        !Finite(frame.Environment.RoadFrictionEstimate) ||
        !Finite(frame.Dynamics.GripScale))
    {
        return result;
    }
    const WheelMotionEstimate wheels =
        MeasureWheelMotion(frame.Wheels);
    if (!wheels.Qualified || wheels.ContactWheelCount <= 0.0)
    {
        return result;
    }
    const double totalLoad = wheels.TotalNormalLoadNewtons;
    if (totalLoad < settings.MinimumContactLoadN)
    {
        return result;
    }
    double compositeSum = 0.0;
    double saturationSum = 0.0;
    double temperatureSum = 0.0;
    double wearSum = 0.0;
    double frontLoad = 0.0;
    double rearLoad = 0.0;
    double leftLoad = 0.0;
    double rightLoad = 0.0;
    double frontSaturation = 0.0;
    double rearSaturation = 0.0;
    int contactCount = 0;
    const double wetFactor =
        GripWetnessFactor(frame.Environment.AmbientWetness, settings);
    const double roadFriction = std::clamp(
        frame.Environment.RoadFrictionEstimate, 0.0, 2.0);
    const double physicsGrip = std::clamp(
        frame.Dynamics.GripScale, 0.0, 2.0);
    for (int i = 0; i < 4; ++i)
    {
        const std::size_t n = static_cast<std::size_t>(i);
        TireGripObservation tire;
        tire.Contact = frame.Wheels.Contact[n];
        tire.AngularSpeedRadS = frame.Wheels.AngularSpeedRadS[n];
        tire.NormalLoadNewtons = frame.Wheels.NormalLoadNewtons[n];
        tire.SlipRatio = frame.Wheels.SlipRatio[n];
        tire.SlipAngleRadians = frame.Wheels.SlipAngleRadians[n];
        tire.TireTemperatureC = frame.Wheels.TireTemperatureCelsius[n];
        tire.TireWear = frame.Wheels.TireWearFraction[n];
        if (tire.Contact && tire.NormalLoadNewtons >=
            settings.MinimumContactLoadN / 4.0)
        {
            ++contactCount;
            tire.LoadFraction =
                SafeRatio(tire.NormalLoadNewtons, totalLoad);
            tire.LoadCondition =
                ClassifyTireLoad(tire.LoadFraction, true);
            tire.TemperatureGripFactor =
                GripTemperatureFactor(tire.TireTemperatureC, settings);
            tire.WearGripFactor =
                GripWearFactor(tire.TireWear, settings);
            tire.WetnessGripFactor = wetFactor;
            tire.CompositeGripFactor =
                tire.TemperatureGripFactor *
                tire.WearGripFactor *
                tire.WetnessGripFactor *
                roadFriction * physicsGrip;
            tire.LongitudinalSaturation = Clamp01(
                std::abs(tire.SlipRatio) /
                settings.CriticalSlipRatio);
            tire.LateralSaturation = Clamp01(
                std::abs(tire.SlipAngleRadians) /
                settings.CriticalSlipAngleRad);
            tire.CombinedSaturation = std::hypot(
                tire.LongitudinalSaturation,
                tire.LateralSaturation);
            tire.Wheelspin = tire.SlipRatio >=
                settings.EnterSlipRatio;
            tire.Locking = tire.SlipRatio <=
                -settings.EnterSlipRatio;
            tire.Hot = tire.TireTemperatureC >=
                settings.TireHotThresholdC;
            tire.Critical = tire.TireTemperatureC >=
                settings.TireCriticalThresholdC;
            tire.Qualified = Finite(tire.CompositeGripFactor);
            const double load = tire.NormalLoadNewtons;
            compositeSum += tire.CompositeGripFactor * load;
            saturationSum += tire.CombinedSaturation;
            temperatureSum += tire.TireTemperatureC;
            wearSum += tire.TireWear;
            result.MaximumCombinedSaturation = std::max(
                result.MaximumCombinedSaturation,
                tire.CombinedSaturation);
            if (i < 2)
            {
                frontLoad += load;
                frontSaturation = std::max(
                    frontSaturation, tire.CombinedSaturation);
            }
            else
            {
                rearLoad += load;
                rearSaturation = std::max(
                    rearSaturation, tire.CombinedSaturation);
            }
            if (i % 2 == 0)
            {
                leftLoad += load;
            }
            else
            {
                rightLoad += load;
            }
        }
        result.Tires[n] = tire;
    }
    if (contactCount < 1)
    {
        return {};
    }
    const double count = static_cast<double>(contactCount);
    result.SpeedCmS = motion.SpeedCmS;
    result.ObservedLongitudinalG =
        motion.LongitudinalAccelerationCmS2 / GravityCmS2;
    result.ObservedLateralG =
        motion.LateralAccelerationCmS2 / GravityCmS2;
    result.EstimatedRequiredFriction = std::hypot(
        result.ObservedLongitudinalG,
        result.ObservedLateralG);
    result.TotalContactLoadNewtons = totalLoad;
    result.ContactFraction = count / 4.0;
    result.LoadWeightedGripFactor =
        SafeRatio(compositeSum, totalLoad);
    result.MeanCombinedSaturation = saturationSum / count;
    result.MeanTireTemperatureC = temperatureSum / count;
    result.MeanWearFraction = wearSum / count;
    result.FrontLoadFraction = SafeRatio(frontLoad, totalLoad);
    result.RearLoadFraction = SafeRatio(rearLoad, totalLoad);
    result.LateralLoadDifferenceFraction =
        SafeRatio(leftLoad - rightLoad, totalLoad);
    result.EstimatedGripG =
        settings.NominalCorneringG * result.LoadWeightedGripFactor;
    result.TractionUtilizationFraction = SafeRatio(
        result.EstimatedRequiredFriction,
        std::max(0.05, result.EstimatedGripG));
    result.WetnessPenalty = 1.0 - wetFactor;
    result.HeatPenalty = Clamp01(
        (result.MeanTireTemperatureC -
         settings.TireHotThresholdC) /
        std::max(1.0, settings.TireCriticalThresholdC -
                      settings.TireHotThresholdC));
    result.WearPenalty =
        1.0 - GripWearFactor(result.MeanWearFraction, settings);
    result.FrontAxleSaturated = frontSaturation >= 0.95;
    result.RearAxleSaturated = rearSaturation >= 0.95;
    result.SuspectedUndersteer =
        result.FrontAxleSaturated && !result.RearAxleSaturated &&
        motion.SpeedCmS >= settings.MinimumSpeedCmS;
    result.SuspectedOversteer =
        result.RearAxleSaturated && !result.FrontAxleSaturated &&
        motion.SpeedCmS >= settings.MinimumSpeedCmS;
    result.InTractionLimit =
        result.MaximumCombinedSaturation >= 0.90 ||
        result.TractionUtilizationFraction >= 1.0;
    result.Confidence = std::clamp(
        result.ContactFraction * 0.70 +
        (wheels.AllWheelsContact ? 0.30 : 0.10),
        0.0, 1.0);
    if (result.SpeedCmS < settings.MinimumSpeedCmS)
    {
        result.State = GripState::Stable;
    }
    else if (result.MaximumCombinedSaturation >= 1.30)
    {
        result.State = GripState::Sliding;
    }
    else if (result.MaximumCombinedSaturation >= 1.0)
    {
        result.State = GripState::Saturated;
    }
    else if (result.InTractionLimit ||
             result.TractionUtilizationFraction >= 0.85)
    {
        result.State = GripState::ApproachingLimit;
    }
    else
    {
        result.State = GripState::Stable;
    }
    if (frame.Environment.AmbientWetness > 0.85 &&
        result.MaximumCombinedSaturation > 1.25 &&
        result.ContactFraction <= 0.5)
    {
        result.State = GripState::AquaplaningSuspected;
    }
    result.Valid = result.Confidence >=
        settings.MinimumSensorConfidence &&
        Finite(result.EstimatedGripG) &&
        Finite(result.TractionUtilizationFraction);
    return result;
}

struct GripEventSummary final
{
    std::uint64_t QualifiedFrames = 0;
    std::uint64_t SaturatedFrames = 0;
    std::uint64_t SlidingFrames = 0;
    std::uint64_t UndersteerProxyFrames = 0;
    std::uint64_t OversteerProxyFrames = 0;
    std::uint64_t CriticalHeatFrames = 0;
    std::uint64_t TireLockFrames = 0;
    std::uint64_t WheelspinFrames = 0;
    double TotalObservedSeconds = 0.0;
    double SaturatedSeconds = 0.0;
    double SlidingSeconds = 0.0;
    double UndersteerSeconds = 0.0;
    double OversteerSeconds = 0.0;
    double CriticalHeatSeconds = 0.0;
    double MaximumDemandG = 0.0;
    double MaximumSaturation = 0.0;
    double MeanEstimatedGripG = 0.0;
    double MeanTireTemperatureC = 0.0;
    double MeanWear = 0.0;
    bool Valid = false;
};

class GripTelemetryMonitor final
{
public:
    bool Configure(GripAnalysisSettings settings) noexcept
    {
        if (!ValidGripSettings(settings))
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
        Summary = {};
        LateralSmooth.Reset();
        GripStats.Reset();
        TemperatureStats.Reset();
        WearStats.Reset();
        SaturationEpisodes = 0;
        SlidingEpisodes = 0;
        PreviousSaturated = false;
        PreviousSliding = false;
    }

    void Suspend() noexcept
    {
        Last = {};
        LateralSmooth.Reset();
        PreviousSaturated = false;
        PreviousSliding = false;
    }

    bool Observe(const TelemetryFrame& frame,
                 const TelemetryKinematics& motion,
                 double dt) noexcept
    {
        if (!Finite(dt) || dt <= 0.0 ||
            dt > MaximumFrameDeltaSeconds)
        {
            Suspend();
            return false;
        }
        GripTelemetry estimate =
            EvaluateGripTelemetry(frame, motion, Settings);
        if (!estimate.Valid)
        {
            Suspend();
            return false;
        }
        estimate.SmoothedLateralG = LateralSmooth.Observe(
            estimate.ObservedLateralG, dt, 0.15);
        const bool saturated =
            estimate.State == GripState::Saturated ||
            estimate.State == GripState::Sliding ||
            estimate.State == GripState::AquaplaningSuspected;
        const bool sliding =
            estimate.State == GripState::Sliding ||
            estimate.State == GripState::AquaplaningSuspected;
        bool hot = false;
        bool locking = false;
        bool wheelspin = false;
        for (const TireGripObservation& tire : estimate.Tires)
        {
            hot = hot || (tire.Qualified && tire.Critical);
            locking = locking || (tire.Qualified && tire.Locking);
            wheelspin = wheelspin || (tire.Qualified && tire.Wheelspin);
        }
        if (saturated && !PreviousSaturated)
        {
            SaturationEpisodes =
                SaturatingIncrement(SaturationEpisodes);
        }
        if (sliding && !PreviousSliding)
        {
            SlidingEpisodes =
                SaturatingIncrement(SlidingEpisodes);
        }
        PreviousSaturated = saturated;
        PreviousSliding = sliding;
        Summary.QualifiedFrames =
            SaturatingIncrement(Summary.QualifiedFrames);
        Summary.TotalObservedSeconds += dt;
        Summary.MaximumDemandG = std::max(
            Summary.MaximumDemandG, estimate.EstimatedRequiredFriction);
        Summary.MaximumSaturation = std::max(
            Summary.MaximumSaturation, estimate.MaximumCombinedSaturation);
        if (saturated)
        {
            Summary.SaturatedFrames =
                SaturatingIncrement(Summary.SaturatedFrames);
            Summary.SaturatedSeconds += dt;
        }
        if (sliding)
        {
            Summary.SlidingFrames =
                SaturatingIncrement(Summary.SlidingFrames);
            Summary.SlidingSeconds += dt;
        }
        if (estimate.SuspectedUndersteer)
        {
            Summary.UndersteerProxyFrames =
                SaturatingIncrement(Summary.UndersteerProxyFrames);
            Summary.UndersteerSeconds += dt;
        }
        if (estimate.SuspectedOversteer)
        {
            Summary.OversteerProxyFrames =
                SaturatingIncrement(Summary.OversteerProxyFrames);
            Summary.OversteerSeconds += dt;
        }
        if (hot)
        {
            Summary.CriticalHeatFrames =
                SaturatingIncrement(Summary.CriticalHeatFrames);
            Summary.CriticalHeatSeconds += dt;
        }
        if (locking)
        {
            Summary.TireLockFrames =
                SaturatingIncrement(Summary.TireLockFrames);
        }
        if (wheelspin)
        {
            Summary.WheelspinFrames =
                SaturatingIncrement(Summary.WheelspinFrames);
        }
        GripStats.Observe(estimate.EstimatedGripG);
        TemperatureStats.Observe(estimate.MeanTireTemperatureC);
        WearStats.Observe(estimate.MeanWearFraction);
        Summary.MeanEstimatedGripG = GripStats.Mean();
        Summary.MeanTireTemperatureC = TemperatureStats.Mean();
        Summary.MeanWear = WearStats.Mean();
        Summary.Valid = Summary.QualifiedFrames > 0;
        Last = estimate;
        return true;
    }

    GripTelemetry Snapshot() const noexcept { return Last; }
    GripEventSummary History() const noexcept { return Summary; }
    std::uint64_t SaturationEventCount() const noexcept
    {
        return SaturationEpisodes;
    }
    std::uint64_t SlidingEventCount() const noexcept
    {
        return SlidingEpisodes;
    }
    double FractionOfTimeAtGripLimit() const noexcept
    {
        return SafeRatio(
            Summary.SaturatedSeconds, Summary.TotalObservedSeconds);
    }
    double FractionOfTimeInSlide() const noexcept
    {
        return SafeRatio(
            Summary.SlidingSeconds, Summary.TotalObservedSeconds);
    }

private:
    GripAnalysisSettings Settings{};
    GripTelemetry Last{};
    GripEventSummary Summary{};
    ExponentialFilter LateralSmooth;
    RunningStatistics GripStats;
    RunningStatistics TemperatureStats;
    RunningStatistics WearStats;
    std::uint64_t SaturationEpisodes = 0;
    std::uint64_t SlidingEpisodes = 0;
    bool PreviousSaturated = false;
    bool PreviousSliding = false;
};
} // namespace latebrake::engineering
