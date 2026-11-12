#include "AI/ExperimentalFleetStartPolicy.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool result, const char* why)
{
    if (!result)
    {
        std::cerr << "ExperimentalFleetStartPolicy FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    ExperimentalFleetStartInput s;
    // Default retail: never spawn extra cars or gate the one-rival clock.
    for (int mask = 0; mask < 256; ++mask)
    {
        s.TerminalSpawnFault = (mask & 1) != 0;
        s.RosterPresent = (mask & 2) != 0;
        s.AllPhysicalCarsReady = (mask & 4) != 0;
        s.OriginalCircuitReady = (mask & 8) != 0;
        s.LocalDriverReady = (mask & 16) != 0;
        s.PrimaryRivalReady = (mask & 32) != 0;
        s.Inactive = (mask & 64) != 0;
        s.RaceFinished = (mask & 128) != 0;
        for (int event = 0; event < 3; ++event)
        {
            s.DeveloperRequested = false;
            s.CircuitEvent = event != 2;
            s.CareerEvent = event == 1;
            const auto out = DecideExperimentalFleetStart(s);
            Require(!out.RequiredForEvent &&
                    !out.MayAttemptPhysicalSpawn &&
                    out.PermitCountdown && !out.BlockedByTerminalFault,
                    "default racing never waits for experimental AI");
        }
    }

    s = {};
    s.DeveloperRequested = true;
    s.CircuitEvent = true;
    s.OriginalCircuitReady = true;
    s.LocalDriverReady = true;
    s.PrimaryRivalReady = true;
    auto out = DecideExperimentalFleetStart(s);
    Require(out.RequiredForEvent && out.MayAttemptPhysicalSpawn &&
            !out.PermitCountdown && !out.BlockedByTerminalFault,
            "opt-in circuit waits for authenticated four extra chassis");
    s.RosterPresent = true;
    out = DecideExperimentalFleetStart(s);
    Require(!out.MayAttemptPhysicalSpawn && !out.PermitCountdown,
            "registered bodies are not necessarily physics ready");
    s.AllPhysicalCarsReady = true;
    out = DecideExperimentalFleetStart(s);
    Require(out.RequiredForEvent && out.PermitCountdown &&
            !out.MayAttemptPhysicalSpawn, "six ready real cars may count down");

    // A failed physical SpawnActor / TryRecoverTo must not flip the
    // request OFF and accidentally let a two-car race go green.
    s.TerminalSpawnFault = true;
    s.RosterPresent = false;
    s.AllPhysicalCarsReady = false;
    out = DecideExperimentalFleetStart(s);
    Require(out.RequiredForEvent && out.BlockedByTerminalFault &&
            !out.MayAttemptPhysicalSpawn && !out.PermitCountdown,
            "fatal physics setup latches a visible failed experimental run");
    s.AllPhysicalCarsReady = true; // stale other evidence cannot clear fault
    Require(!DecideExperimentalFleetStart(s).PermitCountdown,
            "stale readiness does not override terminal spawn failure");
    s.TerminalSpawnFault = false; // ONLY after explicit accepted restart
    s.AllPhysicalCarsReady = false;
    Require(DecideExperimentalFleetStart(s).MayAttemptPhysicalSpawn,
            "accepted restart can deliberately begin a new grid experiment");

    // Leaving the opt-in circuit for solo/career must not freeze those
    // independently valid ordinary event countdowns.
    s.TerminalSpawnFault = true;
    s.CircuitEvent = false;
    Require(DecideExperimentalFleetStart(s).PermitCountdown &&
            !DecideExperimentalFleetStart(s).RequiredForEvent,
            "solo event ignores previously requested six-car grid");
    s.CircuitEvent = true;
    s.CareerEvent = true;
    Require(DecideExperimentalFleetStart(s).PermitCountdown &&
            !DecideExperimentalFleetStart(s).RequiredForEvent,
            "career event always retains original one-rival timing");

    s = {};
    s.DeveloperRequested = true;
    s.CircuitEvent = true;
    s.OriginalCircuitReady = true;
    s.LocalDriverReady = true;
    s.PrimaryRivalReady = true;
    s.Inactive = true;
    Require(!DecideExperimentalFleetStart(s).MayAttemptPhysicalSpawn,
            "paused or menu-blocked world cannot create physical actors");
    s.Inactive = false;
    s.LightsAlreadyGreen = true;
    Require(!DecideExperimentalFleetStart(s).MayAttemptPhysicalSpawn,
            "never spawn extra actors in an already green live race");
    s.LightsAlreadyGreen = false;
    s.RaceFinished = true;
    Require(!DecideExperimentalFleetStart(s).MayAttemptPhysicalSpawn,
            "never create extras after finish");
    s.RaceFinished = false;
    s.PrimaryRivalReady = false;
    Require(!DecideExperimentalFleetStart(s).MayAttemptPhysicalSpawn,
            "unverified primary rival cannot launch fleet");
    s.PrimaryRivalReady = true;
    s.LocalDriverReady = false;
    Require(!DecideExperimentalFleetStart(s).MayAttemptPhysicalSpawn,
            "missing possessed human cannot launch fleet");
    s.LocalDriverReady = true;
    s.OriginalCircuitReady = false;
    Require(!DecideExperimentalFleetStart(s).MayAttemptPhysicalSpawn,
            "unbuilt circuit cannot host a safe physical grid");

    // All eight guard booleans are varied to ensure the two outcomes
    // remain mutually consistent, not merely a few scripted examples.
    for (int mask = 0; mask < 4096; ++mask)
    {
        s.DeveloperRequested = (mask & 1) != 0;
        s.CircuitEvent = (mask & 2) != 0;
        s.CareerEvent = (mask & 4) != 0;
        s.TerminalSpawnFault = (mask & 8) != 0;
        s.RosterPresent = (mask & 16) != 0;
        s.AllPhysicalCarsReady = (mask & 32) != 0;
        s.OriginalCircuitReady = (mask & 64) != 0;
        s.LocalDriverReady = (mask & 128) != 0;
        s.PrimaryRivalReady = (mask & 256) != 0;
        s.Inactive = (mask & 512) != 0;
        s.RaceFinished = (mask & 1024) != 0;
        s.LightsAlreadyGreen = (mask & 2048) != 0;
        out = DecideExperimentalFleetStart(s);
        Require(!out.MayAttemptPhysicalSpawn ||
                (out.RequiredForEvent && !out.BlockedByTerminalFault &&
                 !s.RosterPresent && s.LocalDriverReady &&
                 s.PrimaryRivalReady && s.OriginalCircuitReady &&
                 !s.Inactive && !s.RaceFinished && !s.LightsAlreadyGreen),
                "each physical spawn request is fully qualified");
        Require(!out.BlockedByTerminalFault ||
                (!out.PermitCountdown && !out.MayAttemptPhysicalSpawn),
                "fatal error cannot be promoted to a race-ready grid");
        Require(out.RequiredForEvent || out.PermitCountdown,
                "ordinary events never wait for experimental opponents");
        Require(!out.PermitCountdown || !out.RequiredForEvent ||
                (s.AllPhysicalCarsReady && !s.TerminalSpawnFault),
                "no partial or faulted six-car countdown");
    }
    std::cout << "Experimental fleet launch fault gate passed 4,096 combinations.\n";
    return EXIT_SUCCESS;
}
