#include "Track/BrakingReferenceBoards.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "BrakingReferenceBoards FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b) { return std::abs(a - b) < 1e-9; }
}

int main()
{
    using namespace latebrake;
    static_assert(BrakingBoardCorners == 4);
    static_assert(BrakingBoardBarsPerCorner == 6);
    static_assert(BrakingBoardMarkCount == 24);

    Require(!PlannedBrakingReferenceBoard(-1).Valid &&
            !PlannedBrakingReferenceBoard(24).Valid &&
            !PlannedBrakingReferenceBoard(1000).Valid,
            "invalid index never creates geometry");
    std::array<int, 4> corners{};
    std::array<int, 3> zones{};
    for (int index = 0; index < BrakingBoardMarkCount; ++index)
    {
        const auto board = PlannedBrakingReferenceBoard(index);
        Require(board.Valid && board.Corner == index / 6,
                "each board is assigned to its original directional gate");
        Require(board.Zone >= 0 && board.Zone < 3 &&
                board.BarNumber >= 0 && board.BarNumber < board.BarsInZone &&
                board.BarsInZone == 3 - board.Zone,
                "3,2,1 approach marker groups");
        Require(std::isfinite(board.Center.X) && std::isfinite(board.Center.Y) &&
                std::isfinite(board.YawDegrees) &&
                std::isfinite(board.DistanceToGateCm) &&
                board.ScaleX > 0 && board.ScaleY > 0 && board.ScaleZ > 0,
                "finite positive geometry dimensions");
        Require(IsOnPavedSurface(board.Center),
                "all visual boards are within the original paved lanes");
        Require(board.DistanceToGateCm >= 400.0 &&
                board.DistanceToGateCm <= 1276.0,
                "approach markers do not overlap any gate plane");
        Require(Near(board.HeightCm, 104.0) &&
                Near(board.ScaleX, 0.15) && Near(board.ScaleY, 0.75) &&
                Near(board.ScaleZ, 1.90),
                "original plane and visible geometry remain fixed");
        const auto gate = CircuitGate(board.Corner);
        const double dx = board.Center.X - gate.X;
        const double dy = board.Center.Y - gate.Y;
        if (board.Corner == 0)
        {
            Require(dx < 0.0 && Near(dy, -580.0) &&
                    Near(board.YawDegrees, 0.0), "eastbound brake boards");
        }
        if (board.Corner == 1)
        {
            Require(Near(dx, 580.0) && dy < 0.0 &&
                    Near(board.YawDegrees, 90.0), "northbound brake boards");
        }
        if (board.Corner == 2)
        {
            Require(dx > 0.0 && Near(dy, 580.0) &&
                    Near(board.YawDegrees, 0.0), "westbound brake boards");
        }
        if (board.Corner == 3)
        {
            Require(Near(dx, -580.0) && dy > 0.0 &&
                    Near(board.YawDegrees, 90.0), "southbound brake boards");
        }
        ++corners[board.Corner];
        ++zones[board.Zone];
        // Keep board faces and ends inside the 800cm road half-width,
        // including the entire cube width, not just its center.
        Require(BrakingBoardRightOffsetCm +
                100.0 * board.ScaleY * 0.5 < RoadHalfWidth,
                "cosmetic marker width has road-edge clearance");
    }
    for (int n : corners) Require(n == 6, "six board bars per bend");
    Require(zones[0] == 12 && zones[1] == 8 && zones[2] == 4,
            "twelve far, eight middle, four near stripe bars");

    for (int corner = 0; corner < 4; ++corner)
    {
        const auto far = PlannedBrakingReferenceBoard(corner * 6);
        const auto mid = PlannedBrakingReferenceBoard(corner * 6 + 3);
        const auto near = PlannedBrakingReferenceBoard(corner * 6 + 5);
        Require(Near(far.DistanceToGateCm, 1200.0) &&
                Near(mid.DistanceToGateCm, 800.0) &&
                Near(near.DistanceToGateCm, 400.0),
                "all corners retain 12m/8m/4m braking reference distances");
        Require(far.DistanceToGateCm > mid.DistanceToGateCm &&
                mid.DistanceToGateCm > near.DistanceToGateCm,
                "board count decreases on approach to each corner");
    }
    std::cout << "24 original-circuit braking reference bars validated.\n";
    return EXIT_SUCCESS;
}
