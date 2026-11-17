#pragma once

#include "Telemetry/TelemetryCoach.h"
#include "Telemetry/TelemetryStintPerformance.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class DrillKind : std::uint8_t
{
    None,
    SmoothThrottle,
    ConsistentBrakeMarker,
    ReduceWheelLock,
    CornerEntryControl,
    ApexPrecision,
    CornerExitStability,
    MinimizeTrackExcursions,
    GripLimitAwareness,
    FuelConservation,
    SectorConsistency,
    MaintainTireTemperature,
    MaintainBrakeTemperature,
    SafeFollowingDistance,
    QualifyReliableSensors,
    ControlledSteeringInput
};

enum class DrillStatus : std::uint8_t
{
    NotStarted,
    Observing,
    Progressing,
    TargetReached,
    Inconclusive,
    FailedSafetyGate,
    Suspended
};

enum class DrillMetric : std::uint8_t
{
    LowerIsBetter,
    HigherIsBetter,
    WithinRange,
    EventAvoidance
};

inline constexpr int MaximumTrainingDrills = 16;
inline constexpr int MaximumDrillHistory = 64;

struct TrainingSettings final
{
    double MinimumNativeConfidence = 0.7;
    double MinimumObservedLapSeconds = 5.0;
    double MaximumDrillWindowSeconds = 3600.0;
    double MinimumScoreImprovement = 0.10;
    double MinimumSustainedSeconds = 2.0;
    double MinimumProgressTolerance = 0.03;
    double MaximumAllowedOffTrackSeconds = 0.25;
    double MaximumAllowedContactEvents = 0.0;
    double MaximumAllowedCriticalHeatSeconds = 0.5;
    double MaximumAllowedSensorFailureFraction = 0.1;
    int MinimumSamplesPerDrill = 3;
    int MaximumSamplesPerDrill = 10000;
    int RequiredSuccessfulAttempts = 3;
    bool RequireNativeSource = true;
    bool PauseDuringCaution = true;
    bool RejectSafetyEvents = true;
    bool PreserveHistoricalProgress = true;
};

inline bool ValidTrainingSettings(
    const TrainingSettings& s) noexcept
{
    return Finite(s.MinimumNativeConfidence) &&
           s.MinimumNativeConfidence >= 0.0 &&
           s.MinimumNativeConfidence <= 1.0 &&
           Finite(s.MinimumObservedLapSeconds) &&
           s.MinimumObservedLapSeconds > 0.0 &&
           Finite(s.MaximumDrillWindowSeconds) &&
           s.MaximumDrillWindowSeconds >
               s.MinimumObservedLapSeconds &&
           Finite(s.MinimumScoreImprovement) &&
           s.MinimumScoreImprovement >= 0.0 &&
           Finite(s.MinimumSustainedSeconds) &&
           s.MinimumSustainedSeconds > 0.0 &&
           Finite(s.MinimumProgressTolerance) &&
           s.MinimumProgressTolerance >= 0.0 &&
           Finite(s.MaximumAllowedOffTrackSeconds) &&
           s.MaximumAllowedOffTrackSeconds >= 0.0 &&
           Finite(s.MaximumAllowedContactEvents) &&
           s.MaximumAllowedContactEvents >= 0.0 &&
           Finite(s.MaximumAllowedCriticalHeatSeconds) &&
           s.MaximumAllowedCriticalHeatSeconds >= 0.0 &&
           Finite(s.MaximumAllowedSensorFailureFraction) &&
           s.MaximumAllowedSensorFailureFraction >= 0.0 &&
           s.MinimumSamplesPerDrill > 0 &&
           s.MaximumSamplesPerDrill >= s.MinimumSamplesPerDrill &&
           s.RequiredSuccessfulAttempts > 0 &&
           s.RequiredSuccessfulAttempts <= 100;
}

struct DrillDefinition final
{
    DrillKind Kind = DrillKind::None;
    DrillMetric Metric = DrillMetric::LowerIsBetter;
    double Reference = 0.0;
    double Target = 0.0;
    double Tolerance = 0.0;
    double MinimumObservationSeconds = 0.0;
    double MaximumWindowSeconds = 0.0;
    int RequiredAttempts = 0;
    int SectorIndex = -1;
    bool NeedsCompletedLap = false;
    bool NeedsFourWheelContact = false;
    bool NeedsOpponentRoster = false;
    bool NeedsVerifiedSector = false;
    bool RejectCaution = true;
    bool RejectContact = true;
    bool Enabled = false;
};

inline bool ValidDrillDefinition(
    const DrillDefinition& d) noexcept
{
    return d.Kind != DrillKind::None &&
           Finite(d.Reference) &&
           Finite(d.Target) &&
           Finite(d.Tolerance) &&
           d.Tolerance >= 0.0 &&
           Finite(d.MinimumObservationSeconds) &&
           d.MinimumObservationSeconds >= 0.0 &&
           Finite(d.MaximumWindowSeconds) &&
           d.MaximumWindowSeconds >
               d.MinimumObservationSeconds &&
           d.RequiredAttempts > 0 &&
           d.RequiredAttempts <= 100 &&
           d.SectorIndex >= -1 &&
           d.SectorIndex < MaximumTelemetrySectors &&
           d.Enabled;
}

struct DrillObservation final
{
    DrillKind Kind = DrillKind::None;
    DrillStatus Status = DrillStatus::NotStarted;
    double ObservedValue = 0.0;
    double BaselineValue = 0.0;
    double TargetValue = 0.0;
    double ProgressFraction = 0.0;
    double Confidence = 0.0;
    double SampledSeconds = 0.0;
    double TargetHoldingSeconds = 0.0;
    double MaximumTargetHoldingSeconds = 0.0;
    int QualifiedSamples = 0;
    int RejectedSamples = 0;
    int SuccessfulAttempts = 0;
    int FailedAttempts = 0;
    int RemainingAttempts = 0;
    int SectorIndex = -1;
    bool TargetSatisfied = false;
    bool SafetyViolation = false;
    bool NativeObserved = false;
    bool CompletedLap = false;
    bool Qualified = false;
};

struct DrillSample final
{
    DrillKind Kind = DrillKind::None;
    double MetricValue = 0.0;
    double Confidence = 0.0;
    double MonotonicSeconds = 0.0;
    double DeltaSeconds = 0.0;
    double OffTrackSeconds = 0.0;
    double ContactEvents = 0.0;
    double CriticalHeatSeconds = 0.0;
    double SensorFailureFraction = 0.0;
    int SectorIndex = -1;
    bool NativeObserved = false;
    bool CautionActive = false;
    bool CompletedLap = false;
    bool FourWheelContact = false;
    bool CompleteOpponentRoster = false;
    bool VerifiedSector = false;
    bool Valid = false;
};

inline bool DrillTargetSatisfied(
    const DrillDefinition& definition,
    double observed) noexcept
{
    if (!ValidDrillDefinition(definition) ||
        !Finite(observed))
    {
        return false;
    }
    switch (definition.Metric)
    {
    case DrillMetric::LowerIsBetter:
        return observed <=
            definition.Target + definition.Tolerance;
    case DrillMetric::HigherIsBetter:
        return observed >=
            definition.Target - definition.Tolerance;
    case DrillMetric::WithinRange:
        return std::abs(observed - definition.Target) <=
            definition.Tolerance;
    case DrillMetric::EventAvoidance:
        return observed <= definition.Tolerance;
    }
    return false;
}

inline double DrillProgress(
    const DrillDefinition& definition,
    double observed) noexcept
{
    if (!Finite(observed) || !ValidDrillDefinition(definition))
    {
        return 0.0;
    }
    if (DrillTargetSatisfied(definition, observed))
    {
        return 1.0;
    }
    switch (definition.Metric)
    {
    case DrillMetric::LowerIsBetter:
        return Clamp01(SafeRatio(
            definition.Reference - observed,
            definition.Reference - definition.Target));
    case DrillMetric::HigherIsBetter:
        return Clamp01(SafeRatio(
            observed - definition.Reference,
            definition.Target - definition.Reference));
    case DrillMetric::WithinRange:
        return Clamp01(1.0 -
            std::abs(observed - definition.Target) /
            std::max(0.01, std::abs(definition.Reference -
                                definition.Target)));
    case DrillMetric::EventAvoidance:
        return Clamp01(
            1.0 - SafeRatio(observed,
                    std::max(0.01, definition.Reference)));
    }
    return 0.0;
}

class TrainingDrillTracker final
{
public:
    bool Configure(
        DrillDefinition definition,
        TrainingSettings settings = {}) noexcept
    {
        if (!ValidTrainingSettings(settings) ||
            !ValidDrillDefinition(definition))
        {
            return false;
        }
        Definition = definition;
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Last = {};
        Last.Kind = Definition.Kind;
        Last.BaselineValue = Definition.Reference;
        Last.TargetValue = Definition.Target;
        Last.SectorIndex = Definition.SectorIndex;
        Last.RemainingAttempts = Definition.RequiredAttempts;
        Status = DrillStatus::NotStarted;
        HasTime = false;
        LastTime = 0.0;
        HoldingSeconds = 0.0;
        ConsecutiveSuccesses = 0;
        History.fill({});
        HistoryCount = 0;
        HistoryNext = 0;
    }

    bool Observe(const DrillSample& sample) noexcept
    {
        if (sample.Kind != Definition.Kind ||
            !sample.Valid ||
            !Finite(sample.MetricValue) ||
            !Finite(sample.Confidence) ||
            !Finite(sample.MonotonicSeconds) ||
            !Finite(sample.DeltaSeconds) ||
            sample.DeltaSeconds <= 0.0 ||
            sample.DeltaSeconds > 0.5 ||
            sample.Confidence < Settings.MinimumNativeConfidence ||
            (Settings.RequireNativeSource && !sample.NativeObserved) ||
            (Definition.NeedsFourWheelContact &&
             !sample.FourWheelContact) ||
            (Definition.NeedsOpponentRoster &&
             !sample.CompleteOpponentRoster) ||
            (Definition.NeedsVerifiedSector &&
             !sample.VerifiedSector) ||
            (Definition.NeedsCompletedLap &&
             !sample.CompletedLap) ||
            (Definition.SectorIndex >= 0 &&
             sample.SectorIndex != Definition.SectorIndex) ||
            (Settings.PauseDuringCaution && sample.CautionActive))
        {
            Last.RejectedSamples += 1;
            return false;
        }
        if (HasTime && sample.MonotonicSeconds <= LastTime)
        {
            Last.RejectedSamples += 1;
            return false;
        }
        const bool unsafe =
            sample.OffTrackSeconds >
                Settings.MaximumAllowedOffTrackSeconds ||
            sample.ContactEvents >
                Settings.MaximumAllowedContactEvents ||
            sample.CriticalHeatSeconds >
                Settings.MaximumAllowedCriticalHeatSeconds ||
            sample.SensorFailureFraction >
                Settings.MaximumAllowedSensorFailureFraction;
        if (unsafe && Settings.RejectSafetyEvents)
        {
            Status = DrillStatus::FailedSafetyGate;
            Last.Status = Status;
            Last.SafetyViolation = true;
            Last.FailedAttempts += 1;
            HoldingSeconds = 0.0;
            LastTime = sample.MonotonicSeconds;
            HasTime = true;
            return false;
        }
        if (Status == DrillStatus::TargetReached)
        {
            return true;
        }
        Last.Kind = Definition.Kind;
        Last.ObservedValue = sample.MetricValue;
        Last.Confidence = sample.Confidence;
        Last.SampledSeconds += sample.DeltaSeconds;
        Last.QualifiedSamples += 1;
        Last.NativeObserved = sample.NativeObserved;
        Last.CompletedLap = sample.CompletedLap;
        Last.ProgressFraction =
            DrillProgress(Definition, sample.MetricValue);
        const bool satisfies = DrillTargetSatisfied(
            Definition, sample.MetricValue);
        Last.TargetSatisfied = satisfies;
        if (satisfies)
        {
            HoldingSeconds += sample.DeltaSeconds;
            Last.MaximumTargetHoldingSeconds = std::max(
                Last.MaximumTargetHoldingSeconds,
                HoldingSeconds);
            if (HoldingSeconds >=
                Settings.MinimumSustainedSeconds)
            {
                ++ConsecutiveSuccesses;
                HoldingSeconds = 0.0;
                ++Last.SuccessfulAttempts;
            }
        }
        else
        {
            HoldingSeconds = 0.0;
            ConsecutiveSuccesses = 0;
        }
        Last.TargetHoldingSeconds = HoldingSeconds;
        Last.RemainingAttempts = std::max(
            0, Definition.RequiredAttempts -
               Last.SuccessfulAttempts);
        Status = Last.SuccessfulAttempts >=
            Definition.RequiredAttempts ?
                DrillStatus::TargetReached :
            Last.ProgressFraction > 0.0 ?
                DrillStatus::Progressing :
                DrillStatus::Observing;
        if (Last.SampledSeconds >
            Definition.MaximumWindowSeconds &&
            Status != DrillStatus::TargetReached)
        {
            Status = DrillStatus::Inconclusive;
        }
        Last.Status = Status;
        Last.Qualified =
            Last.QualifiedSamples >=
                Settings.MinimumSamplesPerDrill &&
            Last.SampledSeconds >=
                Definition.MinimumObservationSeconds;
        if (HistoryCount < MaximumDrillHistory)
        {
            ++HistoryCount;
        }
        History[static_cast<std::size_t>(HistoryNext)] = Last;
        HistoryNext =
            (HistoryNext + 1) % MaximumDrillHistory;
        HasTime = true;
        LastTime = sample.MonotonicSeconds;
        return true;
    }

    DrillObservation Snapshot() const noexcept { return Last; }
    DrillDefinition Goal() const noexcept { return Definition; }
    bool Achieved() const noexcept
    {
        return Status == DrillStatus::TargetReached;
    }
    bool FailedSafetyGate() const noexcept
    {
        return Status == DrillStatus::FailedSafetyGate;
    }
    int HistorySize() const noexcept { return HistoryCount; }
    DrillObservation Previous(int fromNewest) const noexcept
    {
        if (fromNewest < 0 || fromNewest >= HistoryCount)
        {
            return {};
        }
        const int index = (
            HistoryNext + MaximumDrillHistory -
            1 - fromNewest) % MaximumDrillHistory;
        return History[static_cast<std::size_t>(index)];
    }

private:
    DrillDefinition Definition{};
    TrainingSettings Settings{};
    DrillObservation Last{};
    std::array<DrillObservation, MaximumDrillHistory> History{};
    DrillStatus Status = DrillStatus::NotStarted;
    double LastTime = 0.0;
    double HoldingSeconds = 0.0;
    int ConsecutiveSuccesses = 0;
    int HistoryCount = 0;
    int HistoryNext = 0;
    bool HasTime = false;
};

struct TrainingProgramSnapshot final
{
    std::array<DrillObservation, MaximumTrainingDrills> Drills{};
    int ConfiguredDrills = 0;
    int CompletedDrills = 0;
    int FailedSafetyDrills = 0;
    int ActiveDrills = 0;
    double MeanProgressFraction = 0.0;
    double MeanEvidenceConfidence = 0.0;
    double TotalQualifiedSeconds = 0.0;
    bool HasProgram = false;
    bool ProgramCompleted = false;
};

class TrainingProgram final
{
public:
    void Reset() noexcept
    {
        Trackers.fill({});
        Enabled.fill(false);
        Count = 0;
        Last = {};
    }

    bool Add(
        DrillDefinition definition,
        TrainingSettings settings = {}) noexcept
    {
        if (!ValidDrillDefinition(definition) ||
            Count >= MaximumTrainingDrills)
        {
            return false;
        }
        for (int i = 0; i < Count; ++i)
        {
            if (Trackers[static_cast<std::size_t>(i)]
                    .Goal().Kind == definition.Kind)
            {
                return false;
            }
        }
        if (!Trackers[static_cast<std::size_t>(Count)]
                 .Configure(definition, settings))
        {
            return false;
        }
        Enabled[static_cast<std::size_t>(Count)] = true;
        ++Count;
        return true;
    }

    bool Observe(DrillSample sample) noexcept
    {
        bool recorded = false;
        for (int i = 0; i < Count; ++i)
        {
            if (Trackers[static_cast<std::size_t>(i)]
                    .Goal().Kind == sample.Kind)
            {
                recorded =
                    Trackers[static_cast<std::size_t>(i)]
                        .Observe(sample);
                break;
            }
        }
        Recalculate();
        return recorded;
    }

    TrainingProgramSnapshot Snapshot() const noexcept
    {
        return Last;
    }

    DrillObservation Drill(int index) const noexcept
    {
        return index >= 0 && index < Count ?
            Trackers[static_cast<std::size_t>(index)].Snapshot() :
            DrillObservation{};
    }

    int CountDrills() const noexcept { return Count; }

private:
    void Recalculate() noexcept
    {
        TrainingProgramSnapshot snapshot;
        snapshot.ConfiguredDrills = Count;
        snapshot.HasProgram = Count > 0;
        for (int i = 0; i < Count; ++i)
        {
            const DrillObservation d =
                Trackers[static_cast<std::size_t>(i)].Snapshot();
            snapshot.Drills[static_cast<std::size_t>(i)] = d;
            snapshot.CompletedDrills +=
                d.Status == DrillStatus::TargetReached ? 1 : 0;
            snapshot.FailedSafetyDrills +=
                d.Status == DrillStatus::FailedSafetyGate ? 1 : 0;
            snapshot.ActiveDrills +=
                d.Status == DrillStatus::Progressing ||
                d.Status == DrillStatus::Observing ? 1 : 0;
            snapshot.MeanProgressFraction += d.ProgressFraction;
            snapshot.MeanEvidenceConfidence += d.Confidence;
            snapshot.TotalQualifiedSeconds += d.SampledSeconds;
        }
        if (Count > 0)
        {
            snapshot.MeanProgressFraction /=
                static_cast<double>(Count);
            snapshot.MeanEvidenceConfidence /=
                static_cast<double>(Count);
        }
        snapshot.ProgramCompleted =
            Count > 0 &&
            snapshot.CompletedDrills == Count &&
            snapshot.FailedSafetyDrills == 0;
        Last = snapshot;
    }

    std::array<TrainingDrillTracker, MaximumTrainingDrills> Trackers{};
    std::array<bool, MaximumTrainingDrills> Enabled{};
    TrainingProgramSnapshot Last{};
    int Count = 0;
};
} // namespace latebrake::engineering
