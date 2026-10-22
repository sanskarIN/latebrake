#include "Track/DecorationBatchPlan.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Decoration batching FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Close(double a, double b)
{
    return std::abs(a - b) < 1.0e-9;
}
} // namespace

int main()
{
    using namespace latebrake;

    static_assert(DecorationBatchCount == 7);
    static_assert(DecorationRoadPieces == 84);
    static_assert(DecorationStandPieces == 8);
    static_assert(DecorationPieceCount == 93);

    Check(!PlannedDecoration(-1).Valid &&
          !PlannedDecoration(DecorationPieceCount).Valid,
          "out-of-range instances do not spawn");
    Check(DecorationBatchIndex(static_cast<DecorationBatch>(-1)) == -1 &&
          PlannedBatchSize(static_cast<DecorationBatch>(300)) == 0,
          "bad batch ID cannot index Unreal component array");

    std::array<int, DecorationBatchCount> actual{};
    for (int i = 0; i < DecorationPieceCount; ++i)
    {
        const DecorationPlacement item = PlannedDecoration(i);
        const int batch = DecorationBatchIndex(item.Batch);
        Check(item.Valid && batch >= 0, "every planned entry is valid");
        Check(std::isfinite(item.Center.X) && std::isfinite(item.Center.Y) &&
              std::isfinite(item.Height) &&
              std::isfinite(item.ScaleX) && std::isfinite(item.ScaleY) &&
              std::isfinite(item.ScaleZ), "all instance coordinates and scales finite");
        Check(item.ScaleX > 0.0 && item.ScaleY > 0.0 && item.ScaleZ > 0.0,
              "all mesh instance dimensions positive");
        ++actual[static_cast<std::size_t>(batch)];

        if (i < DecorationRoadPieces)
        {
            Check(IsOnPavedSurface(item.Center), "road stripe/curb stays on pavement");
            const int straight = i / (MarkersPerStraight * 3);
            const int segment = (i / 3) % MarkersPerStraight;
            if (i % 3 == 0)
            {
                const auto old = LaneStripe(straight, segment);
                Check(item.Batch == DecorationBatch::LaneStripe &&
                      Close(old.Center.X, item.Center.X) &&
                      Close(old.Center.Y, item.Center.Y) &&
                      Close(old.ScaleX, item.ScaleX) &&
                      Close(old.ScaleY, item.ScaleY) &&
                      Close(item.Height, 4.0) && Close(item.ScaleZ, 0.025),
                      "batched center stripe matches prior geometry");
            }
            else
            {
                const bool outer = i % 3 == 2;
                const auto old = TrackCurb(straight, segment, outer);
                const bool red = (segment + (outer ? 1 : 0)) % 2 == 0;
                Check(item.Batch == (red ? DecorationBatch::CurbRed
                                        : DecorationBatch::CurbWhite) &&
                      Close(item.Center.X, old.Center.X) &&
                      Close(item.Center.Y, old.Center.Y) &&
                      Close(item.ScaleX, old.ScaleX) &&
                      Close(item.ScaleY, old.ScaleY) &&
                      Close(item.Height, 6.0) && Close(item.ScaleZ, 0.08),
                      "batched red/white curb matches prior geometry and palette");
            }
        }
        else if (i < DecorationRoadPieces + DecorationStandPieces)
        {
            Check(!IsOnPavedSurface(item.Center),
                  "scenery stands stay off the drivable surface");
        }
    }

    Check(actual[0] == 28 && actual[1] == 28 && actual[2] == 28,
          "all stripes and alternating red/white curbs retained");
    Check(actual[3] == 2 && actual[4] == 4 && actual[5] == 2 &&
          actual[6] == 1, "grandstand bases/tiers and one banner retained");
    for (int b = 0; b < DecorationBatchCount; ++b)
    {
        Check(actual[static_cast<std::size_t>(b)] ==
              PlannedBatchSize(static_cast<DecorationBatch>(b)),
              "static batch group count matches actual generated instances");
    }

    const auto banner = PlannedDecoration(DecorationPieceCount - 1);
    Check(banner.Batch == DecorationBatch::StartFinishBanner &&
          Close(banner.Center.X, -2150.0) && Close(banner.Center.Y, SouthY) &&
          Close(banner.Height, 390.0) && Close(banner.ScaleZ, 1.5),
          "single original start/finish banner remains unchanged");

    // A single scenery actor with seven components can represent all ninety-
    // three cosmetic objects, but runtime draw call performance still needs
    // measurements; this test validates plans, not Unreal renderer behavior.
    std::cout << "Decorations batched: 93 placements in 7 groups.\n";
    return EXIT_SUCCESS;
}
