#include "Vehicles/SuspensionQaCadence.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "Suspension QA cadence failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    SuspensionQaCadence cadence;
    constexpr double tick = 1.0 / 60.0;
    constexpr unsigned allLoaded = 0x0Fu;

    Require(cadence.ShouldReport(tick, allLoaded, true),
            "first real physical frame must be visible immediately");
    int reports = 1;
    for (int i = 0; i < 119; ++i)
    {
        reports += cadence.ShouldReport(tick, allLoaded, true) ? 1 : 0;
    }
    Require(reports == 1,
            "stable ground cannot flood the logs before two-second heartbeat");
    Require(cadence.ShouldReport(tick, allLoaded, true),
            "stable two-second contact requires one periodic heartbeat");
    Require(!cadence.ShouldReport(tick, allLoaded, true),
            "heartbeat must reset the elapsed timer");

    // Use an explicitly fresh publication boundary for an exact 12-tick
    // cooldown. The heartbeat check above intentionally advanced one
    // extra no-report tick; reusing its timer would cross 0.20 s early.
    cadence.Reset();
    Require(cadence.ShouldReport(tick, allLoaded, true),
            "fresh observed boundary is the baseline for wheel-loss timing");
    // Front/rear loss must be recorded as soon as rate limiting permits,
    // rather than being hidden behind the previous two-second schedule.
    for (int i = 0; i < 12; ++i)
    {
        const bool emitted = cadence.ShouldReport(tick, 0x0Cu, true);
        if (i < 11) Require(!emitted,
                           "wheel-loss event must not bypass log rate limiter");
        else Require(emitted,
                     "wheel-loss must be reported after bounded cooldown");
    }
    Require(!cadence.ShouldReport(tick, 0x0Cu, true),
            "unchanged one-axle contact cannot repeatedly log");
    int changedPhase = 0;
    for (int i = 0; i < 12; ++i)
        changedPhase += cadence.ShouldReport(tick, 0x0Cu, false) ? 1 : 0;
    Require(changedPhase == 1,
            "grid-to-GO must be recorded once after cooldown");
    int recovered = 0;
    for (int i = 0; i < 12; ++i)
        recovered += cadence.ShouldReport(tick, allLoaded, false) ? 1 : 0;
    Require(recovered == 1,
            "recontact must be logged without repeated noisy frames");

    // Loaded corner bits can change while geometric spring contact count
    // remains four: damper rebound may unload a physically nearby tire.
    int rebound = 0;
    for (int i = 0; i < 12; ++i)
        rebound += cadence.ShouldReport(tick, 0x0Eu, false) ? 1 : 0;
    Require(rebound == 1, "one unloaded spring must trigger a QA observation");

    cadence.Reset();
    Require(cadence.ShouldReport(tick, 0, false),
            "fresh race after an interrupted session begins with a record");
    Require(!cadence.ShouldReport(0.0, 0, false),
            "zero elapsed time is not a physical QA sample");
    Require(cadence.ShouldReport(tick, 0, false),
            "invalid sample must clear the old per-car QA cadence");
    Require(!cadence.ShouldReport(
                std::numeric_limits<double>::quiet_NaN(), 0, false),
            "nonfinite time must fail closed");
    Require(cadence.ShouldReport(tick, allLoaded, false),
            "NaN frame cannot suppress valid resumed observation");
    Require(!cadence.ShouldReport(0.11, allLoaded, false),
            "large hitch must not be treated as trusted physics sampling");
    Require(!cadence.ShouldReport(tick, 16, false),
            "fifth invented wheel bit must be rejected");

    // Over 24,000 stable frames (~400 s), the heartbeat guarantees a
    // bounded output rate for each independently sampled car. No network,
    // file writes, scores or Unreal headers belong in this policy.
    cadence.Reset();
    int stableReports = 0;
    for (int i = 0; i < 24000; ++i)
        stableReports += cadence.ShouldReport(tick, allLoaded, false) ? 1 : 0;
    Require(stableReports >= 190 && stableReports <= 205,
            "stable car must emit roughly one record per two seconds");

    // Rapid mask chatter must never emit faster than 0.2 seconds even on
    // alternating loaded-corner patterns, including 120 FPS physics.
    cadence.Reset();
    int eventReports = 0;
    for (int i = 0; i < 120000; ++i)
    {
        const unsigned mask = (i & 1) == 0 ? 0x0Fu : 0x03u;
        eventReports += cadence.ShouldReport(1.0 / 120.0, mask, false) ? 1 : 0;
    }
    Require(eventReports <= 5001 && eventReports >= 4900,
            "120k contact-chatter frames cannot spam the Unreal logger");
    std::cout << "Suspension QA cadence passed (144k deterministic frames).\n";
    return EXIT_SUCCESS;
}
