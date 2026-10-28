#include "UI/GhostSeparation.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool good, const char* why)
{
    if (!good)
    {
        std::cerr << "GhostSeparation FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    double meters = -1.0;
    GhostSeparationInput input{
        true, true, 0.0, 0.0, 0.0, 300.0, 400.0, 0.0
    };
    Check(TryComputeGhostSeparationMeters(input, meters) &&
          std::abs(meters - 5.0) < 1e-12,
          "3-4-5 planar world-cm triangle converts to 5 m");
    input.GhostX = 0.0;
    input.GhostY = 0.0;
    input.GhostZ = 1200.0;
    Check(TryComputeGhostSeparationMeters(input, meters) &&
          std::abs(meters - 12.0) < 1e-12,
          "genuine height difference counts, not the beacon's drawn offset");
    input.GhostZ = 0.0;
    Check(TryComputeGhostSeparationMeters(input, meters) && meters == 0.0,
          "exact overlap can be shown without a fake negative lead");
    input.GhostX = 25000.0;
    Check(TryComputeGhostSeparationMeters(input, meters) && meters == 250.0,
          "250 m upper readable display boundary accepted");
    input.GhostX = 25001.0;
    Check(!TryComputeGhostSeparationMeters(input, meters) && meters == 0.0,
          "out-of-range ghost does not clutter the HUD");
    input.GhostX = 300.0;
    input.ReplayAuthorized = false;
    Check(!TryComputeGhostSeparationMeters(input, meters) && meters == 0.0,
          "pause, focus loss, countdown or finish admission denied");
    input.ReplayAuthorized = true;
    input.BeaconVisible = false;
    Check(!TryComputeGhostSeparationMeters(input, meters),
          "hidden, missing or suppressed beacon cannot report distance");
    input.BeaconVisible = true;
    input.PlayerY = std::numeric_limits<double>::quiet_NaN();
    Check(!TryComputeGhostSeparationMeters(input, meters) && meters == 0.0,
          "nonfinite chassis position rejected");
    input.PlayerY = 0.0;
    input.GhostZ = std::numeric_limits<double>::infinity();
    Check(!TryComputeGhostSeparationMeters(input, meters),
          "nonfinite reference pose rejected");
    input.GhostZ = 0.0;
    input.PlayerX = 500001.0;
    Check(!TryComputeGhostSeparationMeters(input, meters),
          "unrealistically distant chassis location rejected");
    input.PlayerX = 500000.0;
    input.GhostX = 500000.0;
    Check(TryComputeGhostSeparationMeters(input, meters) && meters == 0.0,
          "finite at-world-bound coincident poses accepted");
    std::cout << "Ghost separation cosmetic source policies passed.\n";
    return EXIT_SUCCESS;
}
