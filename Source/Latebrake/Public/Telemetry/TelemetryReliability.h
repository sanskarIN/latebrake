#pragma once

#include "Telemetry/TelemetryValidation.h"
#include "Telemetry/TelemetryFilters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class SensorChannel : std::uint8_t
{
    Position,
    Velocity,
    Acceleration,
    Attitude,
    AngularVelocity,
    WheelSpeed,
    WheelLoad,
    WheelSlip,
    TireTemperature,
    TireWear,
    Throttle,
    Brake,
    Steering,
    Fuel,
    EngineTemperature,
    BrakeTemperature,
    RaceClock,
    SessionIdentity,
    TrackPavement,
    OpponentRoster
};

inline constexpr int SensorChannelCount = 20;

enum class SensorTrustStatus : std::uint8_t
{
    Unavailable,
    Qualified,
    Intermittent,
    Suspect,
    Quarantined,
    Recovered
};

enum class SensorFaultKind : std::uint8_t
{
    None,
    Missing,
    NonFinite,
    OutOfBounds,
    Spike,
    TimestampGap,
    ReversedTime,
    Stale,
    IdentityChanged,
    ImplausibleJump,
    UnverifiedSource
};

struct SensorReliabilitySettings final
{
    double MaximumSampleGapSeconds = 0.15;
    double MaximumNormalizedStep = 0.35;
    double MaximumAcceptedAccelerationG = 12.0;
    double MaximumYawRateRadiansPerSecond = 25.0;
    double MinimumConfidence = 0.70;
    double MinimumAvailableFraction = 0.75;
    double QuarantineAfterBadSeconds = 0.30;
    double RecoveryStableSeconds = 1.0;
    double WarningAfterBadSeconds = 0.15;
    double MaximumClockGapSeconds = 0.35;
    double MaximumInputDisagreement = 0.25;
    double MaximumSensorAgeSeconds = 0.25;
    double MaximumAllowedPositionJumpCm = 500.0;
    int MinimumConsecutiveSamples = 3;
    int MaximumConsecutiveFaults = 10;
    bool RequireEngineSource = true;
    bool RequireMatchingSession = true;
};

inline bool ValidSensorReliabilitySettings(
    const SensorReliabilitySettings& s) noexcept
{
    return Finite(s.MaximumSampleGapSeconds) &&
           s.MaximumSampleGapSeconds > 0.0 &&
           s.MaximumSampleGapSeconds <= 1.0 &&
           Finite(s.MaximumNormalizedStep) &&
           s.MaximumNormalizedStep > 0.0 &&
           Finite(s.MaximumAcceptedAccelerationG) &&
           s.MaximumAcceptedAccelerationG > 0.0 &&
           Finite(s.MaximumYawRateRadiansPerSecond) &&
           s.MaximumYawRateRadiansPerSecond > 0.0 &&
           Finite(s.MinimumConfidence) &&
           s.MinimumConfidence >= 0.0 &&
           s.MinimumConfidence <= 1.0 &&
           Finite(s.MinimumAvailableFraction) &&
           s.MinimumAvailableFraction > 0.0 &&
           s.MinimumAvailableFraction <= 1.0 &&
           Finite(s.QuarantineAfterBadSeconds) &&
           s.QuarantineAfterBadSeconds > 0.0 &&
           Finite(s.RecoveryStableSeconds) &&
           s.RecoveryStableSeconds > 0.0 &&
           Finite(s.WarningAfterBadSeconds) &&
           s.WarningAfterBadSeconds > 0.0 &&
           Finite(s.MaximumClockGapSeconds) &&
           s.MaximumClockGapSeconds > 0.0 &&
           Finite(s.MaximumInputDisagreement) &&
           s.MaximumInputDisagreement >= 0.0 &&
           Finite(s.MaximumSensorAgeSeconds) &&
           s.MaximumSensorAgeSeconds > 0.0 &&
           Finite(s.MaximumAllowedPositionJumpCm) &&
           s.MaximumAllowedPositionJumpCm > 0.0 &&
           s.MinimumConsecutiveSamples > 0 &&
           s.MaximumConsecutiveFaults > 0;
}

struct SensorChannelState final
{
    SensorChannel Channel = SensorChannel::Position;
    SensorTrustStatus Status = SensorTrustStatus::Unavailable;
    SensorFaultKind LastFault = SensorFaultKind::None;
    double LastObservedValue = 0.0;
    double LastAcceptedValue = 0.0;
    double LastObservedSeconds = 0.0;
    double LastAcceptedSeconds = 0.0;
    double BadSeconds = 0.0;
    double GoodSeconds = 0.0;
    double TotalAvailableSeconds = 0.0;
    double TotalMissingSeconds = 0.0;
    double MaximumAbsoluteJump = 0.0;
    double MeanAbsoluteJump = 0.0;
    double ReliabilityFraction = 0.0;
    double Confidence = 0.0;
    std::uint64_t QualifiedSamples = 0;
    std::uint64_t RejectedSamples = 0;
    std::uint64_t StaleSamples = 0;
    std::uint64_t QuarantineEpisodes = 0;
    std::uint64_t Recoveries = 0;
    int ConsecutiveGood = 0;
    int ConsecutiveFaults = 0;
    bool HasAcceptedBaseline = false;
};

class SensorChannelValidator final
{
public:
    void Reset(SensorChannel channel) noexcept
    {
        State = {};
        State.Channel = channel;
        AbsoluteDeviation.Reset();
        LastQuality = TelemetryQuality::Missing;
    }

    bool Observe(
        double value,
        double monotonicSeconds, double dt,
        double minimum, double maximum,
        bool sourceVerified,
        const SensorReliabilitySettings& settings) noexcept
    {
        if (!Finite(monotonicSeconds) || !Finite(dt) ||
            dt <= 0.0 || dt > 1.0 ||
            !Finite(minimum) || !Finite(maximum) ||
            minimum >= maximum)
        {
            return Reject(
                SensorFaultKind::TimestampGap, dt, settings);
        }
        if (State.HasAcceptedBaseline &&
            monotonicSeconds <= State.LastAcceptedSeconds)
        {
            State.StaleSamples =
                SaturatingIncrement(State.StaleSamples);
            return Reject(SensorFaultKind::Stale, dt, settings);
        }
        if (!sourceVerified && settings.RequireEngineSource)
        {
            return Reject(
                SensorFaultKind::UnverifiedSource, dt, settings);
        }
        if (!Finite(value))
        {
            return Reject(SensorFaultKind::NonFinite, dt, settings);
        }
        if (value < minimum || value > maximum)
        {
            return Reject(SensorFaultKind::OutOfBounds, dt, settings);
        }
        const double width = maximum - minimum;
        const double delta = State.HasAcceptedBaseline ?
            std::abs(value - State.LastAcceptedValue) : 0.0;
        if (State.HasAcceptedBaseline &&
            delta > width * settings.MaximumNormalizedStep &&
            dt < settings.MaximumSampleGapSeconds)
        {
            return Reject(SensorFaultKind::Spike, dt, settings);
        }
        State.LastObservedValue = value;
        State.LastObservedSeconds = monotonicSeconds;
        State.LastAcceptedValue = value;
        State.LastAcceptedSeconds = monotonicSeconds;
        State.QualifiedSamples =
            SaturatingIncrement(State.QualifiedSamples);
        State.GoodSeconds += dt;
        State.BadSeconds = 0.0;
        State.TotalAvailableSeconds += dt;
        State.ConsecutiveFaults = 0;
        State.ConsecutiveGood = std::min(
            State.ConsecutiveGood + 1, 1000000);
        State.MaximumAbsoluteJump =
            std::max(State.MaximumAbsoluteJump, delta);
        AbsoluteDeviation.Observe(delta);
        State.MeanAbsoluteJump = AbsoluteDeviation.Mean();
        State.HasAcceptedBaseline = true;
        const double total =
            State.TotalAvailableSeconds +
            State.TotalMissingSeconds;
        State.ReliabilityFraction = Clamp01(
            SafeRatio(State.TotalAvailableSeconds, total));
        State.Confidence =
            State.ReliabilityFraction *
            std::clamp(
                static_cast<double>(State.ConsecutiveGood) /
                static_cast<double>(settings.MinimumConsecutiveSamples),
                0.0, 1.0);
        if (State.Status == SensorTrustStatus::Quarantined)
        {
            if (State.GoodSeconds >=
                settings.RecoveryStableSeconds &&
                State.ConsecutiveGood >=
                settings.MinimumConsecutiveSamples)
            {
                State.Status = SensorTrustStatus::Recovered;
                State.Recoveries =
                    SaturatingIncrement(State.Recoveries);
            }
        }
        else
        {
            State.Status = State.Confidence >=
                settings.MinimumConfidence ?
                    SensorTrustStatus::Qualified :
                    SensorTrustStatus::Intermittent;
        }
        State.LastFault = SensorFaultKind::None;
        LastQuality = State.Confidence >=
            settings.MinimumConfidence ?
                TelemetryQuality::Qualified :
                TelemetryQuality::Suspect;
        return true;
    }

    bool MarkMissing(
        double dt,
        const SensorReliabilitySettings& settings) noexcept
    {
        return Reject(SensorFaultKind::Missing, dt, settings);
    }

    SensorChannelState Snapshot() const noexcept { return State; }
    TelemetryQuality Quality() const noexcept { return LastQuality; }
    bool Trusted(double minimumConfidence) const noexcept
    {
        return (State.Status == SensorTrustStatus::Qualified ||
                State.Status == SensorTrustStatus::Recovered) &&
            State.Confidence >= minimumConfidence;
    }

private:
    bool Reject(
        SensorFaultKind reason, double dt,
        const SensorReliabilitySettings& settings) noexcept
    {
        const double step = Finite(dt) && dt > 0.0 ?
            std::min(dt, 1.0) : 0.0;
        State.RejectedSamples =
            SaturatingIncrement(State.RejectedSamples);
        State.BadSeconds += step;
        State.GoodSeconds = 0.0;
        State.TotalMissingSeconds += step;
        State.ConsecutiveFaults = std::min(
            State.ConsecutiveFaults + 1, 1000000);
        State.ConsecutiveGood = 0;
        State.LastFault = reason;
        const double total =
            State.TotalAvailableSeconds +
            State.TotalMissingSeconds;
        State.ReliabilityFraction = Clamp01(
            SafeRatio(State.TotalAvailableSeconds, total));
        State.Confidence = 0.0;
        if (State.Status != SensorTrustStatus::Quarantined &&
            (State.BadSeconds >=
                settings.QuarantineAfterBadSeconds ||
             State.ConsecutiveFaults >=
                settings.MaximumConsecutiveFaults))
        {
            State.Status = SensorTrustStatus::Quarantined;
            State.QuarantineEpisodes =
                SaturatingIncrement(State.QuarantineEpisodes);
        }
        else if (State.Status != SensorTrustStatus::Quarantined)
        {
            State.Status = SensorTrustStatus::Suspect;
        }
        LastQuality = TelemetryQuality::Invalid;
        return false;
    }

    SensorChannelState State{};
    RunningStatistics AbsoluteDeviation;
    TelemetryQuality LastQuality = TelemetryQuality::Missing;
};

struct ReliabilitySnapshot final
{
    std::array<SensorChannelState, SensorChannelCount> Channels{};
    double OverallConfidence = 0.0;
    double PhysicalSensorConfidence = 0.0;
    double DriverInputConfidence = 0.0;
    double TimingConfidence = 0.0;
    double WheelConfidence = 0.0;
    double AvailableFraction = 0.0;
    double TotalQualifiedSeconds = 0.0;
    double TotalRejectedSeconds = 0.0;
    double LongestDiscontinuitySeconds = 0.0;
    int QualifiedChannels = 0;
    int SuspectChannels = 0;
    int QuarantinedChannels = 0;
    int UnavailableChannels = 0;
    std::uint64_t QualifiedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    std::uint64_t EpochResets = 0;
    std::uint64_t ClockFaults = 0;
    std::uint64_t PhysicsFaults = 0;
    bool SuitableForPerformanceComparison = false;
    bool SuitableForSafetyEvent = false;
    bool Qualified = false;
};

struct ReliabilityInput final
{
    std::array<double, SensorChannelCount> Values{};
    std::array<bool, SensorChannelCount> Available{};
    std::array<bool, SensorChannelCount> NativeVerified{};
    double MonotonicSeconds = 0.0;
    double DeltaSeconds = 0.0;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    int SessionRevision = 0;
};

inline ReliabilityInput ExtractReliabilityInput(
    const TelemetryFrame& frame) noexcept
{
    ReliabilityInput result;
    result.Values = {
        Length(frame.Dynamics.WorldPositionCm),
        Length(frame.Dynamics.WorldVelocityCmS),
        Length(frame.Dynamics.WorldAccelerationCmS2),
        frame.Dynamics.YawRadians,
        Length(frame.Dynamics.AngularVelocityRadS),
        frame.Wheels.AngularSpeedRadS[0],
        frame.Wheels.NormalLoadNewtons[0],
        frame.Wheels.SlipRatio[0],
        frame.Wheels.TireTemperatureCelsius[0],
        frame.Wheels.TireWearFraction[0],
        frame.Controls.Throttle,
        frame.Controls.Brake,
        frame.Controls.Steering,
        frame.Dynamics.FuelFraction,
        frame.Dynamics.EngineTemperatureCelsius,
        frame.Dynamics.BrakeTemperatureCelsius,
        frame.RaceClockSeconds,
        static_cast<double>(frame.Identity.SessionRevision),
        frame.Environment.OnPhysicalPavement ? 1.0 : 0.0,
        frame.Race.EnginePhysicsReady ? 1.0 : 0.0
    };
    result.Available.fill(true);
    const bool native =
        frame.Race.EnginePhysicsReady &&
        IsEngineObservation(frame.Identity.Trust);
    result.NativeVerified.fill(native);
    const bool wheelsPresent =
        WheelContactFraction(frame.Wheels) > 0.0;
    for (int i = 5; i <= 9; ++i)
    {
        result.Available[static_cast<std::size_t>(i)] =
            wheelsPresent;
    }
    result.MonotonicSeconds = frame.MonotonicSeconds;
    result.DeltaSeconds = frame.DeltaSeconds;
    result.SessionToken = frame.Identity.SessionToken;
    result.ActorToken = frame.Identity.ActorToken;
    result.SessionRevision = frame.Identity.SessionRevision;
    return result;
}

class TelemetryReliabilityMonitor final
{
public:
    bool Configure(SensorReliabilitySettings settings) noexcept
    {
        if (!ValidSensorReliabilitySettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        for (int i = 0; i < SensorChannelCount; ++i)
        {
            Channels[static_cast<std::size_t>(i)].Reset(
                static_cast<SensorChannel>(i));
        }
        Last = {};
        Session = 0;
        Actor = 0;
        Revision = -1;
        PreviousSeconds = 0.0;
        HasPrevious = false;
        FaultStreakSeconds = 0.0;
    }

    bool Observe(ReliabilityInput input) noexcept
    {
        if (!ValidSensorReliabilitySettings(Settings) ||
            !Finite(input.MonotonicSeconds) ||
            !Finite(input.DeltaSeconds) ||
            input.DeltaSeconds <= 0.0 ||
            input.DeltaSeconds > Settings.MaximumSampleGapSeconds ||
            input.SessionToken == 0 ||
            input.ActorToken == 0)
        {
            Last.RejectedFrames =
                SaturatingIncrement(Last.RejectedFrames);
            FaultStreakSeconds +=
                Finite(input.DeltaSeconds) ?
                std::max(0.0, input.DeltaSeconds) : 0.0;
            Last.LongestDiscontinuitySeconds = std::max(
                Last.LongestDiscontinuitySeconds, FaultStreakSeconds);
            return false;
        }
        if (Session != 0 &&
            (input.SessionToken != Session ||
             input.ActorToken != Actor ||
             input.SessionRevision != Revision))
        {
            const std::uint64_t priorResets = Last.EpochResets;
            Reset();
            Last.EpochResets =
                SaturatingIncrement(priorResets);
        }
        Session = input.SessionToken;
        Actor = input.ActorToken;
        Revision = input.SessionRevision;
        if (HasPrevious &&
            (input.MonotonicSeconds <= PreviousSeconds ||
             input.MonotonicSeconds - PreviousSeconds >
                Settings.MaximumClockGapSeconds))
        {
            Last.ClockFaults =
                SaturatingIncrement(Last.ClockFaults);
            Last.RejectedFrames =
                SaturatingIncrement(Last.RejectedFrames);
            FaultStreakSeconds += input.DeltaSeconds;
            HasPrevious = false;
            return false;
        }
        static constexpr std::array<double, SensorChannelCount> minimum{
            0.0, 0.0, 0.0, -Pi, 0.0,
            -1500.0, 0.0, -12.0, -100.0, 0.0,
            0.0, 0.0, -1.0, 0.0, -100.0,
            -100.0, 0.0, 0.0, 0.0, 0.0
        };
        static constexpr std::array<double, SensorChannelCount> maximum{
            20000000.0, 16000.0, 8000.0, Pi, 25.0,
            1500.0, 250000.0, 12.0, 300.0, 1.0,
            1.0, 1.0, 1.0, 1.0, 1000.0,
            2000.0, MaximumTelemetrySeconds, 100000.0, 1.0, 1.0
        };
        int available = 0;
        int trusted = 0;
        double confidenceSum = 0.0;
        double physicalConfidenceSum = 0.0;
        double driverConfidenceSum = 0.0;
        double timingConfidenceSum = 0.0;
        double wheelConfidenceSum = 0.0;
        int physicsCount = 0;
        int wheelCount = 0;
        for (int i = 0; i < SensorChannelCount; ++i)
        {
            const std::size_t index =
                static_cast<std::size_t>(i);
            SensorChannelValidator& channel = Channels[index];
            const bool present = input.Available[index];
            const bool valid = present ?
                channel.Observe(
                    input.Values[index],
                    input.MonotonicSeconds,
                    input.DeltaSeconds,
                    minimum[index], maximum[index],
                    input.NativeVerified[index], Settings) :
                channel.MarkMissing(input.DeltaSeconds, Settings);
            const SensorChannelState state = channel.Snapshot();
            if (present)
            {
                ++available;
            }
            if (valid && channel.Trusted(Settings.MinimumConfidence))
            {
                ++trusted;
            }
            confidenceSum += state.Confidence;
            if (i < 5)
            {
                physicalConfidenceSum += state.Confidence;
                ++physicsCount;
            }
            else if (i < 10)
            {
                wheelConfidenceSum += state.Confidence;
                ++wheelCount;
            }
            else if (i <= 12)
            {
                driverConfidenceSum += state.Confidence;
            }
            else if (i == 16 || i == 17)
            {
                timingConfidenceSum += state.Confidence;
            }
            Last.Channels[index] = state;
        }
        Last.AvailableFraction =
            static_cast<double>(available) /
            static_cast<double>(SensorChannelCount);
        Last.OverallConfidence =
            confidenceSum / static_cast<double>(SensorChannelCount);
        Last.PhysicalSensorConfidence =
            SafeRatio(physicalConfidenceSum, physicsCount);
        Last.DriverInputConfidence =
            driverConfidenceSum / 3.0;
        Last.TimingConfidence =
            timingConfidenceSum / 2.0;
        Last.WheelConfidence =
            SafeRatio(wheelConfidenceSum, wheelCount);
        Last.QualifiedChannels = trusted;
        Last.SuspectChannels = 0;
        Last.QuarantinedChannels = 0;
        Last.UnavailableChannels = 0;
        for (const SensorChannelState& channel : Last.Channels)
        {
            Last.SuspectChannels +=
                channel.Status == SensorTrustStatus::Suspect ? 1 : 0;
            Last.QuarantinedChannels +=
                channel.Status == SensorTrustStatus::Quarantined ? 1 : 0;
            Last.UnavailableChannels +=
                channel.Status == SensorTrustStatus::Unavailable ? 1 : 0;
        }
        Last.SuitableForPerformanceComparison =
            Last.PhysicalSensorConfidence >= Settings.MinimumConfidence &&
            Last.TimingConfidence >= Settings.MinimumConfidence &&
            Last.DriverInputConfidence >= Settings.MinimumConfidence;
        Last.SuitableForSafetyEvent =
            Last.PhysicalSensorConfidence >=
                Settings.MinimumConfidence &&
            Last.WheelConfidence >= 0.5;
        Last.Qualified =
            Last.AvailableFraction >=
                Settings.MinimumAvailableFraction &&
            Last.OverallConfidence >= Settings.MinimumConfidence;
        if (Last.Qualified)
        {
            Last.QualifiedFrames =
                SaturatingIncrement(Last.QualifiedFrames);
            Last.TotalQualifiedSeconds += input.DeltaSeconds;
            FaultStreakSeconds = 0.0;
        }
        else
        {
            Last.RejectedFrames =
                SaturatingIncrement(Last.RejectedFrames);
            Last.TotalRejectedSeconds += input.DeltaSeconds;
            FaultStreakSeconds += input.DeltaSeconds;
        }
        Last.LongestDiscontinuitySeconds = std::max(
            Last.LongestDiscontinuitySeconds, FaultStreakSeconds);
        PreviousSeconds = input.MonotonicSeconds;
        HasPrevious = true;
        return Last.Qualified;
    }

    ReliabilitySnapshot Snapshot() const noexcept { return Last; }
    SensorChannelState Channel(SensorChannel channel) const noexcept
    {
        const std::size_t index =
            static_cast<std::size_t>(channel);
        return index < Channels.size() ?
            Channels[index].Snapshot() : SensorChannelState{};
    }
    bool CanCompareLaps() const noexcept
    {
        return Last.SuitableForPerformanceComparison;
    }
    bool CanReportPhysicalRisk() const noexcept
    {
        return Last.SuitableForSafetyEvent;
    }

private:
    SensorReliabilitySettings Settings{};
    std::array<SensorChannelValidator, SensorChannelCount> Channels{};
    ReliabilitySnapshot Last{};
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    int Revision = -1;
    double PreviousSeconds = 0.0;
    double FaultStreakSeconds = 0.0;
    bool HasPrevious = false;
};
} // namespace latebrake::engineering
