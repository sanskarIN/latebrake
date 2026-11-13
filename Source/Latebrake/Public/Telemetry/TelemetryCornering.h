#pragma once

#include "Telemetry/TelemetryBraking.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class CornerPhase : std::uint8_t
{
    Straight,
    Approaching,
    TurnIn,
    Apex,
    Unwinding,
    Complete
};

enum class CornerDirection : std::uint8_t
{
    Unknown,
    Left,
    Right
};

struct CornerAnalysisSettings final
{
    double MinimumSpeedCmS = 250.0;
    double EnterSteering = 0.22;
    double ExitSteering = 0.10;
    double MinimumYawRateRadS = 0.12;
    double MinimumCurvaturePerCm = 0.00010;
    double MinimumCornerSeconds = 0.40;
    double MaximumCornerSeconds = 22.0;
    double ApexSpeedRecoveryFraction = 0.12;
    double ExitStableSeconds = 0.35;
    double MaximumEntryDistanceCm = 1200.0;
    double MaximumLineErrorCm = 1000.0;
    double LargeSlipAngleRadians = 0.20;
    double StrongLateralG = 0.85;
    double SuggestedApexClearanceCm = 130.0;
};

inline bool ValidCornerAnalysisSettings(
    const CornerAnalysisSettings& s) noexcept
{
    return Finite(s.MinimumSpeedCmS) && s.MinimumSpeedCmS >= 0.0 &&
           Finite(s.EnterSteering) && s.EnterSteering > 0.0 &&
           s.EnterSteering <= 1.0 &&
           Finite(s.ExitSteering) && s.ExitSteering >= 0.0 &&
           s.ExitSteering < s.EnterSteering &&
           Finite(s.MinimumYawRateRadS) && s.MinimumYawRateRadS > 0.0 &&
           Finite(s.MinimumCurvaturePerCm) &&
           s.MinimumCurvaturePerCm >= 0.0 &&
           Finite(s.MinimumCornerSeconds) &&
           s.MinimumCornerSeconds > 0.0 &&
           Finite(s.MaximumCornerSeconds) &&
           s.MaximumCornerSeconds > s.MinimumCornerSeconds &&
           Finite(s.ApexSpeedRecoveryFraction) &&
           s.ApexSpeedRecoveryFraction >= 0.0 &&
           s.ApexSpeedRecoveryFraction <= 1.0 &&
           Finite(s.ExitStableSeconds) && s.ExitStableSeconds > 0.0 &&
           Finite(s.MaximumEntryDistanceCm) &&
           s.MaximumEntryDistanceCm > 0.0 &&
           Finite(s.MaximumLineErrorCm) &&
           s.MaximumLineErrorCm > 0.0 &&
           Finite(s.LargeSlipAngleRadians) &&
           s.LargeSlipAngleRadians > 0.0 &&
           Finite(s.StrongLateralG) && s.StrongLateralG > 0.0 &&
           Finite(s.SuggestedApexClearanceCm) &&
           s.SuggestedApexClearanceCm >= 0.0;
}

struct CornerEventRecord final
{
    std::uint64_t Ordinal = 0;
    CornerDirection Direction = CornerDirection::Unknown;
    CornerPhase ExitPhase = CornerPhase::Straight;
    int EntrySegment = -1;
    int ApexSegment = -1;
    int ExitSegment = -1;
    double EntryTimeSeconds = 0.0;
    double ApexTimeSeconds = 0.0;
    double ExitTimeSeconds = 0.0;
    double DurationSeconds = 0.0;
    double EntrySpeedCmS = 0.0;
    double ApexSpeedCmS = 0.0;
    double ExitSpeedCmS = 0.0;
    double MinimumSpeedCmS = 0.0;
    double MaximumSpeedCmS = 0.0;
    double PeakLateralG = 0.0;
    double PeakSlipAngleRadians = 0.0;
    double PeakSteering = 0.0;
    double PeakBrake = 0.0;
    double PeakThrottle = 0.0;
    double IntegratedSteeringSeconds = 0.0;
    double IntegratedSlipSeconds = 0.0;
    double IntegratedLateralErrorCmSeconds = 0.0;
    double PeakLateralErrorCm = 0.0;
    double TrackDistanceCm = 0.0;
    double EntryAtTrackCm = 0.0;
    double ApexAtTrackCm = 0.0;
    double ExitAtTrackCm = 0.0;
    double TrailBrakingSeconds = 0.0;
    double ThrottleOverlapSeconds = 0.0;
    double UndersteerProxySeconds = 0.0;
    double OversteerProxySeconds = 0.0;
    double LargeSlipSeconds = 0.0;
    double OffTrackSeconds = 0.0;
    double SettlingSeconds = 0.0;
    double NormalizedLinePrecision = 0.0;
    double NormalizedApexEfficiency = 0.0;
    double NormalizedExitStability = 0.0;
    double CompositeReviewScore = 0.0;
    bool ApexObserved = false;
    bool ContactOnCorner = false;
    bool DirectionReliable = false;
    bool Valid = false;
};

struct CornerLiveMetrics final
{
    CornerPhase Phase = CornerPhase::Straight;
    CornerDirection Direction = CornerDirection::Unknown;
    double CurrentSpeedCmS = 0.0;
    double CurrentLateralG = 0.0;
    double CurrentSlipRadians = 0.0;
    double CurrentSteering = 0.0;
    double CurrentBrake = 0.0;
    double CurrentThrottle = 0.0;
    double CurrentLineErrorCm = 0.0;
    double CurrentTrackCurvature = 0.0;
    double ElapsedInCornerSeconds = 0.0;
    double EstimatedRadiusCm = 0.0;
    double GripDemandFraction = 0.0;
    bool Active = false;
    bool CandidateApex = false;
    bool OffTrack = false;
    bool Valid = false;
};

inline CornerDirection DetermineCornerDirection(
    double yawRateRadS, double trackCurvature,
    double steering) noexcept
{
    const double selected = std::abs(yawRateRadS) > 0.04 ?
        yawRateRadS : std::abs(trackCurvature) > Tiny ?
        trackCurvature : steering;
    if (!Finite(selected) || std::abs(selected) < Tiny)
    {
        return CornerDirection::Unknown;
    }
    return selected > 0.0 ?
        CornerDirection::Left : CornerDirection::Right;
}

inline double CornerReviewScore(
    const CornerEventRecord& c,
    const CornerAnalysisSettings& s) noexcept
{
    if (!c.ApexObserved || c.DurationSeconds < s.MinimumCornerSeconds ||
        !Finite(c.PeakLateralErrorCm) ||
        !Finite(c.PeakSlipAngleRadians))
    {
        return 0.0;
    }
    const double linePenalty = Clamp01(
        c.IntegratedLateralErrorCmSeconds /
        std::max(1.0, c.DurationSeconds * s.MaximumLineErrorCm));
    const double slipPenalty = Clamp01(
        c.LargeSlipSeconds / std::max(0.01, c.DurationSeconds));
    const double offTrackPenalty = Clamp01(
        2.0 * c.OffTrackSeconds /
        std::max(0.01, c.DurationSeconds));
    const double settlePenalty = Clamp01(
        c.SettlingSeconds / std::max(0.01, c.DurationSeconds));
    const double exitPenalty = Clamp01(
        SafeRatio(c.ApexSpeedCmS - c.ExitSpeedCmS,
                  std::max(1.0, c.EntrySpeedCmS)));
    const double quality = 100.0 *
        (1.0 - 0.30 * linePenalty -
         0.18 * slipPenalty -
         0.22 * offTrackPenalty -
         0.12 * settlePenalty -
         0.18 * exitPenalty);
    return std::clamp(quality, 0.0, 100.0);
}

class CornerEventObserver final
{
public:
    bool Configure(CornerAnalysisSettings settings) noexcept
    {
        if (!ValidCornerAnalysisSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        ResetTransient();
        LastFinished = {};
        CompletedCount = 0;
        ValidCount = 0;
        Scores.Reset();
        ApexSpeeds.Reset();
        ExitSpeeds.Reset();
        Slips.Reset();
        TotalCornerSeconds = 0.0;
        TotalOffTrackSeconds = 0.0;
        TotalOversteerSeconds = 0.0;
    }

    void Suspend() noexcept
    {
        ResetTransient();
    }

    bool Observe(
        const TelemetryFrame& frame,
        const TelemetryKinematics& motion,
        const TrackProjection& track,
        double dt) noexcept
    {
        if (!motion.Valid || !Finite(dt) || dt <= 0.0 ||
            dt > MaximumFrameDeltaSeconds ||
            !Finite(frame.Controls.Steering) ||
            !Finite(frame.Controls.Brake) ||
            !Finite(frame.Controls.Throttle))
        {
            Suspend();
            return false;
        }
        const double steering = frame.Controls.Steering;
        const double brake = frame.Controls.Brake;
        const double throttle = frame.Controls.Throttle;
        const double speed = motion.SpeedCmS;
        const double yawRate = motion.YawRateRadS;
        const double lateralG =
            std::abs(motion.LateralAccelerationCmS2) / GravityCmS2;
        const double slip = std::abs(motion.SlipAngleRadians);
        const double curvature =
            track.Valid ? track.CurvaturePerCm : 0.0;
        const double lineError =
            track.Valid ? std::abs(track.SignedLateralCm) : 0.0;
        const bool rotating = speed >= Settings.MinimumSpeedCmS &&
            (std::abs(steering) >= Settings.EnterSteering ||
             std::abs(yawRate) >= Settings.MinimumYawRateRadS ||
             std::abs(curvature) >= Settings.MinimumCurvaturePerCm);
        const bool settling = std::abs(steering) <
            Settings.ExitSteering &&
            std::abs(yawRate) < Settings.MinimumYawRateRadS * 0.6;
        const bool offTrack = track.Valid && track.OffRoad;
        const bool highSlip = slip >= Settings.LargeSlipAngleRadians;
        const bool trailBrake = brake > 0.10 &&
            std::abs(steering) > Settings.ExitSteering;
        const double radius =
            std::abs(motion.EstimatedCurvature) > Tiny ?
                1.0 / std::abs(motion.EstimatedCurvature) : 0.0;
        const double demand =
            std::abs(motion.LateralAccelerationCmS2) /
            (std::max(0.2, frame.Dynamics.GripScale) * GravityCmS2);
        Live.CurrentSpeedCmS = speed;
        Live.CurrentLateralG = lateralG;
        Live.CurrentSlipRadians = slip;
        Live.CurrentSteering = steering;
        Live.CurrentBrake = brake;
        Live.CurrentThrottle = throttle;
        Live.CurrentLineErrorCm = lineError;
        Live.CurrentTrackCurvature = curvature;
        Live.EstimatedRadiusCm = radius;
        Live.GripDemandFraction = std::clamp(demand, 0.0, 3.0);
        Live.OffTrack = offTrack;
        Live.Valid = true;

        if (!Active)
        {
            Live.Phase = CornerPhase::Straight;
            Live.Active = false;
            Live.ElapsedInCornerSeconds = 0.0;
            Live.CandidateApex = false;
            if (!rotating)
            {
                return true;
            }
            Begin(frame, motion, track);
        }
        if (!Active)
        {
            return true;
        }
        Current.DurationSeconds += dt;
        Current.ExitTimeSeconds = frame.MonotonicSeconds;
        Current.ExitSpeedCmS = speed;
        Current.ExitSegment = track.Valid ? track.Segment : -1;
        Current.ExitAtTrackCm =
            track.Valid ? track.DistanceAlongCm : 0.0;
        Current.MinimumSpeedCmS =
            std::min(Current.MinimumSpeedCmS, speed);
        Current.MaximumSpeedCmS =
            std::max(Current.MaximumSpeedCmS, speed);
        Current.PeakLateralG =
            std::max(Current.PeakLateralG, lateralG);
        Current.PeakSlipAngleRadians =
            std::max(Current.PeakSlipAngleRadians, slip);
        Current.PeakSteering =
            std::max(Current.PeakSteering, std::abs(steering));
        Current.PeakBrake = std::max(Current.PeakBrake, brake);
        Current.PeakThrottle = std::max(Current.PeakThrottle, throttle);
        Current.IntegratedSteeringSeconds += std::abs(steering) * dt;
        Current.IntegratedSlipSeconds += slip * dt;
        Current.IntegratedLateralErrorCmSeconds += lineError * dt;
        Current.PeakLateralErrorCm =
            std::max(Current.PeakLateralErrorCm, lineError);
        Current.TrackDistanceCm += speed * dt;
        Current.OffTrackSeconds += offTrack ? dt : 0.0;
        Current.LargeSlipSeconds += highSlip ? dt : 0.0;
        Current.TrailBrakingSeconds += trailBrake ? dt : 0.0;
        Current.ThrottleOverlapSeconds +=
            (throttle > 0.15 && brake > 0.15) ? dt : 0.0;
        const double measuredYawResponse =
            std::abs(yawRate) /
            std::max(0.10, std::abs(steering));
        if (std::abs(steering) > Settings.EnterSteering &&
            measuredYawResponse < 0.12)
        {
            Current.UndersteerProxySeconds += dt;
        }
        if (highSlip && lateralG > Settings.StrongLateralG * 0.6)
        {
            Current.OversteerProxySeconds += dt;
            TotalOversteerSeconds += dt;
        }
        const CornerDirection direction =
            DetermineCornerDirection(yawRate, curvature, steering);
        if (direction != CornerDirection::Unknown)
        {
            if (Current.Direction == CornerDirection::Unknown)
            {
                Current.Direction = direction;
            }
            Current.DirectionReliable = true;
        }
        if (speed <= Current.ApexSpeedCmS ||
            !Current.ApexObserved)
        {
            Current.ApexSpeedCmS = speed;
            Current.ApexTimeSeconds = frame.MonotonicSeconds;
            Current.ApexSegment = track.Valid ? track.Segment : -1;
            Current.ApexAtTrackCm =
                track.Valid ? track.DistanceAlongCm : 0.0;
            Current.ApexObserved = true;
            Phase = CornerPhase::Apex;
        }
        else if (Current.ApexObserved &&
                 speed > Current.ApexSpeedCmS *
                         (1.0 + Settings.ApexSpeedRecoveryFraction))
        {
            Phase = CornerPhase::Unwinding;
        }
        else if (Phase != CornerPhase::Unwinding)
        {
            Phase = CornerPhase::TurnIn;
        }
        if (settling)
        {
            StableExitSeconds += dt;
            Current.SettlingSeconds += dt;
        }
        else
        {
            StableExitSeconds = 0.0;
        }
        Live.Active = true;
        Live.Direction = Current.Direction;
        Live.Phase = Phase;
        Live.ElapsedInCornerSeconds = Current.DurationSeconds;
        Live.CandidateApex = Current.ApexObserved;
        if (StableExitSeconds >= Settings.ExitStableSeconds ||
            Current.DurationSeconds >= Settings.MaximumCornerSeconds)
        {
            Finish();
        }
        return true;
    }

    CornerLiveMetrics LiveMetrics() const noexcept { return Live; }
    CornerEventRecord ActiveEvent() const noexcept { return Current; }
    CornerEventRecord LastEvent() const noexcept { return LastFinished; }
    bool HasActiveEvent() const noexcept { return Active; }
    std::uint64_t CompletedEvents() const noexcept
    {
        return CompletedCount;
    }
    std::uint64_t QualifiedEvents() const noexcept
    {
        return ValidCount;
    }
    double MeanCornerScore() const noexcept
    {
        return Scores.Mean();
    }
    double MeanApexSpeedCmS() const noexcept
    {
        return ApexSpeeds.Mean();
    }
    double MeanExitSpeedCmS() const noexcept
    {
        return ExitSpeeds.Mean();
    }
    double MeanPeakSlipRadians() const noexcept
    {
        return Slips.Mean();
    }
    double TrackedCornerSeconds() const noexcept
    {
        return TotalCornerSeconds;
    }
    double OffTrackCornerSeconds() const noexcept
    {
        return TotalOffTrackSeconds;
    }
    double OversteerProxySeconds() const noexcept
    {
        return TotalOversteerSeconds;
    }

private:
    void ResetTransient() noexcept
    {
        Active = false;
        Phase = CornerPhase::Straight;
        Current = {};
        Live = {};
        StableExitSeconds = 0.0;
    }

    void Begin(
        const TelemetryFrame& frame,
        const TelemetryKinematics& motion,
        const TrackProjection& track) noexcept
    {
        Active = true;
        Phase = CornerPhase::Approaching;
        Current = {};
        Current.Ordinal = CompletedCount + 1;
        Current.EntryTimeSeconds = frame.MonotonicSeconds;
        Current.ExitTimeSeconds = frame.MonotonicSeconds;
        Current.EntrySpeedCmS = motion.SpeedCmS;
        Current.ExitSpeedCmS = motion.SpeedCmS;
        Current.ApexSpeedCmS = motion.SpeedCmS;
        Current.MinimumSpeedCmS = motion.SpeedCmS;
        Current.MaximumSpeedCmS = motion.SpeedCmS;
        Current.Direction = DetermineCornerDirection(
            motion.YawRateRadS,
            track.Valid ? track.CurvaturePerCm : 0.0,
            frame.Controls.Steering);
        Current.EntrySegment = track.Valid ? track.Segment : -1;
        Current.ExitSegment = Current.EntrySegment;
        Current.EntryAtTrackCm =
            track.Valid ? track.DistanceAlongCm : 0.0;
        Current.ExitAtTrackCm = Current.EntryAtTrackCm;
        Current.ApexSegment = Current.EntrySegment;
        Current.ApexAtTrackCm = Current.EntryAtTrackCm;
        StableExitSeconds = 0.0;
    }

    void Finish() noexcept
    {
        Current.ExitPhase = CornerPhase::Complete;
        Current.Valid = Current.DurationSeconds >=
            Settings.MinimumCornerSeconds &&
            Current.ApexObserved &&
            Current.DirectionReliable;
        Current.NormalizedLinePrecision = 1.0 - Clamp01(
            SafeRatio(Current.IntegratedLateralErrorCmSeconds,
                      Current.DurationSeconds *
                      Settings.MaximumLineErrorCm));
        Current.NormalizedApexEfficiency = Clamp01(
            SafeRatio(Current.ApexSpeedCmS,
                      std::max(1.0, Current.EntrySpeedCmS)));
        Current.NormalizedExitStability = 1.0 - Clamp01(
            SafeRatio(Current.OversteerProxySeconds,
                      Current.DurationSeconds));
        Current.CompositeReviewScore =
            CornerReviewScore(Current, Settings);
        CompletedCount = SaturatingIncrement(CompletedCount);
        if (Current.Valid)
        {
            ValidCount = SaturatingIncrement(ValidCount);
            Scores.Observe(Current.CompositeReviewScore);
            ApexSpeeds.Observe(Current.ApexSpeedCmS);
            ExitSpeeds.Observe(Current.ExitSpeedCmS);
            Slips.Observe(Current.PeakSlipAngleRadians);
            TotalCornerSeconds += Current.DurationSeconds;
            TotalOffTrackSeconds += Current.OffTrackSeconds;
        }
        LastFinished = Current;
        ResetTransient();
    }

    CornerAnalysisSettings Settings{};
    CornerEventRecord Current{};
    CornerEventRecord LastFinished{};
    CornerLiveMetrics Live{};
    RunningStatistics Scores;
    RunningStatistics ApexSpeeds;
    RunningStatistics ExitSpeeds;
    RunningStatistics Slips;
    double StableExitSeconds = 0.0;
    double TotalCornerSeconds = 0.0;
    double TotalOffTrackSeconds = 0.0;
    double TotalOversteerSeconds = 0.0;
    std::uint64_t CompletedCount = 0;
    std::uint64_t ValidCount = 0;
    bool Active = false;
    CornerPhase Phase = CornerPhase::Straight;
};

class CornerHistory final
{
public:
    static constexpr int MaximumUniqueCorners = 32;

    void Reset() noexcept
    {
        Last.fill({});
        Best.fill({});
        MeanScore.fill(0.0);
        MeanApexSpeed.fill(0.0);
        Counts.fill(0);
        BadExits.fill(0);
        MissedApexes.fill(0);
        OffTrackEpisodes.fill(0);
        TotalEntries = 0;
    }

    bool Record(int cornerIndex, CornerEventRecord event) noexcept
    {
        if (cornerIndex < 0 ||
            cornerIndex >= MaximumUniqueCorners ||
            !event.Valid || !Finite(event.CompositeReviewScore))
        {
            return false;
        }
        const std::size_t index =
            static_cast<std::size_t>(cornerIndex);
        if (Counts[index] >= 1000000)
        {
            return false;
        }
        ++Counts[index];
        const double n = static_cast<double>(Counts[index]);
        MeanScore[index] +=
            (event.CompositeReviewScore - MeanScore[index]) / n;
        MeanApexSpeed[index] +=
            (event.ApexSpeedCmS - MeanApexSpeed[index]) / n;
        if (event.ExitSpeedCmS < event.ApexSpeedCmS)
        {
            ++BadExits[index];
        }
        if (!event.ApexObserved)
        {
            ++MissedApexes[index];
        }
        if (event.OffTrackSeconds > 0.0)
        {
            ++OffTrackEpisodes[index];
        }
        if (!Best[index].Valid ||
            event.CompositeReviewScore >
                Best[index].CompositeReviewScore)
        {
            Best[index] = event;
        }
        Last[index] = event;
        TotalEntries = SaturatingIncrement(TotalEntries);
        return true;
    }

    CornerEventRecord LastRecord(int cornerIndex) const noexcept
    {
        return ValidIndex(cornerIndex) ?
            Last[static_cast<std::size_t>(cornerIndex)] :
            CornerEventRecord{};
    }

    CornerEventRecord BestRecord(int cornerIndex) const noexcept
    {
        return ValidIndex(cornerIndex) ?
            Best[static_cast<std::size_t>(cornerIndex)] :
            CornerEventRecord{};
    }

    double AverageScore(int cornerIndex) const noexcept
    {
        return ValidIndex(cornerIndex) ?
            MeanScore[static_cast<std::size_t>(cornerIndex)] : 0.0;
    }

    double AverageApexSpeed(int cornerIndex) const noexcept
    {
        return ValidIndex(cornerIndex) ?
            MeanApexSpeed[static_cast<std::size_t>(cornerIndex)] : 0.0;
    }

    int SampleCount(int cornerIndex) const noexcept
    {
        return ValidIndex(cornerIndex) ?
            Counts[static_cast<std::size_t>(cornerIndex)] : 0;
    }

    double OffTrackRate(int cornerIndex) const noexcept
    {
        return ValidIndex(cornerIndex) ?
            SafeRatio(
                OffTrackEpisodes[static_cast<std::size_t>(cornerIndex)],
                Counts[static_cast<std::size_t>(cornerIndex)]) : 0.0;
    }

    double BadExitRate(int cornerIndex) const noexcept
    {
        return ValidIndex(cornerIndex) ?
            SafeRatio(
                BadExits[static_cast<std::size_t>(cornerIndex)],
                Counts[static_cast<std::size_t>(cornerIndex)]) : 0.0;
    }

    std::uint64_t TotalRecords() const noexcept
    {
        return TotalEntries;
    }

private:
    bool ValidIndex(int index) const noexcept
    {
        return index >= 0 &&
            index < MaximumUniqueCorners &&
            Counts[static_cast<std::size_t>(index)] > 0;
    }

    std::array<CornerEventRecord, MaximumUniqueCorners> Last{};
    std::array<CornerEventRecord, MaximumUniqueCorners> Best{};
    std::array<double, MaximumUniqueCorners> MeanScore{};
    std::array<double, MaximumUniqueCorners> MeanApexSpeed{};
    std::array<int, MaximumUniqueCorners> Counts{};
    std::array<int, MaximumUniqueCorners> BadExits{};
    std::array<int, MaximumUniqueCorners> MissedApexes{};
    std::array<int, MaximumUniqueCorners> OffTrackEpisodes{};
    std::uint64_t TotalEntries = 0;
};
} // namespace latebrake::engineering
