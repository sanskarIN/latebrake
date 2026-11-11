#pragma once

#include "Telemetry/TelemetryBraking.h"
#include "Telemetry/TelemetryDistancePace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class BrakeTraceCondition : std::uint8_t
{
    Unknown,
    InitialAttack,
    PeakPressure,
    Modulation,
    Trail,
    Release,
    Coast
};

struct BrakeTraceSettings final
{
    double PedalAttackPerSecond = 1.5;
    double PedalReleasePerSecond = 1.5;
    double PressureModulationPerSecond = 1.0;
    double MinimumBrakePressure = 0.10;
    double StrongBrakePressure = 0.65;
    double TrailSteeringThreshold = 0.20;
    double MaximumThrottleOverlap = 0.12;
    double MinimumPhysicalDecelerationG = 0.05;
    double MaximumLegalDecelerationG = 3.0;
    double MinimumSegmentSeconds = 0.02;
    double MaximumFrameSeconds = 0.10;
    double HeatWarningC = 550.0;
    double HeatCriticalC = 800.0;
};

struct BrakeTraceSample final
{
    double TimeSeconds = 0.0;
    double TrackDistanceCm = 0.0;
    double SpeedCmS = 0.0;
    double BrakePressure = 0.0;
    double ThrottlePressure = 0.0;
    double SteeringMagnitude = 0.0;
    double LongitudinalDecelerationG = 0.0;
    double BrakeTemperatureC = 0.0;
    double WheelLockFraction = 0.0;
    double DeltaSeconds = 0.0;
    bool OnPavedRoad = false;
    bool EngineQualified = false;
};

struct BrakeTraceEvent final
{
    double StartSeconds = 0.0;
    double EndSeconds = 0.0;
    double StartSpeedCmS = 0.0;
    double EndSpeedCmS = 0.0;
    double StartTrackDistanceCm = 0.0;
    double EndTrackDistanceCm = 0.0;
    double PeakPressure = 0.0;
    double PeakDecelerationG = 0.0;
    double MeanDecelerationG = 0.0;
    double PressureIntegral = 0.0;
    double DecelerationIntegral = 0.0;
    double InitialAttackSeconds = 0.0;
    double FullPressureSeconds = 0.0;
    double ModulationSeconds = 0.0;
    double TrailSeconds = 0.0;
    double ReleaseSeconds = 0.0;
    double ThrottleOverlapSeconds = 0.0;
    double WheelLockSeconds = 0.0;
    double HeatWarningSeconds = 0.0;
    double HeatCriticalSeconds = 0.0;
    double TotalSeconds = 0.0;
    double DistanceCm = 0.0;
    double EstimatedLostKineticEnergyJPerKg = 0.0;
    double MaximumBrakeTemperatureC = 0.0;
    int PressureOscillations = 0;
    int QualifiedSamples = 0;
    bool EndedNormally = false;
    bool EngineQualified = false;
    bool Valid = false;
};

inline bool ValidBrakeTraceSettings(
    BrakeTraceSettings settings) noexcept
{
    return Finite(settings.PedalAttackPerSecond) &&
        Finite(settings.PedalReleasePerSecond) &&
        Finite(settings.PressureModulationPerSecond) &&
        settings.PedalAttackPerSecond > 0.0 &&
        settings.PedalReleasePerSecond > 0.0 &&
        settings.PressureModulationPerSecond > 0.0 &&
        Finite(settings.MinimumBrakePressure) &&
        Finite(settings.StrongBrakePressure) &&
        settings.MinimumBrakePressure > 0.0 &&
        settings.StrongBrakePressure >
            settings.MinimumBrakePressure &&
        settings.StrongBrakePressure <= 1.0 &&
        Finite(settings.TrailSteeringThreshold) &&
        settings.TrailSteeringThreshold >= 0.0 &&
        settings.TrailSteeringThreshold <= 1.0 &&
        Finite(settings.MaximumThrottleOverlap) &&
        settings.MaximumThrottleOverlap >= 0.0 &&
        Finite(settings.MinimumPhysicalDecelerationG) &&
        settings.MinimumPhysicalDecelerationG >= 0.0 &&
        Finite(settings.MaximumLegalDecelerationG) &&
        settings.MaximumLegalDecelerationG >
            settings.MinimumPhysicalDecelerationG &&
        Finite(settings.MinimumSegmentSeconds) &&
        settings.MinimumSegmentSeconds > 0.0 &&
        Finite(settings.MaximumFrameSeconds) &&
        settings.MaximumFrameSeconds >
            settings.MinimumSegmentSeconds &&
        Finite(settings.HeatWarningC) &&
        Finite(settings.HeatCriticalC) &&
        settings.HeatCriticalC > settings.HeatWarningC;
}

inline bool ValidBrakeTraceSample(
    BrakeTraceSample sample,
    const BrakeTraceSettings& settings) noexcept
{
    return sample.EngineQualified &&
        Finite(sample.TimeSeconds) &&
        Finite(sample.TrackDistanceCm) &&
        Finite(sample.SpeedCmS) &&
        Finite(sample.BrakePressure) &&
        Finite(sample.ThrottlePressure) &&
        Finite(sample.SteeringMagnitude) &&
        Finite(sample.LongitudinalDecelerationG) &&
        Finite(sample.BrakeTemperatureC) &&
        Finite(sample.WheelLockFraction) &&
        Finite(sample.DeltaSeconds) &&
        sample.TimeSeconds >= 0.0 &&
        sample.TrackDistanceCm >= 0.0 &&
        sample.SpeedCmS >= 0.0 &&
        sample.SpeedCmS <= MaximumPlanarSpeedCmS &&
        sample.BrakePressure >= 0.0 &&
        sample.BrakePressure <= 1.0 &&
        sample.ThrottlePressure >= 0.0 &&
        sample.ThrottlePressure <= 1.0 &&
        sample.SteeringMagnitude >= 0.0 &&
        sample.SteeringMagnitude <= 1.0 &&
        sample.LongitudinalDecelerationG >= 0.0 &&
        sample.LongitudinalDecelerationG <=
            settings.MaximumLegalDecelerationG &&
        sample.WheelLockFraction >= 0.0 &&
        sample.WheelLockFraction <= 1.0 &&
        sample.DeltaSeconds > 0.0 &&
        sample.DeltaSeconds <= settings.MaximumFrameSeconds;
}

inline BrakeTraceCondition ClassifyBrakeTrace(
    BrakeTraceSample previous,
    BrakeTraceSample current,
    const BrakeTraceSettings& settings) noexcept
{
    if (!ValidBrakeTraceSample(current, settings))
    {
        return BrakeTraceCondition::Unknown;
    }
    if (current.BrakePressure <
        settings.MinimumBrakePressure)
    {
        return previous.BrakePressure >=
            settings.MinimumBrakePressure ?
            BrakeTraceCondition::Release :
            BrakeTraceCondition::Coast;
    }
    const double rate =
        SafeRatio(current.BrakePressure -
                  previous.BrakePressure,
                  current.DeltaSeconds);
    if (rate >= settings.PedalAttackPerSecond)
    {
        return BrakeTraceCondition::InitialAttack;
    }
    if (rate <= -settings.PedalReleasePerSecond)
    {
        return BrakeTraceCondition::Release;
    }
    if (current.SteeringMagnitude >=
            settings.TrailSteeringThreshold &&
        current.BrakePressure < settings.StrongBrakePressure)
    {
        return BrakeTraceCondition::Trail;
    }
    if (std::abs(rate) >=
        settings.PressureModulationPerSecond)
    {
        return BrakeTraceCondition::Modulation;
    }
    if (current.BrakePressure >=
        settings.StrongBrakePressure)
    {
        return BrakeTraceCondition::PeakPressure;
    }
    return BrakeTraceCondition::Modulation;
}

class BrakeTraceRecorder final
{
public:
    static constexpr int MaximumRecordedEvents = 128;

    void Reset() noexcept
    {
        Events.fill({});
        Settings = {};
        Current = {};
        Previous = {};
        HasPrevious = false;
        Active = false;
        Count = 0;
        Head = 0;
        TotalEvents = 0;
        Rejected = 0;
        TotalBrakeSeconds = 0.0;
        TotalTrailSeconds = 0.0;
        TotalLockSeconds = 0.0;
        TotalOverheatedSeconds = 0.0;
    }

    bool Configure(BrakeTraceSettings settings) noexcept
    {
        if (!ValidBrakeTraceSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    bool Observe(BrakeTraceSample sample) noexcept
    {
        if (!ValidBrakeTraceSample(sample, Settings))
        {
            ++Rejected;
            Abort();
            return false;
        }
        if (HasPrevious &&
            sample.TimeSeconds <= Previous.TimeSeconds)
        {
            ++Rejected;
            Abort();
            return false;
        }
        const bool pressing =
            sample.BrakePressure >=
            Settings.MinimumBrakePressure;
        if (!Active && pressing)
        {
            Begin(sample);
        }
        if (Active)
        {
            const BrakeTraceCondition phase =
                HasPrevious ?
                ClassifyBrakeTrace(Previous, sample, Settings) :
                BrakeTraceCondition::InitialAttack;
            const double dt = sample.DeltaSeconds;
            Current.TotalSeconds += dt;
            Current.QualifiedSamples++;
            Current.EndSeconds = sample.TimeSeconds;
            Current.EndSpeedCmS = sample.SpeedCmS;
            Current.EndTrackDistanceCm =
                sample.TrackDistanceCm;
            Current.DistanceCm +=
                sample.SpeedCmS * dt;
            Current.PressureIntegral +=
                sample.BrakePressure * dt;
            Current.DecelerationIntegral +=
                sample.LongitudinalDecelerationG * dt;
            Current.PeakPressure = std::max(
                Current.PeakPressure, sample.BrakePressure);
            Current.PeakDecelerationG = std::max(
                Current.PeakDecelerationG,
                sample.LongitudinalDecelerationG);
            Current.MaximumBrakeTemperatureC = std::max(
                Current.MaximumBrakeTemperatureC,
                sample.BrakeTemperatureC);
            if (phase == BrakeTraceCondition::InitialAttack)
            {
                Current.InitialAttackSeconds += dt;
            }
            if (phase == BrakeTraceCondition::PeakPressure)
            {
                Current.FullPressureSeconds += dt;
            }
            if (phase == BrakeTraceCondition::Modulation)
            {
                Current.ModulationSeconds += dt;
            }
            if (phase == BrakeTraceCondition::Trail)
            {
                Current.TrailSeconds += dt;
                TotalTrailSeconds += dt;
            }
            if (phase == BrakeTraceCondition::Release)
            {
                Current.ReleaseSeconds += dt;
            }
            if (sample.ThrottlePressure >
                Settings.MaximumThrottleOverlap && pressing)
            {
                Current.ThrottleOverlapSeconds += dt;
            }
            if (sample.WheelLockFraction > 0.0)
            {
                Current.WheelLockSeconds +=
                    sample.WheelLockFraction * dt;
                TotalLockSeconds +=
                    sample.WheelLockFraction * dt;
            }
            if (sample.BrakeTemperatureC >=
                Settings.HeatWarningC)
            {
                Current.HeatWarningSeconds += dt;
            }
            if (sample.BrakeTemperatureC >=
                Settings.HeatCriticalC)
            {
                Current.HeatCriticalSeconds += dt;
                TotalOverheatedSeconds += dt;
            }
            if (HasPrevious &&
                std::abs(sample.BrakePressure -
                         Previous.BrakePressure) >= 0.15)
            {
                ++Current.PressureOscillations;
            }
            if (pressing)
            {
                TotalBrakeSeconds += dt;
            }
            if (!pressing)
            {
                Finish(true);
            }
        }
        Previous = sample;
        HasPrevious = true;
        return true;
    }

    void Abort() noexcept
    {
        Active = false;
        Current = {};
        Previous = {};
        HasPrevious = false;
    }

    void Finish(bool completedNormally) noexcept
    {
        if (!Active)
        {
            return;
        }
        Current.EndedNormally = completedNormally;
        Current.EngineQualified = true;
        Current.MeanDecelerationG = SafeRatio(
            Current.DecelerationIntegral,
            Current.TotalSeconds);
        const double startMetersPerSecond =
            Current.StartSpeedCmS / CmPerMeter;
        const double endMetersPerSecond =
            Current.EndSpeedCmS / CmPerMeter;
        Current.EstimatedLostKineticEnergyJPerKg =
            0.5 * std::max(
                0.0,
                startMetersPerSecond * startMetersPerSecond -
                endMetersPerSecond * endMetersPerSecond);
        Current.Valid = Current.TotalSeconds >=
            Settings.MinimumSegmentSeconds &&
            Current.PeakPressure >=
            Settings.MinimumBrakePressure &&
            Current.QualifiedSamples >= 2;
        if (Current.Valid)
        {
            Store(Current);
        }
        Active = false;
        Current = {};
    }

    BrakeTraceEvent AtNewest(int offset = 0) const noexcept
    {
        if (offset < 0 || offset >= Count)
        {
            return {};
        }
        const int index = (Head + Count - offset - 1) %
            MaximumRecordedEvents;
        return Events[static_cast<std::size_t>(index)];
    }

    BrakeTraceEvent ActiveEvent() const noexcept
    {
        return Current;
    }

    bool IsActive() const noexcept
    {
        return Active;
    }

    int StoredEvents() const noexcept
    {
        return Count;
    }

    std::uint64_t LifetimeEvents() const noexcept
    {
        return TotalEvents;
    }

    std::uint64_t RejectedSamples() const noexcept
    {
        return Rejected;
    }

    double BrakingSeconds() const noexcept
    {
        return TotalBrakeSeconds;
    }

    double TrailBrakingSeconds() const noexcept
    {
        return TotalTrailSeconds;
    }

    double LockedWheelSeconds() const noexcept
    {
        return TotalLockSeconds;
    }

    double OverheatSeconds() const noexcept
    {
        return TotalOverheatedSeconds;
    }

private:
    void Begin(BrakeTraceSample sample) noexcept
    {
        Current = {};
        Current.StartSeconds = sample.TimeSeconds;
        Current.EndSeconds = sample.TimeSeconds;
        Current.StartSpeedCmS = sample.SpeedCmS;
        Current.EndSpeedCmS = sample.SpeedCmS;
        Current.StartTrackDistanceCm =
            sample.TrackDistanceCm;
        Current.EndTrackDistanceCm =
            sample.TrackDistanceCm;
        Active = true;
    }

    void Store(BrakeTraceEvent event) noexcept
    {
        if (Count == MaximumRecordedEvents)
        {
            Head = (Head + 1) % MaximumRecordedEvents;
        }
        else
        {
            ++Count;
        }
        Events[static_cast<std::size_t>(
            (Head + Count - 1) % MaximumRecordedEvents)] =
            event;
        TotalEvents = SaturatingIncrement(TotalEvents);
    }

    BrakeTraceSettings Settings{};
    std::array<BrakeTraceEvent, MaximumRecordedEvents> Events{};
    BrakeTraceSample Previous{};
    BrakeTraceEvent Current{};
    int Head = 0;
    int Count = 0;
    std::uint64_t TotalEvents = 0;
    std::uint64_t Rejected = 0;
    double TotalBrakeSeconds = 0.0;
    double TotalTrailSeconds = 0.0;
    double TotalLockSeconds = 0.0;
    double TotalOverheatedSeconds = 0.0;
    bool HasPrevious = false;
    bool Active = false;
};
} // namespace latebrake::engineering
