#include "Track/CircuitMap.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

static void Check(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << "Circuit map FAIL: " << message << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    const auto middle = NormalizeCircuitMap({0.0, 0.0});
    Check(middle.U == 0.5 && middle.V == 0.5, "origin mapped to middle");
    const auto left = NormalizeCircuitMap({-10000.0, 0.0});
    Check(left.U == 0.0, "outside left clamps");
    const auto right = NormalizeCircuitMap({10000.0, 0.0});
    Check(right.U == 1.0, "outside right clamps");
    Check(NormalizeCircuitMap({0.0, -10000.0}).V == 0.0, "south clamp");
    Check(NormalizeCircuitMap({0.0, 10000.0}).V == 1.0, "north clamp");
    const auto nan = NormalizeCircuitMap({std::numeric_limits<double>::quiet_NaN(), 0.0});
    Check(nan.U == 0.5 && nan.V == 0.5, "corrupt GPS neutral");
    const auto inf = NormalizeCircuitMap({0.0, std::numeric_limits<double>::infinity()});
    Check(inf.U == 0.5 && inf.V == 0.5, "infinite position neutral");
    for (int i = 0; i < CircuitGateCount; ++i)
    {
        const auto mapped = NormalizeCircuitMap(CircuitGate(i));
        Check(std::isfinite(mapped.U) && std::isfinite(mapped.V), "finite mapped gate");
        Check(mapped.U > 0.0 && mapped.U < 1.0, "gate horizontal inset");
        Check(mapped.V > 0.0 && mapped.V < 1.0, "gate vertical inset");
    }
    const auto p = NormalizeCircuitMap(PlayerStart.Position);
    Check(p.U > 0.0 && p.U < 0.5 && p.V < 0.5, "starter car southwest in map");
    std::cout << "Circuit minimap transformations passed.\n";
}
