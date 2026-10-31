#include "AI/RivalRouteProgress.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* reason)
{
    if (!ok)
    {
        std::cerr << "RivalNoApproachRecovery FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using latebrake::RivalRouteProgress;

    RivalRouteProgress trapped;
    for (int frame = 0; frame < 159; ++frame)
    {
        const auto observation = trapped.Observe(3200.0, 1400.0, 0.05);
        Check(!observation.NeedsRecovery && !observation.Advanced &&
              trapped.GetRecoveryCount() == 0,
              "moving against wall cannot teleport before eight seconds");
    }
    Check(trapped.NoApproachObservationSeconds() >= 7.94,
          "fast but non-approaching car accumulates continuous stall time");
    const auto first = trapped.Observe(3200.0, 1400.0, 0.05);
    Check(first.NeedsRecovery && trapped.HasPendingRecovery() &&
          trapped.GetRecoveryCount() == 1,
          "speed alone must not suppress verified recovery after eight seconds");
    Check(!trapped.Observe(300.0, 1400.0, 8.0).Advanced &&
          !trapped.Observe(3200.0, 1400.0, 8.0).NeedsRecovery,
          "pending physical teleport blocks new laps and duplicate requests");
    Check(trapped.ReportRecoveryResult(false) &&
          trapped.GetFailedRecoveryCount() == 1 &&
          !trapped.HasPendingRecovery(),
          "failed high-speed recovery consumes the same auditable budget");
    for (int i = 0; i < 159; ++i)
    {
        Check(!trapped.Observe(3200.0, 1400.0, 0.05).NeedsRecovery,
              "fresh failed-recovery interval is mandatory");
    }
    Check(trapped.Observe(3200.0, 1400.0, 0.05).NeedsRecovery &&
          trapped.ReportRecoveryResult(true) &&
          trapped.GetSuccessfulRecoveryCount() == 1 &&
          trapped.GetFailedRecoveryCount() == 1,
          "verified retry succeeds only after fresh observed non-approach");

    // A small loop around the same position may have substantial measured
    // velocity but never approach its next checkpoint by 260cm.
    RivalRouteProgress circling;
    for (int i = 0; i < 159; ++i)
    {
        const double distance = 3300.0 +
            150.0 * std::sin(static_cast<double>(i) * 0.08);
        Check(!circling.Observe(distance, 1500.0, 0.05).NeedsRecovery,
              "small-radius looping is not healthy waypoint progress");
    }
    Check(circling.Observe(3300.0, 1500.0, 0.05).NeedsRecovery,
          "circling rival eventually requests controlled recovery");

    // Hundreds of genuine approach samples cannot accumulate artificial
    // stalled time. Meaningful 260cm progress repeatedly resets the window.
    RivalRouteProgress progressing;
    for (int i = 0; i < 200; ++i)
    {
        const double distance = 15000.0 - 30.0 * static_cast<double>(i);
        Check(!progressing.Observe(distance, 1800.0, 0.05).NeedsRecovery,
              "real gate approach must never trigger high-speed recovery");
    }
    Check(progressing.GetRecoveryCount() == 0,
          "successful approach spends no recovery budget");
    progressing.Reset();
    Check(progressing.NoApproachObservationSeconds() == 0.0,
          "race reset clears the previous high-speed stall observation");

    RivalRouteProgress smallMovement;
    for (int i = 0; i < 159; ++i)
    {
        Check(!smallMovement.Observe(
            4000.0 - (i % 3) * 20.0, 1200.0, 0.05).NeedsRecovery,
            "small waypoint jitter does not count as an actual approach");
    }
    Check(smallMovement.Observe(4000.0, 1200.0, 0.05).NeedsRecovery,
          "oscillating small distance samples must not reset stall timer");

    RivalRouteProgress frameHitch;
    for (int i = 0; i < 100; ++i)
    {
        frameHitch.Observe(4000.0, 1400.0, 0.05);
    }
    Check(frameHitch.NoApproachObservationSeconds() > 4.9,
          "regular physical frames establish a real non-approach observation");
    frameHitch.Observe(4000.0, 1400.0, 0.11);
    Check(frameHitch.NoApproachObservationSeconds() == 0.0,
          "invalid long frame cannot certify continuous high-speed stall");
    for (int i = 0; i < 159; ++i)
    {
        Check(!frameHitch.Observe(4000.0, 1400.0, 0.05).NeedsRecovery,
              "new clean eight-second window required after frame hitch");
    }
    Check(frameHitch.Observe(4000.0, 1400.0, 0.05).NeedsRecovery,
          "clean continuous observation eventually permits recovery");

    RivalRouteProgress marginal;
    for (int i = 0; i < 500; ++i)
    {
        Check(!marginal.Observe(4000.0, 300.0, 0.05).NeedsRecovery,
              "low non-stationary crawl does not assert high-speed spinning");
    }
    Check(marginal.GetRecoveryCount() == 0,
          "marginal-speed observations never authorize this new recovery");
    Check(!marginal.Observe(4000.0, 0.0, 7.9).NeedsRecovery &&
          marginal.Observe(4000.0, 0.0, 0.1).NeedsRecovery,
          "original stationary eight-second recovery remains intact");

    RivalRouteProgress invalid;
    for (int i = 0; i < 140; ++i)
    {
        invalid.Observe(6000.0, 1800.0, 0.05);
    }
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Check(!invalid.Observe(nan, 1800.0, 0.05).NeedsRecovery &&
          invalid.NoApproachObservationSeconds() == 0.0,
          "invalid waypoint distance clears stale no-approach authority");
    Check(!invalid.Observe(6000.0, 1800.0, 0.05).NeedsRecovery,
          "invalid data cannot authorize a same-frame recovery");
    invalid.Reset();
    for (int i = 0; i < 140; ++i)
    {
        invalid.Observe(6000.0, 1800.0, 0.05);
    }
    Check(!invalid.Observe(60000.0, 1800.0, 0.05).NeedsRecovery &&
          invalid.NoApproachObservationSeconds() == 0.0,
          "implausible target range cannot manufacture route stall evidence");
    invalid.Configure(4, 2);
    Check(invalid.GetRecoveryCount() == 0 &&
          invalid.NoApproachObservationSeconds() == 0.0,
          "event reconfiguration cannot carry old rival stall state");

    RivalRouteProgress arrival(4, 2);
    for (int i = 0; i < 130; ++i)
    {
        arrival.Observe(3400.0, 1200.0, 0.05);
    }
    Check(arrival.Observe(400.0, 1200.0, 0.05).Advanced &&
          arrival.GetNextWaypoint() == 1 &&
          arrival.NoApproachObservationSeconds() == 0.0,
          "real checkpoint arrival clears old waypoint stall history");
    for (int i = 0; i < 159; ++i)
    {
        Check(!arrival.Observe(3400.0, 1200.0, 0.05).NeedsRecovery,
              "new waypoint must establish its own fresh observation window");
    }
    Check(arrival.Observe(3400.0, 1200.0, 0.05).NeedsRecovery,
          "non-approach can then recover safely at the new waypoint");

    // 200000 real-pattern samples: forward progress >260cm every few
    // frames must never be mistaken for high-speed looping.
    RivalRouteProgress healthy;
    for (int i = 0; i < 200000; ++i)
    {
        if (i % 200 == 0)
        {
            healthy.Reset();
        }
        const int frame = i % 200;
        const double distance = 17000.0 - frame * 30.0;
        const auto o = healthy.Observe(distance, 1550.0, 0.05);
        Check(!o.NeedsRecovery && !o.Advanced && !o.Completed,
              "200000 approaching frames preserve valid rival route progress");
    }

    // 200000 continuous wall-slide frames must each consume exactly one
    // budget slot per eight seconds and report outcomes synchronously.
    RivalRouteProgress longStall;
    int issued = 0;
    for (int i = 0; i < 200000; ++i)
    {
        const auto o = longStall.Observe(3900.0, 1500.0, 0.05);
        Check(!o.Advanced && !o.Completed, "wall sliding never earns laps");
        if (o.NeedsRecovery)
        {
            Check(longStall.HasPendingRecovery(),
                  "every requested teleport has a pending audited outcome");
            Check(longStall.ReportRecoveryResult((issued % 2) == 0),
                  "every counted attempt must be synchronously acknowledged");
            ++issued;
        }
    }
    Check(issued == 1250 &&
          longStall.GetRecoveryCount() == 1250 &&
          longStall.GetSuccessfulRecoveryCount() == 625 &&
          longStall.GetFailedRecoveryCount() == 625 &&
          !longStall.HasPendingRecovery(),
          "200000 moving-but-stranded samples conserve the recovery budget");

    std::cout << "RivalNoApproachRecovery: 400000 stress samples passed (no Unreal)\n";
    return EXIT_SUCCESS;
}
