#pragma once

#include "Telemetry/TelemetryKinematics.h"
#include "Telemetry/TelemetryTrack.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class BrakePhase : std::uint8_t
{
    Idle,
    Attack,
    Sustained,
    Trail,
    Release
};

struct BrakeAnalysisSettings final
{
    double EnterPedal = 0.30;
    double ReleasePedal = 0.12;
    double TrailPedal = 0.42;
    double MinimumBrakingG = 0.08;
    double StrongBrakingG = 0.55;
    double MinimumEventSeconds = 0.12;
    double ReleaseGraceSeconds = 0.15;
    double MaximumEventSeconds = 15.0;
    double LockingSlipRatio = -0.22;
    double HeavyBrakeTemperatureC = 480.0;
    double CriticalBrakeTemperatureC = 750.0;
    double MinimumEventStartSpeedCmS = 300.0;
    double BrakeAccelerationReferenceG = 0.85;
};

inline bool ValidBrakeAnalysisSettings(
    const BrakeAnalysisSettings& s) noexcept
{
    return Finite(s.EnterPedal) &&
           Finite(s.ReleasePedal) &&
           Finite(s.TrailPedal) &&
           Finite(s.MinimumBrakingG) &&
           Finite(s.StrongBrakingG) &&
           Finite(s.MinimumEventSeconds) &&
           Finite(s.ReleaseGraceSeconds) &&
           Finite(s.MaximumEventSeconds) &&
           Finite(s.LockingSlipRatio) &&
           Finite(s.HeavyBrakeTemperatureC) &&
           Finite(s.CriticalBrakeTemperatureC) &&
           Finite(s.MinimumEventStartSpeedCmS) &&
           Finite(s.BrakeAccelerationReferenceG) &&
           s.EnterPedal > s.ReleasePedal &&
           s.EnterPedal <= 1.0 &&
           s.ReleasePedal >= 0.0 &&
           s.TrailPedal >= s.ReleasePedal &&
           s.TrailPedal <= 1.0 &&
           s.MinimumBrakingG >= 0.0 &&
           s.StrongBrakingG > s.MinimumBrakingG &&
           s.MinimumEventSeconds > 0.0 &&
           s.ReleaseGraceSeconds >= 0.0 &&
           s.MaximumEventSeconds > s.MinimumEventSeconds &&
           s.HeavyBrakeTemperatureC > 0.0 &&
           s.CriticalBrakeTemperatureC > s.HeavyBrakeTemperatureC &&
           s.MinimumEventStartSpeedCmS >= 0.0 &&
           s.BrakeAccelerationReferenceG > 0.0;
}

struct BrakeEventRecord final
{
    std::uint64_t EventOrdinal = 0;
    BrakePhase ExitPhase = BrakePhase::Idle;
    double StartedSeconds = 0.0;
    double EndedSeconds = 0.0;
    double DurationSeconds = 0.0;
    double StartingSpeedCmS = 0.0;
    double EndingSpeedCmS = 0.0;
    double LowestSpeedCmS = 0.0;
    double PeakPedal = 0.0;
    double MeanPedal = 0.0;
    double PeakDecelerationG = 0.0;
    double MeanDecelerationG = 0.0;
    double StrongBrakingSeconds = 0.0;
    double TrailBrakingSeconds = 0.0;
    double WheelLockSeconds = 0.0;
    double ElevatedHeatSeconds = 0.0;
    double DistanceCm = 0.0;
    double LostKineticEnergyPerKg = 0.0;
    double MaximumSteeringMagnitude = 0.0;
    double PeakBrakeTemperatureC = 0.0;
    double StartedAtTrackCm = 0.0;
    double EndedAtTrackCm = 0.0;
    int StartedSegment = -1;
    int EndedSegment = -1;
    bool HighHeat = false;
    bool Overheating = false;
    bool WheelLockObserved = false;
    bool Completed = false;
    bool Valid = false;
};

struct BrakeLiveMetrics final
{
    BrakePhase Phase = BrakePhase::Idle;
    double CurrentPedal = 0.0;
    double DecelerationG = 0.0;
    double SmoothedDecelerationG = 0.0;
    double CurrentSpeedCmS = 0.0;
    double CurrentBrakeTemperatureC = 0.0;
    double EventSeconds = 0.0;
    double ReleaseGraceRemainingSeconds = 0.0;
    double CurrentStopDistanceCm = 0.0;
    double BrakePowerPerKg = 0.0;
    double CurrentWheelLockFraction = 0.0;
    bool CurrentlyBraking = false;
    bool TrailBraking = false;
    bool WheelLocking = false;
    bool HeatLimited = false;
    bool Valid = false;
};

class BrakeEventMonitor final
{
public:
    void Reset() noexcept
    {
        Settings = BrakeAnalysisSettings{};
        ResetState();
        EventCount = 0;
        LastFinished = {};
        TotalBrakingSeconds = 0.0;
        TotalTrailSeconds = 0.0;
        TotalLockSeconds = 0.0;
        TotalDistanceCm = 0.0;
        TotalEnergyPerKg = 0.0;
        AccelerationSmooth.Reset();
        EventDurationStats.Reset();
        EventDecelerationStats.Reset();
    }

    bool Configure(BrakeAnalysisSettings settings) noexcept
    {
        if (!ValidBrakeAnalysisSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    void Suspend() noexcept
    {
        ResetState();
        AccelerationSmooth.Reset();
    }

    bool Observe(const TelemetryFrame& frame,
                 const TelemetryKinematics& motion,
                 const TrackProjection& track,
                 double dt) noexcept
    {
        if (!motion.Valid || !Finite(dt) || dt <= 0.0 ||
            dt > MaximumFrameDeltaSeconds ||
            !Finite(frame.Controls.Brake) ||
            !Finite(frame.Dynamics.BrakeTemperatureCelsius) ||
            !Finite(motion.SpeedCmS) ||
            !Finite(motion.LongitudinalAccelerationCmS2))
        {
            Suspend();
            return false;
        }
        const double pedal = std::clamp(frame.Controls.Brake, 0.0, 1.0);
        const double deceleration = std::max(
            0.0, -motion.LongitudinalAccelerationCmS2 / GravityCmS2);
        const double smoothDeceleration =
            AccelerationSmooth.Observe(deceleration, dt, 0.10);
        const double speed = motion.SpeedCmS;
        double locks = 0.0;
        double wheels = 0.0;
        for (int i = 0; i < 4; ++i)
        {
            const std::size_t n = static_cast<std::size_t>(i);
            if (!frame.Wheels.Contact[n])
            {
                continue;
            }
            ++wheels;
            if (Finite(frame.Wheels.SlipRatio[n]) &&
                frame.Wheels.SlipRatio[n] < Settings.LockingSlipRatio)
            {
                ++locks;
            }
        }
        const double lockingFraction =
            wheels > 0.0 ? locks / wheels : 0.0;
        const bool activelyPressed = pedal >= Settings.EnterPedal;
        const bool sustaining = pedal > Settings.ReleasePedal;
        const bool brakingPhysics = deceleration >= Settings.MinimumBrakingG;
        const bool wheelLocking = lockingFraction > 0.0 &&
            sustaining && speed > 200.0;
        const bool trail = sustaining && pedal <= Settings.TrailPedal &&
            std::abs(frame.Controls.Steering) > 0.20;
        const double powerPerKg = deceleration * GravityCmS2 * speed *
            0.01;

        Live.CurrentPedal = pedal;
        Live.DecelerationG = deceleration;
        Live.SmoothedDecelerationG = smoothDeceleration;
        Live.CurrentSpeedCmS = speed;
        Live.CurrentBrakeTemperatureC =
            frame.Dynamics.BrakeTemperatureCelsius;
        Live.CurrentStopDistanceCm = BrakingDistance(
            speed, std::max(smoothDeceleration, 0.15) * GravityCmS2, 0.18);
        Live.BrakePowerPerKg = powerPerKg;
        Live.CurrentWheelLockFraction = lockingFraction;
        Live.WheelLocking = wheelLocking;
        Live.HeatLimited = frame.Dynamics.BrakeTemperatureCelsius >=
            Settings.HeavyBrakeTemperatureC;
        Live.Valid = true;

        if (!HasActive)
        {
            Live.Phase = BrakePhase::Idle;
            Live.CurrentlyBraking = false;
            Live.TrailBraking = false;
            Live.EventSeconds = 0.0;
            Live.ReleaseGraceRemainingSeconds = 0.0;
            if (!activelyPressed ||
                speed < Settings.MinimumEventStartSpeedCmS)
            {
                return true;
            }
            Begin(frame, motion, track);
        }
        if (!HasActive)
        {
            return true;
        }
        const bool released = !sustaining;
        if (released)
        {
            ReleaseSeconds += dt;
            Phase = BrakePhase::Release;
        }
        else
        {
            ReleaseSeconds = 0.0;
            if (trail)
            {
                Phase = BrakePhase::Trail;
            }
            else if (smoothDeceleration >= Settings.StrongBrakingG)
            {
                Phase = BrakePhase::Attack;
            }
            else
            {
                Phase = BrakePhase::Sustained;
            }
        }
        Current.EndedSeconds = frame.MonotonicSeconds;
        Current.EndingSpeedCmS = speed;
        Current.LowestSpeedCmS =
            std::min(Current.LowestSpeedCmS, speed);
        Current.DurationSeconds += dt;
        Current.PeakPedal = std::max(Current.PeakPedal, pedal);
        Current.PeakDecelerationG =
            std::max(Current.PeakDecelerationG, deceleration);
        Current.PeakBrakeTemperatureC =
            std::max(Current.PeakBrakeTemperatureC,
                     frame.Dynamics.BrakeTemperatureCelsius);
        Current.MaximumSteeringMagnitude =
            std::max(Current.MaximumSteeringMagnitude,
                     std::abs(frame.Controls.Steering));
        Current.DistanceCm += speed * dt;
        Current.EndedSegment = track.Valid ? track.Segment : -1;
        Current.EndedAtTrackCm =
            track.Valid ? track.DistanceAlongCm : 0.0;
        PressureIntegral += pedal * dt;
        DecelerationIntegral += deceleration * dt;
        if (deceleration >= Settings.StrongBrakingG)
        {
            Current.StrongBrakingSeconds += dt;
        }
        if (trail)
        {
            Current.TrailBrakingSeconds += dt;
            TotalTrailSeconds += dt;
        }
        if (wheelLocking)
        {
            Current.WheelLockSeconds += dt;
            TotalLockSeconds += dt;
            Current.WheelLockObserved = true;
        }
        if (Live.HeatLimited)
        {
            Current.ElevatedHeatSeconds += dt;
            Current.HighHeat = true;
        }
        if (Current.PeakBrakeTemperatureC >=
            Settings.CriticalBrakeTemperatureC)
        {
            Current.Overheating = true;
        }
        if (sustaining && brakingPhysics)
        {
            TotalBrakingSeconds += dt;
        }
        Live.Phase = Phase;
        Live.CurrentlyBraking = sustaining;
        Live.TrailBraking = trail;
        Live.EventSeconds = Current.DurationSeconds;
        Live.ReleaseGraceRemainingSeconds =
            std::max(0.0, Settings.ReleaseGraceSeconds - ReleaseSeconds);
        if (ReleaseSeconds >= Settings.ReleaseGraceSeconds ||
            Current.DurationSeconds >= Settings.MaximumEventSeconds)
        {
            Finish();
        }
        return true;
    }

    bool HasActiveEvent() const noexcept { return HasActive; }
    BrakeLiveMetrics CurrentMetrics() const noexcept { return Live; }
    BrakeEventRecord CurrentEvent() const noexcept { return Current; }
    BrakeEventRecord LastEvent() const noexcept { return LastFinished; }
    std::uint64_t RecordedEvents() const noexcept { return EventCount; }
    double BrakingSeconds() const noexcept { return TotalBrakingSeconds; }
    double TrailSeconds() const noexcept { return TotalTrailSeconds; }
    double WheelLockSeconds() const noexcept { return TotalLockSeconds; }
    double DistanceUnderBrakeCm() const noexcept
    {
        return TotalDistanceCm;
    }
    double LostKineticEnergyPerKg() const noexcept
    {
        return TotalEnergyPerKg;
    }
    double AverageEventDuration() const noexcept
    {
        return EventDurationStats.Mean();
    }
    double AverageEventDecelerationG() const noexcept
    {
        return EventDecelerationStats.Mean();
    }

private:
    void ResetState() noexcept
    {
        HasActive = false;
        Phase = BrakePhase::Idle;
        ReleaseSeconds = 0.0;
        PressureIntegral = 0.0;
        DecelerationIntegral = 0.0;
        Current = {};
        Live = {};
    }

    void Begin(const TelemetryFrame& frame,
               const TelemetryKinematics& motion,
               const TrackProjection& track) noexcept
    {
        HasActive = true;
        Phase = BrakePhase::Attack;
        Current = {};
        Current.EventOrdinal = EventCount + 1;
        Current.StartedSeconds = frame.MonotonicSeconds;
        Current.EndedSeconds = frame.MonotonicSeconds;
        Current.StartingSpeedCmS = motion.SpeedCmS;
        Current.EndingSpeedCmS = motion.SpeedCmS;
        Current.LowestSpeedCmS = motion.SpeedCmS;
        Current.StartedSegment = track.Valid ? track.Segment : -1;
        Current.EndedSegment = Current.StartedSegment;
        Current.StartedAtTrackCm =
            track.Valid ? track.DistanceAlongCm : 0.0;
        Current.EndedAtTrackCm = Current.StartedAtTrackCm;
        ReleaseSeconds = 0.0;
        PressureIntegral = 0.0;
        DecelerationIntegral = 0.0;
    }

    void Finish() noexcept
    {
        Current.ExitPhase = Phase;
        Current.MeanPedal = SafeRatio(
            PressureIntegral, Current.DurationSeconds);
        Current.MeanDecelerationG = SafeRatio(
            DecelerationIntegral, Current.DurationSeconds);
        const double entry = Current.StartingSpeedCmS * 0.01;
        const double exit = Current.EndingSpeedCmS * 0.01;
        Current.LostKineticEnergyPerKg =
            0.5 * std::max(0.0, entry * entry - exit * exit);
        Current.Completed = Current.DurationSeconds >=
            Settings.MinimumEventSeconds;
        Current.Valid = Current.Completed &&
            Current.StartingSpeedCmS >=
            Settings.MinimumEventStartSpeedCmS &&
            Current.PeakPedal >= Settings.EnterPedal;
        if (Current.Valid)
        {
            LastFinished = Current;
            EventCount = SaturatingIncrement(EventCount);
            TotalDistanceCm = SaturatingAdd(
                TotalDistanceCm, Current.DistanceCm);
            TotalEnergyPerKg = SaturatingAdd(
                TotalEnergyPerKg, Current.LostKineticEnergyPerKg);
            EventDurationStats.Observe(Current.DurationSeconds);
            EventDecelerationStats.Observe(Current.MeanDecelerationG);
        }
        ResetState();
    }

    BrakeAnalysisSettings Settings{};
    ExponentialFilter AccelerationSmooth;
    RunningStatistics EventDurationStats;
    RunningStatistics EventDecelerationStats;
    BrakeEventRecord Current{};
    BrakeEventRecord LastFinished{};
    BrakeLiveMetrics Live{};
    double ReleaseSeconds = 0.0;
    double PressureIntegral = 0.0;
    double DecelerationIntegral = 0.0;
    double TotalBrakingSeconds = 0.0;
    double TotalTrailSeconds = 0.0;
    double TotalLockSeconds = 0.0;
    double TotalDistanceCm = 0.0;
    double TotalEnergyPerKg = 0.0;
    std::uint64_t EventCount = 0;
    bool HasActive = false;
    BrakePhase Phase = BrakePhase::Idle;
};

struct BrakeZoneAssessment final
{
    double PlannedBrakeDistanceCm = 0.0;
    double ObservedBrakeDistanceCm = 0.0;
    double BrakeOffsetCm = 0.0;
    double PredictedStoppingDistanceCm = 0.0;
    double ConservativeTargetSpeedCmS = 0.0;
    double SafetyMarginCm = 0.0;
    double GripLimitedDecelerationCmS2 = 0.0;
    double ReactionDistanceCm = 0.0;
    double BrakeFadeFraction = 0.0;
    bool LateBraking = false;
    bool EarlyBraking = false;
    bool GripEstimateQualified = false;
    bool Valid = false;
};

inline BrakeZoneAssessment AssessBrakeZone(
    const BrakeEventRecord& event, const TelemetryZone& zone,
    double decelerationCmS2, double gripScale,
    double reactionSeconds = 0.20) noexcept
{
    BrakeZoneAssessment result;
    if (!event.Valid || !zone.Valid ||
        !Finite(decelerationCmS2) || !Finite(gripScale) ||
        !Finite(reactionSeconds) || decelerationCmS2 <= 0.0 ||
        gripScale <= 0.0 || gripScale > 2.0 ||
        reactionSeconds < 0.0 || reactionSeconds > 2.0)
    {
        return result;
    }
    result.GripEstimateQualified = true;
    result.GripLimitedDecelerationCmS2 =
        decelerationCmS2 * std::min(gripScale, 1.0);
    result.PlannedBrakeDistanceCm =
        std::max(0.0, zone.CenterCm - zone.StartCm);
    result.ObservedBrakeDistanceCm = event.DistanceCm;
    result.BrakeOffsetCm =
        event.StartedAtTrackCm - zone.StartCm;
    result.ReactionDistanceCm =
        event.StartingSpeedCmS * reactionSeconds;
    result.PredictedStoppingDistanceCm = BrakingDistance(
        event.StartingSpeedCmS,
        result.GripLimitedDecelerationCmS2,
        reactionSeconds);
    const double safety = std::max(
        0.0, zone.CenterCm - event.StartedAtTrackCm);
    result.SafetyMarginCm =
        safety - result.PredictedStoppingDistanceCm;
    result.ConservativeTargetSpeedCmS = SafeStoppingSpeed(
        safety, result.GripLimitedDecelerationCmS2);
    result.BrakeFadeFraction = Clamp01(
        (event.PeakBrakeTemperatureC - 450.0) / 450.0);
    result.LateBraking = result.SafetyMarginCm < -100.0;
    result.EarlyBraking = event.StartedAtTrackCm <
        zone.StartCm - 300.0;
    result.Valid = Finite(result.SafetyMarginCm) &&
        Finite(result.PredictedStoppingDistanceCm);
    return result;
}

class BrakeZoneHistory final
{
public:
    static constexpr int MaximumZones = 32;
    void Reset() noexcept
    {
        Records.fill({});
        Samples.fill(0);
        LateCounts.fill(0);
        EarlyCounts.fill(0);
        SumOffset.fill(0.0);
        SumDeceleration.fill(0.0);
        SumHeat.fill(0.0);
        Count = 0;
    }
    bool Record(int zone, BrakeZoneAssessment review,
                BrakeEventRecord event) noexcept
    {
        if (zone < 0 || zone >= MaximumZones ||
            !review.Valid || !event.Valid)
        {
            return false;
        }
        const std::size_t i = static_cast<std::size_t>(zone);
        if (Samples[i] >= 1000000)
        {
            return false;
        }
        ++Samples[i];
        LateCounts[i] += review.LateBraking ? 1 : 0;
        EarlyCounts[i] += review.EarlyBraking ? 1 : 0;
        SumOffset[i] += review.BrakeOffsetCm;
        SumDeceleration[i] += event.MeanDecelerationG;
        SumHeat[i] += event.PeakBrakeTemperatureC;
        Records[i] = event;
        Count = std::max(Count, zone + 1);
        return true;
    }
    int ZoneCount() const noexcept { return Count; }
    BrakeEventRecord Latest(int zone) const noexcept
    {
        return ValidZone(zone) ?
            Records[static_cast<std::size_t>(zone)] : BrakeEventRecord{};
    }
    double MeanBrakeOffsetCm(int zone) const noexcept
    {
        return ValidZone(zone) ?
            SafeRatio(SumOffset[static_cast<std::size_t>(zone)],
                      Samples[static_cast<std::size_t>(zone)]) : 0.0;
    }
    double MeanBrakeDecelerationG(int zone) const noexcept
    {
        return ValidZone(zone) ?
            SafeRatio(SumDeceleration[static_cast<std::size_t>(zone)],
                      Samples[static_cast<std::size_t>(zone)]) : 0.0;
    }
    double MeanPeakBrakeHeatC(int zone) const noexcept
    {
        return ValidZone(zone) ?
            SafeRatio(SumHeat[static_cast<std::size_t>(zone)],
                      Samples[static_cast<std::size_t>(zone)]) : 0.0;
    }
    int LateBrakeCount(int zone) const noexcept
    {
        return ValidZone(zone) ?
            LateCounts[static_cast<std::size_t>(zone)] : 0;
    }
    int EarlyBrakeCount(int zone) const noexcept
    {
        return ValidZone(zone) ?
            EarlyCounts[static_cast<std::size_t>(zone)] : 0;
    }
private:
    bool ValidZone(int zone) const noexcept
    {
        return zone >= 0 && zone < MaximumZones &&
            Samples[static_cast<std::size_t>(zone)] > 0;
    }
    std::array<BrakeEventRecord, MaximumZones> Records{};
    std::array<int, MaximumZones> Samples{};
    std::array<int, MaximumZones> LateCounts{};
    std::array<int, MaximumZones> EarlyCounts{};
    std::array<double, MaximumZones> SumOffset{};
    std::array<double, MaximumZones> SumDeceleration{};
    std::array<double, MaximumZones> SumHeat{};
    int Count = 0;
};
} // namespace latebrake::engineering
