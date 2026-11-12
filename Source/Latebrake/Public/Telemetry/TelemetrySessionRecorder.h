#pragma once

#include "Telemetry/TelemetryValidation.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class RecordingMode : std::uint8_t
{
    Disabled,
    ObserveOnly,
    QualifiedEngineOnly
};

enum class RecordingStopReason : std::uint8_t
{
    None,
    UserStopped,
    SessionChanged,
    ActorChanged,
    ClockDiscontinuity,
    InvalidPhysics,
    Overrun,
    RaceFinished
};

struct RecordingConfiguration final
{
    RecordingMode Mode = RecordingMode::QualifiedEngineOnly;
    double MaximumGapSeconds = 0.15;
    double MinimumIntervalSeconds = 0.001;
    int DownsampleStride = 1;
    bool KeepMostRecentFrames = true;
    bool SuspendOnFault = true;
    bool RecordOnCaution = true;
    bool RequireActiveRace = true;
    bool PreserveQualifiedEventHistory = true;
};

inline bool ValidRecordingConfiguration(
    const RecordingConfiguration& value) noexcept
{
    return Finite(value.MaximumGapSeconds) &&
           Finite(value.MinimumIntervalSeconds) &&
           value.MaximumGapSeconds > value.MinimumIntervalSeconds &&
           value.MaximumGapSeconds <= 5.0 &&
           value.MinimumIntervalSeconds > 0.0 &&
           value.DownsampleStride >= 1 &&
           value.DownsampleStride <= 1000;
}

struct RecordedFrame final
{
    TelemetryFrame Frame{};
    TelemetryFrameDecision Gate{};
    std::uint64_t Sequence = 0;
    int Segment = 0;
    bool Qualified = false;
};

struct RecordingReceipt final
{
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    int DriverSlot = -1;
    int Revision = -1;
    int SegmentCount = 0;
    int CapturedFrames = 0;
    int QualifiedFrames = 0;
    int DroppedFrames = 0;
    int RejectedFrames = 0;
    std::uint64_t TotalAccepted = 0;
    std::uint64_t TotalRejected = 0;
    std::uint64_t TotalOverwrites = 0;
    std::uint64_t TotalDiscontinuities = 0;
    double StartedSeconds = 0.0;
    double EndedSeconds = 0.0;
    double DurationSeconds = 0.0;
    double LastQualifiedSeconds = 0.0;
    double SampledDistanceCm = 0.0;
    RecordingStopReason StopReason = RecordingStopReason::None;
    bool Complete = false;
    bool WasEngineObserved = false;
    bool HasEvidence = false;
};

template<std::size_t Capacity>
class TelemetrySessionRecorder final
{
    static_assert(Capacity >= 4 && Capacity <= 16384);
public:
    void Reset() noexcept
    {
        Buffer.fill({});
        Configuration = {};
        Gate.Reset();
        Head = 0;
        Count = 0;
        Segment = 0;
        DownsampleCounter = 0;
        Sequence = 0;
        Recording = false;
        Suspended = false;
        HasSession = false;
        Receipt = {};
    }

    bool Configure(
        RecordingConfiguration config,
        TelemetryGateSettings settings = {}) noexcept
    {
        if (!ValidRecordingConfiguration(config) ||
            !ValidTelemetryGateSettings(settings))
        {
            return false;
        }
        Reset();
        Configuration = config;
        Gate.SetSettings(settings);
        return true;
    }

    bool Start(const TelemetryFrame& first) noexcept
    {
        if (Recording || !ValidRecordingConfiguration(Configuration) ||
            Configuration.Mode == RecordingMode::Disabled ||
            ValidateFrameFields(first, Gate.GetSettings()) !=
                TelemetryReject::None)
        {
            return false;
        }
        ResetCapture();
        Receipt.Session = first.Identity.SessionToken;
        Receipt.Actor = first.Identity.ActorToken;
        Receipt.DriverSlot = first.Identity.DriverSlot;
        Receipt.Revision = first.Identity.SessionRevision;
        Receipt.StartedSeconds = first.MonotonicSeconds;
        Receipt.LastQualifiedSeconds = first.MonotonicSeconds;
        Receipt.WasEngineObserved =
            IsEngineObservation(first.Identity.Trust);
        Recording = true;
        Suspended = false;
        HasSession = true;
        return Observe(first);
    }

    bool Observe(const TelemetryFrame& frame) noexcept
    {
        if (!Recording || Suspended ||
            Configuration.Mode == RecordingMode::Disabled)
        {
            return false;
        }
        const auto stopOnFault =
            [this](RecordingStopReason reason) noexcept
        {
            Receipt.TotalRejected =
                SaturatingIncrement(Receipt.TotalRejected);
            ++Receipt.RejectedFrames;
            if (Configuration.SuspendOnFault)
            {
                Stop(reason);
            }
        };
        if (!HasSession ||
            frame.Identity.SessionToken != Receipt.Session)
        {
            stopOnFault(RecordingStopReason::SessionChanged);
            return false;
        }
        if (frame.Identity.ActorToken != Receipt.Actor ||
            frame.Identity.DriverSlot != Receipt.DriverSlot ||
            frame.Identity.SessionRevision != Receipt.Revision)
        {
            stopOnFault(RecordingStopReason::ActorChanged);
            return false;
        }
        if (Configuration.RequireActiveRace &&
            !IsRaceMoving(frame.Race.Phase))
        {
            stopOnFault(RecordingStopReason::RaceFinished);
            return false;
        }
        if (!Configuration.RecordOnCaution &&
            frame.Race.Phase == TelemetryPhase::Caution)
        {
            ++Receipt.DroppedFrames;
            return false;
        }
        if (Configuration.Mode ==
                RecordingMode::QualifiedEngineOnly &&
            !IsEngineObservation(frame.Identity.Trust))
        {
            stopOnFault(RecordingStopReason::InvalidPhysics);
            return false;
        }
        const TelemetryFrameDecision decision = Gate.Observe(frame);
        if (!decision.Accepted)
        {
            if (decision.Reason == TelemetryReject::ClockReversed ||
                decision.Reason == TelemetryReject::InvalidClock ||
                decision.Reason == TelemetryReject::ImpossibleJump)
            {
                Receipt.TotalDiscontinuities =
                    SaturatingIncrement(Receipt.TotalDiscontinuities);
            }
            stopOnFault(RecordingStopReason::ClockDiscontinuity);
            return false;
        }
        ++DownsampleCounter;
        if ((DownsampleCounter - 1) %
            Configuration.DownsampleStride != 0)
        {
            ++Receipt.DroppedFrames;
            return true;
        }
        if (Count == Capacity && !Configuration.KeepMostRecentFrames)
        {
            Stop(RecordingStopReason::Overrun);
            return false;
        }
        if (decision.StartNewSegment)
        {
            ++Segment;
        }
        if (Count == Capacity)
        {
            ++Receipt.TotalOverwrites;
            ++Head;
            Head %= Capacity;
        }
        else
        {
            ++Count;
        }
        const std::size_t destination =
            (Head + Count - 1) % Capacity;
        RecordedFrame& record = Buffer[destination];
        record.Frame = frame;
        record.Gate = decision;
        record.Sequence = ++Sequence;
        record.Segment = Segment;
        record.Qualified = decision.Quality ==
            TelemetryQuality::Qualified;
        Receipt.EndedSeconds = frame.MonotonicSeconds;
        Receipt.LastQualifiedSeconds = frame.MonotonicSeconds;
        Receipt.DurationSeconds = std::max(
            0.0, Receipt.EndedSeconds - Receipt.StartedSeconds);
        Receipt.CapturedFrames = static_cast<int>(Count);
        Receipt.QualifiedFrames = static_cast<int>(Count);
        Receipt.TotalAccepted =
            SaturatingIncrement(Receipt.TotalAccepted);
        Receipt.SampledDistanceCm +=
            decision.DistanceSincePreviousCm;
        Receipt.SegmentCount = Segment;
        Receipt.HasEvidence = true;
        return true;
    }

    void Stop(
        RecordingStopReason reason =
            RecordingStopReason::UserStopped) noexcept
    {
        if (!Recording)
        {
            return;
        }
        Recording = false;
        Suspended = true;
        Receipt.StopReason = reason;
        Receipt.Complete = reason ==
            RecordingStopReason::UserStopped ||
            reason == RecordingStopReason::RaceFinished;
    }

    void Resume() noexcept
    {
        if (!HasSession ||
            Configuration.Mode == RecordingMode::Disabled)
        {
            return;
        }
        Gate.Suspend();
        Suspended = false;
        Recording = true;
        ++Segment;
    }

    void ClearRecordedFrames() noexcept
    {
        Buffer.fill({});
        Head = 0;
        Count = 0;
        Receipt.CapturedFrames = 0;
        Receipt.QualifiedFrames = 0;
        Receipt.HasEvidence = false;
    }

    bool Active() const noexcept
    {
        return Recording && !Suspended;
    }

    bool SuspendedOnFault() const noexcept
    {
        return Suspended;
    }

    std::size_t Size() const noexcept
    {
        return Count;
    }

    std::size_t CapacityValue() const noexcept
    {
        return Capacity;
    }

    bool Empty() const noexcept
    {
        return Count == 0;
    }

    RecordedFrame AtOldest(std::size_t index) const noexcept
    {
        return index < Count ?
            Buffer[(Head + index) % Capacity] :
            RecordedFrame{};
    }

    RecordedFrame AtNewest(std::size_t offset = 0) const noexcept
    {
        return offset < Count ?
            Buffer[(Head + Count - offset - 1) % Capacity] :
            RecordedFrame{};
    }

    RecordedFrame FindSequence(std::uint64_t sequence) const noexcept
    {
        if (sequence == 0)
        {
            return {};
        }
        for (std::size_t i = 0; i < Count; ++i)
        {
            const RecordedFrame& frame =
                Buffer[(Head + i) % Capacity];
            if (frame.Sequence == sequence)
            {
                return frame;
            }
        }
        return {};
    }

    std::size_t FramesInSegment(int segment) const noexcept
    {
        std::size_t result = 0;
        for (std::size_t i = 0; i < Count; ++i)
        {
            const RecordedFrame& frame =
                Buffer[(Head + i) % Capacity];
            result += frame.Segment == segment ? 1U : 0U;
        }
        return result;
    }

    std::size_t FramesInTimeRange(
        double beginSeconds, double endSeconds) const noexcept
    {
        if (!Finite(beginSeconds) || !Finite(endSeconds) ||
            beginSeconds > endSeconds)
        {
            return 0;
        }
        std::size_t result = 0;
        for (std::size_t i = 0; i < Count; ++i)
        {
            const double timestamp =
                AtOldest(i).Frame.MonotonicSeconds;
            result += timestamp >= beginSeconds &&
                timestamp <= endSeconds ? 1U : 0U;
        }
        return result;
    }

    RecordedFrame NearestTime(double seconds) const noexcept
    {
        RecordedFrame best;
        if (!Finite(seconds))
        {
            return best;
        }
        double minimum = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < Count; ++i)
        {
            const RecordedFrame frame = AtOldest(i);
            const double error =
                std::abs(frame.Frame.MonotonicSeconds - seconds);
            if (error < minimum)
            {
                minimum = error;
                best = frame;
            }
        }
        return best;
    }

    RecordingReceipt Snapshot() const noexcept
    {
        return Receipt;
    }

    TelemetryCounters GateCounters() const noexcept
    {
        return Gate.Snapshot();
    }

    RecordingConfiguration GetConfiguration() const noexcept
    {
        return Configuration;
    }

    void NotifyExternalDiscontinuity() noexcept
    {
        Gate.Suspend();
        ++Segment;
        Receipt.TotalDiscontinuities =
            SaturatingIncrement(Receipt.TotalDiscontinuities);
        if (Configuration.SuspendOnFault)
        {
            Stop(RecordingStopReason::ClockDiscontinuity);
        }
    }

private:
    void ResetCapture() noexcept
    {
        Buffer.fill({});
        Gate.Reset();
        Head = 0;
        Count = 0;
        Segment = 0;
        DownsampleCounter = 0;
        Sequence = 0;
        Receipt = {};
        HasSession = false;
        Recording = false;
        Suspended = false;
    }

    std::array<RecordedFrame, Capacity> Buffer{};
    RecordingConfiguration Configuration{};
    TelemetryFrameGate Gate{};
    RecordingReceipt Receipt{};
    std::size_t Head = 0;
    std::size_t Count = 0;
    int Segment = 0;
    int DownsampleCounter = 0;
    std::uint64_t Sequence = 0;
    bool Recording = false;
    bool Suspended = false;
    bool HasSession = false;
};

struct RecordingHealthAssessment final
{
    double AcceptedFraction = 0.0;
    double RejectFraction = 0.0;
    double CaptureFraction = 0.0;
    double AverageSampleRateHz = 0.0;
    double OverwriteFraction = 0.0;
    double DistanceKm = 0.0;
    bool NoMissingIdentity = false;
    bool EngineQualified = false;
    bool ReplaySafe = false;
    bool TrustworthyForOfficialTiming = false;
};

inline RecordingHealthAssessment AssessRecordingHealth(
    RecordingReceipt receipt) noexcept
{
    RecordingHealthAssessment health;
    const double attempts = static_cast<double>(
        receipt.TotalAccepted + receipt.TotalRejected);
    const double handled = attempts +
        static_cast<double>(receipt.DroppedFrames);
    health.AcceptedFraction = SafeRatio(
        static_cast<double>(receipt.TotalAccepted), attempts);
    health.RejectFraction = SafeRatio(
        static_cast<double>(receipt.TotalRejected), attempts);
    health.CaptureFraction = SafeRatio(
        static_cast<double>(receipt.CapturedFrames), handled);
    health.AverageSampleRateHz = SafeRatio(
        static_cast<double>(receipt.CapturedFrames),
        receipt.DurationSeconds);
    health.OverwriteFraction = SafeRatio(
        static_cast<double>(receipt.TotalOverwrites),
        static_cast<double>(receipt.TotalAccepted));
    health.DistanceKm = receipt.SampledDistanceCm / CmPerKilometer;
    health.NoMissingIdentity = receipt.Session != 0 &&
        receipt.Actor != 0 &&
        receipt.DriverSlot >= 0 &&
        receipt.DriverSlot < MaximumTelemetryDrivers;
    health.EngineQualified = receipt.WasEngineObserved;
    health.ReplaySafe = receipt.HasEvidence &&
        health.NoMissingIdentity &&
        receipt.TotalDiscontinuities == 0 &&
        receipt.TotalRejected == 0 &&
        health.EngineQualified;
    health.TrustworthyForOfficialTiming = false;
    return health;
}
} // namespace latebrake::engineering
