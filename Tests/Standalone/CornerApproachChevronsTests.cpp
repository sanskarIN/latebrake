#include "Track/CornerApproachChevrons.h"

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
        std::cerr << "CornerApproachChevrons FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b) { return std::abs(a - b) < 1.0e-9; }
} // namespace

int main()
{
    using namespace latebrake;
    static_assert(ChevronCorners == 4);
    static_assert(ChevronRowsPerCorner == 3);
    static_assert(ChevronArmsPerRow == 2);
    static_assert(CornerChevronPieceCount == 24);
    static_assert(ChevronApproachesCm[0] > ChevronApproachesCm[1] &&
                  ChevronApproachesCm[1] > ChevronApproachesCm[2]);
    Require(!PlannedCornerApproachChevron(-1).Valid &&
            !PlannedCornerApproachChevron(24).Valid &&
            !PlannedCornerApproachChevron(10000).Valid,
            "out-of-range transforms fail closed");

    std::array<int, ChevronCorners> cornerCount{};
    std::array<int, ChevronRowsPerCorner> rowCount{};
    for (int index = 0; index < CornerChevronPieceCount; ++index)
    {
        const auto piece = PlannedCornerApproachChevron(index);
        Require(piece.Valid && piece.Corner == index / 6 &&
                piece.Row == (index / 2) % 3 && piece.Arm == index % 2,
                "stable mapping to the original four directional gates");
        Require(std::isfinite(piece.Center.X) &&
                std::isfinite(piece.Center.Y) &&
                std::isfinite(piece.HeightCm) &&
                std::isfinite(piece.YawDegrees), "finite transforms");
        Require(IsOnPavedSurface(piece.Center),
                "all diagonal markers are on original pavement");
        Require(Near(piece.ScaleX, 1.90) &&
                Near(piece.ScaleY, 0.19) &&
                Near(piece.ScaleZ, 0.03) &&
                Near(piece.HeightCm, 1.5),
                "fixed shallow stripe dimensions");
        Require(Near(piece.BeforeGateCm,
                     ChevronApproachesCm[static_cast<unsigned>(piece.Row)]),
                "each row has its specified gate approach distance");

        const double fx = piece.Corner == 0 ? 1.0 :
                          piece.Corner == 2 ? -1.0 : 0.0;
        const double fy = piece.Corner == 1 ? 1.0 :
                          piece.Corner == 3 ? -1.0 : 0.0;
        const double lx = -fy, ly = fx;
        const auto gate = CircuitGate(piece.Corner);
        const double dx = piece.Center.X - gate.X;
        const double dy = piece.Center.Y - gate.Y;
        const double approach = -(dx * fx + dy * fy);
        const double shoulder = dx * lx + dy * ly;
        Require(Near(approach, piece.BeforeGateCm + 80.0) &&
                approach >= 680.0 && approach <= 1680.0,
                "arrow strokes never lie on or beyond the scoring gate");
        Require(Near(shoulder, ChevronLeftOffsetCm +
                    (piece.Arm == 0 ? 51.0 : -51.0)),
                "both arms sit on the opposite side from braking boards");
        // The full stripe must remain inside the 800cm asphalt edge.
        // 190cm diagonal length with its 19cm width bounds offset safely.
        Require(shoulder + 95.0 + 9.5 < RoadHalfWidth &&
                shoulder - 95.0 - 9.5 > -RoadHalfWidth,
                "whole cube has enough paved lateral clearance");

        const double heading = piece.Corner == 0 ? 0.0 :
                               piece.Corner == 1 ? 90.0 :
                               piece.Corner == 2 ? 180.0 : -90.0;
        Require(Near(piece.YawDegrees,
                     heading + (piece.Arm == 0 ? -32.0 : 32.0)),
                "symmetric chevron arms point in the actual travel direction");
        ++cornerCount[static_cast<unsigned>(piece.Corner)];
        ++rowCount[static_cast<unsigned>(piece.Row)];
    }
    for (int n : cornerCount) Require(n == 6, "six strokes per turn");
    for (int n : rowCount) Require(n == 8, "eight strokes per approach row");

    for (int corner = 0; corner < ChevronCorners; ++corner)
    {
        for (int row = 0; row < ChevronRowsPerCorner; ++row)
        {
            const int base = corner * 6 + row * 2;
            const auto first = PlannedCornerApproachChevron(base);
            const auto second = PlannedCornerApproachChevron(base + 1);
            const double separation = std::hypot(
                first.Center.X - second.Center.X,
                first.Center.Y - second.Center.Y);
            Require(Near(separation, 102.0),
                    "paired strokes form one V instead of a straight stripe");
        }
    }
    std::cout << "24 low-profile turn chevrons checked against 4 track gates.\n";
    return EXIT_SUCCESS;
}
