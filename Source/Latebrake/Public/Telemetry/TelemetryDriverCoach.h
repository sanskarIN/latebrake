#pragma once

#include "Telemetry/TelemetryBrakeTrace.h"
#include "Telemetry/TelemetryCornerSequence.h"
#include "Telemetry/TelemetryVehicleBalance.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class CoachingTopic : std::uint8_t
{
    None,
    BrakeEarlier,
    BrakeLaterWithinGrip,
    ReleaseBrakeProgressively,
    AvoidWheelLock,
    BuildExitThrottleGradually,
    ReduceCornerSlip,
    ReduceLineError,
    PreserveTireTemperature,
    StabilizeSteering,
    AvoidThrottleBrakeOverlap,
    ProtectGripOnWetTrack,
    ImproveCornerExit,
    ReviewIncompleteSensors,
    ReviewPhysicalDamage
};

enum class CoachingPriority : std::uint8_t
{
    Informational,
    Improvement,
    Important,
    SafetyReview
};

struct CoachingEvidence final
{
    BrakeEventRecord Brake{};
    CornerEventRecord Corner{};
    HandlingBalanceReview Balance{};
    GripTelemetry Grip{};
    TrackProjection Track{};
    double ReferenceEntrySpeedCmS = 0.0;
    double ReferenceApexSpeedCmS = 0.0;
    double ReferenceExitSpeedCmS = 0.0;
    double ReferenceBrakingDistanceCm = 0.0;
    double MaximumPhysicalTrackSpeedCmS = 0.0;
    double TrackWetnessFraction = 0.0;
    double ChassisDamageFraction = 0.0;
    double TimeSeconds = 0.0;
    int DriverSlot = -1;
    int LapIndex = -1;
    bool OriginalTrackVerified = false;
    bool PhysicalSensorsReady = false;
    bool EngineQualified = false;
};

struct CoachingSettings final
{
    double MinimumConfidence = 0.60;
    double HighConfidence = 0.84;
    double MinimumCoachingIntervalSeconds = 4.0;
    double MinimumRelativeSpeedDifferenceFraction = 0.06;
    double LateBrakeOffsetThresholdCm = 200.0;
    double EarlyBrakeOffsetThresholdCm = 500.0;
    double ExcessiveCornerSlipRadians = 0.23;
    double ExcessiveWheelSlipRatio = 0.35;
    double ExcessiveLineErrorCm = 350.0;
    double ExcessiveBrakeOverlapSeconds = 0.20;
    double ExcessiveThrottleOverlapSeconds = 0.20;
    double ExcessiveTireHeatC = 140.0;
    double UnsafeChassisDamageFraction = 0.60;
    double WetTrackWarningFraction = 0.65;
    double MaximumMissingSensorTolerance = 0.0;
    int MaximumAdvicePerLap = 6;
    bool IgnoreSyntheticEvidence = true;
};

inline bool ValidCoachingSettings(
    CoachingSettings s) noexcept
{
    return Finite(s.MinimumConfidence) &&
        s.MinimumConfidence >= 0.0 &&
        s.MinimumConfidence <= 1.0 &&
        Finite(s.HighConfidence) &&
        s.HighConfidence >= s.MinimumConfidence &&
        s.HighConfidence <= 1.0 &&
        Finite(s.MinimumCoachingIntervalSeconds) &&
        s.MinimumCoachingIntervalSeconds > 0.0 &&
        Finite(s.MinimumRelativeSpeedDifferenceFraction) &&
        s.MinimumRelativeSpeedDifferenceFraction > 0.0 &&
        Finite(s.LateBrakeOffsetThresholdCm) &&
        s.LateBrakeOffsetThresholdCm >= 0.0 &&
        Finite(s.EarlyBrakeOffsetThresholdCm) &&
        s.EarlyBrakeOffsetThresholdCm >= 0.0 &&
        Finite(s.ExcessiveCornerSlipRadians) &&
        s.ExcessiveCornerSlipRadians > 0.0 &&
        Finite(s.ExcessiveWheelSlipRatio) &&
        s.ExcessiveWheelSlipRatio > 0.0 &&
        Finite(s.ExcessiveLineErrorCm) &&
        s.ExcessiveLineErrorCm > 0.0 &&
        Finite(s.ExcessiveBrakeOverlapSeconds) &&
        s.ExcessiveBrakeOverlapSeconds >= 0.0 &&
        Finite(s.ExcessiveThrottleOverlapSeconds) &&
        s.ExcessiveThrottleOverlapSeconds >= 0.0 &&
        Finite(s.ExcessiveTireHeatC) &&
        s.ExcessiveTireHeatC > 0.0 &&
        Finite(s.UnsafeChassisDamageFraction) &&
        s.UnsafeChassisDamageFraction > 0.0 &&
        s.UnsafeChassisDamageFraction <= 1.0 &&
        Finite(s.WetTrackWarningFraction) &&
        s.WetTrackWarningFraction >= 0.0 &&
        s.WetTrackWarningFraction <= 1.0 &&
        s.MaximumAdvicePerLap >= 1 &&
        s.MaximumAdvicePerLap <= 64;
}

struct CoachingCandidate final
{
    CoachingTopic Topic = CoachingTopic::None;
    CoachingPriority Priority = CoachingPriority::Informational;
    double Confidence = 0.0;
    double Severity = 0.0;
    double EstimatedPotentialSeconds = 0.0;
    double ObservedValue = 0.0;
    double ReferenceValue = 0.0;
    double SuggestedTargetValue = 0.0;
    double TrackDistanceCm = 0.0;
    double GeneratedAtSeconds = 0.0;
    int LapIndex = -1;
    int DriverSlot = -1;
    int EvidenceCount = 0;
    bool NeedsGenuinePhysics = true;
    bool RequiresHumanReview = true;
    bool CommandsVehicle = false;
    bool Valid = false;
};

inline CoachingCandidate Candidate(
    CoachingTopic topic, CoachingPriority priority,
    double confidence, double observed,
    double reference, double severity,
    CoachingEvidence evidence) noexcept
{
    CoachingCandidate c;
    c.Topic = topic;
    c.Priority = priority;
    c.Confidence = Clamp01(confidence);
    c.ObservedValue = observed;
    c.ReferenceValue = reference;
    c.SuggestedTargetValue = reference;
    c.Severity = Clamp01(severity);
    c.GeneratedAtSeconds = evidence.TimeSeconds;
    c.TrackDistanceCm = evidence.Track.Valid ?
        evidence.Track.DistanceAlongCm : 0.0;
    c.LapIndex = evidence.LapIndex;
    c.DriverSlot = evidence.DriverSlot;
    c.EvidenceCount =
        static_cast<int>(evidence.Brake.Valid) +
        static_cast<int>(evidence.Corner.Valid) +
        static_cast<int>(evidence.Balance.Valid) +
        static_cast<int>(evidence.Grip.Valid) +
        static_cast<int>(evidence.Track.Valid);
    c.NeedsGenuinePhysics = true;
    c.RequiresHumanReview = true;
    c.CommandsVehicle = false;
    c.Valid = topic != CoachingTopic::None &&
        evidence.EngineQualified &&
        evidence.OriginalTrackVerified &&
        evidence.PhysicalSensorsReady &&
        Finite(observed) && Finite(reference);
    return c;
}

inline CoachingCandidate EvaluateCoachingEvidence(
    CoachingEvidence e,
    CoachingSettings settings = {}) noexcept
{
    CoachingCandidate none;
    if (!ValidCoachingSettings(settings) ||
        !e.EngineQualified ||
        !e.OriginalTrackVerified ||
        !e.PhysicalSensorsReady ||
        !Finite(e.TimeSeconds) ||
        !Finite(e.TrackWetnessFraction) ||
        !Finite(e.ChassisDamageFraction) ||
        e.TimeSeconds < 0.0 ||
        e.TrackWetnessFraction < 0.0 ||
        e.TrackWetnessFraction > 1.0 ||
        e.ChassisDamageFraction < 0.0 ||
        e.ChassisDamageFraction > 1.0)
    {
        return none;
    }
    if (e.ChassisDamageFraction >=
        settings.UnsafeChassisDamageFraction)
    {
        return Candidate(
            CoachingTopic::ReviewPhysicalDamage,
            CoachingPriority::SafetyReview,
            0.95,
            e.ChassisDamageFraction,
            settings.UnsafeChassisDamageFraction,
            e.ChassisDamageFraction, e);
    }
    if (e.Brake.Valid &&
        e.Brake.WheelLockSeconds > 0.10)
    {
        return Candidate(
            CoachingTopic::AvoidWheelLock,
            CoachingPriority::SafetyReview,
            0.85, e.Brake.WheelLockSeconds,
            0.0, Clamp01(e.Brake.WheelLockSeconds), e);
    }
    if (e.Grip.Valid &&
        e.Grip.MeanTireTemperatureC >=
            settings.ExcessiveTireHeatC)
    {
        return Candidate(
            CoachingTopic::PreserveTireTemperature,
            CoachingPriority::Important,
            e.Grip.Confidence,
            e.Grip.MeanTireTemperatureC,
            settings.ExcessiveTireHeatC,
            Clamp01((e.Grip.MeanTireTemperatureC -
                     settings.ExcessiveTireHeatC) / 60.0), e);
    }
    if (e.Corner.Valid &&
        e.Corner.PeakLateralErrorCm >
            settings.ExcessiveLineErrorCm)
    {
        return Candidate(
            CoachingTopic::ReduceLineError,
            CoachingPriority::Improvement,
            0.80, e.Corner.PeakLateralErrorCm,
            settings.ExcessiveLineErrorCm,
            Clamp01(SafeRatio(
                e.Corner.PeakLateralErrorCm -
                    settings.ExcessiveLineErrorCm,
                settings.ExcessiveLineErrorCm)), e);
    }
    if (e.Corner.Valid &&
        e.Corner.PeakSlipAngleRadians >
            settings.ExcessiveCornerSlipRadians)
    {
        return Candidate(
            CoachingTopic::ReduceCornerSlip,
            CoachingPriority::Improvement,
            0.80, e.Corner.PeakSlipAngleRadians,
            settings.ExcessiveCornerSlipRadians,
            Clamp01(SafeRatio(
                e.Corner.PeakSlipAngleRadians,
                settings.ExcessiveCornerSlipRadians) - 1.0), e);
    }
    if (e.Corner.Valid &&
        e.Corner.ExitSpeedCmS > 0.0 &&
        e.ReferenceExitSpeedCmS > 0.0 &&
        e.Corner.ExitSpeedCmS <
            e.ReferenceExitSpeedCmS *
            (1.0 - settings.MinimumRelativeSpeedDifferenceFraction))
    {
        return Candidate(
            CoachingTopic::ImproveCornerExit,
            CoachingPriority::Improvement,
            0.76, e.Corner.ExitSpeedCmS,
            e.ReferenceExitSpeedCmS,
            Clamp01(1.0 - SafeRatio(
                e.Corner.ExitSpeedCmS,
                e.ReferenceExitSpeedCmS)), e);
    }
    if (e.Brake.Valid &&
        e.ReferenceBrakingDistanceCm > 0.0)
    {
        const double offset =
            e.Brake.DistanceCm -
            e.ReferenceBrakingDistanceCm;
        if (offset < -settings.LateBrakeOffsetThresholdCm)
        {
            return Candidate(
                CoachingTopic::BrakeEarlier,
                CoachingPriority::Important,
                0.72, e.Brake.DistanceCm,
                e.ReferenceBrakingDistanceCm,
                Clamp01(-offset /
                    e.ReferenceBrakingDistanceCm), e);
        }
        if (offset >
            settings.EarlyBrakeOffsetThresholdCm)
        {
            return Candidate(
                CoachingTopic::BrakeLaterWithinGrip,
                CoachingPriority::Improvement,
                0.66, e.Brake.DistanceCm,
                e.ReferenceBrakingDistanceCm,
                Clamp01(offset /
                    e.ReferenceBrakingDistanceCm), e);
        }
    }
    if (e.Corner.Valid &&
        e.Corner.ThrottleOverlapSeconds >
            settings.ExcessiveThrottleOverlapSeconds)
    {
        return Candidate(
            CoachingTopic::AvoidThrottleBrakeOverlap,
            CoachingPriority::Improvement,
            0.70, e.Corner.ThrottleOverlapSeconds,
            0.0,
            Clamp01(e.Corner.ThrottleOverlapSeconds), e);
    }
    return none;
}

struct CoachingHistoryReview final
{
    std::array<int, 15> TopicsSeen{};
    int TotalCandidates = 0;
    int AcceptedSuggestions = 0;
    int RejectedSuggestions = 0;
    int SafetyReviewSuggestions = 0;
    int UniqueTopics = 0;
    double MeanConfidence = 0.0;
    double MeanSeverity = 0.0;
    double MeanIntervalSeconds = 0.0;
    double MostRecentSuggestionSeconds = 0.0;
    bool AnyVehicleCommands = false;
    bool AdviceOnly = true;
};

class DriverCoachingJournal final
{
public:
    void Reset() noexcept
    {
        Settings = {};
        Recent.fill({});
        Count = 0;
        Head = 0;
        LastTime = -std::numeric_limits<double>::infinity();
        CurrentLap = -1;
        AdviceThisLap = 0;
        Summary = {};
        ConfidenceStats.Reset();
        SeverityStats.Reset();
        IntervalStats.Reset();
    }

    bool Configure(CoachingSettings settings) noexcept
    {
        if (!ValidCoachingSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    bool Observe(CoachingEvidence evidence) noexcept
    {
        const CoachingCandidate c =
            EvaluateCoachingEvidence(evidence, Settings);
        if (!c.Valid ||
            c.Confidence < Settings.MinimumConfidence)
        {
            ++Summary.RejectedSuggestions;
            return false;
        }
        if (evidence.LapIndex != CurrentLap)
        {
            CurrentLap = evidence.LapIndex;
            AdviceThisLap = 0;
        }
        if (AdviceThisLap >= Settings.MaximumAdvicePerLap ||
            evidence.TimeSeconds - LastTime <
                Settings.MinimumCoachingIntervalSeconds)
        {
            ++Summary.RejectedSuggestions;
            return false;
        }
        if (Count >= static_cast<int>(Recent.size()))
        {
            Head = (Head + 1) %
                static_cast<int>(Recent.size());
        }
        else
        {
            ++Count;
        }
        Recent[static_cast<std::size_t>(
            (Head + Count - 1) %
            static_cast<int>(Recent.size()))] = c;
        if (Finite(LastTime))
        {
            IntervalStats.Observe(
                evidence.TimeSeconds - LastTime);
        }
        LastTime = evidence.TimeSeconds;
        ++AdviceThisLap;
        ++Summary.TotalCandidates;
        ++Summary.AcceptedSuggestions;
        const std::size_t index =
            static_cast<std::size_t>(c.Topic);
        if (index < Summary.TopicsSeen.size())
        {
            if (Summary.TopicsSeen[index] == 0)
            {
                ++Summary.UniqueTopics;
            }
            ++Summary.TopicsSeen[index];
        }
        Summary.SafetyReviewSuggestions +=
            c.Priority ==
                CoachingPriority::SafetyReview ? 1 : 0;
        ConfidenceStats.Observe(c.Confidence);
        SeverityStats.Observe(c.Severity);
        Summary.MeanConfidence =
            ConfidenceStats.Mean();
        Summary.MeanSeverity =
            SeverityStats.Mean();
        Summary.MeanIntervalSeconds =
            IntervalStats.Mean();
        Summary.MostRecentSuggestionSeconds =
            c.GeneratedAtSeconds;
        Summary.AnyVehicleCommands = false;
        Summary.AdviceOnly = true;
        return true;
    }

    CoachingCandidate AtNewest(int offset = 0) const noexcept
    {
        return offset >= 0 && offset < Count ?
            Recent[static_cast<std::size_t>(
                (Head + Count - offset - 1) %
                static_cast<int>(Recent.size()))] :
            CoachingCandidate{};
    }

    CoachingHistoryReview History() const noexcept
    {
        return Summary;
    }

    int StoredSuggestions() const noexcept
    {
        return Count;
    }

private:
    CoachingSettings Settings{};
    std::array<CoachingCandidate, 64> Recent{};
    CoachingHistoryReview Summary{};
    RunningStatistics ConfidenceStats;
    RunningStatistics SeverityStats;
    RunningStatistics IntervalStats;
    int Count = 0;
    int Head = 0;
    int CurrentLap = -1;
    int AdviceThisLap = 0;
    double LastTime =
        -std::numeric_limits<double>::infinity();
};
} // namespace latebrake::engineering
