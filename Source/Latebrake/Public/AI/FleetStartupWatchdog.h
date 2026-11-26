#pragma once

#include "AI/ExperimentalFleetAttempt.h"
#include "Racing/RaceStartLights.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace latebrake
{
// An experimental starting-grid watchdog consumes only eligible game
// ticks, never wall time, menu time or a renderer hitch. It cannot turn
// the start lights green. Its only permission is to allow the existing
// authoritative countdown to consume its next qualified frame.
enum class FleetStartupPhase
{
    AwaitingRoster,
    AwaitingPlacement,
    Settling,
    ReadyForCountdown,
    Suspended,
    TimedOut,
    Terminal
};

enum class FleetStartupCause
{
    None,
    CircuitUnavailable,
    DriverUnavailable,
    RestartPlacementUnverified,
    RosterUnavailable,
    UnsafeGrid,
    InvalidFrame,
    Interrupted
};

struct FleetStartupSample final
{
    std::uint64_t Attempt=0;
    std::uint64_t Frame=0;
    std::uint64_t SessionEpoch=0;
    double DeltaSeconds=0.0;
    bool Active=false;
    bool CircuitReady=false;
    bool LocalDriverReady=false;
    bool PlacementReady=false;
    bool GridAccepted=false;
    FleetAttemptReadout AttemptState{};
};

struct FleetStartupReadout final
{
    FleetStartupPhase Phase=FleetStartupPhase::AwaitingRoster;
    FleetStartupCause Cause=FleetStartupCause::None;
    double ActiveSeconds=0.0;
    double StableSeconds=0.0;
    double RemainingSeconds=0.0;
    bool CountdownAllowed=false;
    bool ResetCountdown=false;
    std::uint64_t Attempt=0;
    std::uint64_t Frame=0;
    std::uint64_t SessionEpoch=0;
    LiveGridValidation Grid{};
};

inline constexpr double FleetStartupBudgetSeconds=15.0;
inline constexpr double FleetStartupStableSeconds=0.30;
inline constexpr double FleetStartupMaxFrameSeconds=0.10;
inline constexpr double FleetStartupTimeTolerance=1.0e-9;

inline const char* FleetGridFailureName(LiveGridFailure cause) noexcept
{
    switch(cause)
    {
    case LiveGridFailure::None:return "grid verified";
    case LiveGridFailure::MissingPhysicalCar:return "missing car";
    case LiveGridFailure::InvalidPhysics:return "invalid physics";
    case LiveGridFailure::DuplicateActorIdentity:return "duplicate car identity";
    case LiveGridFailure::DisplacedFromAssignedSlot:return "car displaced from grid";
    case LiveGridFailure::MovingBeforeGo:return "car still moving";
    case LiveGridFailure::UnsafeVerticalPosition:return "car height not settled";
    case LiveGridFailure::InsufficientGroundSupport:return "road support incomplete";
    case LiveGridFailure::OffPavement:return "car outside pavement";
    case LiveGridFailure::BarrierOverlap:return "car overlaps barrier";
    case LiveGridFailure::CarOverlap:return "cars overlap";
    default:return "unknown grid problem";
    }
}

inline const char* FleetAttemptFaultName(FleetAttemptFault fault) noexcept
{
    switch(fault)
    {
    case FleetAttemptFault::None:return "none";
    case FleetAttemptFault::InvalidAttempt:return "invalid attempt";
    case FleetAttemptFault::SpawnFailed:return "spawn failed";
    case FleetAttemptFault::InvalidRoster:return "invalid roster";
    case FleetAttemptFault::ReplacedActor:return "actor replaced";
    case FleetAttemptFault::MissingActor:return "actor missing";
    case FleetAttemptFault::InvalidPhysics:return "invalid physics";
    case FleetAttemptFault::UnexpectedGreen:return "start authorization lost";
    case FleetAttemptFault::InvalidFrame:return "invalid frame";
    case FleetAttemptFault::GridTimeout:return "grid readiness timeout";
    default:return "unknown fleet fault";
    }
}

inline const char* FleetStartupCauseName(FleetStartupCause cause) noexcept
{
    switch(cause)
    {
    case FleetStartupCause::None:return "ready";
    case FleetStartupCause::CircuitUnavailable:return "circuit unavailable";
    case FleetStartupCause::DriverUnavailable:return "waiting for player";
    case FleetStartupCause::RestartPlacementUnverified:return "player placement unverified";
    case FleetStartupCause::RosterUnavailable:return "waiting for six cars";
    case FleetStartupCause::UnsafeGrid:return "grid not settled";
    case FleetStartupCause::InvalidFrame:return "invalid frame";
    case FleetStartupCause::Interrupted:return "start suspended";
    default:return "unknown startup problem";
    }
}

class FleetStartupWatchdog final
{
public:
    bool Begin(std::uint64_t attempt) noexcept
    {
        if(attempt==0 || attempt<=State.Attempt) return false;
        State={};
        State.Attempt=attempt;
        State.RemainingSeconds=FleetStartupBudgetSeconds;
        HaveEpoch=false;
        EverAllowed=false;
        LastBlockingCause=FleetStartupCause::None;
        return true;
    }

    FleetStartupReadout Readout() const noexcept {return State;}

    bool IsCurrentPermission(std::uint64_t attempt,std::uint64_t frame,
        std::uint64_t epoch) const noexcept
    {
        return State.CountdownAllowed && State.Phase==FleetStartupPhase::ReadyForCountdown &&
            State.Attempt==attempt && State.Frame==frame && State.SessionEpoch==epoch;
    }

    FleetStartupReadout Observe(const FleetStartupSample& s) noexcept
    {
        // Terminal outcomes are latched until an explicit new attempt.
        // A stale sample can never unfreeze a timed-out or started grid.
        State.ResetCountdown=false;
        State.CountdownAllowed=false;
        if(State.Phase==FleetStartupPhase::TimedOut ||
            State.Phase==FleetStartupPhase::Terminal)
            return State;
        if(s.Attempt==0 || s.Attempt!=State.Attempt ||
            s.Frame==0 || s.Frame<=State.Frame ||
            s.AttemptState.AttemptToken!=s.Attempt ||
            !s.AttemptState.Required)
            return Break(FleetStartupPhase::Suspended,FleetStartupCause::InvalidFrame);
        State.Frame=s.Frame;
        const bool changedEpoch=HaveEpoch && State.SessionEpoch!=s.SessionEpoch;
        HaveEpoch=true;
        State.SessionEpoch=s.SessionEpoch;
        State.Grid=s.AttemptState.Grid;
        switch(s.AttemptState.Phase)
        {
        case FleetAttemptPhase::Running:
        case FleetAttemptPhase::Faulted:
        case FleetAttemptPhase::Finished:
        case FleetAttemptPhase::Disabled:
            return Break(FleetStartupPhase::Terminal,FleetStartupCause::None);
        default:break;
        }
        if(!s.Active)
            return Break(FleetStartupPhase::Suspended,FleetStartupCause::Interrupted);
        if(!std::isfinite(s.DeltaSeconds) || s.DeltaSeconds<=0.0 ||
            s.DeltaSeconds>FleetStartupMaxFrameSeconds)
            return Break(FleetStartupPhase::Suspended,FleetStartupCause::InvalidFrame);
        if(changedEpoch)
        {
            Break(FleetStartupPhase::Suspended,FleetStartupCause::Interrupted);
            // Admit only a new uninterrupted sequence after this frame.
            return State;
        }

        // Charge every valid pre-GO active sample, including time spent
        // on the normal countdown. This imposes a strict upper bound on
        // pathological readiness flapping. It is NOT part of race time.
        State.ActiveSeconds=std::min(FleetStartupBudgetSeconds,
            State.ActiveSeconds+s.DeltaSeconds);
        State.RemainingSeconds=FleetStartupBudgetSeconds-State.ActiveSeconds;
        if(State.RemainingSeconds<=FleetStartupTimeTolerance)
        {
            State.RemainingSeconds=0.0;
            return Break(FleetStartupPhase::TimedOut,LastBlockingCause);
        }
        if(!s.CircuitReady)
            return Block(FleetStartupCause::CircuitUnavailable,FleetStartupPhase::AwaitingPlacement);
        if(!s.LocalDriverReady)
            return Block(FleetStartupCause::DriverUnavailable,FleetStartupPhase::AwaitingPlacement);
        if(!s.PlacementReady)
            return Block(FleetStartupCause::RestartPlacementUnverified,FleetStartupPhase::AwaitingPlacement);
        if(s.AttemptState.Phase==FleetAttemptPhase::AwaitingRoster)
            return Block(FleetStartupCause::RosterUnavailable,FleetStartupPhase::AwaitingRoster);
        if(!s.GridAccepted || !s.AttemptState.CountdownAllowed ||
            s.AttemptState.Phase!=FleetAttemptPhase::CountdownQualified ||
            !s.AttemptState.Grid.Safe() || s.AttemptState.LastFrame!=s.Frame)
            return Block(FleetStartupCause::UnsafeGrid,FleetStartupPhase::AwaitingPlacement);

        State.StableSeconds=std::min(FleetStartupStableSeconds,
            State.StableSeconds+s.DeltaSeconds);
        State.Cause=FleetStartupCause::None;
        if(State.StableSeconds+FleetStartupTimeTolerance<FleetStartupStableSeconds)
        {
            State.Phase=FleetStartupPhase::Settling;
            return State;
        }
        State.Phase=FleetStartupPhase::ReadyForCountdown;
        State.CountdownAllowed=true;
        EverAllowed=true;
        return State;
    }

private:
    FleetStartupReadout Break(FleetStartupPhase phase,FleetStartupCause cause) noexcept
    {
        State.Phase=phase;
        State.Cause=cause;
        State.StableSeconds=0.0;
        State.CountdownAllowed=false;
        // Only a previously authorized red countdown needs resetting.
        // Never ask the adapter to rewind the live race after GO.
        State.ResetCountdown=EverAllowed && phase!=FleetStartupPhase::Terminal;
        EverAllowed=false;
        return State;
    }

    FleetStartupReadout Block(FleetStartupCause cause,FleetStartupPhase phase) noexcept
    {
        LastBlockingCause=cause;
        return Break(phase,cause);
    }

    FleetStartupReadout State{};
    FleetStartupCause LastBlockingCause=FleetStartupCause::None;
    bool HaveEpoch=false;
    bool EverAllowed=false;
};
} // namespace latebrake
