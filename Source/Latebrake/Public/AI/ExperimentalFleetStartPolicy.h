#pragma once

namespace latebrake
{
// Non-shipping experimental fleet launch policy. Pure decision logic:
// no actor spawn, Unreal API, clock mutation, race award or save authority.
// The *request* is independent from fleet construction success so a
// failed six-car test never silently becomes a two-car "successful" start.
struct ExperimentalFleetStartInput final
{
    bool DeveloperRequested = false;
    bool CircuitEvent = false;
    bool CareerEvent = false;
    bool TerminalSpawnFault = false;
    bool RosterPresent = false;
    bool AllPhysicalCarsReady = false;
    bool OriginalCircuitReady = false;
    bool LocalDriverReady = false;
    bool PrimaryRivalReady = false;
    bool Inactive = false;
    bool RaceFinished = false;
    bool LightsAlreadyGreen = false;
};

struct ExperimentalFleetStartDecision final
{
    bool RequiredForEvent = false;
    bool MayAttemptPhysicalSpawn = false;
    bool PermitCountdown = true;
    bool BlockedByTerminalFault = false;
};

constexpr ExperimentalFleetStartDecision DecideExperimentalFleetStart(
    ExperimentalFleetStartInput s) noexcept
{
    // Career and solo Time Attack never require a six-car fleet, even
    // when the developer left the test flag on from another event.
    const bool required =
        s.DeveloperRequested && s.CircuitEvent && !s.CareerEvent;
    const bool fault = required && s.TerminalSpawnFault;
    const bool trySpawn =
        required && !fault && !s.RosterPresent &&
        s.OriginalCircuitReady && s.LocalDriverReady &&
        s.PrimaryRivalReady && !s.Inactive && !s.RaceFinished &&
        !s.LightsAlreadyGreen;
    const bool countdown =
        !required || (!fault && s.AllPhysicalCarsReady);
    return {required, trySpawn, countdown, fault};
}
} // namespace latebrake
