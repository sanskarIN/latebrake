#pragma once
#include "Racecraft/RacecraftFrames.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace latebrake::racecraft
{
enum class ClockTransition : std::uint8_t
{
    None, ArmGrid, BeginCountdown, CountdownComplete,
    EnterCaution, ResumeFromCaution, Pause, Unpause,
    RedFlag, ResumeFromRedFlag, Finish, Abort, Restart
};
enum class ClockFault : std::uint8_t
{
    None, WrongPhase, WrongSession, InvalidTimestamp,
    DuplicateEvent, ClockReversal, InsufficientPhysicalCars,
    MissingTrack, MissingRaceAuthority, NotYetReady,
    UnverifiedWinner, FinishedImmutable
};
struct RaceClockParameters final
{
    double MinimumCountdownSeconds = 1.0;
    double MaximumCountdownSeconds = 15.0;
    double DefaultCountdownSeconds = 5.0;
    double MaximumFrameSeconds = 0.10;
    double MaximumClockGapSeconds = 0.11;
    double CautionSpeedLimitCmS = 1200.0;
    double MaximumSessionSeconds = 86400.0;
    double MinimumFinishSeconds = 8.0;
    int MinimumPhysicalCars = 2;
    int MaximumPhysicalCars = PhysicalFleetCapacity;
    int RequiredLaps = 3;
    bool RequireOriginalTrack = true;
    bool FreezeClockDuringCaution = false;
    bool FreezeClockDuringPause = true;
    bool OnlyAuthoritativeGameModeCanFinish = true;
};
struct ClockTransitionRequest final
{
    ClockTransition Kind = ClockTransition::None;
    double AtSessionSeconds = 0.0;
    double DeltaSeconds = 0.0;
    double CountdownSeconds = 0.0;
    int QualifiedPhysicalCars = 0;
    int WinnerSlot = -1;
    int WinnerCompletedLaps = 0;
    std::uint64_t SessionToken = 0;
    std::uint32_t Revision = 0;
    std::uint64_t EventOrdinal = 0;
    bool OriginalMapVerified = false;
    bool ActualRaceGameMode = false;
    bool EngineClockObserved = false;
    bool WinningGateVerified = false;
};
struct RaceClockReceipt final
{
    RacePhase Phase = RacePhase::Dormant;
    RacePhase BeforePause = RacePhase::Dormant;
    double SessionSeconds = 0.0;
    double ElapsedRaceSeconds = 0.0;
    double CountdownRemainingSeconds = 0.0;
    double CautionElapsedSeconds = 0.0;
    double PausedElapsedSeconds = 0.0;
    double RaceStartSessionSeconds = 0.0;
    double RaceFinishSessionSeconds = 0.0;
    double LastEngineFrameSeconds = 0.0;
    double LastTransitionSeconds = 0.0;
    double TotalAuthorizedRunningSeconds = 0.0;
    double LastFrameDeltaSeconds = 0.0;
    std::uint64_t SessionToken = 0;
    std::uint32_t Revision = 0;
    std::uint64_t EventOrdinal = 0;
    std::uint64_t FrameOrdinal = 0;
    int QualifiedCarsAtStart = 0;
    int WinnerSlot = -1;
    int WinnerCompletedLaps = 0;
    int CautionPeriods = 0;
    int PausePeriods = 0;
    int Restarts = 0;
    int RejectedTransitions = 0;
    int AcceptedTransitions = 0;
    ClockFault LastFault = ClockFault::None;
    bool OriginalTrackVerified = false;
    bool PhysicalRosterAtGo = false;
    bool OfficialRaceClockModified = false;
    bool Finalized = false;
};
inline bool ValidRaceClockParameters(
    RaceClockParameters s) noexcept
{
    return Finite(s.MinimumCountdownSeconds) &&
        s.MinimumCountdownSeconds > 0.0 &&
        Finite(s.MaximumCountdownSeconds) &&
        s.MaximumCountdownSeconds > s.MinimumCountdownSeconds &&
        Finite(s.DefaultCountdownSeconds) &&
        s.DefaultCountdownSeconds >=
            s.MinimumCountdownSeconds &&
        s.DefaultCountdownSeconds <=
            s.MaximumCountdownSeconds &&
        Finite(s.MaximumFrameSeconds) &&
        s.MaximumFrameSeconds > 0.0 &&
        s.MaximumFrameSeconds <= MaxSimulationFrameSeconds &&
        Finite(s.MaximumClockGapSeconds) &&
        s.MaximumClockGapSeconds >= s.MaximumFrameSeconds &&
        Finite(s.CautionSpeedLimitCmS) &&
        s.CautionSpeedLimitCmS > 0.0 &&
        Finite(s.MaximumSessionSeconds) &&
        s.MaximumSessionSeconds >= 60.0 &&
        Finite(s.MinimumFinishSeconds) &&
        s.MinimumFinishSeconds > 0.0 &&
        s.MinimumPhysicalCars >= 1 &&
        s.MaximumPhysicalCars <= PhysicalFleetCapacity &&
        s.MaximumPhysicalCars >= s.MinimumPhysicalCars &&
        s.RequiredLaps >= 1 && s.RequiredLaps <= 500;
}
inline bool ValidClockEvent(ClockTransitionRequest e,
                            RaceClockParameters s) noexcept
{
    return e.SessionToken != 0 &&
        e.EventOrdinal != 0 &&
        e.EngineClockObserved &&
        e.ActualRaceGameMode &&
        (!s.RequireOriginalTrack || e.OriginalMapVerified) &&
        Finite(e.AtSessionSeconds) &&
        e.AtSessionSeconds >= 0.0 &&
        e.AtSessionSeconds <= s.MaximumSessionSeconds &&
        Finite(e.DeltaSeconds) &&
        e.DeltaSeconds >= 0.0 &&
        e.DeltaSeconds <= s.MaximumFrameSeconds &&
        Finite(e.CountdownSeconds) &&
        e.CountdownSeconds >= 0.0 &&
        e.CountdownSeconds <= s.MaximumCountdownSeconds &&
        e.QualifiedPhysicalCars >= 0 &&
        e.QualifiedPhysicalCars <= PhysicalFleetCapacity &&
        e.WinnerSlot >= -1 &&
        e.WinnerSlot < PhysicalFleetCapacity &&
        e.WinnerCompletedLaps >= 0 &&
        e.WinnerCompletedLaps <= 500;
}
class RaceClockDirector final
{
public:
    bool Configure(RaceClockParameters parameters) noexcept
    {
        if (!ValidRaceClockParameters(parameters))
            return false;
        Settings = parameters;
        Reset();
        return true;
    }
    void Reset() noexcept
    {
        Receipt = {};
        HasSession = false;
        LastInputSeconds = 0.0;
    }
    ClockFault Apply(ClockTransitionRequest event) noexcept
    {
        if (!ValidClockEvent(event, Settings))
            return Reject(ClockFault::InvalidTimestamp);
        if (HasSession && event.SessionToken !=
            Receipt.SessionToken)
            return Reject(ClockFault::WrongSession);
        if (HasSession &&
            (event.EventOrdinal <= Receipt.EventOrdinal ||
             event.Revision != Receipt.Revision))
            return Reject(ClockFault::DuplicateEvent);
        if (HasSession && event.AtSessionSeconds <
            LastInputSeconds)
            return Reject(ClockFault::ClockReversal);
        if (Receipt.Finalized &&
            event.Kind != ClockTransition::Restart)
            return Reject(ClockFault::FinishedImmutable);
        if (!HasSession)
        {
            if (event.Kind != ClockTransition::ArmGrid)
                return Reject(ClockFault::WrongPhase);
            Receipt.SessionToken = event.SessionToken;
            Receipt.Revision = event.Revision;
            HasSession = true;
        }
        RacePhase next = Receipt.Phase;
        switch (event.Kind)
        {
        case ClockTransition::ArmGrid:
            if (Receipt.Phase != RacePhase::Dormant &&
                Receipt.Phase != RacePhase::Aborted)
                return Reject(ClockFault::WrongPhase);
            if (event.QualifiedPhysicalCars <
                Settings.MinimumPhysicalCars)
                return Reject(ClockFault::InsufficientPhysicalCars);
            next = RacePhase::Grid;
            Receipt.QualifiedCarsAtStart =
                event.QualifiedPhysicalCars;
            break;
        case ClockTransition::BeginCountdown:
            if (Receipt.Phase != RacePhase::Grid)
                return Reject(ClockFault::WrongPhase);
            if (event.CountdownSeconds <
                    Settings.MinimumCountdownSeconds ||
                event.CountdownSeconds >
                    Settings.MaximumCountdownSeconds)
                return Reject(ClockFault::InvalidTimestamp);
            Receipt.CountdownRemainingSeconds =
                event.CountdownSeconds;
            next = RacePhase::Countdown;
            break;
        case ClockTransition::CountdownComplete:
            if (Receipt.Phase != RacePhase::Countdown ||
                Receipt.CountdownRemainingSeconds > 0.0001)
                return Reject(ClockFault::NotYetReady);
            if (event.QualifiedPhysicalCars <
                Settings.MinimumPhysicalCars)
                return Reject(ClockFault::InsufficientPhysicalCars);
            next = RacePhase::Green;
            Receipt.RaceStartSessionSeconds =
                event.AtSessionSeconds;
            Receipt.PhysicalRosterAtGo = true;
            break;
        case ClockTransition::EnterCaution:
            if (Receipt.Phase != RacePhase::Green)
                return Reject(ClockFault::WrongPhase);
            next = RacePhase::Caution;
            ++Receipt.CautionPeriods;
            break;
        case ClockTransition::ResumeFromCaution:
            if (Receipt.Phase != RacePhase::Caution)
                return Reject(ClockFault::WrongPhase);
            next = RacePhase::Green;
            break;
        case ClockTransition::Pause:
            if (Receipt.Phase != RacePhase::Green &&
                Receipt.Phase != RacePhase::Caution)
                return Reject(ClockFault::WrongPhase);
            Receipt.BeforePause = Receipt.Phase;
            next = RacePhase::Paused;
            ++Receipt.PausePeriods;
            break;
        case ClockTransition::Unpause:
            if (Receipt.Phase != RacePhase::Paused)
                return Reject(ClockFault::WrongPhase);
            next = Receipt.BeforePause;
            break;
        case ClockTransition::RedFlag:
            if (Receipt.Phase != RacePhase::Green &&
                Receipt.Phase != RacePhase::Caution)
                return Reject(ClockFault::WrongPhase);
            next = RacePhase::RedFlag;
            break;
        case ClockTransition::ResumeFromRedFlag:
            if (Receipt.Phase != RacePhase::RedFlag)
                return Reject(ClockFault::WrongPhase);
            next = RacePhase::Caution;
            break;
        case ClockTransition::Finish:
            if (Receipt.Phase != RacePhase::Green &&
                Receipt.Phase != RacePhase::Caution)
                return Reject(ClockFault::WrongPhase);
            if (event.WinnerSlot < 0 ||
                event.WinnerCompletedLaps < Settings.RequiredLaps ||
                !event.WinningGateVerified)
                return Reject(ClockFault::UnverifiedWinner);
            if (Receipt.ElapsedRaceSeconds <
                Settings.MinimumFinishSeconds)
                return Reject(ClockFault::NotYetReady);
            Receipt.WinnerSlot = event.WinnerSlot;
            Receipt.WinnerCompletedLaps =
                event.WinnerCompletedLaps;
            Receipt.RaceFinishSessionSeconds =
                event.AtSessionSeconds;
            next = RacePhase::Finished;
            Receipt.Finalized = true;
            break;
        case ClockTransition::Abort:
            if (Receipt.Phase == RacePhase::Dormant)
                return Reject(ClockFault::WrongPhase);
            next = RacePhase::Aborted;
            Receipt.Finalized = true;
            break;
        case ClockTransition::Restart:
            if (Receipt.Phase != RacePhase::Aborted &&
                Receipt.Phase != RacePhase::Finished)
                return Reject(ClockFault::WrongPhase);
            if (event.Revision <= Receipt.Revision)
                return Reject(ClockFault::WrongSession);
            {
                const int priorRestarts = Receipt.Restarts;
                const std::uint64_t session = event.SessionToken;
                Reset();
                Receipt.SessionToken = session;
                Receipt.Revision = event.Revision;
                Receipt.Restarts = priorRestarts + 1;
                HasSession = true;
            }
            next = RacePhase::Grid;
            break;
        case ClockTransition::None:
        default:
            return Reject(ClockFault::WrongPhase);
        }
        Receipt.Phase = next;
        Receipt.SessionSeconds =
            event.AtSessionSeconds;
        Receipt.LastTransitionSeconds =
            event.AtSessionSeconds;
        Receipt.LastEngineFrameSeconds =
            event.AtSessionSeconds;
        Receipt.EventOrdinal = event.EventOrdinal;
        Receipt.OriginalTrackVerified =
            event.OriginalMapVerified;
        Receipt.LastFault = ClockFault::None;
        Receipt.OfficialRaceClockModified = false;
        LastInputSeconds = event.AtSessionSeconds;
        ++Receipt.AcceptedTransitions;
        return ClockFault::None;
    }
    bool AdvanceObservedFrame(
        std::uint64_t frameOrdinal,
        double sessionSeconds, double deltaSeconds,
        bool engineMeasured) noexcept
    {
        if (!HasSession || !engineMeasured ||
            frameOrdinal <= Receipt.FrameOrdinal ||
            !Finite(sessionSeconds) ||
            !ValidFrameDelta(deltaSeconds) ||
            sessionSeconds < Receipt.SessionSeconds ||
            sessionSeconds > Settings.MaximumSessionSeconds ||
            std::abs(
                sessionSeconds - Receipt.LastEngineFrameSeconds -
                deltaSeconds) >
                Settings.MaximumClockGapSeconds)
            return false;
        Receipt.FrameOrdinal = frameOrdinal;
        Receipt.LastEngineFrameSeconds = sessionSeconds;
        Receipt.LastFrameDeltaSeconds = deltaSeconds;
        Receipt.SessionSeconds = sessionSeconds;
        if (Receipt.Phase == RacePhase::Countdown)
        {
            Receipt.CountdownRemainingSeconds = std::max(
                0.0, Receipt.CountdownRemainingSeconds -
                     deltaSeconds);
        }
        else if (Receipt.Phase == RacePhase::Green)
        {
            Receipt.ElapsedRaceSeconds += deltaSeconds;
            Receipt.TotalAuthorizedRunningSeconds +=
                deltaSeconds;
        }
        else if (Receipt.Phase == RacePhase::Caution)
        {
            Receipt.CautionElapsedSeconds += deltaSeconds;
            if (!Settings.FreezeClockDuringCaution)
                Receipt.ElapsedRaceSeconds += deltaSeconds;
        }
        else if (Receipt.Phase == RacePhase::Paused)
            Receipt.PausedElapsedSeconds += deltaSeconds;
        Receipt.OfficialRaceClockModified = false;
        LastInputSeconds = sessionSeconds;
        return true;
    }
    RaceClockReceipt Snapshot() const noexcept { return Receipt; }
    bool Racing() const noexcept
    {
        return Receipt.Phase == RacePhase::Green ||
            Receipt.Phase == RacePhase::Caution;
    }
    bool Finished() const noexcept
    {
        return Receipt.Phase == RacePhase::Finished;
    }
    bool CanIssueGreen() const noexcept
    {
        return Receipt.Phase == RacePhase::Countdown &&
            Receipt.CountdownRemainingSeconds <= 0.0001 &&
            Receipt.QualifiedCarsAtStart >=
                Settings.MinimumPhysicalCars;
    }
    double RaceSeconds() const noexcept
    {
        return Receipt.ElapsedRaceSeconds;
    }
private:
    ClockFault Reject(ClockFault reason) noexcept
    {
        ++Receipt.RejectedTransitions;
        Receipt.LastFault = reason;
        return reason;
    }
    RaceClockParameters Settings{};
    RaceClockReceipt Receipt{};
    double LastInputSeconds = 0.0;
    bool HasSession = false;
};
} // namespace latebrake::racecraft
