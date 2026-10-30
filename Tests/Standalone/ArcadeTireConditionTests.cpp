#include "Vehicles/ArcadeTireCondition.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "ArcadeTireCondition FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using latebrake::ArcadeTireCondition;
    using latebrake::TireConditionSample;
    ArcadeTireCondition tire;
    Check(tire.GripMultiplier() == 1.0 && tire.WearFraction() == 0.0,
          "fresh tire is fully effective");
    Check(tire.Observe({0.05, 3000.0, 0.0, false, true}) &&
          tire.HasValidSample() && tire.GripMultiplier() <= 1.0,
          "straight session sample accepted");
    const double before = tire.WearFraction();
    Check(!tire.Observe({0.05, 3000.0, 0.0, false, false}) &&
          !tire.HasValidSample() && tire.WearFraction() == before,
          "pause never ages tires");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (auto bad : {TireConditionSample{nan, 3000, 0, false, true},
                     TireConditionSample{inf, 3000, 0, false, true},
                     TireConditionSample{-0.05, 3000, 0, false, true},
                     TireConditionSample{0.11, 3000, 0, false, true},
                     TireConditionSample{0.05, nan, 0, false, true},
                     TireConditionSample{0.05, inf, 0, false, true},
                     TireConditionSample{0.05, 3000, nan, false, true},
                     TireConditionSample{0.05, 3000, 4000, false, true},
                     TireConditionSample{0.05, 10001, 0, false, true}})
    {
        const auto wear = tire.WearFraction();
        const auto temp = tire.TemperatureFraction();
        Check(!tire.Observe(bad) && tire.WearFraction() == wear &&
              tire.TemperatureFraction() == temp,
              "invalid physics freezes tire model");
    }
    tire.Reset();
    for (int i = 0; i < 2000; ++i)
    {
        Check(tire.Observe({0.05, 3000.0, 1600.0, true, true}),
              "high slip/brake valid");
        Check(std::isfinite(tire.GripMultiplier()) &&
              tire.GripMultiplier() <= 1.0 &&
              tire.GripMultiplier() >= ArcadeTireCondition::MinimumGripMultiplier &&
              tire.WearFraction() >= 0.0 && tire.WearFraction() <= 1.0 &&
              tire.TemperatureFraction() >= 0.0 && tire.TemperatureFraction() <= 1.0,
              "100 s of aggressive turns remains bounded");
    }
    Check(tire.WearFraction() > 0.25 && tire.GripMultiplier() < 0.95,
          "aggressive driving degrades traction over time");
    const double wearAtSlide = tire.WearFraction();
    for (int i = 0; i < 200; ++i)
    {
        tire.Observe({0.05, 3000.0, 0.0, false, true});
    }
    Check(tire.WearFraction() >= wearAtSlide, "ordinary driving never repairs wear");
    tire.Reset();
    Check(tire.GripMultiplier() == 1.0 && tire.WearFraction() == 0.0 &&
          tire.TemperatureFraction() == 0.42 && !tire.HasValidSample(),
          "new race starts with no wear");
    for (int run = 0; run < 5000; ++run)
    {
        tire.Reset();
        for (int j = 0; j < 80; ++j)
        {
            tire.Observe({0.05, 4700.0, 900.0, (j % 5) == 0, true});
        }
        Check(std::isfinite(tire.GripMultiplier()) &&
              tire.GripMultiplier() >= ArcadeTireCondition::MinimumGripMultiplier,
              "stress-tested deterministic bounds");
    }
    std::cout << "ArcadeTireCondition passed\n";
}
