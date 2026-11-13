#pragma once

#include "Telemetry/TelemetryFilters.h"
#include "Telemetry/TelemetryTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class ShiftQuality : std::uint8_t
{
    Unknown,
    Smooth,
    Abrupt,
    OverRev,
    Lugging,
    MissedShift,
    ShiftUnderBraking,
    UnsafeGearChange
};

struct DrivetrainSample final
{
    double TimeSeconds = 0.0;
    double EngineRpm = 0.0;
    double WheelSpeedCmS = 0.0;
    double LongitudinalAccelerationCmS2 = 0.0;
    double EngineTorqueNm = 0.0;
    double Throttle = 0.0;
    double Clutch = 0.0;
    double Brake = 0.0;
    double MotorTemperatureC = 0.0;
    int Gear = 0;
    bool TransmissionEngaged = false;
    bool ManualShiftRequested = false;
    bool EnginePhysicalReadout = false;
    bool SourceQualified = false;
};

struct TransmissionReviewSettings final
{
    int MaximumForwardGear = 12;
    double IdleRpm = 800.0;
    double RecommendedShiftRpm = 6500.0;
    double PeakPowerRpm = 7200.0;
    double RedlineRpm = 8200.0;
    double UnsafeOverspeedRpm = 9200.0;
    double LuggingRpm = 1100.0;
    double MinimumMotionSpeedCmS = 75.0;
    double MaximumPhysicalRpm = 30000.0;
    double MaximumFrameGapSeconds = 0.15;
    double MaximumRpmChangePerSecond = 80000.0;
    double ShiftTransientMaximumSeconds = 0.65;
    double StrongShiftShockCmS2 = 800.0;
    double AbruptClutchChangePerSecond = 8.0;
    double MinimumShiftThrottleOverlap = 0.30;
    double MaximumShiftBrakeOverlap = 0.35;
};

inline bool ValidTransmissionSettings(
    TransmissionReviewSettings s) noexcept
{
    return s.MaximumForwardGear >= 1 &&
        s.MaximumForwardGear <= 12 &&
        Finite(s.IdleRpm) && s.IdleRpm >= 0.0 &&
        Finite(s.RecommendedShiftRpm) &&
        s.RecommendedShiftRpm > s.IdleRpm &&
        Finite(s.PeakPowerRpm) &&
        s.PeakPowerRpm > s.RecommendedShiftRpm &&
        Finite(s.RedlineRpm) &&
        s.RedlineRpm > s.PeakPowerRpm &&
        Finite(s.UnsafeOverspeedRpm) &&
        s.UnsafeOverspeedRpm > s.RedlineRpm &&
        Finite(s.LuggingRpm) &&
        s.LuggingRpm >= 0.0 &&
        Finite(s.MinimumMotionSpeedCmS) &&
        s.MinimumMotionSpeedCmS >= 0.0 &&
        Finite(s.MaximumPhysicalRpm) &&
        s.MaximumPhysicalRpm > s.UnsafeOverspeedRpm &&
        Finite(s.MaximumFrameGapSeconds) &&
        s.MaximumFrameGapSeconds > 0.0 &&
        Finite(s.MaximumRpmChangePerSecond) &&
        s.MaximumRpmChangePerSecond > 0.0 &&
        Finite(s.ShiftTransientMaximumSeconds) &&
        s.ShiftTransientMaximumSeconds > 0.0 &&
        Finite(s.StrongShiftShockCmS2) &&
        s.StrongShiftShockCmS2 > 0.0 &&
        Finite(s.AbruptClutchChangePerSecond) &&
        s.AbruptClutchChangePerSecond > 0.0 &&
        Finite(s.MinimumShiftThrottleOverlap) &&
        s.MinimumShiftThrottleOverlap >= 0.0 &&
        Finite(s.MaximumShiftBrakeOverlap) &&
        s.MaximumShiftBrakeOverlap >= 0.0;
}

struct GearShiftEvent final
{
    double StartSeconds = 0.0;
    double EndSeconds = 0.0;
    double DurationSeconds = 0.0;
    double StartRpm = 0.0;
    double EndRpm = 0.0;
    double RpmDrop = 0.0;
    double StartSpeedCmS = 0.0;
    double EndSpeedCmS = 0.0;
    double StartTorqueNm = 0.0;
    double EndTorqueNm = 0.0;
    double PeakLongitudinalShockCmS2 = 0.0;
    double MaximumThrottleOverlap = 0.0;
    double MaximumBrakeOverlap = 0.0;
    double MaximumClutchChangeRate = 0.0;
    double ShiftEfficiencyFraction = 0.0;
    double PowerInterruptionSeconds = 0.0;
    int GearBefore = 0;
    int GearAfter = 0;
    int GearDifference = 0;
    ShiftQuality Quality = ShiftQuality::Unknown;
    bool UpShift = false;
    bool DownShift = false;
    bool PhysicallyObserved = false;
    bool Incomplete = false;
    bool Valid = false;
};

struct TransmissionReview final
{
    GearShiftEvent LastShift{};
    double MeanShiftDurationSeconds = 0.0;
    double MeanShiftEfficiencyFraction = 0.0;
    double MeanRpmAtUpshift = 0.0;
    double MeanRpmAtDownshift = 0.0;
    double PeakObservedRpm = 0.0;
    double MinimumDrivingRpm = 0.0;
    double MeanMotorTorqueNm = 0.0;
    double MeanMotorTemperatureC = 0.0;
    double TotalUnderPowerSeconds = 0.0;
    double TotalLuggingSeconds = 0.0;
    double TotalOverRevSeconds = 0.0;
    double TotalNeutralCoastSeconds = 0.0;
    double TotalShiftInterruptionSeconds = 0.0;
    int ShiftCount = 0;
    int UpshiftCount = 0;
    int DownshiftCount = 0;
    int AbruptShiftCount = 0;
    int UnsafeShiftCount = 0;
    int OverRevCount = 0;
    int SensorDiscontinuities = 0;
    int RejectedFrames = 0;
    int QualifiedFrames = 0;
    bool HasPhysicalEngineEvidence = false;
    bool AdvisoryOnly = true;
    bool Valid = false;
};

inline bool ValidDrivetrainSample(
    DrivetrainSample s,
    TransmissionReviewSettings settings) noexcept
{
    return s.SourceQualified &&
        s.EnginePhysicalReadout &&
        Finite(s.TimeSeconds) &&
        Finite(s.EngineRpm) &&
        Finite(s.WheelSpeedCmS) &&
        Finite(s.LongitudinalAccelerationCmS2) &&
        Finite(s.EngineTorqueNm) &&
        Finite(s.Throttle) &&
        Finite(s.Clutch) &&
        Finite(s.Brake) &&
        Finite(s.MotorTemperatureC) &&
        s.TimeSeconds >= 0.0 &&
        s.EngineRpm >= 0.0 &&
        s.EngineRpm <= settings.MaximumPhysicalRpm &&
        s.WheelSpeedCmS >= 0.0 &&
        s.WheelSpeedCmS <= MaximumPlanarSpeedCmS &&
        std::abs(s.LongitudinalAccelerationCmS2) <= 8000.0 &&
        std::abs(s.EngineTorqueNm) <= 20000.0 &&
        s.Throttle >= 0.0 && s.Throttle <= 1.0 &&
        s.Clutch >= 0.0 && s.Clutch <= 1.0 &&
        s.Brake >= 0.0 && s.Brake <= 1.0 &&
        s.MotorTemperatureC >= -100.0 &&
        s.MotorTemperatureC <= 1000.0 &&
        s.Gear >= -1 &&
        s.Gear <= settings.MaximumForwardGear;
}

class TransmissionShiftMonitor final
{
public:
    void Reset() noexcept
    {
        Settings = {};
        Previous = {};
        HasPrevious = false;
        HasActiveShift = false;
        Pending = {};
        Events.fill({});
        Head = 0;
        Count = 0;
        Review = {};
        RpmAtUpshift.Reset();
        RpmAtDownshift.Reset();
        ShiftDuration.Reset();
        ShiftEfficiency.Reset();
        TorqueStats.Reset();
        TemperatureStats.Reset();
    }

    bool Configure(TransmissionReviewSettings s) noexcept
    {
        if (!ValidTransmissionSettings(s))
        {
            return false;
        }
        Reset();
        Settings = s;
        return true;
    }

    bool Observe(DrivetrainSample sample) noexcept
    {
        if (!ValidDrivetrainSample(sample, Settings))
        {
            ++Review.RejectedFrames;
            HasPrevious = false;
            HasActiveShift = false;
            return false;
        }
        if (!HasPrevious)
        {
            Previous = sample;
            HasPrevious = true;
            ++Review.QualifiedFrames;
            return true;
        }
        const double dt =
            sample.TimeSeconds - Previous.TimeSeconds;
        if (dt <= 0.0 ||
            dt > Settings.MaximumFrameGapSeconds)
        {
            ++Review.SensorDiscontinuities;
            Previous = sample;
            HasActiveShift = false;
            return false;
        }
        const double rpmRate =
            (sample.EngineRpm - Previous.EngineRpm) / dt;
        if (std::abs(rpmRate) >
            Settings.MaximumRpmChangePerSecond)
        {
            ++Review.SensorDiscontinuities;
            Previous = sample;
            HasActiveShift = false;
            return false;
        }
        if (sample.Gear != Previous.Gear)
        {
            if (HasActiveShift)
            {
                FinalizeShift(false);
            }
            BeginShift(Previous, sample);
        }
        if (HasActiveShift)
        {
            Pending.EndSeconds = sample.TimeSeconds;
            Pending.DurationSeconds =
                Pending.EndSeconds - Pending.StartSeconds;
            Pending.EndRpm = sample.EngineRpm;
            Pending.EndSpeedCmS = sample.WheelSpeedCmS;
            Pending.EndTorqueNm = sample.EngineTorqueNm;
            Pending.PeakLongitudinalShockCmS2 =
                std::max(Pending.PeakLongitudinalShockCmS2,
                         std::abs(sample.LongitudinalAccelerationCmS2 -
                                  Previous.LongitudinalAccelerationCmS2));
            Pending.MaximumThrottleOverlap = std::max(
                Pending.MaximumThrottleOverlap,
                sample.Throttle);
            Pending.MaximumBrakeOverlap = std::max(
                Pending.MaximumBrakeOverlap,
                sample.Brake);
            Pending.MaximumClutchChangeRate = std::max(
                Pending.MaximumClutchChangeRate,
                std::abs(sample.Clutch -
                         Previous.Clutch) / dt);
            Pending.PowerInterruptionSeconds +=
                sample.Throttle < 0.10 ||
                !sample.TransmissionEngaged ? dt : 0.0;
            if (sample.TransmissionEngaged &&
                Pending.DurationSeconds >= 0.08)
            {
                FinalizeShift(true);
            }
            else if (Pending.DurationSeconds >=
                     Settings.ShiftTransientMaximumSeconds)
            {
                FinalizeShift(false);
            }
        }
        Review.TotalUnderPowerSeconds +=
            sample.Throttle >= 0.18 &&
            sample.TransmissionEngaged ? dt : 0.0;
        Review.TotalNeutralCoastSeconds +=
            sample.Gear == 0 && sample.WheelSpeedCmS >
                Settings.MinimumMotionSpeedCmS ? dt : 0.0;
        Review.TotalLuggingSeconds +=
            sample.EngineRpm < Settings.LuggingRpm &&
            sample.Throttle > 0.35 &&
            sample.Gear > 0 ? dt : 0.0;
        Review.TotalOverRevSeconds +=
            sample.EngineRpm >=
            Settings.RedlineRpm ? dt : 0.0;
        Review.PeakObservedRpm = std::max(
            Review.PeakObservedRpm, sample.EngineRpm);
        if (sample.WheelSpeedCmS >
            Settings.MinimumMotionSpeedCmS)
        {
            Review.MinimumDrivingRpm =
                Review.MinimumDrivingRpm <= 0.0 ?
                    sample.EngineRpm :
                std::min(Review.MinimumDrivingRpm,
                         sample.EngineRpm);
        }
        TorqueStats.Observe(sample.EngineTorqueNm);
        TemperatureStats.Observe(
            sample.MotorTemperatureC);
        Review.MeanMotorTorqueNm =
            TorqueStats.Mean();
        Review.MeanMotorTemperatureC =
            TemperatureStats.Mean();
        Review.HasPhysicalEngineEvidence = true;
        Review.Valid = true;
        ++Review.QualifiedFrames;
        Previous = sample;
        return true;
    }

    TransmissionReview Snapshot() const noexcept
    {
        return Review;
    }

    GearShiftEvent AtNewest(int offset = 0) const noexcept
    {
        return offset >= 0 && offset < Count ?
            Events[static_cast<std::size_t>(
                (Head + Count - offset - 1) %
                static_cast<int>(Events.size()))] :
            GearShiftEvent{};
    }

    int StoredShifts() const noexcept { return Count; }
    bool ShiftActive() const noexcept { return HasActiveShift; }

private:
    void BeginShift(
        DrivetrainSample before,
        DrivetrainSample after) noexcept
    {
        Pending = {};
        Pending.StartSeconds = before.TimeSeconds;
        Pending.EndSeconds = after.TimeSeconds;
        Pending.StartRpm = before.EngineRpm;
        Pending.EndRpm = after.EngineRpm;
        Pending.StartSpeedCmS = before.WheelSpeedCmS;
        Pending.EndSpeedCmS = after.WheelSpeedCmS;
        Pending.StartTorqueNm = before.EngineTorqueNm;
        Pending.EndTorqueNm = after.EngineTorqueNm;
        Pending.GearBefore = before.Gear;
        Pending.GearAfter = after.Gear;
        Pending.GearDifference = after.Gear -
            before.Gear;
        Pending.UpShift = Pending.GearDifference > 0;
        Pending.DownShift = Pending.GearDifference < 0;
        Pending.PhysicallyObserved = true;
        HasActiveShift = true;
    }

    void FinalizeShift(bool complete) noexcept
    {
        if (!HasActiveShift)
        {
            return;
        }
        Pending.Incomplete = !complete;
        Pending.RpmDrop =
            Pending.StartRpm - Pending.EndRpm;
        Pending.ShiftEfficiencyFraction =
            Clamp01(1.0 - SafeRatio(
                Pending.PowerInterruptionSeconds,
                std::max(0.01, Pending.DurationSeconds)));
        Pending.Valid = complete &&
            Pending.GearDifference != 0 &&
            Pending.DurationSeconds <=
                Settings.ShiftTransientMaximumSeconds;
        if (Pending.GearDifference > 1 ||
            Pending.GearDifference < -2)
        {
            Pending.Quality =
                ShiftQuality::UnsafeGearChange;
        }
        else if (Pending.StartRpm >=
                 Settings.UnsafeOverspeedRpm)
        {
            Pending.Quality =
                ShiftQuality::OverRev;
        }
        else if (Pending.PeakLongitudinalShockCmS2 >=
                 Settings.StrongShiftShockCmS2 ||
                 Pending.MaximumClutchChangeRate >=
                 Settings.AbruptClutchChangePerSecond)
        {
            Pending.Quality = ShiftQuality::Abrupt;
        }
        else if (Pending.MaximumBrakeOverlap >=
                 Settings.MaximumShiftBrakeOverlap)
        {
            Pending.Quality =
                ShiftQuality::ShiftUnderBraking;
        }
        else if (Pending.StartRpm <
                 Settings.LuggingRpm)
        {
            Pending.Quality = ShiftQuality::Lugging;
        }
        else
        {
            Pending.Quality = ShiftQuality::Smooth;
        }
        if (Pending.Valid)
        {
            Store(Pending);
            ++Review.ShiftCount;
            if (Pending.UpShift)
            {
                ++Review.UpshiftCount;
                RpmAtUpshift.Observe(Pending.StartRpm);
            }
            else
            {
                ++Review.DownshiftCount;
                RpmAtDownshift.Observe(Pending.StartRpm);
            }
            Review.AbruptShiftCount +=
                Pending.Quality == ShiftQuality::Abrupt ? 1 : 0;
            Review.UnsafeShiftCount +=
                Pending.Quality ==
                    ShiftQuality::UnsafeGearChange ? 1 : 0;
            Review.OverRevCount +=
                Pending.Quality == ShiftQuality::OverRev ? 1 : 0;
            Review.TotalShiftInterruptionSeconds +=
                Pending.PowerInterruptionSeconds;
            ShiftDuration.Observe(Pending.DurationSeconds);
            ShiftEfficiency.Observe(
                Pending.ShiftEfficiencyFraction);
            Review.MeanShiftDurationSeconds =
                ShiftDuration.Mean();
            Review.MeanShiftEfficiencyFraction =
                ShiftEfficiency.Mean();
            Review.MeanRpmAtUpshift =
                RpmAtUpshift.Mean();
            Review.MeanRpmAtDownshift =
                RpmAtDownshift.Mean();
            Review.LastShift = Pending;
        }
        Pending = {};
        HasActiveShift = false;
    }

    void Store(GearShiftEvent event) noexcept
    {
        if (Count == static_cast<int>(Events.size()))
        {
            Head = (Head + 1) %
                static_cast<int>(Events.size());
        }
        else
        {
            ++Count;
        }
        Events[static_cast<std::size_t>(
            (Head + Count - 1) %
            static_cast<int>(Events.size()))] = event;
    }

    TransmissionReviewSettings Settings{};
    DrivetrainSample Previous{};
    GearShiftEvent Pending{};
    TransmissionReview Review{};
    std::array<GearShiftEvent, 128> Events{};
    RunningStatistics RpmAtUpshift;
    RunningStatistics RpmAtDownshift;
    RunningStatistics ShiftDuration;
    RunningStatistics ShiftEfficiency;
    RunningStatistics TorqueStats;
    RunningStatistics TemperatureStats;
    int Head = 0;
    int Count = 0;
    bool HasPrevious = false;
    bool HasActiveShift = false;
};
} // namespace latebrake::engineering
