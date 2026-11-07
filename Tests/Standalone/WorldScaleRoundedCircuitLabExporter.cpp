#include "Track/WorldScaleRoundedCircuit.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <locale>

// JSON for the dedicated 1.70-km Unreal collision inspection map.
// This exports actual planned 9-m-wide physical road slabs, not the
// cosmetic OBJ and not a usable race map or a cooked Unreal asset.
int main()
{
    using namespace latebrake;
    static_assert(WorldScaleRoadPieceCount == 68);
    static_assert(WorldScaleRoadHalfWidthCm == 450.0);
    // Cross-platform double arithmetic is not bit-exact: use the same
    // centimetre tolerance as IsValidRoundedPhysicalRoadPiece. Check ALL
    // pieces before printing the first byte to avoid partial JSON on failure.
    for (int i = 0; i < WorldScaleRoadPieceCount; ++i)
    {
        const RoundedPhysicalRoadPiece road = PlannedWorldScaleRoadPiece(i);
        if (!IsValidRoundedPhysicalRoadPiece(road) ||
            std::abs(road.ScaleY - 9.0) > 1e-9 ||
            std::abs(road.HeightCm + 50.0 * road.ScaleZ - RoundedRoadTopCm) > 1e-9)
        {
            std::cerr << "Invalid full-scale 9-m physical road index=" << i << '\n';
            return 2;
        }
    }
    std::cout.imbue(std::locale::classic());
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "{\"schema_version\":1,\"units\":\"cm\",\"road_top_cm\":-1.0,"
                 "\"piece_count\":" << WorldScaleRoadPieceCount << ",\"pieces\":[";
    for (int i = 0; i < WorldScaleRoadPieceCount; ++i)
    {
        const RoundedPhysicalRoadPiece road = PlannedWorldScaleRoadPiece(i);
        if (i != 0) std::cout << ',';
        std::cout << "{\"index\":" << i << ",\"position\":["
                  << road.Center.X << ',' << road.Center.Y << ',' << road.HeightCm
                  << "],\"rotation_yaw\":" << road.YawDegrees
                  << ",\"scale\":[" << road.ScaleX << ',' << road.ScaleY
                  << ',' << road.ScaleZ << "]}";
    }
    std::cout << "]}\n";
    return std::cout.good() ? 0 : 3;
}
