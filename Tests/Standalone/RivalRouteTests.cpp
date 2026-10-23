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
    // A restarted 4-lap event must clear waypoint, lap and stuck-recovery
    // history while preserving its newly selected 4-lap target.
    careerRival.Configure(4, 4);
    Check(careerRival.Observe(200, 200, 0.1).Advanced, "replay route moves");
    Check(!careerRival.Observe(3000, 0, 8.0).Advanced &&
          careerRival.GetRecoveryCount() == 1, "stuck replay records recovery");
    careerRival.Configure(4, 4);
    Check(careerRival.GetNextWaypoint() == 0 &&
          careerRival.GetCompletedLaps() == 0 &&
          careerRival.GetRecoveryCount() == 0 &&
          careerRival.GetRequiredLaps() == 4 &&
          !careerRival.IsFinished(), "restarting same event clears route state");

    // RestartRace uses ResetPilot() on an already configured rival;
    // verify that reset clears only progress and not the current lap goal.
    careerRival.Configure(4, 4);
    Check(careerRival.Observe(200, 200, 0.1).Advanced,
          "configured 4-lap route can advance");
    careerRival.Reset();
    Check(careerRival.GetRequiredLaps() == 4 &&
          careerRival.GetWaypointCount() == 4 &&
          careerRival.GetNextWaypoint() == 0 &&
          careerRival.GetCompletedLaps() == 0 && !careerRival.IsFinished(),
          "race restart retains current event configuration");

    careerRival.Configure(0, 0);
    Check(careerRival.GetWaypointCount() == 1 && careerRival.GetRequiredLaps() == 1,
          "bad reconfiguration clamps both dimensions to one");
    Check(careerRival.Observe(0, 100, .1).Completed,
          "minimum reconfigured route can finish");
    careerRival.Configure(999, 999);
    Check(careerRival.GetWaypointCount() == 256 &&
          careerRival.GetRequiredLaps() == 100 &&
          !careerRival.IsFinished(), "hostile route dimensions are bounded");

    // Extremely long-running sessions must never be granted free
    // unrecorded teleports once the public recovery telemetry saturates.
    latebrake::RivalRouteProgress budget(4, 3);
    for (int recovery = 0;
         recovery < latebrake::RivalRouteProgress::MaxRecoveries; ++recovery)
    {
        const auto result = budget.Observe(3000.0, 0.0, 8.0);
        Check(result.NeedsRecovery && !result.Advanced && !result.Completed,
              "every approved rival teleport must be counted");
    }
    Check(budget.GetRecoveryCount() ==
          latebrake::RivalRouteProgress::MaxRecoveries,
          "rival recovery budget is bounded and exact");
    Check(!budget.Observe(3000.0, 0.0, 8.0).NeedsRecovery,
          "exhausted rival budget cannot authorize a free teleport");
    Check(budget.GetRecoveryCount() ==
          latebrake::RivalRouteProgress::MaxRecoveries,
          "budget exhaustion cannot wrap or overcount");
    budget.Observe(3000.0, 200.0, 0.1);
    Check(!budget.Observe(3000.0, 0.0, 8.0).NeedsRecovery,
          "moving between stuck intervals cannot replenish exhausted budget");
    budget.Reset();
    Check(budget.GetRecoveryCount() == 0 &&
          budget.Observe(3000.0, 0.0, 8.0).NeedsRecovery &&
          budget.GetRecoveryCount() == 1,
          "a new race attempt starts a fresh finite rival recovery budget");

    std::cout << "Autonomous route tests passed.\n";
    return EXIT_SUCCESS;
}
