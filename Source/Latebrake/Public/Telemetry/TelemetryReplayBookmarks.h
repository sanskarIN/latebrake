#pragma once

#include "Telemetry/TelemetryReplaySampler.h"
#include "Telemetry/TelemetryIncidentJournal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class ReplayBookmarkKind : std::uint8_t
{
    Unknown,
    LapStart,
    FastestSector,
    BrakeAttack,
    CornerApex,
    Overtake,
    WallContact,
    OpponentContact,
    Recovery,
    TireOverheat,
    EngineFault,
    RaceFinish,
    ManualMarker,
    SensorDropout
};

struct ReplayBookmarkSettings final
{
    double MinimumBookmarkSpacingSeconds = 0.15;
    double DefaultBeforeSeconds = 2.0;
    double DefaultAfterSeconds = 3.0;
    double MaximumClipSeconds = 90.0;
    double MaximumSourceSessionSeconds = MaximumTelemetrySeconds;
    double MaximumBookmarkMagnitude = 1000000.0;
    int MaximumMarkersPerLap = 256;
    bool RequirePhysicalSource = true;
    bool RequireSameActor = true;
    bool PreserveCriticalBookmarks = true;
    bool AllowManualMarkers = true;
};

inline bool ValidReplayBookmarkSettings(
    ReplayBookmarkSettings s) noexcept
{
    return Finite(s.MinimumBookmarkSpacingSeconds) &&
        s.MinimumBookmarkSpacingSeconds >= 0.0 &&
        Finite(s.DefaultBeforeSeconds) &&
        s.DefaultBeforeSeconds >= 0.0 &&
        Finite(s.DefaultAfterSeconds) &&
        s.DefaultAfterSeconds >= 0.0 &&
        Finite(s.MaximumClipSeconds) &&
        s.MaximumClipSeconds >=
            s.DefaultBeforeSeconds + s.DefaultAfterSeconds &&
        Finite(s.MaximumSourceSessionSeconds) &&
        s.MaximumSourceSessionSeconds > 0.0 &&
        Finite(s.MaximumBookmarkMagnitude) &&
        s.MaximumBookmarkMagnitude > 0.0 &&
        s.MaximumMarkersPerLap >= 1 &&
        s.MaximumMarkersPerLap <= 4096;
}

struct ReplayBookmark final
{
    ReplayBookmarkKind Kind = ReplayBookmarkKind::Unknown;
    double CenterSeconds = 0.0;
    double StartSeconds = 0.0;
    double EndSeconds = 0.0;
    double TrackDistanceCm = 0.0;
    double Magnitude = 0.0;
    double Confidence = 0.0;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    std::uint64_t SourceFrameOrdinal = 0;
    int LapIndex = -1;
    int SectorIndex = -1;
    int DriverSlot = -1;
    int SourceSegment = -1;
    int RelatedDriverSlot = -1;
    bool PhysicallyObserved = false;
    bool CriticalReview = false;
    bool UserSelected = false;
    bool HasVerifiedReplayFrames = false;
    bool CanControlRace = false;
    bool Valid = false;
};

struct ReplayClipWindow final
{
    double BeginSeconds = 0.0;
    double EndSeconds = 0.0;
    double DurationSeconds = 0.0;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    int Bookmarks = 0;
    int CriticalBookmarks = 0;
    int DistinctKinds = 0;
    bool TimeRangeQualified = false;
    bool SameActor = false;
    bool ExportedMovie = false;
};

inline bool ValidReplayBookmark(
    ReplayBookmark b,
    ReplayBookmarkSettings settings) noexcept
{
    return b.Kind != ReplayBookmarkKind::Unknown &&
        (!settings.RequirePhysicalSource ||
         b.PhysicallyObserved) &&
        (settings.AllowManualMarkers ||
         b.Kind != ReplayBookmarkKind::ManualMarker) &&
        b.SessionToken != 0 &&
        b.ActorToken != 0 &&
        b.DriverSlot >= 0 &&
        b.DriverSlot < MaximumTelemetryDrivers &&
        Finite(b.CenterSeconds) &&
        Finite(b.StartSeconds) &&
        Finite(b.EndSeconds) &&
        Finite(b.TrackDistanceCm) &&
        Finite(b.Magnitude) &&
        Finite(b.Confidence) &&
        b.CenterSeconds >= 0.0 &&
        b.CenterSeconds <= settings.MaximumSourceSessionSeconds &&
        b.StartSeconds >= 0.0 &&
        b.EndSeconds >= b.StartSeconds &&
        b.CenterSeconds >= b.StartSeconds &&
        b.CenterSeconds <= b.EndSeconds &&
        b.EndSeconds - b.StartSeconds <=
            settings.MaximumClipSeconds &&
        b.TrackDistanceCm >= 0.0 &&
        std::abs(b.Magnitude) <=
            settings.MaximumBookmarkMagnitude &&
        b.Confidence >= 0.0 &&
        b.Confidence <= 1.0 &&
        b.LapIndex >= 0 &&
        b.LapIndex <= MaximumTelemetryLaps &&
        b.SectorIndex >= -1 &&
        b.SectorIndex < MaximumTelemetrySectors &&
        !b.CanControlRace;
}

inline ReplayBookmark MakeBookmark(
    ReplayBookmarkKind kind,
    const TelemetryFrame& source,
    double beforeSeconds, double afterSeconds,
    double trackDistanceCm,
    double magnitude,
    ReplayBookmarkSettings settings = {}) noexcept
{
    ReplayBookmark bookmark;
    if (!ValidReplayBookmarkSettings(settings) ||
        !Finite(beforeSeconds) ||
        !Finite(afterSeconds) ||
        beforeSeconds < 0.0 ||
        afterSeconds < 0.0 ||
        beforeSeconds + afterSeconds >
            settings.MaximumClipSeconds ||
        !Finite(trackDistanceCm) ||
        trackDistanceCm < 0.0 ||
        !Finite(magnitude) ||
        std::abs(magnitude) >
            settings.MaximumBookmarkMagnitude)
    {
        return bookmark;
    }
    bookmark.Kind = kind;
    bookmark.CenterSeconds = source.MonotonicSeconds;
    bookmark.StartSeconds = std::max(
        0.0, source.MonotonicSeconds - beforeSeconds);
    bookmark.EndSeconds =
        source.MonotonicSeconds + afterSeconds;
    bookmark.TrackDistanceCm = trackDistanceCm;
    bookmark.Magnitude = magnitude;
    bookmark.Confidence = 1.0;
    bookmark.SessionToken =
        source.Identity.SessionToken;
    bookmark.ActorToken =
        source.Identity.ActorToken;
    bookmark.SourceFrameOrdinal =
        source.Identity.FrameOrdinal;
    bookmark.LapIndex = source.Race.LapIndex;
    bookmark.SectorIndex = source.Race.SectorIndex;
    bookmark.DriverSlot = source.Identity.DriverSlot;
    bookmark.PhysicallyObserved =
        source.Race.EnginePhysicsReady &&
        IsEngineObservation(source.Identity.Trust);
    bookmark.UserSelected =
        kind == ReplayBookmarkKind::ManualMarker;
    bookmark.CriticalReview =
        kind == ReplayBookmarkKind::WallContact ||
        kind == ReplayBookmarkKind::OpponentContact ||
        kind == ReplayBookmarkKind::EngineFault;
    bookmark.CanControlRace = false;
    bookmark.Valid = ValidReplayBookmark(bookmark, settings);
    return bookmark;
}

class ReplayBookmarkIndex final
{
public:
    static constexpr int MaximumBookmarks = 256;

    void Reset() noexcept
    {
        Settings = {};
        Bookmarks.fill({});
        Head = 0;
        Count = 0;
        Accepted = 0;
        Rejected = 0;
        Evicted = 0;
        Session = 0;
        Actor = 0;
        LastLap = -1;
        BookmarksThisLap = 0;
    }

    bool Configure(ReplayBookmarkSettings settings) noexcept
    {
        if (!ValidReplayBookmarkSettings(settings))
        {
            return false;
        }
        Reset();
        Settings = settings;
        return true;
    }

    bool Add(ReplayBookmark bookmark) noexcept
    {
        if (!ValidReplayBookmark(bookmark, Settings) ||
            (Session != 0 &&
             (bookmark.SessionToken != Session ||
              (Settings.RequireSameActor &&
               bookmark.ActorToken != Actor))))
        {
            ++Rejected;
            return false;
        }
        if (Session == 0)
        {
            Session = bookmark.SessionToken;
            Actor = bookmark.ActorToken;
        }
        if (bookmark.LapIndex != LastLap)
        {
            LastLap = bookmark.LapIndex;
            BookmarksThisLap = 0;
        }
        if (BookmarksThisLap >= Settings.MaximumMarkersPerLap)
        {
            ++Rejected;
            return false;
        }
        for (int i = 0; i < Count; ++i)
        {
            const ReplayBookmark existing =
                AtOldest(i);
            if (existing.Kind == bookmark.Kind &&
                existing.LapIndex == bookmark.LapIndex &&
                std::abs(existing.CenterSeconds -
                         bookmark.CenterSeconds) <
                    Settings.MinimumBookmarkSpacingSeconds)
            {
                ++Rejected;
                return false;
            }
        }
        if (Count >= MaximumBookmarks &&
            Settings.PreserveCriticalBookmarks &&
            AtOldest(0).CriticalReview)
        {
            ++Rejected;
            return false;
        }
        if (Count == MaximumBookmarks)
        {
            Head = (Head + 1) %
                MaximumBookmarks;
            ++Evicted;
        }
        else
        {
            ++Count;
        }
        Bookmarks[static_cast<std::size_t>(
            (Head + Count - 1) % MaximumBookmarks)] =
            bookmark;
        ++BookmarksThisLap;
        ++Accepted;
        return true;
    }

    ReplayBookmark AtOldest(int index) const noexcept
    {
        return index >= 0 && index < Count ?
            Bookmarks[static_cast<std::size_t>(
                (Head + index) % MaximumBookmarks)] :
            ReplayBookmark{};
    }

    ReplayBookmark AtNewest(int offset = 0) const noexcept
    {
        return offset >= 0 && offset < Count ?
            AtOldest(Count - 1 - offset) :
            ReplayBookmark{};
    }

    ReplayBookmark Nearest(double timeSeconds) const noexcept
    {
        ReplayBookmark best;
        if (!Finite(timeSeconds))
        {
            return best;
        }
        double minimum =
            std::numeric_limits<double>::infinity();
        for (int i = 0; i < Count; ++i)
        {
            const ReplayBookmark bookmark =
                AtOldest(i);
            const double error =
                std::abs(bookmark.CenterSeconds -
                         timeSeconds);
            if (error < minimum)
            {
                minimum = error;
                best = bookmark;
            }
        }
        return best;
    }

    ReplayClipWindow BuildClipWindow(
        int firstIndex, int lastIndex) const noexcept
    {
        ReplayClipWindow clip;
        if (firstIndex < 0 ||
            lastIndex < firstIndex ||
            lastIndex >= Count)
        {
            return clip;
        }
        const ReplayBookmark initial =
            AtOldest(firstIndex);
        clip.BeginSeconds = initial.StartSeconds;
        clip.EndSeconds = initial.EndSeconds;
        clip.Session = initial.SessionToken;
        clip.Actor = initial.ActorToken;
        std::array<bool, 14> kinds{};
        for (int i = firstIndex; i <= lastIndex; ++i)
        {
            const ReplayBookmark b = AtOldest(i);
            if (!b.Valid ||
                b.SessionToken != clip.Session ||
                b.ActorToken != clip.Actor)
            {
                return {};
            }
            clip.BeginSeconds = std::min(
                clip.BeginSeconds, b.StartSeconds);
            clip.EndSeconds = std::max(
                clip.EndSeconds, b.EndSeconds);
            ++clip.Bookmarks;
            clip.CriticalBookmarks +=
                b.CriticalReview ? 1 : 0;
            const std::size_t kind =
                static_cast<std::size_t>(b.Kind);
            if (kind < kinds.size())
            {
                kinds[kind] = true;
            }
        }
        clip.DurationSeconds = clip.EndSeconds -
            clip.BeginSeconds;
        for (bool present : kinds)
        {
            clip.DistinctKinds += present ? 1 : 0;
        }
        clip.SameActor = true;
        clip.TimeRangeQualified =
            clip.DurationSeconds <=
                Settings.MaximumClipSeconds;
        clip.ExportedMovie = false;
        return clip;
    }

    int StoredBookmarks() const noexcept
    {
        return Count;
    }

    std::uint64_t AcceptedBookmarks() const noexcept
    {
        return Accepted;
    }

    std::uint64_t RejectedBookmarks() const noexcept
    {
        return Rejected;
    }

    std::uint64_t EvictedBookmarks() const noexcept
    {
        return Evicted;
    }

private:
    ReplayBookmarkSettings Settings{};
    std::array<ReplayBookmark, MaximumBookmarks> Bookmarks{};
    int Head = 0;
    int Count = 0;
    int LastLap = -1;
    int BookmarksThisLap = 0;
    std::uint64_t Session = 0;
    std::uint64_t Actor = 0;
    std::uint64_t Accepted = 0;
    std::uint64_t Rejected = 0;
    std::uint64_t Evicted = 0;
};
} // namespace latebrake::engineering
