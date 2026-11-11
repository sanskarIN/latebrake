#pragma once

#include "Telemetry/TelemetryTypes.h"
#include "Telemetry/TelemetryValidation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace latebrake::engineering
{
enum class TimelineDiscontinuity : std::uint8_t
{
    None,
    NewSession,
    NewActor,
    RevisionChanged,
    IllegalTransition,
    ClockReversal,
    ExcessiveGap,
    StaleFrame,
    FinishedRestart,
    InvalidPhase,
    InvalidRaceClock,
    InvalidPhysics,
    CapacityExceeded
};

enum class TimelineBoundary : std::uint8_t
{
    None,
    SessionStarted,
    GridEstablished,
    LightsStarted,
    RaceGreen,
    CautionStarted,
    CautionEnded,
    Paused,
    Resumed,
    RaceFinished,
    RaceAborted,
    SessionReplaced
};

struct TimelineSettings final
{
    double MaximumFrameSeconds = 0.1;
    double MaximumClockGapSeconds = 0.5;
    double PhaseClockToleranceSeconds = 0.12;
    double MaximumMonotonicSeconds = MaximumTelemetrySeconds;
    double MinimumStablePhaseSeconds = 0.03;
    double MaximumObservationGapSeconds = 0.5;
    bool RequireVerifiedNativePhysics = true;
    bool RejectBackwardsRaceClock = true;
    bool RejectCompletedToRacing = true;
    bool RequireStableSessionIdentity = true;
};

inline bool ValidTimelineSettings(const TimelineSettings& s) noexcept
{
    return Finite(s.MaximumFrameSeconds) &&
           s.MaximumFrameSeconds > 0.0 &&
           s.MaximumFrameSeconds <= 1.0 &&
           Finite(s.MaximumClockGapSeconds) &&
           s.MaximumClockGapSeconds >= s.MaximumFrameSeconds &&
           Finite(s.PhaseClockToleranceSeconds) &&
           s.PhaseClockToleranceSeconds >= 0.0 &&
           s.PhaseClockToleranceSeconds <= 1.0 &&
           Finite(s.MaximumMonotonicSeconds) &&
           s.MaximumMonotonicSeconds > 0.0 &&
           Finite(s.MinimumStablePhaseSeconds) &&
           s.MinimumStablePhaseSeconds >= 0.0 &&
           Finite(s.MaximumObservationGapSeconds) &&
           s.MaximumObservationGapSeconds >= s.MaximumFrameSeconds;
}

inline bool TimelineLegalTransition(
    TelemetryPhase previous, TelemetryPhase next,
    bool allowRestart) noexcept
{
    if (previous == next)
    {
        return true;
    }
    switch (previous)
    {
    case TelemetryPhase::PreGrid:
        return next == TelemetryPhase::Countdown ||
               next == TelemetryPhase::Aborted;
    case TelemetryPhase::Countdown:
        return next == TelemetryPhase::Racing ||
               next == TelemetryPhase::PreGrid ||
               next == TelemetryPhase::Aborted;
    case TelemetryPhase::Racing:
        return next == TelemetryPhase::Caution ||
               next == TelemetryPhase::Paused ||
               next == TelemetryPhase::Finished ||
               next == TelemetryPhase::Aborted;
    case TelemetryPhase::Caution:
        return next == TelemetryPhase::Racing ||
               next == TelemetryPhase::Paused ||
               next == TelemetryPhase::Aborted ||
               next == TelemetryPhase::Finished;
    case TelemetryPhase::Paused:
        return next == TelemetryPhase::Racing ||
               next == TelemetryPhase::Caution ||
               next == TelemetryPhase::Aborted ||
               next == TelemetryPhase::PreGrid;
    case TelemetryPhase::Finished:
        return allowRestart && next == TelemetryPhase::PreGrid;
    case TelemetryPhase::Aborted:
        return allowRestart && next == TelemetryPhase::PreGrid;
    }
    return false;
}

inline TimelineBoundary ClassifyTimelineBoundary(
    TelemetryPhase previous, TelemetryPhase next) noexcept
{
    if (previous == next)
    {
        return TimelineBoundary::None;
    }
    if (previous == TelemetryPhase::PreGrid &&
        next == TelemetryPhase::Countdown)
    {
        return TimelineBoundary::LightsStarted;
    }
    if (previous == TelemetryPhase::Countdown &&
        next == TelemetryPhase::Racing)
    {
        return TimelineBoundary::RaceGreen;
    }
    if (next == TelemetryPhase::Caution)
    {
        return TimelineBoundary::CautionStarted;
    }
    if (previous == TelemetryPhase::Caution &&
        next == TelemetryPhase::Racing)
    {
        return TimelineBoundary::CautionEnded;
    }
    if (next == TelemetryPhase::Paused)
    {
        return TimelineBoundary::Paused;
    }
    if (previous == TelemetryPhase::Paused &&
        (next == TelemetryPhase::Racing ||
         next == TelemetryPhase::Caution))
    {
        return TimelineBoundary::Resumed;
    }
    if (next == TelemetryPhase::Finished)
    {
        return TimelineBoundary::RaceFinished;
    }
    if (next == TelemetryPhase::Aborted)
    {
        return TimelineBoundary::RaceAborted;
    }
    if (next == TelemetryPhase::PreGrid)
    {
        return TimelineBoundary::GridEstablished;
    }
    return TimelineBoundary::None;
}

struct TimelineObservation final
{
    TimelineBoundary Boundary = TimelineBoundary::None;
    TimelineDiscontinuity Discontinuity =
        TimelineDiscontinuity::None;
    TelemetryPhase PreviousPhase = TelemetryPhase::PreGrid;
    TelemetryPhase CurrentPhase = TelemetryPhase::PreGrid;
    TelemetryQuality Quality = TelemetryQuality::Missing;
    double MonotonicSeconds = 0.0;
    double RaceClockSeconds = 0.0;
    double FrameDeltaSeconds = 0.0;
    double PhaseDurationSeconds = 0.0;
    double AccumulatedRacingSeconds = 0.0;
    double AccumulatedCautionSeconds = 0.0;
    double AccumulatedPausedSeconds = 0.0;
    double ClockDriftSeconds = 0.0;
    int Epoch = 0;
    std::uint64_t FrameCount = 0;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    bool Accepted = false;
    bool SegmentReset = false;
    bool ClockMoving = false;
    bool RaceActive = false;
    bool PhaseChanged = false;
};

struct TimelineHistory final
{
    std::array<std::uint64_t, 7> PhaseFrames{};
    std::array<double, 7> PhaseSeconds{};
    std::array<std::uint64_t, 13> DiscontinuityCounts{};
    std::uint64_t AcceptedFrames = 0;
    std::uint64_t RejectedFrames = 0;
    std::uint64_t PhaseTransitions = 0;
    std::uint64_t SessionReplacements = 0;
    std::uint64_t TimeBacktracks = 0;
    std::uint64_t ClockAnomalies = 0;
    double StartedMonotonicSeconds = 0.0;
    double LatestMonotonicSeconds = 0.0;
    double LatestRaceClockSeconds = 0.0;
    double LongestActivePeriodSeconds = 0.0;
    double LongestPausedPeriodSeconds = 0.0;
    double LongestCautionPeriodSeconds = 0.0;
    double LargestFrameGapSeconds = 0.0;
    bool HasStart = false;
};

inline constexpr std::size_t TimelinePhaseIndex(
    TelemetryPhase phase) noexcept
{
    return static_cast<std::size_t>(phase);
}

inline constexpr std::size_t TimelineDiscontinuityIndex(
    TimelineDiscontinuity cause) noexcept
{
    return static_cast<std::size_t>(cause);
}

class SessionTimeline final
{
public:
    explicit SessionTimeline(TimelineSettings settings = {}) noexcept
        : Settings(settings)
    {
    }

    void Reset() noexcept
    {
        Previous = {};
        Last = {};
        History = {};
        CurrentPhase = TelemetryPhase::PreGrid;
        SessionToken = 0;
        ActorToken = 0;
        FrameOrdinal = 0;
        Revision = 0;
        Epoch = 0;
        ActivePeriod = 0.0;
        PausedPeriod = 0.0;
        CautionPeriod = 0.0;
        PhaseSeconds = 0.0;
        HasPrevious = false;
        HasIdentity = false;
    }

    bool Configure(TimelineSettings settings) noexcept
    {
        if (!ValidTimelineSettings(settings))
        {
            return false;
        }
        Settings = settings;
        Reset();
        return true;
    }

    TimelineObservation Observe(
        const TelemetryFrame& frame) noexcept
    {
        TimelineObservation observation;
        observation.CurrentPhase = frame.Race.Phase;
        observation.PreviousPhase = CurrentPhase;
        observation.MonotonicSeconds = frame.MonotonicSeconds;
        observation.RaceClockSeconds = frame.RaceClockSeconds;
        observation.SessionToken = frame.Identity.SessionToken;
        observation.ActorToken = frame.Identity.ActorToken;
        observation.FrameCount = History.AcceptedFrames + 1;
        observation.Epoch = Epoch;
        if (!ValidTimelineSettings(Settings))
        {
            return Reject(TimelineDiscontinuity::InvalidPhase,
                          observation);
        }
        if (!Finite(frame.MonotonicSeconds) ||
            !Finite(frame.RaceClockSeconds) ||
            !Finite(frame.DeltaSeconds) ||
            frame.MonotonicSeconds < 0.0 ||
            frame.MonotonicSeconds > Settings.MaximumMonotonicSeconds ||
            frame.RaceClockSeconds < 0.0 ||
            frame.RaceClockSeconds > MaximumTelemetrySeconds ||
            frame.DeltaSeconds <= 0.0 ||
            frame.DeltaSeconds > Settings.MaximumFrameSeconds)
        {
            return Reject(TimelineDiscontinuity::InvalidRaceClock,
                          observation);
        }
        if (frame.Identity.SessionToken == 0 ||
            frame.Identity.ActorToken == 0 ||
            frame.Identity.DriverSlot < 0 ||
            frame.Identity.DriverSlot >= MaximumTelemetryDrivers)
        {
            return Reject(TimelineDiscontinuity::InvalidPhysics,
                          observation);
        }
        if (Settings.RequireVerifiedNativePhysics &&
            (!frame.Race.EnginePhysicsReady ||
             !IsEngineObservation(frame.Identity.Trust)))
        {
            return Reject(TimelineDiscontinuity::InvalidPhysics,
                          observation);
        }
        if (!ValidTelemetryRaceStatus(frame.Race))
        {
            return Reject(TimelineDiscontinuity::InvalidPhase,
                          observation);
        }
        const bool newSession = HasIdentity &&
            frame.Identity.SessionToken != SessionToken;
        const bool replacedActor = HasIdentity &&
            frame.Identity.ActorToken != ActorToken;
        const bool revised = HasIdentity &&
            frame.Identity.SessionRevision != Revision;
        if (newSession || replacedActor || revised)
        {
            if (Settings.RequireStableSessionIdentity &&
                !newSession && !revised && replacedActor)
            {
                return Reject(TimelineDiscontinuity::NewActor,
                              observation);
            }
            History.SessionReplacements =
                SaturatingIncrement(History.SessionReplacements);
            ++Epoch;
            HasPrevious = false;
            PhaseSeconds = 0.0;
            ActivePeriod = 0.0;
            PausedPeriod = 0.0;
            CautionPeriod = 0.0;
            observation.Boundary = TimelineBoundary::SessionReplaced;
            observation.SegmentReset = true;
            observation.Discontinuity = newSession ?
                TimelineDiscontinuity::NewSession :
                replacedActor ?
                    TimelineDiscontinuity::NewActor :
                    TimelineDiscontinuity::RevisionChanged;
            observation.Epoch = Epoch;
        }
        if (!HasIdentity || newSession || replacedActor || revised)
        {
            SessionToken = frame.Identity.SessionToken;
            ActorToken = frame.Identity.ActorToken;
            Revision = frame.Identity.SessionRevision;
            HasIdentity = true;
        }

        double elapsed = frame.DeltaSeconds;
        if (HasPrevious)
        {
            if (frame.Identity.FrameOrdinal <= FrameOrdinal)
            {
                return Reject(TimelineDiscontinuity::StaleFrame,
                              observation);
            }
            const double monotonicDelta =
                frame.MonotonicSeconds - Previous.MonotonicSeconds;
            if (!Finite(monotonicDelta) || monotonicDelta <= 0.0)
            {
                History.TimeBacktracks =
                    SaturatingIncrement(History.TimeBacktracks);
                return Reject(TimelineDiscontinuity::ClockReversal,
                              observation);
            }
            if (monotonicDelta > Settings.MaximumClockGapSeconds)
            {
                return Reject(TimelineDiscontinuity::ExcessiveGap,
                              observation);
            }
            if (Settings.RejectBackwardsRaceClock &&
                frame.RaceClockSeconds +
                    Settings.PhaseClockToleranceSeconds <
                    Previous.RaceClockSeconds)
            {
                History.ClockAnomalies =
                    SaturatingIncrement(History.ClockAnomalies);
                return Reject(TimelineDiscontinuity::ClockReversal,
                              observation);
            }
            if (!TimelineLegalTransition(
                    Previous.Race.Phase, frame.Race.Phase,
                    !Settings.RejectCompletedToRacing))
            {
                return Reject(TimelineDiscontinuity::IllegalTransition,
                              observation);
            }
            elapsed = monotonicDelta;
        }
        const TelemetryPhase previousPhase =
            HasPrevious ? Previous.Race.Phase : frame.Race.Phase;
        const bool changed = HasPrevious &&
            previousPhase != frame.Race.Phase;
        if (changed)
        {
            History.PhaseTransitions =
                SaturatingIncrement(History.PhaseTransitions);
            PhaseSeconds = 0.0;
            observation.Boundary = ClassifyTimelineBoundary(
                previousPhase, frame.Race.Phase);
        }
        PhaseSeconds += elapsed;
        const std::size_t index =
            TimelinePhaseIndex(frame.Race.Phase);
        if (index < History.PhaseSeconds.size())
        {
            History.PhaseSeconds[index] += elapsed;
            History.PhaseFrames[index] =
                SaturatingIncrement(History.PhaseFrames[index]);
        }
        if (frame.Race.Phase == TelemetryPhase::Racing)
        {
            ActivePeriod += elapsed;
            PausedPeriod = 0.0;
            CautionPeriod = 0.0;
            History.LongestActivePeriodSeconds = std::max(
                History.LongestActivePeriodSeconds, ActivePeriod);
        }
        else if (frame.Race.Phase == TelemetryPhase::Paused)
        {
            PausedPeriod += elapsed;
            ActivePeriod = 0.0;
            CautionPeriod = 0.0;
            History.LongestPausedPeriodSeconds = std::max(
                History.LongestPausedPeriodSeconds, PausedPeriod);
        }
        else if (frame.Race.Phase == TelemetryPhase::Caution)
        {
            CautionPeriod += elapsed;
            ActivePeriod = 0.0;
            PausedPeriod = 0.0;
            History.LongestCautionPeriodSeconds = std::max(
                History.LongestCautionPeriodSeconds, CautionPeriod);
        }
        else
        {
            ActivePeriod = 0.0;
            PausedPeriod = 0.0;
            CautionPeriod = 0.0;
        }
        if (!History.HasStart)
        {
            History.HasStart = true;
            History.StartedMonotonicSeconds = frame.MonotonicSeconds;
        }
        History.AcceptedFrames =
            SaturatingIncrement(History.AcceptedFrames);
        History.LargestFrameGapSeconds = std::max(
            History.LargestFrameGapSeconds, elapsed);
        History.LatestMonotonicSeconds = frame.MonotonicSeconds;
        History.LatestRaceClockSeconds = frame.RaceClockSeconds;
        const double measuredClockDelta = HasPrevious ?
            frame.RaceClockSeconds - Previous.RaceClockSeconds : 0.0;
        observation.PreviousPhase = previousPhase;
        observation.CurrentPhase = frame.Race.Phase;
        observation.Accepted = true;
        observation.Quality = TelemetryQuality::Qualified;
        observation.PhaseChanged = changed;
        observation.FrameDeltaSeconds = elapsed;
        observation.PhaseDurationSeconds = PhaseSeconds;
        observation.AccumulatedRacingSeconds =
            History.PhaseSeconds[TimelinePhaseIndex(TelemetryPhase::Racing)];
        observation.AccumulatedCautionSeconds =
            History.PhaseSeconds[TimelinePhaseIndex(TelemetryPhase::Caution)];
        observation.AccumulatedPausedSeconds =
            History.PhaseSeconds[TimelinePhaseIndex(TelemetryPhase::Paused)];
        observation.ClockDriftSeconds =
            measuredClockDelta - (frame.Race.LocalRaceClockRunning ?
                                     elapsed : 0.0);
        observation.ClockMoving = frame.Race.LocalRaceClockRunning &&
            IsRaceMoving(frame.Race.Phase);
        observation.RaceActive =
            IsRaceMoving(frame.Race.Phase);
        Last = observation;
        Previous = frame;
        CurrentPhase = frame.Race.Phase;
        FrameOrdinal = frame.Identity.FrameOrdinal;
        HasPrevious = true;
        return observation;
    }

    TimelineObservation Latest() const noexcept { return Last; }
    TimelineHistory Statistics() const noexcept { return History; }
    bool Ready() const noexcept { return HasPrevious; }
    std::uint64_t CurrentSessionToken() const noexcept
    {
        return SessionToken;
    }
    std::uint64_t CurrentActorToken() const noexcept
    {
        return ActorToken;
    }
    TelemetryPhase CurrentRacePhase() const noexcept
    {
        return CurrentPhase;
    }
    int CurrentEpoch() const noexcept { return Epoch; }
    double PhaseElapsedSeconds() const noexcept
    {
        return PhaseSeconds;
    }
    double TotalRacingSeconds() const noexcept
    {
        return History.PhaseSeconds[TimelinePhaseIndex(TelemetryPhase::Racing)];
    }
    double TotalPausedSeconds() const noexcept
    {
        return History.PhaseSeconds[TimelinePhaseIndex(TelemetryPhase::Paused)];
    }
    double TotalCautionSeconds() const noexcept
    {
        return History.PhaseSeconds[TimelinePhaseIndex(TelemetryPhase::Caution)];
    }
    bool SessionFrozen() const noexcept
    {
        return CurrentPhase == TelemetryPhase::Finished ||
            CurrentPhase == TelemetryPhase::Aborted;
    }
    bool HasStablePhase() const noexcept
    {
        return HasPrevious &&
            PhaseSeconds >= Settings.MinimumStablePhaseSeconds;
    }

private:
    TimelineObservation Reject(
        TimelineDiscontinuity reason,
        TimelineObservation observation) noexcept
    {
        History.RejectedFrames =
            SaturatingIncrement(History.RejectedFrames);
        const std::size_t index =
            TimelineDiscontinuityIndex(reason);
        if (index < History.DiscontinuityCounts.size())
        {
            History.DiscontinuityCounts[index] =
                SaturatingIncrement(History.DiscontinuityCounts[index]);
        }
        observation.Quality = TelemetryQuality::Invalid;
        observation.Discontinuity = reason;
        observation.Accepted = false;
        observation.SegmentReset = true;
        observation.Epoch = Epoch;
        Last = observation;
        HasPrevious = false;
        return observation;
    }

    TimelineSettings Settings{};
    TelemetryFrame Previous{};
    TimelineObservation Last{};
    TimelineHistory History{};
    TelemetryPhase CurrentPhase = TelemetryPhase::PreGrid;
    std::uint64_t SessionToken = 0;
    std::uint64_t ActorToken = 0;
    std::uint64_t FrameOrdinal = 0;
    int Revision = 0;
    int Epoch = 0;
    double ActivePeriod = 0.0;
    double PausedPeriod = 0.0;
    double CautionPeriod = 0.0;
    double PhaseSeconds = 0.0;
    bool HasPrevious = false;
    bool HasIdentity = false;
};
} // namespace latebrake::engineering
