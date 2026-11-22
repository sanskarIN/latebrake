#include "AI/RivalPassTrafficResolution.h"
#include "AI/RivalStraightPassAssist.h"
#include "Racing/BestLapGhost.h"
#include "Racing/RaceProgress.h"
#include "Racing/RaceStartLights.h"
#include "UI/HudInstrumentDriveBadge.h"
#include "UI/NitroHudReadout.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool value, const char* what)
{
    if (!value)
    {
        std::cerr << "Cross-system live race safety FAIL: " << what << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double x, double y)
{
    return std::abs(x - y) < 1.0e-8;
}

latebrake::RivalPassSample ValidPassSample()
{
    using namespace latebrake;
    // Physical original four-gate geometry: 5,600cm eastward straight.
    // The rival is 550cm into the straight with a player 1,900cm ahead.
    constexpr double southY = SouthY;
    return {1.0 / 60.0, 0,
            {WestX + 550.0, southY},
            {WestX + 2450.0, southY},
            0.0, 2200.0, 0.0, 1800.0, 0.0, true, true};
}

latebrake::LapGhostPose GenuineLapPose(double t, int frame)
{
    // Native positions and headings are sampled AFTER RaceProgress::Advance,
    // not derived from a score or from the rival's speculative route.
    return {t, frame * 5.0, latebrake::SouthY, 180.0,
            std::remainder(static_cast<double>(frame) * 2.0, 360.0)};
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceStartLights start;
    RaceProgress official(4, 2);
    BestLapGhost ghost;
    RivalStraightPassAssist rival;
    ArcadeNitroBoost nitro;
    const PilotCommand route{0.72, 0.08, false, false};

    // Red lights cannot advance the official lap clock, create a ghost,
    // or grant any speculative AI passing authority.
    for (int frame = 0; frame < 6; ++frame)
    {
        const double live = start.Advance(0.5, true);
        official.Advance(live);
        Check(live == 0.0 && Near(official.GetElapsedSeconds(), 0.0) &&
              !ghost.HasBest() && !rival.IsActive(),
              "pre-green game never fabricates lap time or vehicle history");
    }
    Check(start.IsGreen() && official.GetNextCheckpoint() == 0,
          "three eligible pre-race seconds transition cleanly to GO");

    const auto nitroBefore = ReadNitroHud(nitro, 0.0, true);
    Check(nitroBefore.Valid &&
          nitroBefore.Phase == NitroHudPhase::BuildSpeed,
          "actual unspent booster is not optimistically labeled active");

    // Representative 20Hz verified post-GO race frames. No physics is
    // invented: the portable policy receives real-format authoritative
    // samples, and the score progression is driven ONLY by the official
    // clock and explicit ordered checkpoint transitions.
    for (int frame = 1; frame <= 100; ++frame)
    {
        const double live = start.Advance(0.05, true);
        Check(Near(live, 0.05), "green clock credits exactly real frame time");
        official.Advance(live);
        const double actualLapSeconds = official.GetCurrentLapSeconds();
        Check(ghost.Record(GenuineLapPose(actualLapSeconds, frame), true),
              "consecutive valid physical poses remain ghost-eligible");

        // A physically observed rival pass is non-authoritative for
        // scoring. Real traffic may brake/abort it independently of the
        // user's lap clock and the visual best-lap recorder.
        const double clockBeforeAI = official.GetElapsedSeconds();
        const double ghostTimeBeforeAI = actualLapSeconds;
        if (frame == 10)
        {
            const auto passing = rival.Apply(route, ValidPassSample());
            Check(passing.Passing &&
                  passing.Command.Throttle == route.Throttle,
                  "legitimate initial rival pass is still possible");
        }
        else if (frame == 11)
        {
            const RivalPassOutput active =
                rival.Apply(route, ValidPassSample());
            const PilotCommand trafficBrake{0.0, 0.0, true, false};
            const auto resolved = ResolveRivalPassTraffic(
                route, active, trafficBrake);
            Check(active.Passing && resolved.AbortPassing &&
                  resolved.Command.Brake &&
                  resolved.Command.Steering == route.Steering,
                  "actual pass/traffic composition returns to safe route");
            rival.AbortUntilGateChange();
            Check(rival.IsRetryBlockedForGate(0) && !rival.IsActive(),
                  "emergency remembers unsafe original checkpoint");
        }
        else if (frame > 11)
        {
            const auto retry = rival.Apply(route, ValidPassSample());
            Check(!retry.Passing && rival.IsRetryBlockedForGate(0) &&
                  retry.Command.Throttle == route.Throttle,
                  "continued live driving never restarts retired same-gate pass");
        }
        Check(Near(official.GetElapsedSeconds(), clockBeforeAI) &&
              Near(official.GetCurrentLapSeconds(), ghostTimeBeforeAI),
              "AI decision and traffic brake cannot mutate player race clock");

        if (frame % 25 == 0)
        {
            const int checkpoint = frame / 25 - 1;
            Check(official.GetNextCheckpoint() == checkpoint &&
                  official.PassCheckpoint(checkpoint),
                  "only explicit ordered gate receives official progression");
        }
    }
    Check(official.GetCompletedLaps() == 1 &&
          official.GetNextCheckpoint() == 0 &&
          official.GetActiveTimedLapCount() == 1 &&
          Near(official.GetLastLapSeconds(), 5.0) &&
          !official.IsFinished(),
          "genuine lap completes while rival passing is disabled");
    Check(ghost.FinishLap(official.GetLastLapSeconds(),
                         official.GetActiveTimedLapCount() ==
                             official.GetCompletedLaps(),
                         official.GetLastLapPenaltySeconds() == 0.0) &&
          ghost.HasBest() &&
          Near(ghost.GetBestSeconds(), 5.0),
          "same authoritative timed lap qualifies the in-run ghost");

    LapGhostPose visible{};
    Check(ghost.Evaluate(2.5, visible) &&
          std::isfinite(visible.YawDegrees) &&
          Near(visible.Seconds, 2.5),
          "ghost time lookup renders valid finite observation only");
    const double completedClock = official.GetElapsedSeconds();
    const int completedLaps = official.GetCompletedLaps();
    const auto readout = ReadNitroHud(nitro, 0.0, true);
    Check(readout.Valid && readout.Phase == nitroBefore.Phase &&
          Near(official.GetElapsedSeconds(), completedClock),
          "read-only nitro and ghost presentation never spend lap time");

    // A corrupted new lap cannot replace the existing best, nor make
    // an ineligible AI pass mutate official progress.
    Check(ghost.Record({0.05, 0.0, SouthY, 180.0, 0.0}, true),
          "new lap records first valid physics observation");
    Check(!ghost.Record({0.06, 0.0, SouthY, 180.0, 179.0}, true) &&
          ghost.HasBest() && Near(ghost.GetBestSeconds(), 5.0),
          "implausible heading snap poisons only the candidate ghost");
    Check(!ghost.FinishLap(2.0, true, true) &&
          official.GetCompletedLaps() == completedLaps &&
          Near(official.GetElapsedSeconds(), completedClock),
          "invalid replay candidate cannot award a phantom second lap");

    // Observing a new verified next gate is the only non-session reset
    // that clears the aborted pass. It is still a short straight, so the
    // route planner refuses to create a risky fake passing manoeuvre.
    auto next = ValidPassSample();
    next.NextGate = 1;
    const auto newGate = rival.Apply(route, next);
    Check(!newGate.Passing && !rival.IsRetryBlockedForGate(0),
          "passing cooldown re-arms only on route progression");
    rival.Reset();
    ghost.ResetRun();
    official.Reset();
    start.Reset();
    Check(!rival.IsActive() && !ghost.HasBest() &&
          official.GetCompletedLaps() == 0 &&
          Near(official.GetElapsedSeconds(), 0.0) &&
          !start.IsGreen(),
          "genuine restart clears every separate session-owned state");

    // Presentation-only badge observes bounded text geometry, never
    // changes player score, AI throttle or the ghost's recorded poses.
    const InstrumentRect desktopGauge{120.0, 200.0, 278.0, 214.0};
    const auto gear = CalculateHudInstrumentDriveBadge(
        desktopGauge, 1.0, 88.0, 24.0, 14.0, 110.0, 14.0);
    Check(gear.Visible && gear.Width > 0.0 &&
          official.GetCompletedLaps() == 0,
          "HUD badge has no authority over reset race progression");

    std::cout << "Live race clock, AI passing, visual ghost and HUD"
              << " authority isolation passed.\n";
    return EXIT_SUCCESS;
}
