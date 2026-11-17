#pragma once

#include "Telemetry/TelemetryValidation.h"
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
inline constexpr int MaximumReplaySamples = 4096;

enum class ReplayFrameStatus : std::uint8_t
{
    Missing,
    Accepted,
    SessionReset,
    TimeDiscontinuity,
    InvalidNativeSource,
    NonFinite,
    InvalidMotion,
    CapacityExceeded,
    DuplicateFrame,
    BackwardsFrame,
    InvalidTrackProgress
};

enum class ReplaySampleMode : std::uint8_t
{
    Nearest,
    Linear,
    HoldPrevious,
    HoldNext
};

struct ReplaySettings final
{
    double MaximumSampleSpacingSeconds = 0.1;
    double MinimumSampleSpacingSeconds = 1.0 / 240.0;
    double MaximumInterpolationGapSeconds = 0.2;
    double MaximumVelocityCmS = 16000.0;
    double MaximumPositionCm = MaximumWorldMagnitudeCm;
    double MaximumSegmentPositionJumpCm = 600.0;
    double MaximumYawStepRadians = 1.0;
    int MaximumStoredSamples = MaximumReplaySamples;
    bool RequireNativeEngineSource = true;
    bool RejectUnverifiedTrackPosition = false;
    bool RejectDuplicateFrameOrdinals = true;
    bool RetainOnSegmentReset = true;
};

inline bool ValidReplaySettings(
    const ReplaySettings& s) noexcept
{
    return Finite(s.MaximumSampleSpacingSeconds) &&
           s.MaximumSampleSpacingSeconds > 0.0 &&
           s.MaximumSampleSpacingSeconds <= 1.0 &&
           Finite(s.MinimumSampleSpacingSeconds) &&
           s.MinimumSampleSpacingSeconds >= 0.0 &&
           s.MinimumSampleSpacingSeconds <
               s.MaximumSampleSpacingSeconds &&
           Finite(s.MaximumInterpolationGapSeconds) &&
           s.MaximumInterpolationGapSeconds >=
               s.MaximumSampleSpacingSeconds &&
           Finite(s.MaximumVelocityCmS) &&
           s.MaximumVelocityCmS > 0.0 &&
           Finite(s.MaximumPositionCm) &&
           s.MaximumPositionCm > 0.0 &&
           Finite(s.MaximumSegmentPositionJumpCm) &&
           s.MaximumSegmentPositionJumpCm > 0.0 &&
           Finite(s.MaximumYawStepRadians) &&
           s.MaximumYawStepRadians > 0.0 &&
           s.MaximumYawStepRadians <= Pi &&
           s.MaximumStoredSamples > 1 &&
           s.MaximumStoredSamples <= MaximumReplaySamples;
}

struct ReplaySample final
{
    Vector3 PositionCm{};
    Vector3 VelocityCmS{};
    Vector2 AccelerationCmS2{};
    double YawRadians = 0.0;
    double YawRateRadS = 0.0;
    double MonotonicSeconds = 0.0;
    double RaceClockSeconds = 0.0;
    double LapClockSeconds = 0.0;
    double TrackDistanceCm = 0.0;
    double TrackLateralCm = 0.0;
    double SpeedCmS = 0.0;
    double Throttle = 0.0;
    double Brake = 0.0;
    double Steering = 0.0;
    double SlipRadians = 0.0;
    double TireWear = 0.0;
    double EngineTemperatureC = 0.0;
    double BrakeTemperatureC = 0.0;
    double FuelFraction = 0.0;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    std::uint64_t FrameOrdinal = 0;
    int LapIndex = -1;
    int SectorIndex = -1;
    int SegmentOrdinal = -1;
    TelemetrySurface Surface = TelemetrySurface::Unknown;
    bool OnPavement = false;
    bool NativeObserved = false;
    bool TrackQualified = false;
    bool Valid = false;
};

struct ReplayRecordDecision final
{
    ReplayFrameStatus Status = ReplayFrameStatus::Missing;
    std::uint64_t SampleCount = 0;
    double LatestRecordedSeconds = 0.0;
    int SegmentOrdinal = -1;
    bool Accepted = false;
    bool NewSegment = false;
    bool OverwroteOldest = false;
};

struct ReplayCounters final
{
    std::uint64_t Received = 0;
    std::uint64_t Recorded = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t DuplicateFrames = 0;
    std::uint64_t TimeBacktracks = 0;
    std::uint64_t IdentityChanges = 0;
    std::uint64_t GapResets = 0;
    std::uint64_t Overwritten = 0;
    std::uint64_t Interpolated = 0;
    std::uint64_t InterpolationRefused = 0;
    std::uint64_t SeekRequests = 0;
    double StoredWindowSeconds = 0.0;
    double LargestAcceptedFrameDeltaSeconds = 0.0;
    double LongestRecordedSegmentSeconds = 0.0;
    bool CapacityReached = false;
};

inline bool ValidReplaySample(
    const ReplaySample& s,
    const ReplaySettings& settings) noexcept
{
    return s.Valid &&
           s.SessionToken != 0 &&
           s.ActorToken != 0 &&
           s.FrameOrdinal != 0 &&
           s.SegmentOrdinal >= 0 &&
           s.LapIndex >= 0 &&
           s.LapIndex <= MaximumTelemetryLaps &&
           s.SectorIndex >= 0 &&
           s.SectorIndex <= MaximumTelemetrySectors &&
           Finite(s.PositionCm) &&
           Finite(s.VelocityCmS) &&
           Finite(s.AccelerationCmS2) &&
           Length(s.PositionCm) <= settings.MaximumPositionCm &&
           Length(s.VelocityCmS) <= settings.MaximumVelocityCmS &&
           Finite(s.YawRadians) &&
           Finite(s.YawRateRadS) &&
           Finite(s.MonotonicSeconds) &&
           Finite(s.RaceClockSeconds) &&
           Finite(s.LapClockSeconds) &&
           Finite(s.TrackDistanceCm) &&
           Finite(s.TrackLateralCm) &&
           Finite(s.SpeedCmS) &&
           Finite(s.Throttle) &&
           Finite(s.Brake) &&
           Finite(s.Steering) &&
           Finite(s.SlipRadians) &&
           Finite(s.TireWear) &&
           Finite(s.EngineTemperatureC) &&
           Finite(s.BrakeTemperatureC) &&
           Finite(s.FuelFraction) &&
           s.MonotonicSeconds >= 0.0 &&
           s.RaceClockSeconds >= 0.0 &&
           s.LapClockSeconds >= 0.0 &&
           s.SpeedCmS >= 0.0 &&
           s.SpeedCmS <= settings.MaximumVelocityCmS &&
           s.Throttle >= 0.0 && s.Throttle <= 1.0 &&
           s.Brake >= 0.0 && s.Brake <= 1.0 &&
           s.Steering >= -1.0 && s.Steering <= 1.0 &&
           s.TireWear >= 0.0 && s.TireWear <= 1.0 &&
           s.FuelFraction >= 0.0 && s.FuelFraction <= 1.0 &&
           (!settings.RequireNativeEngineSource || s.NativeObserved) &&
           (!settings.RejectUnverifiedTrackPosition || s.TrackQualified);
}

inline ReplaySample MakeReplaySample(
    const TelemetryFrame& frame,
    const TelemetryKinematics& motion,
    const TrackProjection& projection,
    int segmentOrdinal) noexcept
{
    ReplaySample sample;
    sample.PositionCm = frame.Dynamics.WorldPositionCm;
    sample.VelocityCmS = frame.Dynamics.WorldVelocityCmS;
    sample.AccelerationCmS2 = motion.AccelerationCmS2;
    sample.YawRadians = frame.Dynamics.YawRadians;
    sample.YawRateRadS = motion.YawRateRadS;
    sample.MonotonicSeconds = frame.MonotonicSeconds;
    sample.RaceClockSeconds = frame.RaceClockSeconds;
    sample.LapClockSeconds = frame.Race.CurrentLapSeconds;
    sample.TrackDistanceCm =
        projection.Valid ? projection.DistanceAlongCm : 0.0;
    sample.TrackLateralCm =
        projection.Valid ? projection.SignedLateralCm : 0.0;
    sample.SpeedCmS = motion.SpeedCmS;
    sample.Throttle = frame.Controls.Throttle;
    sample.Brake = frame.Controls.Brake;
    sample.Steering = frame.Controls.Steering;
    sample.SlipRadians = motion.SlipAngleRadians;
    sample.TireWear = MeanWheelWear(frame.Wheels);
    sample.EngineTemperatureC =
        frame.Dynamics.EngineTemperatureCelsius;
    sample.BrakeTemperatureC =
        frame.Dynamics.BrakeTemperatureCelsius;
    sample.FuelFraction = frame.Dynamics.FuelFraction;
    sample.SessionToken = frame.Identity.SessionToken;
    sample.ActorToken = frame.Identity.ActorToken;
    sample.FrameOrdinal = frame.Identity.FrameOrdinal;
    sample.LapIndex = frame.Race.LapIndex;
    sample.SectorIndex = frame.Race.SectorIndex;
    sample.SegmentOrdinal = segmentOrdinal;
    sample.Surface = frame.Environment.Surface;
    sample.OnPavement = frame.Environment.OnPhysicalPavement;
    sample.NativeObserved =
        IsEngineObservation(frame.Identity.Trust) &&
        frame.Race.EnginePhysicsReady;
    sample.TrackQualified = projection.Valid;
    sample.Valid = motion.Valid;
    return sample;
}

inline ReplaySample InterpolateReplay(
    const ReplaySample& begin,
    const ReplaySample& end,
    double fraction) noexcept
{
    ReplaySample out = begin;
    const double t = Clamp01(fraction);
    out.PositionCm = Lerp(begin.PositionCm, end.PositionCm, t);
    out.VelocityCmS = Lerp(begin.VelocityCmS, end.VelocityCmS, t);
    out.AccelerationCmS2 = Lerp(
        begin.AccelerationCmS2, end.AccelerationCmS2, t);
    out.YawRadians = WrapRadians(
        begin.YawRadians +
        AngleDifference(begin.YawRadians, end.YawRadians) * t);
    out.YawRateRadS = Lerp(begin.YawRateRadS, end.YawRateRadS, t);
    out.MonotonicSeconds = Lerp(
        begin.MonotonicSeconds, end.MonotonicSeconds, t);
    out.RaceClockSeconds = Lerp(
        begin.RaceClockSeconds, end.RaceClockSeconds, t);
    out.LapClockSeconds = Lerp(
        begin.LapClockSeconds, end.LapClockSeconds, t);
    out.TrackDistanceCm = Lerp(
        begin.TrackDistanceCm, end.TrackDistanceCm, t);
    out.TrackLateralCm = Lerp(
        begin.TrackLateralCm, end.TrackLateralCm, t);
    out.SpeedCmS = Lerp(begin.SpeedCmS, end.SpeedCmS, t);
    out.Throttle = Lerp(begin.Throttle, end.Throttle, t);
    out.Brake = Lerp(begin.Brake, end.Brake, t);
    out.Steering = Lerp(begin.Steering, end.Steering, t);
    out.SlipRadians = Lerp(begin.SlipRadians, end.SlipRadians, t);
    out.TireWear = Lerp(begin.TireWear, end.TireWear, t);
    out.EngineTemperatureC = Lerp(
        begin.EngineTemperatureC, end.EngineTemperatureC, t);
    out.BrakeTemperatureC = Lerp(
        begin.BrakeTemperatureC, end.BrakeTemperatureC, t);
    out.FuelFraction = Lerp(begin.FuelFraction, end.FuelFraction, t);
    out.FrameOrdinal =
        t < 0.5 ? begin.FrameOrdinal : end.FrameOrdinal;
    out.SectorIndex = t < 0.5 ? begin.SectorIndex : end.SectorIndex;
    out.Surface = t < 0.5 ? begin.Surface : end.Surface;
    out.OnPavement = begin.OnPavement && end.OnPavement;
    out.NativeObserved = false;
    out.TrackQualified =
        begin.TrackQualified && end.TrackQualified;
    out.Valid = begin.Valid && end.Valid;
    return out;
}

class ReplayRecorder final
{
public:
    bool Configure(ReplaySettings settings) noexcept
    {
        if (!ValidReplaySettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Samples.fill({});
        Stats = {};
        Head = 0;
        Count = 0;
        LastSession = 0;
        LastActor = 0;
        LastOrdinal = 0;
        SegmentOrdinal = 0;
        HasPrevious = false;
        Previous = {};
    }

    void Suspend() noexcept
    {
        HasPrevious = false;
        ++SegmentOrdinal;
    }

    ReplayRecordDecision Observe(
        const TelemetryFrame& frame,
        const TelemetryFrameDecision& decision,
        const TelemetryKinematics& motion,
        const TrackProjection& projection) noexcept
    {
        Stats.Received = SaturatingIncrement(Stats.Received);
        ReplayRecordDecision result;
        if (!decision.Accepted || !motion.Valid)
        {
            ++Stats.Rejected;
            result.Status = ReplayFrameStatus::InvalidNativeSource;
            return result;
        }
        if (LastSession != 0 &&
            (LastSession != frame.Identity.SessionToken ||
             LastActor != frame.Identity.ActorToken))
        {
            Stats.IdentityChanges =
                SaturatingIncrement(Stats.IdentityChanges);
            if (!Settings.RetainOnSegmentReset)
            {
                Reset();
            }
            Suspend();
            result.NewSegment = true;
            result.Status = ReplayFrameStatus::SessionReset;
        }
        ReplaySample sample = MakeReplaySample(
            frame, motion, projection, SegmentOrdinal);
        if (!ValidReplaySample(sample, Settings))
        {
            Stats.Rejected = SaturatingIncrement(Stats.Rejected);
            result.Status = ReplayFrameStatus::NonFinite;
            return result;
        }
        if (HasPrevious)
        {
            if (Settings.RejectDuplicateFrameOrdinals &&
                sample.FrameOrdinal <= Previous.FrameOrdinal)
            {
                Stats.DuplicateFrames =
                    SaturatingIncrement(Stats.DuplicateFrames);
                Stats.Rejected = SaturatingIncrement(Stats.Rejected);
                result.Status = ReplayFrameStatus::DuplicateFrame;
                return result;
            }
            const double dt = sample.MonotonicSeconds -
                Previous.MonotonicSeconds;
            if (!Finite(dt) || dt <= 0.0)
            {
                Stats.TimeBacktracks =
                    SaturatingIncrement(Stats.TimeBacktracks);
                Stats.Rejected = SaturatingIncrement(Stats.Rejected);
                result.Status = ReplayFrameStatus::BackwardsFrame;
                return result;
            }
            const double travel = Length(
                sample.PositionCm - Previous.PositionCm);
            if (dt > Settings.MaximumSampleSpacingSeconds ||
                travel > Settings.MaximumSegmentPositionJumpCm ||
                std::abs(AngleDifference(
                    Previous.YawRadians, sample.YawRadians)) >
                    Settings.MaximumYawStepRadians)
            {
                Stats.GapResets =
                    SaturatingIncrement(Stats.GapResets);
                Suspend();
                sample.SegmentOrdinal = SegmentOrdinal;
                result.NewSegment = true;
                result.Status = ReplayFrameStatus::TimeDiscontinuity;
            }
            else if (dt < Settings.MinimumSampleSpacingSeconds)
            {
                Stats.Rejected = SaturatingIncrement(Stats.Rejected);
                result.Status = ReplayFrameStatus::DuplicateFrame;
                return result;
            }
            Stats.LargestAcceptedFrameDeltaSeconds = std::max(
                Stats.LargestAcceptedFrameDeltaSeconds, dt);
        }
        const int limit = Settings.MaximumStoredSamples;
        const bool full = Count == limit;
        const std::size_t index = full ? Head :
            (Head + static_cast<std::size_t>(Count)) %
                static_cast<std::size_t>(limit);
        Samples[index] = sample;
        if (full)
        {
            Head = (Head + 1) % static_cast<std::size_t>(limit);
            Stats.Overwritten = SaturatingIncrement(Stats.Overwritten);
            Stats.CapacityReached = true;
        }
        else
        {
            ++Count;
        }
        Stats.Recorded = SaturatingIncrement(Stats.Recorded);
        result.Accepted = true;
        result.Status = ReplayFrameStatus::Accepted;
        result.SampleCount = Stats.Recorded;
        result.LatestRecordedSeconds = sample.MonotonicSeconds;
        result.SegmentOrdinal = SegmentOrdinal;
        result.OverwroteOldest = full;
        Previous = sample;
        LastSession = sample.SessionToken;
        LastActor = sample.ActorToken;
        LastOrdinal = sample.FrameOrdinal;
        HasPrevious = true;
        if (Count >= 2)
        {
            const ReplaySample begin = At(0);
            const ReplaySample finish = At(Count - 1);
            Stats.StoredWindowSeconds = std::max(
                0.0, finish.MonotonicSeconds - begin.MonotonicSeconds);
            if (begin.SegmentOrdinal == finish.SegmentOrdinal)
            {
                Stats.LongestRecordedSegmentSeconds = std::max(
                    Stats.LongestRecordedSegmentSeconds,
                    Stats.StoredWindowSeconds);
            }
        }
        return result;
    }

    ReplaySample At(int index) const noexcept
    {
        if (index < 0 || index >= Count)
        {
            return {};
        }
        const std::size_t i =
            (Head + static_cast<std::size_t>(index)) %
            static_cast<std::size_t>(Settings.MaximumStoredSamples);
        return Samples[i];
    }

    ReplaySample Latest() const noexcept
    {
        return Count ? At(Count - 1) : ReplaySample{};
    }

    ReplaySample Oldest() const noexcept
    {
        return Count ? At(0) : ReplaySample{};
    }

    int FirstIndexAtOrAfter(double seconds) const noexcept
    {
        if (Count == 0 || !Finite(seconds))
        {
            return -1;
        }
        int low = 0;
        int high = Count;
        while (low < high)
        {
            const int middle = low + (high - low) / 2;
            if (At(middle).MonotonicSeconds < seconds)
            {
                low = middle + 1;
            }
            else
            {
                high = middle;
            }
        }
        return low < Count ? low : -1;
    }

    ReplaySample SampleAt(
        double seconds,
        ReplaySampleMode mode = ReplaySampleMode::Linear) noexcept
    {
        Stats.SeekRequests = SaturatingIncrement(Stats.SeekRequests);
        if (Count == 0 || !Finite(seconds))
        {
            return {};
        }
        if (seconds <= Oldest().MonotonicSeconds)
        {
            return Oldest();
        }
        if (seconds >= Latest().MonotonicSeconds)
        {
            return Latest();
        }
        const int upper = FirstIndexAtOrAfter(seconds);
        if (upper <= 0)
        {
            return {};
        }
        const ReplaySample a = At(upper - 1);
        const ReplaySample b = At(upper);
        if (!a.Valid || !b.Valid ||
            a.SessionToken != b.SessionToken ||
            a.ActorToken != b.ActorToken ||
            a.SegmentOrdinal != b.SegmentOrdinal ||
            b.MonotonicSeconds <= a.MonotonicSeconds ||
            b.MonotonicSeconds - a.MonotonicSeconds >
                Settings.MaximumInterpolationGapSeconds)
        {
            Stats.InterpolationRefused =
                SaturatingIncrement(Stats.InterpolationRefused);
            return {};
        }
        if (mode == ReplaySampleMode::HoldPrevious)
        {
            return a;
        }
        if (mode == ReplaySampleMode::HoldNext)
        {
            return b;
        }
        const double fraction = Clamp01(
            (seconds - a.MonotonicSeconds) /
            (b.MonotonicSeconds - a.MonotonicSeconds));
        if (mode == ReplaySampleMode::Nearest)
        {
            return fraction < 0.5 ? a : b;
        }
        Stats.Interpolated = SaturatingIncrement(Stats.Interpolated);
        return InterpolateReplay(a, b, fraction);
    }

    int CopyWindow(
        ReplaySample* destination, int capacity,
        double startSeconds, double endSeconds) const noexcept
    {
        if (!destination || capacity <= 0 ||
            !Finite(startSeconds) || !Finite(endSeconds) ||
            startSeconds > endSeconds)
        {
            return 0;
        }
        int count = 0;
        for (int i = 0; i < Count && count < capacity; ++i)
        {
            const ReplaySample sample = At(i);
            if (sample.MonotonicSeconds >= startSeconds &&
                sample.MonotonicSeconds <= endSeconds)
            {
                destination[count++] = sample;
            }
        }
        return count;
    }

    int CountForLap(int lapIndex) const noexcept
    {
        if (lapIndex < 0)
        {
            return 0;
        }
        int total = 0;
        for (int i = 0; i < Count; ++i)
        {
            total += At(i).LapIndex == lapIndex ? 1 : 0;
        }
        return total;
    }

    ReplayCounters Statistics() const noexcept { return Stats; }
    int Size() const noexcept { return Count; }
    bool Empty() const noexcept { return Count == 0; }
    int ActiveSegment() const noexcept { return SegmentOrdinal; }

private:
    ReplaySettings Settings{};
    std::array<ReplaySample, MaximumReplaySamples> Samples{};
    ReplayCounters Stats{};
    ReplaySample Previous{};
    std::size_t Head = 0;
    int Count = 0;
    std::uint64_t LastSession = 0;
    std::uint64_t LastActor = 0;
    std::uint64_t LastOrdinal = 0;
    int SegmentOrdinal = 0;
    bool HasPrevious = false;
};

class ReplayPlaybackClock final
{
public:
    void Reset() noexcept
    {
        CursorSeconds = 0.0;
        SpeedScale = 1.0;
        From = 0.0;
        To = 0.0;
        Loop = false;
        Paused = true;
        Ready = false;
    }

    bool Configure(
        double start, double end, double speed = 1.0,
        bool looping = false) noexcept
    {
        if (!Finite(start) || !Finite(end) ||
            !Finite(speed) || start < 0.0 ||
            end <= start || speed <= 0.0 || speed > 8.0)
        {
            return false;
        }
        From = start;
        To = end;
        SpeedScale = speed;
        CursorSeconds = start;
        Loop = looping;
        Paused = true;
        Ready = true;
        return true;
    }

    bool Seek(double seconds) noexcept
    {
        if (!Ready || !Finite(seconds))
        {
            return false;
        }
        CursorSeconds = std::clamp(seconds, From, To);
        return true;
    }

    bool Advance(double wallDeltaSeconds) noexcept
    {
        if (!Ready || Paused ||
            !Finite(wallDeltaSeconds) ||
            wallDeltaSeconds < 0.0 || wallDeltaSeconds > 0.5)
        {
            return false;
        }
        CursorSeconds += wallDeltaSeconds * SpeedScale;
        if (CursorSeconds >= To)
        {
            if (Loop)
            {
                CursorSeconds = From +
                    std::fmod(CursorSeconds - From, To - From);
            }
            else
            {
                CursorSeconds = To;
                Paused = true;
            }
        }
        return true;
    }

    bool Play() noexcept
    {
        if (!Ready)
        {
            return false;
        }
        Paused = false;
        return true;
    }

    void Pause() noexcept { Paused = true; }
    double PositionSeconds() const noexcept { return CursorSeconds; }
    double PlaybackRate() const noexcept { return SpeedScale; }
    bool Playing() const noexcept { return Ready && !Paused; }
    bool HasTimeline() const noexcept { return Ready; }

private:
    double CursorSeconds = 0.0;
    double From = 0.0;
    double To = 0.0;
    double SpeedScale = 1.0;
    bool Loop = false;
    bool Paused = true;
    bool Ready = false;
};
} // namespace latebrake::engineering
