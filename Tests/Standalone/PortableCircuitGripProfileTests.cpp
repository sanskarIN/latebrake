#include "Track/PortableCircuitGripProfile.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool value, const char* label)
{
    if (!value)
    {
        std::cerr << "PortableCircuitGrip FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}
}

int main()
{
    using namespace latebrake;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    PortableCircuitGripProfile atlas;
    Check(!atlas.Reset(nan) && !atlas.Reset(0.0) &&
          !atlas.Reset(1e9), "malformed lap length");
    Check(atlas.Reset(1000.0), "valid length");
    Check(!atlas.Complete() && !atlas.Evaluate(1.0, 0.0).Valid,
          "no nonexistent surface profile");
    Check(!atlas.Append({1.0, 100.0, 1.0, 0.8, 0.0, 0.0}) &&
          !atlas.Append({0.0, 0.0, 1.0, 0.8, 0.0, 0.0}) &&
          !atlas.Append({0.0, 500.0, nan, 0.8, 0.0, 0.0}) &&
          !atlas.Append({0.0, 500.0, 1.0, 0.8, 1.0, 1.0}),
          "no gaps, zero intervals or unbounded coefficients");
    Check(atlas.Append({0.0, 400.0, 1.0, 0.75, 0.4, 0.0}),
          "first patch");
    Check(!atlas.Append({400.01, 600.0, 1.0, 0.6, 0.0, 0.0}) &&
          !atlas.Append({300.0, 600.0, 1.0, 0.6, 0.0, 0.0}),
          "gaps and overlaps are rejected");
    Check(atlas.Append({400.0, 750.0, 0.85, 0.4, 0.0, 0.5}),
          "second patch");
    Check(!atlas.Complete(), "partial atlas unavailable");
    Check(atlas.Append({750.0, 1000.0, 1.3, 0.6, 0.0, 0.0}) &&
          atlas.Complete() && atlas.Size() == 3,
          "fully authored course");
    const auto dry = atlas.Evaluate(100.0, 0.0);
    Check(dry.Valid && dry.SegmentIndex == 0 &&
          Near(dry.GripMultiplier, 1.032),
          "dry rubber adds small grip increment");
    const auto wet = atlas.Evaluate(100.0, 1.0);
    Check(wet.Valid && Near(wet.GripMultiplier, 0.702),
          "wet rubber reduces multiplier");
    const auto debris = atlas.Evaluate(500.0, 0.0);
    Check(debris.Valid && debris.SegmentIndex == 1 &&
          Near(debris.GripMultiplier, 0.76),
          "debris subtracts grip from second surface");
    Check(atlas.Evaluate(400.0, 0.0).SegmentIndex == 1 &&
          atlas.Evaluate(750.0, 0.0).SegmentIndex == 2 &&
          Near(atlas.Evaluate(999.9, 0.0).GripMultiplier, 1.3),
          "adjacent boundaries use right hand segment");
    for (double distance = 0.0; distance < 1000.0; distance += 0.125)
    {
        const auto grip = atlas.Evaluate(distance, 0.5);
        Check(grip.Valid && std::isfinite(grip.GripMultiplier) &&
              grip.GripMultiplier >= 0.10 && grip.GripMultiplier <= 1.6,
              "8000 deterministic bounded lookups");
    }
    Check(!atlas.Evaluate(1000.0, 0.0).Valid &&
          !atlas.Evaluate(-1.0, 0.0).Valid &&
          !atlas.Evaluate(nan, 0.0).Valid &&
          !atlas.Evaluate(5.0, nan).Valid &&
          !atlas.Evaluate(5.0, 1.01).Valid,
          "no extrapolation or bogus weather");
    Check(atlas.Reset(64.0), "capacity test");
    for (std::size_t index = 0; index < PortableCircuitGripProfile::Capacity; ++index)
    {
        const double from = static_cast<double>(index);
        Check(atlas.Append({from, from + 1.0, 1.0, 0.7, 0.0, 0.0}),
              "64 contiguous physical sections");
    }
    Check(atlas.Complete() && atlas.Size() ==
          PortableCircuitGripProfile::Capacity &&
          !atlas.Append({64.0, 65.0, 1.0, 0.7, 0.0, 0.0}),
          "fixed capacity rejects overflow");
    std::cout << "Portable circuit grip tests passed.\n";
}
