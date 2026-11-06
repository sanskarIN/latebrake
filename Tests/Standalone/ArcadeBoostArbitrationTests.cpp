#include "Vehicles/ArcadeBoostArbitration.h"
#include "Vehicles/TractionLimitedPropulsion.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "ArcadeBoostArbitration FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
void Near(double actual, double expected)
{
    Check(std::abs(actual - expected) < 1e-10, "force allocation mismatch");
}
}

int main()
{
    using latebrake::AllocateArcadeBoost;
    const auto none = AllocateArcadeBoost(1.0, 0.0, false);
    Near(none.TotalExtra(), 0.0);
    const auto draft = AllocateArcadeBoost(1.0, 0.06, false);
    Near(draft.NitroExtra, 0.0);
    Near(draft.DraftExtra, 0.06);
    Near(draft.TotalExtra(), 0.06);
    const auto nitro = AllocateArcadeBoost(1.38, 0.0, true);
    Near(nitro.NitroExtra, 0.38);
    Near(nitro.TotalExtra(), 0.38);
    const auto both = AllocateArcadeBoost(1.38, 0.06, true);
    Near(both.NitroExtra, 0.38);
    Near(both.DraftExtra, 0.02);
    Near(both.TotalExtra(), 0.4);
    Check(both.TotalExtra() <= 0.40000000001,
          "boost cannot stack beyond 40 percent");
    const auto partial = AllocateArcadeBoost(1.2, 0.06, true);
    Near(partial.TotalExtra(), 0.26);

    // A visually awarded draft bonus is not necessarily motor force.
    // The physical Unreal pawn now reports this only AFTER the real
    // shared motor-path modifiers and final traction scale are applied.
    using latebrake::CalculateDeliveredDraftFraction;
    Near(CalculateDeliveredDraftFraction(draft.DraftExtra, 1.0), 0.06);
    Near(CalculateDeliveredDraftFraction(draft.DraftExtra, 0.5), 0.03);
    Near(CalculateDeliveredDraftFraction(draft.DraftExtra, 0.0), 0.0);
    Near(CalculateDeliveredDraftFraction(both.DraftExtra, 0.5), 0.01);
    Near(CalculateDeliveredDraftFraction(0.0, 1.0), 0.0);
    Near(CalculateDeliveredDraftFraction(0.06, 1.0), 0.06);
    for (int fps : {30, 60, 120})
    {
        for (double axleLoad : {0.0, 0.05, 0.25, 0.65, 1.0})
        {
            for (double lateralUsed : {0.0, 500.0, 1100.0, 1540.0})
            {
                constexpr double mass = 1100.0;
                constexpr double baseForce = 1500000.0;
                const auto traction = latebrake::CalculateTractionLimitedPropulsion({
                    1.0 / fps, mass,
                    baseForce * axleLoad * (1.0 + both.TotalExtra()),
                    mass * lateralUsed, 1.0, 1.0, axleLoad, true});
                Check(traction.Valid, "physics traction sample accepted");
                // Emulate the exact real path: rear gear scale already
                // multiplies the base, nitro and draft before final grip.
                const double delivered = CalculateDeliveredDraftFraction(
                    both.DraftExtra, axleLoad * traction.ForceScale);
                const double expected = both.DraftExtra * axleLoad *
                    traction.ForceScale;
                Check(std::isfinite(delivered) &&
                    std::abs(delivered - expected) < 1.0e-10 &&
                    delivered >= 0.0 &&
                    delivered <= both.DraftExtra + 1.0e-10,
                    "reported draft cannot exceed actually applied bonus");
                if (axleLoad == 0.0)
                {
                    Near(delivered, 0.0);
                }
            }
        }
    }
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double invalid : {nan, inf, -inf, -0.1, 0.061})
    {
        Near(CalculateDeliveredDraftFraction(invalid, 0.5), 0.0);
    }
    for (double invalid : {nan, inf, -inf, -0.1, 1.01})
    {
        Near(CalculateDeliveredDraftFraction(0.06, invalid), 0.0);
    }
    for (double f : {0.0, 0.001, 0.03, 0.06})
    {
        for (int i = 0; i <= 10000; ++i)
        {
            const double scale = static_cast<double>(i) / 10000.0;
            const double observed = CalculateDeliveredDraftFraction(f, scale);
            Check(std::isfinite(observed) && observed >= 0.0 &&
                  observed <= f + 1.0e-10,
                  "40k delivery samples obey physical motor force cap");
        }
    }
    for (double invalid : {nan, inf, -inf, 0.0, 0.99, 1.39})
    {
        const auto out = AllocateArcadeBoost(invalid, 0.06, true);
        Near(out.TotalExtra(), 0.0);
    }
    for (double invalid : {nan, inf, -inf, -0.1, 0.07})
    {
        const auto out = AllocateArcadeBoost(1.38, invalid, true);
        Near(out.TotalExtra(), 0.0);
    }
    Near(AllocateArcadeBoost(1.38, 0.06, false).TotalExtra(), 0.0);
    // Verify stable budget across many legitimate parameter pairs.
    for (int i = 0; i < 200000; ++i)
    {
        const double requested = static_cast<double>(i % 61) / 1000.0;
        const bool nitroActive = (i % 2) == 0;
        const auto out = AllocateArcadeBoost(
            nitroActive ? 1.38 : 1.0, requested, nitroActive);
        Check(std::isfinite(out.TotalExtra()) && out.TotalExtra() >= 0.0 &&
              out.TotalExtra() <= 0.40000000001 &&
              out.DraftExtra <= 0.06000000001 &&
              out.NitroExtra <= 0.38000000001,
              "stress combination never exceeds global budget");
    }
    std::cout << "ArcadeBoostArbitration passed\n";
}
