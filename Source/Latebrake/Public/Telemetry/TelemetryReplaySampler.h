#pragma once

#include "Telemetry/TelemetrySessionRecorder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class ReplaySamplingMode : std::uint8_t
{
    HoldNearest,
    HoldPrevious,
    LinearInterpolation,
    RejectAcrossGap
};

struct ReplaySamplerSettings final
{
    ReplaySamplingMode Mode = ReplaySamplingMode::LinearInterpolation;
    double MaximumInterpolationSeconds = 0.10;
    double MaximumRequestedTimeSeconds = MaximumTelemetrySeconds;
    bool RequireIdenticalActor = true;
    bool RequireIdenticalSegment = true;
    bool RequirePhysicsReadiness = true;
    bool ClampToRecordedRange = true;
    bool PreserveDiscreteControls = true;
};

struct ReplayInterpolatedFrame final
{
    TelemetryFrame Frame{};
    double RequestedSeconds = 0.0;
    double SampledSeconds = 0.0;
    double LeftSeconds = 0.0;
    double RightSeconds = 0.0;
    double Blend = 0.0;
    double TemporalErrorSeconds = 0.0;
    std::uint64_t LeftSequence = 0;
    std::uint64_t RightSequence = 0;
    int SourceSegment = -1;
    bool Interpolated = false;
    bool Qualified = false;
    bool Extrapolated = false;
    bool CrossedSegmentBoundary = false;
    bool AuthoritativeGameState = false;
};

inline bool ValidReplaySettings(
    ReplaySamplerSettings settings) noexcept
{
    return Finite(settings.MaximumInterpolationSeconds) &&
           settings.MaximumInterpolationSeconds > 0.0 &&
           settings.MaximumInterpolationSeconds <= 5.0 &&
           Finite(settings.MaximumRequestedTimeSeconds) &&
           settings.MaximumRequestedTimeSeconds >= 1.0 &&
           settings.MaximumRequestedTimeSeconds <=
               MaximumTelemetrySeconds;
}

inline bool ReplayFrameIdentityMatches(
    const RecordedFrame& a, const RecordedFrame& b) noexcept
{
    return a.Qualified && b.Qualified &&
        a.Frame.Identity.SessionToken ==
            b.Frame.Identity.SessionToken &&
        a.Frame.Identity.ActorToken ==
            b.Frame.Identity.ActorToken &&
        a.Frame.Identity.DriverSlot ==
            b.Frame.Identity.DriverSlot &&
        a.Frame.Identity.SessionRevision ==
            b.Frame.Identity.SessionRevision;
}

inline TelemetryDynamics InterpolateDynamics(
    const TelemetryDynamics& a,
    const TelemetryDynamics& b,
    double fraction) noexcept
{
    const double t = Clamp01(fraction);
    TelemetryDynamics result = a;
    result.WorldPositionCm = Lerp(
        a.WorldPositionCm, b.WorldPositionCm, t);
    result.WorldVelocityCmS = Lerp(
        a.WorldVelocityCmS, b.WorldVelocityCmS, t);
    result.WorldAccelerationCmS2 = Lerp(
        a.WorldAccelerationCmS2, b.WorldAccelerationCmS2, t);
    result.AngularVelocityRadS = Lerp(
        a.AngularVelocityRadS, b.AngularVelocityRadS, t);
    result.YawRadians = WrapRadians(
        a.YawRadians + AngleDifference(
            a.YawRadians, b.YawRadians) * t);
    result.PitchRadians = Lerp(
        a.PitchRadians, b.PitchRadians, t);
    result.RollRadians = Lerp(
        a.RollRadians, b.RollRadians, t);
    result.EngineRpm = Lerp(a.EngineRpm, b.EngineRpm, t);
    result.MotorTorqueNm = Lerp(
        a.MotorTorqueNm, b.MotorTorqueNm, t);
    result.BrakeTemperatureCelsius = Lerp(
        a.BrakeTemperatureCelsius,
        b.BrakeTemperatureCelsius, t);
    result.EngineTemperatureCelsius = Lerp(
        a.EngineTemperatureCelsius,
        b.EngineTemperatureCelsius, t);
    result.FuelFraction = Lerp(
        a.FuelFraction, b.FuelFraction, t);
    result.ChassisDamageFraction = Lerp(
        a.ChassisDamageFraction,
        b.ChassisDamageFraction, t);
    result.GripScale = Lerp(
        a.GripScale, b.GripScale, t);
    result.RoadSlopeRadians = Lerp(
        a.RoadSlopeRadians, b.RoadSlopeRadians, t);
    result.SuspensionCompressionCm = Lerp(
        a.SuspensionCompressionCm,
        b.SuspensionCompressionCm, t);
    result.RideHeightCm = Lerp(
        a.RideHeightCm, b.RideHeightCm, t);
    return result;
}

inline TelemetryControls InterpolateControls(
    const TelemetryControls& a,
    const TelemetryControls& b,
    double fraction, bool preserveDiscrete) noexcept
{
    const double t = Clamp01(fraction);
    TelemetryControls result = preserveDiscrete ?
        (t < 0.5 ? a : b) : a;
    result.Throttle = Lerp(a.Throttle, b.Throttle, t);
    result.Brake = Lerp(a.Brake, b.Brake, t);
    result.Steering = Lerp(a.Steering, b.Steering, t);
    result.Handbrake = Lerp(a.Handbrake, b.Handbrake, t);
    result.Clutch = Lerp(a.Clutch, b.Clutch, t);
    return result;
}

inline TelemetryWheels InterpolateWheels(
    const TelemetryWheels& a,
    const TelemetryWheels& b,
    double fraction) noexcept
{
    const double t = Clamp01(fraction);
    TelemetryWheels result = t < 0.5 ? a : b;
    for (std::size_t i = 0; i < 4; ++i)
    {
        result.AngularSpeedRadS[i] = Lerp(
            a.AngularSpeedRadS[i], b.AngularSpeedRadS[i], t);
        result.NormalLoadNewtons[i] = Lerp(
            a.NormalLoadNewtons[i], b.NormalLoadNewtons[i], t);
        result.SlipRatio[i] = Lerp(
            a.SlipRatio[i], b.SlipRatio[i], t);
        result.SlipAngleRadians[i] = Lerp(
            a.SlipAngleRadians[i], b.SlipAngleRadians[i], t);
        result.TireTemperatureCelsius[i] = Lerp(
            a.TireTemperatureCelsius[i],
            b.TireTemperatureCelsius[i], t);
        result.TireWearFraction[i] = Lerp(
            a.TireWearFraction[i],
            b.TireWearFraction[i], t);
    }
    return result;
}

inline TelemetryEnvironment InterpolateEnvironment(
    const TelemetryEnvironment& a,
    const TelemetryEnvironment& b,
    double fraction) noexcept
{
    const double t = Clamp01(fraction);
    TelemetryEnvironment result = t < 0.5 ? a : b;
    result.AirTemperatureCelsius = Lerp(
        a.AirTemperatureCelsius, b.AirTemperatureCelsius, t);
    result.TrackTemperatureCelsius = Lerp(
        a.TrackTemperatureCelsius,
        b.TrackTemperatureCelsius, t);
    result.AmbientWetness = Lerp(
        a.AmbientWetness, b.AmbientWetness, t);
    result.WindSpeedCmS = Lerp(
        a.WindSpeedCmS, b.WindSpeedCmS, t);
    result.WindDirectionRadians = WrapRadians(
        a.WindDirectionRadians +
        AngleDifference(a.WindDirectionRadians,
                        b.WindDirectionRadians) * t);
    result.VisibilityFraction = Lerp(
        a.VisibilityFraction, b.VisibilityFraction, t);
    result.RoadFrictionEstimate = Lerp(
        a.RoadFrictionEstimate,
        b.RoadFrictionEstimate, t);
    return result;
}

template<std::size_t Capacity>
class TelemetryReplaySampler final
{
public:
    bool Configure(
        const TelemetrySessionRecorder<Capacity>* recording,
        ReplaySamplerSettings settings = {}) noexcept
    {
        if (!recording || !ValidReplaySettings(settings))
        {
            return false;
        }
        Recording = recording;
        Settings = settings;
        Cursor = 0;
        PreviousSampleTime = 0.0;
        HasCursor = false;
        QueryCount = 0;
        RejectedQueries = 0;
        InterpolatedQueries = 0;
        return true;
    }

    void Reset() noexcept
    {
        Cursor = 0;
        PreviousSampleTime = 0.0;
        HasCursor = false;
        QueryCount = 0;
        RejectedQueries = 0;
        InterpolatedQueries = 0;
    }

    ReplayInterpolatedFrame Sample(double seconds) noexcept
    {
        QueryCount = SaturatingIncrement(QueryCount);
        ReplayInterpolatedFrame result;
        result.RequestedSeconds = seconds;
        if (!Recording || Recording->Empty() ||
            !Finite(seconds) || seconds < 0.0 ||
            seconds > Settings.MaximumRequestedTimeSeconds)
        {
            RejectedQueries = SaturatingIncrement(RejectedQueries);
            return result;
        }
        const std::size_t count = Recording->Size();
        const RecordedFrame first = Recording->AtOldest(0);
        const RecordedFrame last =
            Recording->AtNewest(0);
        const double beginning =
            first.Frame.MonotonicSeconds;
        const double ending = last.Frame.MonotonicSeconds;
        if (!Finite(beginning) || !Finite(ending) ||
            ending < beginning)
        {
            RejectedQueries = SaturatingIncrement(RejectedQueries);
            return result;
        }
        if (!Settings.ClampToRecordedRange &&
            (seconds < beginning || seconds > ending))
        {
            RejectedQueries = SaturatingIncrement(RejectedQueries);
            return result;
        }
        const double chosen = std::clamp(
            seconds, beginning, ending);
        if (count == 1 || chosen <= beginning)
        {
            return SingleSample(first, seconds);
        }
        if (chosen >= ending)
        {
            return SingleSample(last, seconds);
        }

        std::size_t left = 0;
        std::size_t right = count - 1;
        while (right > left + 1)
        {
            const std::size_t mid =
                left + (right - left) / 2;
            const double time =
                Recording->AtOldest(mid).Frame.MonotonicSeconds;
            if (time <= chosen)
            {
                left = mid;
            }
            else
            {
                right = mid;
            }
        }
        const RecordedFrame a = Recording->AtOldest(left);
        const RecordedFrame b = Recording->AtOldest(right);
        const double ta = a.Frame.MonotonicSeconds;
        const double tb = b.Frame.MonotonicSeconds;
        if (ta == chosen)
        {
            return SingleSample(a, seconds);
        }
        if (tb == chosen)
        {
            return SingleSample(b, seconds);
        }
        if (!a.Qualified || !b.Qualified ||
            !ReplayFrameIdentityMatches(a, b) ||
            (Settings.RequireIdenticalSegment &&
             a.Segment != b.Segment) ||
            (Settings.RequirePhysicsReadiness &&
             (!a.Frame.Race.EnginePhysicsReady ||
              !b.Frame.Race.EnginePhysicsReady)))
        {
            RejectedQueries = SaturatingIncrement(RejectedQueries);
            result.CrossedSegmentBoundary = a.Segment != b.Segment;
            return result;
        }
        const double gap = tb - ta;
        if (!Finite(gap) || gap <= 0.0 ||
            gap > Settings.MaximumInterpolationSeconds)
        {
            RejectedQueries = SaturatingIncrement(RejectedQueries);
            return result;
        }
        if (Settings.Mode ==
            ReplaySamplingMode::RejectAcrossGap)
        {
            RejectedQueries = SaturatingIncrement(RejectedQueries);
            return result;
        }
        if (Settings.Mode ==
            ReplaySamplingMode::HoldPrevious)
        {
            return SingleSample(a, seconds);
        }
        if (Settings.Mode ==
            ReplaySamplingMode::HoldNearest)
        {
            return SingleSample(
                chosen - ta <= tb - chosen ? a : b, seconds);
        }
        const double t = Clamp01((chosen - ta) / gap);
        TelemetryFrame out = a.Frame;
        out.Dynamics = InterpolateDynamics(
            a.Frame.Dynamics, b.Frame.Dynamics, t);
        out.Wheels = InterpolateWheels(
            a.Frame.Wheels, b.Frame.Wheels, t);
        out.Controls = InterpolateControls(
            a.Frame.Controls, b.Frame.Controls, t,
            Settings.PreserveDiscreteControls);
        out.Environment = InterpolateEnvironment(
            a.Frame.Environment, b.Frame.Environment, t);
        out.MonotonicSeconds = chosen;
        out.RaceClockSeconds = Lerp(
            a.Frame.RaceClockSeconds,
            b.Frame.RaceClockSeconds, t);
        out.SpeedometerKmH = Lerp(
            a.Frame.SpeedometerKmH,
            b.Frame.SpeedometerKmH, t);
        out.InputLatencySeconds = Lerp(
            a.Frame.InputLatencySeconds,
            b.Frame.InputLatencySeconds, t);
        out.DeltaSeconds = 0.0;
        result.Frame = out;
        result.SampledSeconds = chosen;
        result.LeftSeconds = ta;
        result.RightSeconds = tb;
        result.Blend = t;
        result.TemporalErrorSeconds = 0.0;
        result.LeftSequence = a.Sequence;
        result.RightSequence = b.Sequence;
        result.SourceSegment = a.Segment;
        result.Qualified = true;
        result.Interpolated = true;
        result.AuthoritativeGameState = false;
        InterpolatedQueries =
            SaturatingIncrement(InterpolatedQueries);
        Cursor = left;
        PreviousSampleTime = chosen;
        HasCursor = true;
        return result;
    }

    ReplayInterpolatedFrame SampleForward(
        double deltaSeconds) noexcept
    {
        if (!Finite(deltaSeconds) || deltaSeconds < 0.0 ||
            deltaSeconds > 2.0)
        {
            return {};
        }
        const double requested = HasCursor ?
            PreviousSampleTime + deltaSeconds :
            Recording && !Recording->Empty() ?
                Recording->AtOldest(0).Frame.MonotonicSeconds : 0.0;
        return Sample(requested);
    }

    bool Seek(double seconds) noexcept
    {
        const ReplayInterpolatedFrame sample = Sample(seconds);
        return sample.Qualified;
    }

    std::uint64_t Queries() const noexcept
    {
        return QueryCount;
    }

    std::uint64_t Rejected() const noexcept
    {
        return RejectedQueries;
    }

    std::uint64_t Interpolated() const noexcept
    {
        return InterpolatedQueries;
    }

    double LastSampledSeconds() const noexcept
    {
        return PreviousSampleTime;
    }

private:
    ReplayInterpolatedFrame SingleSample(
        RecordedFrame source, double seconds) noexcept
    {
        ReplayInterpolatedFrame result;
        result.Frame = source.Frame;
        result.RequestedSeconds = seconds;
        result.SampledSeconds =
            source.Frame.MonotonicSeconds;
        result.LeftSeconds = result.SampledSeconds;
        result.RightSeconds = result.SampledSeconds;
        result.TemporalErrorSeconds =
            std::abs(seconds - result.SampledSeconds);
        result.LeftSequence = source.Sequence;
        result.RightSequence = source.Sequence;
        result.SourceSegment = source.Segment;
        result.Qualified = source.Qualified;
        result.Interpolated = false;
        result.AuthoritativeGameState = false;
        PreviousSampleTime = result.SampledSeconds;
        HasCursor = true;
        return result;
    }

    const TelemetrySessionRecorder<Capacity>* Recording = nullptr;
    ReplaySamplerSettings Settings{};
    std::size_t Cursor = 0;
    double PreviousSampleTime = 0.0;
    bool HasCursor = false;
    std::uint64_t QueryCount = 0;
    std::uint64_t RejectedQueries = 0;
    std::uint64_t InterpolatedQueries = 0;
};
} // namespace latebrake::engineering
