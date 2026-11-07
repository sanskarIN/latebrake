#include "Track/RoundedCircuitRoadPlan.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <locale>

// Outputs JSON to stdout; a genuine Unreal Editor consumes it in a dedicated
// physics-lab map. No .umap, .uasset, or fake installed build is generated.
int main()
{
    using namespace latebrake;
    std::cout.imbue(std::locale::classic());
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "{\"schema_version\":1,\"units\":\"cm\",\"road_top_cm\":-1.0,"
                 "\"piece_count\":" << RoundedPhysicalRoadCount << ",\"pieces\":[";
    for (int i = 0; i < RoundedPhysicalRoadCount; ++i)
    {
        const RoundedPhysicalRoadPiece road = PlannedRoundedPhysicalRoadPiece(i);
        if (!IsValidRoundedPhysicalRoadPiece(road))
        {
            std::cerr << "Invalid authored road transform index=" << i << '\n';
            return 2;
        }
        if (i) std::cout << ',';
        std::cout << "{\"index\":" << i << ",\"position\":["
                  << road.Center.X << ',' << road.Center.Y << ',' << road.HeightCm
                  << "],\"rotation_yaw\":" << road.YawDegrees
                  << ",\"scale\":[" << road.ScaleX << ',' << road.ScaleY
                  << ',' << road.ScaleZ << "]}";
    }
    std::cout << "]}\n";
    if (!std::cout.good()) return 3;
    return 0;
}
