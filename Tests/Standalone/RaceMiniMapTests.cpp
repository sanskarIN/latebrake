#include "Track/RaceMiniMap.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool pass, const char* message)
{
    if (!pass)
    {
        std::cerr << "RaceMiniMap FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double lhs, double rhs)
{
    return std::abs(lhs - rhs) <= 1e-9;
}
}

int main()
{
    using namespace latebrake;
    const CircuitPoint p = PlayerStart.Position;
    const CircuitPoint rival{-2250.0, SouthY};
    const auto normal = BuildRaceMiniMapSnapshot(
        p, 0.0, true, 0, true, rival, true);
    Check(normal.Player.Position.Valid && normal.Rival.Valid &&
          normal.NextGate.Valid && normal.DirectRivalSeparationValid,
          "healthy player/rival/gate have three distinct real markers");
    Check(Near(normal.Player.DirectionX, 1.0) &&
          Near(normal.Player.DirectionY, 0.0) &&
          Near(normal.DirectRivalSeparationMeters, 3.0),
          "eastbound heading and direct 3m separation are grounded");
    Check(normal.Player.Position.U < normal.Rival.U &&
          Near(normal.Player.Position.V, normal.Rival.V),
          "rival to east appears right of player on north-up map");
    Check(Near(normal.NextGate.U, NormalizeCircuitMap(CircuitGate(0)).U),
          "next gate comes from actual circuit geometry");

    for (const auto [angle, x, y] : {
        std::array<double, 3>{90.0, 0.0, -1.0},
        {180.0, -1.0, 0.0},
        {-90.0, 0.0, 1.0},
        {360.0, 1.0, 0.0},
        {450.0, 0.0, -1.0}
    })
    {
        const auto heading = BuildRaceMiniMapSnapshot(
            p, angle, true, 2, false, {}, false);
        Check(heading.Player.Position.Valid &&
              Near(heading.Player.DirectionX, x) &&
              Near(heading.Player.DirectionY, y),
              "cardinal, negative and wrapped yaw point correctly on screen");
    }

    const auto noRival = BuildRaceMiniMapSnapshot(
        p, 0.0, true, 0, false, rival, true);
    const auto invalidRival = BuildRaceMiniMapSnapshot(
        p, 0.0, true, 0, true, rival, false);
    Check(!noRival.Rival.Valid && !noRival.DirectRivalSeparationValid &&
          !invalidRival.Rival.Valid && !invalidRival.DirectRivalSeparationValid,
          "time attack, absent or unhealthy rival never creates a ghost");
    const auto blocked = BuildRaceMiniMapSnapshot(
        p, 0.0, false, 0, true, rival, true);
    Check(!blocked.Player.Position.Valid && !blocked.Rival.Valid &&
          !blocked.DirectRivalSeparationValid,
          "unverified player cannot leak opponent distance from invalid race state");
    Check(blocked.NextGate.Valid,
          "known official gate position is independent of vehicle availability");

    for (double invalid : {
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::max()
    })
    {
        const auto badPlayer = BuildRaceMiniMapSnapshot(
            {invalid, 0.0}, 0.0, true, 0, true, rival, true);
        const auto badYaw = BuildRaceMiniMapSnapshot(
            p, invalid, true, 0, true, rival, true);
        const auto badRival = BuildRaceMiniMapSnapshot(
            p, 0.0, true, 0, true, {0.0, invalid}, true);
        Check(!badPlayer.Player.Position.Valid && !badPlayer.Rival.Valid &&
              !badYaw.Player.Position.Valid && !badYaw.Rival.Valid &&
              !badRival.Rival.Valid && !badRival.DirectRivalSeparationValid,
              "NaN/Inf/extreme player, heading and rival suppress fake radar");
    }
    Check(!BuildRaceMiniMapSnapshot(
          {5000.0, 0.0}, 0.0, true, 0, true, rival, true)
          .Player.Position.Valid,
          "off-display actor must not be clamped to fabricated edge position");
    Check(!BuildRaceMiniMapSnapshot(
          p, 0.0, true, -1, true, rival, true).NextGate.Valid &&
          !BuildRaceMiniMapSnapshot(
          p, 0.0, true, 4, true, rival, true).NextGate.Valid,
          "invalid scoring gate does not get a fake center marker");
    const auto overlapping = BuildRaceMiniMapSnapshot(
        p, -45.0, true, 2, true, p, true);
    Check(overlapping.DirectRivalSeparationValid &&
          overlapping.DirectRivalSeparationMeters == 0.0,
          "overlapping positions are a legitimate zero-distance sample");
    std::cout << "Race radar minimap passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
