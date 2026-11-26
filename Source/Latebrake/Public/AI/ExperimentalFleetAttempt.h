#pragma once

#include "AI/LiveSixCarStartingGrid.h"
#include "AI/RivalFleetTraffic.h"

#include <array>
#include <cstdint>
#include <limits>

namespace latebrake
{
// Portable contract for the opt-in six-car experiment. An engine adapter
// must obtain one complete same-frame snapshot on the game thread; this
// class cannot spawn actors, read Chaos, advance a clock, or award gates.
using SixCarObservations = std::array<RecoveryObstacle, StartGridCapacity>;
using SixCarVerticalObservations = std::array<LiveGridVerticalSample, StartGridCapacity>;
using SixCarIdentity = std::array<std::uint64_t, StartGridCapacity>;

enum class FleetAttemptPhase
{
    Disabled,
    AwaitingRoster,
    AwaitingGrid,
    CountdownQualified,
    Running,
    Faulted,
    Finished
};

enum class FleetAttemptFault
{
    None,
    InvalidAttempt,
    SpawnFailed,
    InvalidRoster,
    ReplacedActor,
    MissingActor,
    InvalidPhysics,
    UnexpectedGreen,
    InvalidFrame
};

struct FleetAttemptConfig final
{
    bool DeveloperRequested = false;
    bool CircuitEvent = false;
    bool CareerEvent = false;
    std::uint64_t AttemptToken = 0;
};

struct FleetPhysicalFrame final
{
    std::uint64_t AttemptToken = 0;
    std::uint64_t FrameToken = 0;
    int Count = 0;
    bool Inactive = false;
    SixCarObservations Cars{};
    SixCarVerticalObservations Vertical{};
};

struct FleetAttemptReadout final
{
    FleetAttemptPhase Phase = FleetAttemptPhase::Disabled;
    FleetAttemptFault Fault = FleetAttemptFault::None;
    LiveGridValidation Grid{};
    int FaultSlot = -1;
    bool Required = false;
    bool CountdownAllowed = false;
    bool RunningAllowed = false;
    std::uint64_t AttemptToken = 0;
    std::uint64_t LastFrame = 0;
};

struct FleetRosterCheck final
{
    FleetAttemptFault Fault = FleetAttemptFault::None;
    int Slot = -1;
    SixCarIdentity Tokens{};
    bool Valid() const noexcept { return Fault == FleetAttemptFault::None; }
};

// Validate identity before geometry. Slot order is an explicit part of
// the adapter contract, never an inference from array position alone.
inline FleetRosterCheck InspectFleetRoster(const FleetPhysicalFrame& frame) noexcept
{
    FleetRosterCheck result;
    if (frame.Count != StartGridCapacity)
    {
        result.Fault = FleetAttemptFault::MissingActor;
        return result;
    }
    for (int i = 0; i < StartGridCapacity; ++i)
    {
        const auto index = static_cast<std::size_t>(i);
        const auto& car = frame.Cars[index];
        if (car.Slot != i || car.ActorToken == 0 || !car.Verified)
        {
            result.Fault = FleetAttemptFault::MissingActor;
            result.Slot = i;
            return result;
        }
        if (!ValidFleetPhysicalObservation(car))
        {
            result.Fault = FleetAttemptFault::InvalidPhysics;
            result.Slot = i;
            return result;
        }
        for (int j = 0; j < i; ++j)
        {
            if (car.ActorToken == result.Tokens[static_cast<std::size_t>(j)])
            {
                result.Fault = FleetAttemptFault::InvalidRoster;
                result.Slot = i;
                return result;
            }
        }
        result.Tokens[index] = car.ActorToken;
    }
    return result;
}

// One accepted restart creates a new attempt token. Only a pre-GO
// snapshot can establish a roster. A lost or replaced actor after that
// binding requires explicit restart: no silent fallback or respawn.
// The pre-GO grid check is intentionally absent from the running path.
class ExperimentalFleetAttempt final
{
public:
    bool Begin(FleetAttemptConfig config) noexcept
    {
        const bool required = config.DeveloperRequested &&
            config.CircuitEvent && !config.CareerEvent;
        // Reject stale lifecycle calls without resetting a valid attempt.
        if (config.AttemptToken == 0 ||
            config.AttemptToken <= State.AttemptToken)
        {
            return false;
        }
        State = {};
        State.AttemptToken = config.AttemptToken;
        State.Required = required;
        State.Phase = required ? FleetAttemptPhase::AwaitingRoster
                               : FleetAttemptPhase::Disabled;
        Roster = {};
        HasRoster = false;
        Current = {};
        CurrentValid = false;
        return true;
    }

    FleetAttemptReadout Readout() const noexcept { return State; }
    const SixCarIdentity& BoundIdentity() const noexcept { return Roster; }

    bool CanAttemptSpawn() const noexcept
    {
        return State.Required && State.Phase == FleetAttemptPhase::AwaitingRoster;
    }

    void ReportSpawnFailure(std::uint64_t attempt) noexcept
    {
        if (attempt == State.AttemptToken && CanAttemptSpawn())
        {
            Fail(FleetAttemptFault::SpawnFailed, -1);
        }
    }

    // Only a newly collected frame may authorize countdown or traffic.
    // Invalid frame IDs do not overwrite the previous identity binding.
    // Any admitted frame consumes its token, including paused frames.
    bool Observe(const FleetPhysicalFrame& frame) noexcept
    {
        ClearPermission();
        if (!State.Required || State.Phase == FleetAttemptPhase::Faulted ||
            State.Phase == FleetAttemptPhase::Finished)
        {
            return false;
        }
        if (frame.AttemptToken != State.AttemptToken || frame.FrameToken == 0 ||
            frame.FrameToken <= State.LastFrame)
        {
            Fail(FleetAttemptFault::InvalidFrame, -1);
            return false;
        }
        State.LastFrame = frame.FrameToken;
        // OS pause may suppress physics observations. Do not interpret an
        // inactive empty sample as a missing physical actor. No authority
        // survives it; the first resumed frame must qualify independently.
        if (frame.Inactive)
        {
            if (State.Phase == FleetAttemptPhase::CountdownQualified)
            {
                State.Phase = FleetAttemptPhase::AwaitingGrid;
            }
            return false;
        }
        const auto checked = InspectFleetRoster(frame);
        if (!checked.Valid())
        {
            if (HasRoster)
            {
                Fail(checked.Fault, checked.Slot);
            }
            else
            {
                // A not-yet-created grid is distinct from a failed spawn.
                // Explicit spawn failure is reported by the engine adapter.
                State.Grid = {LiveGridFailure::MissingPhysicalCar, checked.Slot, -1};
            }
            return false;
        }
        if (!HasRoster)
        {
            Roster = checked.Tokens;
            HasRoster = true;
            State.Phase = FleetAttemptPhase::AwaitingGrid;
        }
        else
        {
            for (int i = 0; i < StartGridCapacity; ++i)
            {
                const auto index = static_cast<std::size_t>(i);
                if (Roster[index] != checked.Tokens[index])
                {
                    Fail(FleetAttemptFault::ReplacedActor, i);
                    return false;
                }
            }
        }
        if (State.Phase == FleetAttemptPhase::Running)
        {
            // Vehicles should leave their assigned grid cells at GO.
            // Only live identity and physics remain required while racing.
            Current = frame;
            CurrentValid = true;
            State.RunningAllowed = true;
            return true;
        }

        State.Grid = EvaluateLiveSixCarStartingGrid(
            frame.Cars, frame.Vertical, frame.Count);
        if (!State.Grid.Safe())
        {
            State.Phase = FleetAttemptPhase::AwaitingGrid;
            return false;
        }
        Current = frame;
        CurrentValid = true;
        State.Phase = FleetAttemptPhase::CountdownQualified;
        State.CountdownAllowed = true;
        return true;
    }

    // Called only on the actual start-light transition, after Observe
    // admitted the very same frame. It cannot itself turn lights green.
    bool CommitGreen(std::uint64_t attempt, std::uint64_t frame) noexcept
    {
        if (!State.Required)
        {
            return false;
        }
        if (State.Phase == FleetAttemptPhase::Faulted ||
            State.Phase == FleetAttemptPhase::Finished)
        {
            return false;
        }
        if (State.Phase != FleetAttemptPhase::CountdownQualified ||
            !State.CountdownAllowed || !CurrentValid ||
            attempt != State.AttemptToken || frame != State.LastFrame)
        {
            Fail(FleetAttemptFault::UnexpectedGreen, -1);
            return false;
        }
        State.Phase = FleetAttemptPhase::Running;
        State.CountdownAllowed = false;
        State.RunningAllowed = true;
        return true;
    }

    // Consumers must use this exact admitted frame, not a saved snapshot.
    // The player cannot masquerade as a rival subject; all other five
    // slots are present exactly once and retain their physical identity.
    bool MakeRivalTraffic(int subjectSlot, std::uint64_t subjectToken,
                          std::uint64_t attempt, std::uint64_t frame,
                          bool brakeObservationQualified,
                          double conservativeBrakeScale,
                          RivalFleetTrafficSample& output) const noexcept
    {
        output = {};
        if (!MatchesRunningFrame(attempt, frame) ||
            subjectSlot <= HumanGridSlot || subjectSlot >= StartGridCapacity ||
            subjectToken == 0 ||
            Roster[static_cast<std::size_t>(subjectSlot)] != subjectToken ||
            (brakeObservationQualified &&
             (!std::isfinite(conservativeBrakeScale) ||
              conservativeBrakeScale <= 0.0 || conservativeBrakeScale > 1.0)))
        {
            return false;
        }
        output.Self = Current.Cars[static_cast<std::size_t>(subjectSlot)];
        for (int i = 0; i < StartGridCapacity; ++i)
        {
            if (i == subjectSlot)
            {
                continue;
            }
            output.Blockers[static_cast<std::size_t>(output.BlockerCount++)] =
                Current.Cars[static_cast<std::size_t>(i)];
        }
        output.SessionQualified = true;
        output.BrakeObservationQualified = brakeObservationQualified;
        output.ConservativeBrakeScale =
            brakeObservationQualified ? conservativeBrakeScale : 0.0;
        return true;
    }

    // Shared recovery preflight: for either the player or an AI, omit
    // only the exact subject. Failure always yields an empty result.
    bool MakeRecoveryBlockers(int subjectSlot, std::uint64_t subjectToken,
                              std::uint64_t attempt, std::uint64_t frame,
                              std::array<RecoveryObstacle, MaxRecoveryObstacles>& output,
                              int& count) const noexcept
    {
        output = {};
        count = 0;
        if (!MatchesRunningFrame(attempt, frame) ||
            subjectSlot < HumanGridSlot || subjectSlot >= StartGridCapacity ||
            subjectToken == 0 ||
            Roster[static_cast<std::size_t>(subjectSlot)] != subjectToken)
        {
            return false;
        }
        for (int i = 0; i < StartGridCapacity; ++i)
        {
            if (i != subjectSlot)
            {
                output[static_cast<std::size_t>(count++)] =
                    Current.Cars[static_cast<std::size_t>(i)];
            }
        }
        return true;
    }

    void Finish(std::uint64_t attempt) noexcept
    {
        if (attempt != State.AttemptToken || !State.Required ||
            State.Phase != FleetAttemptPhase::Running)
        {
            return;
        }
        ClearPermission();
        State.Phase = FleetAttemptPhase::Finished;
    }

private:
    bool MatchesRunningFrame(std::uint64_t attempt, std::uint64_t frame) const noexcept
    {
        return CurrentValid && State.RunningAllowed &&
            State.Phase == FleetAttemptPhase::Running &&
            attempt == State.AttemptToken && frame == State.LastFrame;
    }

    void ClearPermission() noexcept
    {
        State.CountdownAllowed = false;
        State.RunningAllowed = false;
        CurrentValid = false;
        Current = {};
    }

    void Fail(FleetAttemptFault fault, int slot) noexcept
    {
        ClearPermission();
        State.Phase = FleetAttemptPhase::Faulted;
        State.Fault = fault;
        State.FaultSlot = slot;
    }

    FleetAttemptReadout State{};
    SixCarIdentity Roster{};
    bool HasRoster = false;
    FleetPhysicalFrame Current{};
    bool CurrentValid = false;
};
} // namespace latebrake
