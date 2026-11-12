#pragma once

#include "Telemetry/TelemetryTypes.h"
#include "Telemetry/TelemetryFilters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class EventSeverity : std::uint8_t
{
    Informational,
    DrivingNote,
    PerformanceLoss,
    PhysicalRisk,
    SensorFault,
    Critical
};

enum class EventSource : std::uint8_t
{
    None,
    NativeRace,
    NativePhysics,
    QualifiedTelemetry,
    DerivedEstimate,
    ImportedReplay
};

enum class EventChannel : std::uint8_t
{
    Session,
    Timing,
    Driving,
    Braking,
    Cornering,
    Grip,
    Opponents,
    Environment,
    Reliability,
    Strategy,
    Recovery,
    System
};

struct EventJournalSettings final
{
    double MaximumEventAgeSeconds = 3600.0;
    double MinimumSeparationSeconds = 0.05;
    double MaximumMergeGapSeconds = 0.25;
    double MaximumDurationSeconds = 120.0;
    double MinimumMagnitude = 0.0;
    double MaximumMagnitude = 1.0e9;
    int MaximumEvents = MaximumTelemetryEvents;
    bool RequireVerifiedIdentity = true;
    bool AllowDerivedEstimates = true;
    bool CoalesceAdjacentEvents = true;
    bool RetainInvalidEvents = false;
};

inline bool ValidEventJournalSettings(
    const EventJournalSettings& s) noexcept
{
    return Finite(s.MaximumEventAgeSeconds) &&
           s.MaximumEventAgeSeconds > 0.0 &&
           Finite(s.MinimumSeparationSeconds) &&
           s.MinimumSeparationSeconds >= 0.0 &&
           Finite(s.MaximumMergeGapSeconds) &&
           s.MaximumMergeGapSeconds >= 0.0 &&
           s.MaximumMergeGapSeconds <= 10.0 &&
           Finite(s.MaximumDurationSeconds) &&
           s.MaximumDurationSeconds > 0.0 &&
           Finite(s.MinimumMagnitude) &&
           s.MinimumMagnitude >= 0.0 &&
           Finite(s.MaximumMagnitude) &&
           s.MaximumMagnitude > s.MinimumMagnitude &&
           s.MaximumEvents > 0 &&
           s.MaximumEvents <= MaximumTelemetryEvents;
}

inline EventChannel ClassifyEventChannel(
    TelemetryEventKind kind) noexcept
{
    switch (kind)
    {
    case TelemetryEventKind::LapStart:
    case TelemetryEventKind::SectorSplit:
    case TelemetryEventKind::LapComplete:
        return EventChannel::Timing;
    case TelemetryEventKind::BrakeApplication:
    case TelemetryEventKind::BrakeRelease:
    case TelemetryEventKind::RiskBrake:
        return EventChannel::Braking;
    case TelemetryEventKind::CornerEntry:
    case TelemetryEventKind::Apex:
    case TelemetryEventKind::CornerExit:
        return EventChannel::Cornering;
    case TelemetryEventKind::TractionLimit:
    case TelemetryEventKind::WheelSlip:
    case TelemetryEventKind::TireCritical:
        return EventChannel::Grip;
    case TelemetryEventKind::Overtake:
    case TelemetryEventKind::PositionLost:
    case TelemetryEventKind::CarContact:
        return EventChannel::Opponents;
    case TelemetryEventKind::OffTrack:
    case TelemetryEventKind::WallContact:
        return EventChannel::Driving;
    case TelemetryEventKind::Recovery:
        return EventChannel::Recovery;
    case TelemetryEventKind::RaceInterrupted:
        return EventChannel::Session;
    case TelemetryEventKind::SensorDiscontinuity:
    case TelemetryEventKind::ObservationRejected:
        return EventChannel::Reliability;
    case TelemetryEventKind::EngineOverheat:
        return EventChannel::Environment;
    case TelemetryEventKind::FuelLow:
        return EventChannel::Strategy;
    case TelemetryEventKind::Unknown:
        return EventChannel::System;
    }
    return EventChannel::System;
}

inline EventSeverity ClassifyEventSeverity(
    TelemetryEventKind kind,
    double magnitude) noexcept
{
    switch (kind)
    {
    case TelemetryEventKind::SensorDiscontinuity:
    case TelemetryEventKind::ObservationRejected:
        return EventSeverity::SensorFault;
    case TelemetryEventKind::WallContact:
    case TelemetryEventKind::CarContact:
    case TelemetryEventKind::EngineOverheat:
        return EventSeverity::Critical;
    case TelemetryEventKind::OffTrack:
    case TelemetryEventKind::TireCritical:
    case TelemetryEventKind::RiskBrake:
        return EventSeverity::PhysicalRisk;
    case TelemetryEventKind::WheelSlip:
    case TelemetryEventKind::TractionLimit:
        return magnitude >= 0.8 ?
            EventSeverity::PhysicalRisk :
            EventSeverity::PerformanceLoss;
    case TelemetryEventKind::PositionLost:
    case TelemetryEventKind::FuelLow:
    case TelemetryEventKind::Recovery:
        return EventSeverity::PerformanceLoss;
    case TelemetryEventKind::BrakeApplication:
    case TelemetryEventKind::BrakeRelease:
    case TelemetryEventKind::CornerEntry:
    case TelemetryEventKind::Apex:
    case TelemetryEventKind::CornerExit:
    case TelemetryEventKind::Overtake:
        return EventSeverity::DrivingNote;
    default:
        return EventSeverity::Informational;
    }
}

struct EventRecord final
{
    TelemetryEvent Event{};
    EventSource Source = EventSource::None;
    EventChannel Channel = EventChannel::System;
    EventSeverity Severity = EventSeverity::Informational;
    std::uint64_t SessionToken = 0;
    std::uint64_t SourceRevision = 0;
    std::uint64_t FirstFrame = 0;
    std::uint64_t LastFrame = 0;
    std::uint64_t Sequence = 0;
    std::uint32_t MergeCount = 0;
    double PeakMagnitude = 0.0;
    double SumMagnitude = 0.0;
    double FirstSeenSeconds = 0.0;
    double LastSeenSeconds = 0.0;
    bool EngineObserved = false;
    bool Provisional = true;
    bool Accepted = false;
};

struct EventJournalCounters final
{
    std::uint64_t Observed = 0;
    std::uint64_t Accepted = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t Merged = 0;
    std::uint64_t Overwritten = 0;
    std::uint64_t IdentityResets = 0;
    std::uint64_t StaleRejected = 0;
    std::array<std::uint64_t, 12> ByChannel{};
    std::array<std::uint64_t, 6> BySeverity{};
    double LatestEventSeconds = 0.0;
    double LongestEventDurationSeconds = 0.0;
    bool CapacityReached = false;
};

inline bool SameEventIdentity(
    const EventRecord& a, const EventRecord& b) noexcept
{
    return a.Event.Kind == b.Event.Kind &&
           a.Event.DriverSlot == b.Event.DriverSlot &&
           a.Event.RelatedSlot == b.Event.RelatedSlot &&
           a.Event.ActorToken == b.Event.ActorToken &&
           a.Event.RelatedToken == b.Event.RelatedToken &&
           a.SessionToken == b.SessionToken &&
           a.SourceRevision == b.SourceRevision &&
           a.Source == b.Source;
}

inline bool ValidEvent(
    const TelemetryEvent& event,
    const EventJournalSettings& settings) noexcept
{
    return event.Kind != TelemetryEventKind::Unknown &&
           event.DriverSlot >= 0 &&
           event.DriverSlot < MaximumTelemetryDrivers &&
           event.SectorIndex >= -1 &&
           event.SectorIndex < MaximumTelemetrySectors &&
           event.LapIndex >= -1 &&
           event.LapIndex < MaximumTelemetryLaps &&
           event.RelatedSlot >= -1 &&
           event.RelatedSlot < MaximumTelemetryDrivers &&
           Finite(event.MonotonicSeconds) &&
           event.MonotonicSeconds >= 0.0 &&
           event.MonotonicSeconds <= MaximumTelemetrySeconds &&
           Finite(event.RaceClockSeconds) &&
           event.RaceClockSeconds >= 0.0 &&
           event.RaceClockSeconds <= MaximumTelemetrySeconds &&
           Finite(event.TrackDistanceCm) &&
           std::abs(event.TrackDistanceCm) <= 1.0e8 &&
           Finite(event.Magnitude) &&
           std::abs(event.Magnitude) <= settings.MaximumMagnitude &&
           Finite(event.DurationSeconds) &&
           event.DurationSeconds >= 0.0 &&
           event.DurationSeconds <= settings.MaximumDurationSeconds;
}

class EventJournal final
{
public:
    bool Configure(EventJournalSettings settings) noexcept
    {
        if (!ValidEventJournalSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    void Reset() noexcept
    {
        Items.fill({});
        Count = 0;
        Head = 0;
        Sequence = 0;
        Session = 0;
        Revision = 0;
        LastTime = 0.0;
        HasTime = false;
        Counters = {};
    }

    void BeginSession(
        std::uint64_t sessionToken, std::uint64_t revision) noexcept
    {
        if (sessionToken == 0)
        {
            Reset();
            return;
        }
        if (Session != sessionToken || Revision != revision)
        {
            const auto previousResets = Counters.IdentityResets;
            Reset();
            Session = sessionToken;
            Revision = revision;
            Counters.IdentityResets =
                SaturatingIncrement(previousResets);
        }
    }

    bool Append(
        TelemetryEvent event, EventSource source,
        std::uint64_t sessionToken, std::uint64_t revision,
        std::uint64_t frameOrdinal) noexcept
    {
        Counters.Observed =
            SaturatingIncrement(Counters.Observed);
        if (!ValidEventJournalSettings(Settings) ||
            !ValidEvent(event, Settings) ||
            sessionToken == 0 || frameOrdinal == 0 ||
            source == EventSource::None ||
            (!Settings.AllowDerivedEstimates &&
             (source == EventSource::DerivedEstimate ||
              source == EventSource::ImportedReplay)) ||
            (Settings.RequireVerifiedIdentity &&
             event.ActorToken == 0))
        {
            Counters.Rejected =
                SaturatingIncrement(Counters.Rejected);
            return false;
        }
        if (Session != 0 &&
            (Session != sessionToken || Revision != revision))
        {
            BeginSession(sessionToken, revision);
        }
        if (Session == 0)
        {
            Session = sessionToken;
            Revision = revision;
        }
        if (HasTime &&
            event.MonotonicSeconds + Settings.MaximumMergeGapSeconds <
                LastTime)
        {
            Counters.StaleRejected =
                SaturatingIncrement(Counters.StaleRejected);
            Counters.Rejected =
                SaturatingIncrement(Counters.Rejected);
            return false;
        }
        EventRecord incoming;
        incoming.Event = event;
        incoming.Source = source;
        incoming.Channel = ClassifyEventChannel(event.Kind);
        incoming.Severity = ClassifyEventSeverity(
            event.Kind, event.Magnitude);
        incoming.SessionToken = sessionToken;
        incoming.SourceRevision = revision;
        incoming.FirstFrame = frameOrdinal;
        incoming.LastFrame = frameOrdinal;
        incoming.Sequence = Sequence + 1;
        incoming.MergeCount = 1;
        incoming.PeakMagnitude = std::abs(event.Magnitude);
        incoming.SumMagnitude = event.Magnitude;
        incoming.FirstSeenSeconds = event.MonotonicSeconds;
        incoming.LastSeenSeconds = event.MonotonicSeconds;
        incoming.EngineObserved = source == EventSource::NativeRace ||
                                  source == EventSource::NativePhysics;
        incoming.Provisional = !incoming.EngineObserved ||
            event.Quality != TelemetryQuality::Qualified;
        incoming.Accepted = true;
        if (Settings.CoalesceAdjacentEvents && Count > 0)
        {
            EventRecord& previous = Items[
                (Head + static_cast<std::size_t>(Count) - 1) %
                static_cast<std::size_t>(Settings.MaximumEvents)];
            const double gap = event.MonotonicSeconds -
                previous.LastSeenSeconds;
            if (SameEventIdentity(previous, incoming) &&
                gap >= 0.0 &&
                gap <= Settings.MaximumMergeGapSeconds &&
                frameOrdinal > previous.LastFrame)
            {
                previous.MergeCount =
                    previous.MergeCount < std::numeric_limits<
                        std::uint32_t>::max() ?
                        previous.MergeCount + 1 : previous.MergeCount;
                previous.LastFrame = frameOrdinal;
                previous.LastSeenSeconds = event.MonotonicSeconds;
                previous.Event.DurationSeconds = std::max(
                    previous.Event.DurationSeconds,
                    event.MonotonicSeconds -
                        previous.FirstSeenSeconds +
                        event.DurationSeconds);
                previous.Event.DurationSeconds = std::min(
                    Settings.MaximumDurationSeconds,
                    previous.Event.DurationSeconds);
                previous.PeakMagnitude = std::max(
                    previous.PeakMagnitude, std::abs(event.Magnitude));
                previous.SumMagnitude = SaturatingAdd(
                    previous.SumMagnitude, event.Magnitude);
                previous.Severity =
                    static_cast<int>(incoming.Severity) >
                        static_cast<int>(previous.Severity) ?
                        incoming.Severity : previous.Severity;
                previous.Provisional =
                    previous.Provisional || incoming.Provisional;
                Counters.Merged = SaturatingIncrement(Counters.Merged);
                LastTime = std::max(LastTime, event.MonotonicSeconds);
                HasTime = true;
                return true;
            }
        }
        const int capacity = Settings.MaximumEvents;
        std::size_t index = 0;
        if (Count < capacity)
        {
            index = (Head + static_cast<std::size_t>(Count)) %
                static_cast<std::size_t>(capacity);
            ++Count;
        }
        else
        {
            index = Head;
            Head = (Head + 1) % static_cast<std::size_t>(capacity);
            Counters.Overwritten =
                SaturatingIncrement(Counters.Overwritten);
            Counters.CapacityReached = true;
        }
        Sequence = SaturatingIncrement(Sequence);
        incoming.Sequence = Sequence;
        Items[index] = incoming;
        Counters.Accepted =
            SaturatingIncrement(Counters.Accepted);
        Counters.LongestEventDurationSeconds = std::max(
            Counters.LongestEventDurationSeconds,
            event.DurationSeconds);
        Counters.ByChannel[static_cast<std::size_t>(
            incoming.Channel)] =
            SaturatingIncrement(Counters.ByChannel[
                static_cast<std::size_t>(incoming.Channel)]);
        Counters.BySeverity[static_cast<std::size_t>(
            incoming.Severity)] =
            SaturatingIncrement(Counters.BySeverity[
                static_cast<std::size_t>(incoming.Severity)]);
        LastTime = std::max(LastTime, event.MonotonicSeconds);
        HasTime = true;
        Counters.LatestEventSeconds = LastTime;
        return true;
    }

    EventRecord AtFromOldest(int index) const noexcept
    {
        if (index < 0 || index >= Count)
        {
            return {};
        }
        const std::size_t location =
            (Head + static_cast<std::size_t>(index)) %
            static_cast<std::size_t>(Settings.MaximumEvents);
        return Items[location];
    }

    EventRecord AtFromNewest(int index) const noexcept
    {
        if (index < 0 || index >= Count)
        {
            return {};
        }
        return AtFromOldest(Count - 1 - index);
    }

    EventRecord MostSevereWithin(
        double fromSeconds, double toSeconds) const noexcept
    {
        if (!Finite(fromSeconds) || !Finite(toSeconds) ||
            fromSeconds > toSeconds)
        {
            return {};
        }
        EventRecord selected;
        for (int i = 0; i < Count; ++i)
        {
            const EventRecord item = AtFromOldest(i);
            if (!item.Accepted ||
                item.Event.MonotonicSeconds < fromSeconds ||
                item.Event.MonotonicSeconds > toSeconds)
            {
                continue;
            }
            if (!selected.Accepted ||
                static_cast<int>(item.Severity) >
                    static_cast<int>(selected.Severity) ||
                (item.Severity == selected.Severity &&
                 item.PeakMagnitude > selected.PeakMagnitude))
            {
                selected = item;
            }
        }
        return selected;
    }

    int CopyRecent(
        EventRecord* destination, int capacity,
        EventSeverity minimumSeverity,
        double sinceSeconds) const noexcept
    {
        if (!destination || capacity <= 0 ||
            !Finite(sinceSeconds))
        {
            return 0;
        }
        int copied = 0;
        for (int i = Count - 1; i >= 0 && copied < capacity; --i)
        {
            const EventRecord event = AtFromOldest(i);
            if (event.Accepted &&
                static_cast<int>(event.Severity) >=
                    static_cast<int>(minimumSeverity) &&
                event.Event.MonotonicSeconds >= sinceSeconds)
            {
                destination[copied++] = event;
            }
        }
        return copied;
    }

    int CountByChannel(EventChannel channel) const noexcept
    {
        int count = 0;
        for (int i = 0; i < Count; ++i)
        {
            if (AtFromOldest(i).Channel == channel)
            {
                ++count;
            }
        }
        return count;
    }

    int CountByActor(std::uint64_t actorToken) const noexcept
    {
        if (actorToken == 0)
        {
            return 0;
        }
        int count = 0;
        for (int i = 0; i < Count; ++i)
        {
            if (AtFromOldest(i).Event.ActorToken == actorToken)
            {
                ++count;
            }
        }
        return count;
    }

    bool ContainsKind(TelemetryEventKind kind) const noexcept
    {
        for (int i = 0; i < Count; ++i)
        {
            if (AtFromOldest(i).Event.Kind == kind)
            {
                return true;
            }
        }
        return false;
    }

    int Size() const noexcept { return Count; }
    bool Empty() const noexcept { return Count == 0; }
    std::uint64_t CurrentSession() const noexcept { return Session; }
    EventJournalCounters Statistics() const noexcept { return Counters; }

private:
    EventJournalSettings Settings{};
    std::array<EventRecord, MaximumTelemetryEvents> Items{};
    EventJournalCounters Counters{};
    std::size_t Head = 0;
    int Count = 0;
    std::uint64_t Sequence = 0;
    std::uint64_t Session = 0;
    std::uint64_t Revision = 0;
    double LastTime = 0.0;
    bool HasTime = false;
};
} // namespace latebrake::engineering
