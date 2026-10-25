#include "Track/RivalProximityIndicator.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool pass, const char* what)
{
    if (!pass)
    {
        std::cerr << "RivalProximityIndicator FAIL: " << what << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    const CircuitPoint player = PlayerStart.Position;
    const auto radarAt = [&](double meters, bool rivalEnabled = true,
                             bool rivalReady = true, bool playerReady = true)
    {
        return BuildRaceMiniMapSnapshot(
            player, 0.0, playerReady, 0, rivalEnabled,
            {player.X + meters * 100.0, player.Y}, rivalReady);
    };
    RivalProximityIndicator warning;

    const auto near = radarAt(10.0);
    Check(near.DirectRivalSeparationValid &&
          std::abs(near.DirectRivalSeparationMeters - 10.0) < 1e-9,
          "production radar confirms real planar 10m separation");
    Check(!warning.Update(near, 0.20, true), "no one-frame alarm");
    Check(!warning.Update(near, 0.20, true), "no two-frame alarm");
    Check(warning.Update(near, 0.20, true) && warning.IsActive(),
          "only sustained near rival creates alert");
    Check(warning.Update(radarAt(19.0), 0.1, true) &&
          warning.Update(radarAt(20.0), 0.1, true),
          "active state persists through 20m hysteresis");
    Check(!warning.Update(radarAt(21.0), 0.1, true) &&
          !warning.IsActive() && warning.GetNearSeconds() == 0.0,
          "beyond 20m clears immediately");
    Check(!warning.Update(radarAt(17.0), 0.2, true) &&
          warning.GetNearSeconds() == 0.0,
          "17m cannot newly enter a warning");
    Check(!warning.Update(near, 0.2, true) &&
          !warning.Update(radarAt(16.0), 0.1, true) &&
          !warning.Update(near, 0.2, true),
          "leaving 15m entry corridor resets debounce");
    Check(!warning.Update(near, 0.2, true) &&
          warning.Update(near, 0.2, true),
          "new continuous samples can trigger after interruption");
    Check(!warning.Update(near, 0.1, false) && !warning.IsActive(),
          "pause, results and loss of race eligibility clear alert");

    for (const auto invalid : {
        radarAt(10.0, false, true, true),
        radarAt(10.0, true, false, true),
        radarAt(10.0, true, true, false),
        BuildRaceMiniMapSnapshot(player,
            std::numeric_limits<double>::quiet_NaN(), true, 0,
            true, player, true),
        radarAt(1000.0)
    })
    {
        Check(!warning.Update(near, 0.2, true) &&
              !warning.Update(invalid, 0.2, true) &&
              !warning.IsActive() && warning.GetNearSeconds() == 0.0,
              "no player, AI, time attack, corrupt yaw or off-map ghost");
    }
    for (double badDt : {0.0, -0.01, 0.30,
                         std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()})
    {
        warning.Update(near, 0.2, true);
        Check(!warning.Update(near, badDt, true) &&
              warning.GetNearSeconds() == 0.0,
              "invalid, paused or hitch samples cannot accumulate");
    }

    auto fabricated = near;
    for (double badDistance : {-0.1,
                               std::numeric_limits<double>::quiet_NaN(),
                               std::numeric_limits<double>::infinity()})
    {
        fabricated.DirectRivalSeparationMeters = badDistance;
        Check(!warning.Update(fabricated, 0.2, true),
              "malformed distance cannot trigger HUD advice");
    }

    Check(!warning.Update(near, 0.2, true), "re-arm first sample");
    warning.Reset(); // Simulate player teleport, pawn replacement or new session.
    Check(!warning.Update(near, 0.2, true) &&
          !warning.IsActive(), "explicit reset cancels prior accumulation");
    for (int n = 0; n < 10000; ++n)
    {
        warning.Update(near, 0.1, true);
    }
    Check(warning.IsActive() && warning.GetNearSeconds() == 0.6,
          "long sessions remain finite, bounded and activated");
    const auto coincident = radarAt(0.0);
    warning.Reset();
    Check(!warning.Update(coincident, 0.2, true) &&
          !warning.Update(coincident, 0.2, true) &&
          warning.Update(coincident, 0.2, true),
          "true zero-distance overlap is real proximity, not invalid data");
    std::cout << "Rival proximity advisory tests passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
