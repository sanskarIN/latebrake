#include "AI/RivalRouteProgress.h"
#include "Career/CareerCatalog.h"
#include "Racing/RaceEvents.h"
#include <cstdlib>
#include <iostream>
#include <limits>

void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "Route FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    latebrake::RivalRouteProgress route(4, 2);
    Check(!route.IsFinished(), "new route active");
    Check(!route.Observe(900, 100, 0.1).Advanced, "far waypoint not reached");
    Check(route.Observe(439, 10, 0.1).Advanced, "inside radius reaches waypoint");
    Check(route.GetNextWaypoint() == 1, "next waypoint increments");
    Check(!route.Observe(0, 0, -1).Advanced, "reject negative delta");
    Check(!route.Observe(-1, 0, 0.1).Advanced, "reject negative distance");
    Check(!route.Observe(std::numeric_limits<double>::infinity(), 0, 0.1).Advanced,
          "reject infinite distance");
    Check(!route.Observe(0, std::numeric_limits<double>::quiet_NaN(), 0.1).Advanced,
          "reject NaN speed");
    for (int i = 1; i < 4; ++i)
    {
        Check(route.Observe(200, 300, 0.1).Advanced, "waypoint pass");
    }
    Check(route.GetCompletedLaps() == 1, "one lap after four waypoints");
    Check(route.GetNextWaypoint() == 0, "waypoint index wraps");

    Check(!route.Observe(3000, 20, 3.0).NeedsRecovery, "not stuck before threshold");
    Check(!route.Observe(3000, 20, 4.9).NeedsRecovery, "7.9 seconds below threshold");
    Check(route.Observe(3000, 20, 0.1).NeedsRecovery, "8 seconds requests recovery");
    Check(route.GetRecoveryCount() == 1, "recovery count increases");
    Check(!route.Observe(3000, 20, 2.0).NeedsRecovery, "recovery timer restarted");
    route.Observe(3000, 100, 0.1);
    Check(!route.Observe(3000, 20, 6.5).NeedsRecovery,
          "moving resets stuck timer");
    Check(route.Observe(3000, 20, 1.5).NeedsRecovery, "stuck timer reaches 8");
    Check(route.GetRecoveryCount() == 2, "second recovery recorded");

    for (int i = 0; i < 4; ++i)
    {
        auto update = route.Observe(200, 200, 0.1);
        Check(update.Advanced, "second lap waypoint pass");
        Check(update.Completed == (i == 3), "finish only on last waypoint");
    }
    Check(route.IsFinished(), "two laps finish route");
    Check(!route.Observe(0, 0, 1).Advanced, "cannot progress beyond finish");
    route.Reset();
    Check(!route.IsFinished() && route.GetCompletedLaps() == 0 &&
          route.GetNextWaypoint() == 0 && route.GetRecoveryCount() == 0,
          "reset clears all route state");

    latebrake::RivalRouteProgress clamped(0, 0);
    Check(clamped.Observe(0, 0, 1).Completed, "invalid config clamps to one each");

    // Every career round that actually spawns a rival must use the selected
    // event's lap count, not the three-lap quick-race default.
    latebrake::RivalRouteProgress careerRival;
    for (const auto& event : latebrake::CareerEvents)
    {
        if (!event.SpawnRival) continue;
        careerRival.Configure(latebrake::GetRaceEvent(latebrake::EventKind::Circuit).CheckpointCount,
                              event.Laps);
        Check(careerRival.GetRequiredLaps() == event.Laps &&
              careerRival.GetWaypointCount() == 4,
              "rival route must match the current career lap target");
        Check(!careerRival.IsFinished() && careerRival.GetCompletedLaps() == 0,
              "switching career rounds must reset progress");
        for (int lap = 0; lap < event.Laps; ++lap)
        {
            for (int waypoint = 0; waypoint < 4; ++waypoint)
            {
                const auto result = careerRival.Observe(200, 200, 0.1);
                Check(result.Advanced, "waypoint advancement in event-specific route");
                Check(result.Completed == (lap == event.Laps - 1 && waypoint == 3),
                      "rival completes only at selected event's final waypoint");
            }
        }
        Check(careerRival.IsFinished() &&
              careerRival.GetCompletedLaps() == event.Laps,
              "career route never overcounts selected event laps");
        Check(!careerRival.Observe(200, 200, 0.1).Advanced,
              "a completed career rival route stays finished");
    }
    std::cout << "Autonomous route tests passed.\n";
    return EXIT_SUCCESS;
}
