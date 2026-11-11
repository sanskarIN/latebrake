#pragma once

#include "Telemetry/TelemetrySessionTimeline.h"
#include "Telemetry/TelemetryKinematics.h"
#include "Telemetry/TelemetryGrip.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class LapObservationIssue : std::uint8_t
{
    None,
    InvalidFrame,
    InvalidMotion,
    InvalidTiming,
    WrongActor,
    StaleLap,
    SkippedLap,
    UnverifiedGate,
    ClockDiscontinuity,
    SessionReplaced,
    LapCapacity,
    ImplausibleDistance,
    NoActiveClock,
    IncompleteSector
};

struct LapAnalysisSettings final
{
    double MinimumLapSeconds = 3.0;
    double MaximumLapSeconds = 7200.0;
    double MinimumLapDistanceCm = 1000.0;
    double MaximumLapDistanceCm = 5000000.0;
    double MaximumFrameGapSeconds = MaximumFrameDeltaSeconds;
    double MaximumBrakePedalDisagreement = 0.40;
    double HighSlipRadians = 0.20;
    double StrongBrakingG = 0.45;
    double HighLateralG = 0.90;
    double MinimumMeasuredSpeedCmS = 10.0;
    double MinimumSectorSeconds = 0.02;
    int MaximumRecordedLaps = MaximumTelemetryLaps;
    bool RequireForwardGate = true;
    bool RequireMotion = true;
    bool RejectEstimatedClock = true;
};

inline bool ValidLapAnalysisSettings(
    const LapAnalysisSettings& s) noexcept
{
    return Finite(s.MinimumLapSeconds) && s.MinimumLapSeconds > 0.0 &&
           Finite(s.MaximumLapSeconds) &&
           s.MaximumLapSeconds > s.MinimumLapSeconds &&
           Finite(s.MinimumLapDistanceCm) &&
           s.MinimumLapDistanceCm >= 0.0 &&
           Finite(s.MaximumLapDistanceCm) &&
           s.MaximumLapDistanceCm > s.MinimumLapDistanceCm &&
           Finite(s.MaximumFrameGapSeconds) &&
           s.MaximumFrameGapSeconds > 0.0 &&
           s.MaximumFrameGapSeconds <= 1.0 &&
           Finite(s.MaximumBrakePedalDisagreement) &&
           s.MaximumBrakePedalDisagreement >= 0.0 &&
           Finite(s.HighSlipRadians) && s.HighSlipRadians > 0.0 &&
           Finite(s.StrongBrakingG) && s.StrongBrakingG > 0.0 &&
           Finite(s.HighLateralG) && s.HighLateralG > 0.0 &&
           Finite(s.MinimumMeasuredSpeedCmS) &&
           s.MinimumMeasuredSpeedCmS >= 0.0 &&
           Finite(s.MinimumSectorSeconds) &&
           s.MinimumSectorSeconds > 0.0 &&
           s.MaximumRecordedLaps > 0 &&
           s.MaximumRecordedLaps <= MaximumTelemetryLaps;
}

struct LapChannelAccumulator final
{
    double Seconds = 0.0;
    double DistanceCm = 0.0;
    double SpeedIntegralCm = 0.0;
    double TopSpeedCmS = 0.0;
    double MinimumMovingSpeedCmS = 0.0;
    double ThrottleSeconds = 0.0;
    double BrakingSeconds = 0.0;
    double CoastingSeconds = 0.0;
    double FullThrottleSeconds = 0.0;
    double HeavyBrakeSeconds = 0.0;
    double TrailBrakeSeconds = 0.0;
    double HandbrakeSeconds = 0.0;
    double HighSlipSeconds = 0.0;
    double HighLateralGSeconds = 0.0;
    double WheelspinSeconds = 0.0;
    double OffRoadSeconds = 0.0;
    double PitLaneSeconds = 0.0;
    double CautionSeconds = 0.0;
    double EngineTemperatureIntegral = 0.0;
    double BrakeTemperatureIntegral = 0.0;
    double TireWearIntegral = 0.0;
    double LateralGIntegral = 0.0;
    double LongitudinalGIntegral = 0.0;
    double SteeringIntegral = 0.0;
    double ThrottleIntegral = 0.0;
    double BrakeIntegral = 0.0;
    double PeakLateralG = 0.0;
    double PeakBrakingG = 0.0;
    double PeakBrakeTemperatureC = 0.0;
    double PeakEngineTemperatureC = 0.0;
    double PeakSteering = 0.0;
    int BrakeApplications = 0;
    int ThrottleApplications = 0;
    int OffRoadEntries = 0;
    int ConfirmedRecoveries = 0;
    int DataDiscontinuities = 0;
    std::uint64_t FrameCount = 0;
    bool LastBraking = false;
    bool LastAccelerating = false;
    bool LastOffRoad = false;
    bool HasMovingSpeed = false;
};

struct LapObservation final
{
    LapObservationIssue Issue = LapObservationIssue::None;
    TelemetryLap Completed{};
    LapChannelAccumulator Channels{};
    int CurrentLapIndex = -1;
    int CompletedLapIndex = -1;
    int RecordedCount = 0;
    std::uint64_t AcceptedSamples = 0;
    std::uint64_t RejectedSamples = 0;
    bool Accepted = false;
    bool BeganNewLap = false;
    bool CompletedLap = false;
    bool HasVerifiedLap = false;
    bool SessionReset = false;
    bool RaceClockAuthoritative = false;
};

inline double LapMean(
    double integral, double seconds) noexcept
{
    return SafeRatio(integral, seconds);
}

inline double LapFraction(
    double selectedSeconds, double totalSeconds) noexcept
{
    return Clamp01(SafeRatio(selectedSeconds, totalSeconds));
}

class LapMeasurementJournal final
{
public:
    bool Configure(LapAnalysisSettings settings) noexcept
    {
        if (!ValidLapAnalysisSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Completed.fill({});
        Working = {};
        Channels = {};
        Last = {};
        Recorded = 0;
        CurrentLap = -1;
        AcceptedCount = 0;
        RejectedCount = 0;
        FirstLapClockSeconds = 0.0;
        LastClockSeconds = 0.0;
        LastMonotonicSeconds = 0.0;
        SessionToken = 0;
        ActorToken = 0;
        Revision = -1;
        HasClock = false;
        LastRecovery = false;
        HasActiveLap = false;
    }

    void Suspend() noexcept
    {
        HasClock = false;
        ++Channels.DataDiscontinuities;
        Last = {};
    }

    LapObservation Observe(
        const TelemetryFrame& frame,
        const TelemetryFrameDecision& decision,
        const TelemetryKinematics& motion) noexcept
    {
        LapObservation out;
        out.AcceptedSamples = AcceptedCount;
        out.RejectedSamples = RejectedCount;
        out.RecordedCount = Recorded;
        if (!decision.Accepted ||
            !IsEngineObservation(frame.Identity.Trust) ||
            !frame.Race.EnginePhysicsReady)
        {
            return Reject(LapObservationIssue::InvalidFrame, out);
        }
        if (Settings.RequireMotion && !motion.Valid)
        {
            return Reject(LapObservationIssue::InvalidMotion, out);
        }
        const double dt = decision.ValidatedDeltaSeconds;
        if (!Finite(dt) || dt <= 0.0 ||
            dt > Settings.MaximumFrameGapSeconds ||
            !Finite(frame.RaceClockSeconds) ||
            !Finite(frame.MonotonicSeconds) ||
            frame.RaceClockSeconds < 0.0 ||
            frame.MonotonicSeconds < 0.0)
        {
            return Reject(LapObservationIssue::InvalidTiming, out);
        }
        if (!frame.Race.LocalRaceClockRunning ||
            !IsRaceMoving(frame.Race.Phase))
        {
            return Reject(LapObservationIssue::NoActiveClock, out);
        }
        const bool changedSession =
            SessionToken != 0 &&
            (SessionToken != frame.Identity.SessionToken ||
             ActorToken != frame.Identity.ActorToken ||
             Revision != frame.Identity.SessionRevision);
        if (changedSession)
        {
            Reset();
            out.SessionReset = true;
        }
        if (SessionToken == 0)
        {
            SessionToken = frame.Identity.SessionToken;
            ActorToken = frame.Identity.ActorToken;
            Revision = frame.Identity.SessionRevision;
        }
        if (CurrentLap < 0)
        {
            StartLap(frame);
            out.BeganNewLap = true;
        }
        if (frame.Race.LapIndex < CurrentLap)
        {
            return Reject(LapObservationIssue::StaleLap, out);
        }
        if (frame.Race.LapIndex > CurrentLap + 1)
        {
            return Reject(LapObservationIssue::SkippedLap, out);
        }
        if (HasClock &&
            (frame.RaceClockSeconds < LastClockSeconds ||
             frame.MonotonicSeconds <= LastMonotonicSeconds))
        {
            Suspend();
            return Reject(LapObservationIssue::ClockDiscontinuity,
                          out);
        }
        if (frame.Race.LapIndex == CurrentLap + 1)
        {
            if (Settings.RequireForwardGate &&
                !frame.Race.VerifiedForwardGate)
            {
                return Reject(LapObservationIssue::UnverifiedGate, out);
            }
            if (Recorded >= Settings.MaximumRecordedLaps)
            {
                return Reject(LapObservationIssue::LapCapacity, out);
            }
            const double officialDuration =
                frame.RaceClockSeconds - FirstLapClockSeconds;
            if (!Finite(officialDuration) ||
                officialDuration < Settings.MinimumLapSeconds ||
                officialDuration > Settings.MaximumLapSeconds)
            {
                return Reject(LapObservationIssue::InvalidTiming, out);
            }
            if (Channels.DistanceCm < Settings.MinimumLapDistanceCm ||
                Channels.DistanceCm > Settings.MaximumLapDistanceCm)
            {
                return Reject(LapObservationIssue::ImplausibleDistance,
                              out);
            }
            CompleteLap(frame, officialDuration);
            out.Completed = Completed[static_cast<std::size_t>(
                Recorded - 1)];
            out.CompletedLap = true;
            out.CompletedLapIndex = out.Completed.LapIndex;
            out.HasVerifiedLap = true;
            StartLap(frame);
            out.BeganNewLap = true;
        }
        Integrate(frame, motion, dt);
        LastClockSeconds = frame.RaceClockSeconds;
        LastMonotonicSeconds = frame.MonotonicSeconds;
        HasClock = true;
        AcceptedCount = SaturatingIncrement(AcceptedCount);
        out.Issue = LapObservationIssue::None;
        out.Accepted = true;
        out.RaceClockAuthoritative = true;
        out.CurrentLapIndex = CurrentLap;
        out.RecordedCount = Recorded;
        out.AcceptedSamples = AcceptedCount;
        out.RejectedSamples = RejectedCount;
        out.Channels = Channels;
        Last = out;
        return out;
    }

    LapObservation LastObservation() const noexcept { return Last; }
    int RecordedLaps() const noexcept { return Recorded; }
    int ObservedLapIndex() const noexcept { return CurrentLap; }
    std::uint64_t AcceptedSamples() const noexcept { return AcceptedCount; }
    std::uint64_t RejectedSamples() const noexcept { return RejectedCount; }
    LapChannelAccumulator LiveChannels() const noexcept
    {
        return Channels;
    }
    TelemetryLap RecordedLap(int index) const noexcept
    {
        return index >= 0 && index < Recorded ?
            Completed[static_cast<std::size_t>(index)] :
            TelemetryLap{};
    }
    TelemetryLap LatestCompletedLap() const noexcept
    {
        return Recorded > 0 ?
            Completed[static_cast<std::size_t>(Recorded - 1)] :
            TelemetryLap{};
    }
    double BestLapSeconds() const noexcept
    {
        double best = std::numeric_limits<double>::infinity();
        for (int i = 0; i < Recorded; ++i)
        {
            const TelemetryLap& lap =
                Completed[static_cast<std::size_t>(i)];
            if (lap.Quality == TelemetryQuality::Qualified &&
                lap.DurationSeconds > 0.0)
            {
                best = std::min(best, lap.DurationSeconds);
            }
        }
        return Finite(best) ? best : 0.0;
    }
    double MeanLapSeconds() const noexcept
    {
        double sum = 0.0;
        int count = 0;
        for (int i = 0; i < Recorded; ++i)
        {
            const TelemetryLap& lap =
                Completed[static_cast<std::size_t>(i)];
            if (lap.Quality == TelemetryQuality::Qualified &&
                lap.DurationSeconds > 0.0)
            {
                sum += lap.DurationSeconds;
                ++count;
            }
        }
        return count ? sum / static_cast<double>(count) : 0.0;
    }
    double TheoreticalBestSectorSum() const noexcept
    {
        std::array<double, MaximumTelemetrySectors> best{};
        int largestSector = 0;
        for (int i = 0; i < Recorded; ++i)
        {
            const TelemetryLap& lap =
                Completed[static_cast<std::size_t>(i)];
            if (lap.Quality != TelemetryQuality::Qualified)
            {
                continue;
            }
            largestSector = std::max(largestSector, lap.SectorCount);
            for (int s = 0; s < lap.SectorCount; ++s)
            {
                const double seconds =
                    lap.SectorTimes[static_cast<std::size_t>(s)];
                if (seconds > Settings.MinimumSectorSeconds &&
                    (!best[static_cast<std::size_t>(s)] ||
                     seconds < best[static_cast<std::size_t>(s)]))
                {
                    best[static_cast<std::size_t>(s)] = seconds;
                }
            }
        }
        if (!largestSector)
        {
            return 0.0;
        }
        double sum = 0.0;
        for (int i = 0; i < largestSector; ++i)
        {
            if (best[static_cast<std::size_t>(i)] <= 0.0)
            {
                return 0.0;
            }
            sum += best[static_cast<std::size_t>(i)];
        }
        return sum;
    }

private:
    LapObservation Reject(
        LapObservationIssue issue, LapObservation out) noexcept
    {
        RejectedCount = SaturatingIncrement(RejectedCount);
        out.Issue = issue;
        out.Accepted = false;
        out.RejectedSamples = RejectedCount;
        Last = out;
        return out;
    }

    void StartLap(const TelemetryFrame& frame) noexcept
    {
        CurrentLap = frame.Race.LapIndex;
        FirstLapClockSeconds = frame.RaceClockSeconds;
        Working = {};
        Working.LapIndex = CurrentLap;
        Working.StartedAtSeconds = frame.RaceClockSeconds;
        Working.Quality = TelemetryQuality::Missing;
        Channels = {};
        HasActiveLap = true;
    }

    void Integrate(
        const TelemetryFrame& frame,
        const TelemetryKinematics& motion,
        double dt) noexcept
    {
        const double speed =
            std::max(0.0, motion.Valid ?
                         motion.SpeedCmS : PlanarSpeed(frame));
        const double brakingG = std::max(0.0,
            -motion.LongitudinalAccelerationCmS2 / GravityCmS2);
        const double lateralG = std::abs(
            motion.LateralAccelerationCmS2 / GravityCmS2);
        const double throttle = frame.Controls.Throttle;
        const double brake = frame.Controls.Brake;
        const bool braking = brake > 0.12;
        const bool accelerating = throttle > 0.14;
        const bool offroad =
            !frame.Environment.OnPhysicalPavement;
        const bool newRecovery = frame.Race.VerifiedRecovery &&
            !LastRecovery;
        Channels.Seconds += dt;
        Channels.DistanceCm += speed * dt;
        Channels.SpeedIntegralCm += speed * dt;
        Channels.TopSpeedCmS =
            std::max(Channels.TopSpeedCmS, speed);
        if (speed > Settings.MinimumMeasuredSpeedCmS)
        {
            Channels.MinimumMovingSpeedCmS =
                !Channels.HasMovingSpeed ? speed :
                    std::min(Channels.MinimumMovingSpeedCmS, speed);
            Channels.HasMovingSpeed = true;
        }
        Channels.ThrottleIntegral += throttle * dt;
        Channels.BrakeIntegral += brake * dt;
        Channels.SteeringIntegral +=
            std::abs(frame.Controls.Steering) * dt;
        Channels.ThrottleSeconds += accelerating ? dt : 0.0;
        Channels.BrakingSeconds += braking ? dt : 0.0;
        Channels.CoastingSeconds +=
            !braking && !accelerating ? dt : 0.0;
        Channels.FullThrottleSeconds += throttle >= 0.95 ? dt : 0.0;
        Channels.HeavyBrakeSeconds +=
            brakingG >= Settings.StrongBrakingG ? dt : 0.0;
        Channels.TrailBrakeSeconds +=
            braking && std::abs(frame.Controls.Steering) > 0.2 ?
                dt : 0.0;
        Channels.HandbrakeSeconds +=
            frame.Controls.Handbrake > 0.15 ? dt : 0.0;
        Channels.HighSlipSeconds +=
            std::abs(motion.SlipAngleRadians) >
                    Settings.HighSlipRadians ? dt : 0.0;
        Channels.HighLateralGSeconds +=
            lateralG >= Settings.HighLateralG ? dt : 0.0;
        Channels.OffRoadSeconds += offroad ? dt : 0.0;
        Channels.PitLaneSeconds +=
            frame.Environment.InPitLane ? dt : 0.0;
        Channels.CautionSeconds +=
            frame.Race.Phase == TelemetryPhase::Caution ? dt : 0.0;
        Channels.EngineTemperatureIntegral +=
            frame.Dynamics.EngineTemperatureCelsius * dt;
        Channels.BrakeTemperatureIntegral +=
            frame.Dynamics.BrakeTemperatureCelsius * dt;
        Channels.TireWearIntegral +=
            MeanWheelWear(frame.Wheels) * dt;
        Channels.LateralGIntegral += lateralG * dt;
        Channels.LongitudinalGIntegral += brakingG * dt;
        Channels.PeakLateralG = std::max(
            Channels.PeakLateralG, lateralG);
        Channels.PeakBrakingG = std::max(
            Channels.PeakBrakingG, brakingG);
        Channels.PeakBrakeTemperatureC = std::max(
            Channels.PeakBrakeTemperatureC,
            frame.Dynamics.BrakeTemperatureCelsius);
        Channels.PeakEngineTemperatureC = std::max(
            Channels.PeakEngineTemperatureC,
            frame.Dynamics.EngineTemperatureCelsius);
        Channels.PeakSteering = std::max(
            Channels.PeakSteering,
            std::abs(frame.Controls.Steering));
        if (braking && !Channels.LastBraking)
        {
            ++Channels.BrakeApplications;
        }
        if (accelerating && !Channels.LastAccelerating)
        {
            ++Channels.ThrottleApplications;
        }
        if (offroad && !Channels.LastOffRoad)
        {
            ++Channels.OffRoadEntries;
        }
        if (newRecovery)
        {
            ++Channels.ConfirmedRecoveries;
        }
        Channels.LastBraking = braking;
        Channels.LastAccelerating = accelerating;
        Channels.LastOffRoad = offroad;
        LastRecovery = frame.Race.VerifiedRecovery;
        Channels.FrameCount =
            SaturatingIncrement(Channels.FrameCount);
    }

    void CompleteLap(
        const TelemetryFrame& frame,
        double officialDuration) noexcept
    {
        TelemetryLap lap;
        lap.LapIndex = CurrentLap;
        lap.StartedAtSeconds = FirstLapClockSeconds;
        lap.FinishedAtSeconds = frame.RaceClockSeconds;
        lap.DurationSeconds = officialDuration;
        lap.DistanceCm = Channels.DistanceCm;
        lap.TopSpeedKmh =
            Channels.TopSpeedCmS * CmPerSecondToKmh;
        lap.MeanSpeedKmh = SafeRatio(
            Channels.DistanceCm, Channels.Seconds) *
            CmPerSecondToKmh;
        lap.TotalBrakingSeconds = Channels.BrakingSeconds;
        lap.TotalCoastingSeconds = Channels.CoastingSeconds;
        lap.TotalOffTrackSeconds = Channels.OffRoadSeconds;
        lap.TotalHighSlipSeconds = Channels.HighSlipSeconds;
        lap.TotalThrottleSeconds = Channels.ThrottleSeconds;
        lap.TotalWheelspinSeconds = Channels.WheelspinSeconds;
        lap.TotalTractionLimitedSeconds =
            Channels.HighLateralGSeconds;
        lap.TotalBrakeTemperatureLoad =
            Channels.BrakeTemperatureIntegral;
        lap.BrakeApplications = Channels.BrakeApplications;
        lap.RecoveryCount = Channels.ConfirmedRecoveries;
        lap.Quality = Channels.DataDiscontinuities == 0 ?
            TelemetryQuality::Qualified :
            TelemetryQuality::Suspect;
        Completed[static_cast<std::size_t>(Recorded)] = lap;
        ++Recorded;
    }

    LapAnalysisSettings Settings{};
    std::array<TelemetryLap, MaximumTelemetryLaps> Completed{};
    TelemetryLap Working{};
    LapChannelAccumulator Channels{};
    LapObservation Last{};
    int Recorded = 0;
    int CurrentLap = -1;
    std::uint64_t AcceptedCount = 0;
    std::uint64_t RejectedCount = 0;
    double FirstLapClockSeconds = 0.0;
    double LastClockSeconds = 0.0;
    double LastMonotonicSeconds = 0.0;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    int Revision = -1;
    bool HasClock = false;
    bool LastRecovery = false;
    bool HasActiveLap = false;
};
} // namespace latebrake::engineering
