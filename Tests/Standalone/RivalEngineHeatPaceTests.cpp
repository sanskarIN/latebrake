#include "AI/RivalEngineHeatPace.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool okay, const char* message)
{
    if (!okay)
    {
        std::cerr << "RivalEngineHeatPace FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
latebrake::RivalEngineHeatPaceSample Sample(
    double heat, double speed = 2500.0, bool ready = true)
{
    return {0.05, speed, heat, ready, true};
}
latebrake::PilotCommand Command(double throttle = 0.85)
{
    return {throttle, -0.45, false, false};
}
void CheckFailsClosed(latebrake::RivalEngineHeatPaceSample bad,
                      const char* message)
{
    const auto output = latebrake::ModerateRivalEngineHeatPace(
        Command(), bad);
    Check(output.Throttle == 0.0 && output.Steering == 0.0 &&
          output.Brake && !output.Arrived, message);
}
} // namespace

int main()
{
    using latebrake::ArcadeEngineHeat;
    using latebrake::ModerateRivalEngineHeatPace;
    using latebrake::PilotCommand;

    const auto baseline = Command();
    auto value = ModerateRivalEngineHeatPace(
        baseline, Sample(ArcadeEngineHeat::IdleHeat));
    Check(value.Throttle == baseline.Throttle &&
          value.Steering == baseline.Steering && !value.Brake,
          "cold real engine does not alter normal rival pace");
    value = ModerateRivalEngineHeatPace(baseline, Sample(0.69));
    Check(value.Throttle == baseline.Throttle,
          "below thermal threshold preserves pace");
    value = ModerateRivalEngineHeatPace(baseline, Sample(0.83));
    Check(value.Throttle < baseline.Throttle && value.Throttle > 0.30 &&
          value.Steering == baseline.Steering && !value.Brake,
          "hot engine reduces throttle without steering or forced brake");
    value = ModerateRivalEngineHeatPace(baseline, Sample(1.0));
    Check(std::abs(value.Throttle - 0.30) < 1e-12,
          "fully hot rival keeps bounded low acceleration");
    value = ModerateRivalEngineHeatPace(baseline, Sample(1.0, 650.0));
    Check(value.Throttle == baseline.Throttle,
          "low-speed recovery never stalls engine by cooling limiter");
    value = ModerateRivalEngineHeatPace(
        baseline, Sample(1.0, 2500.0, false));
    Check(value.Throttle == baseline.Throttle,
          "missing previous thermal reading cannot invent heat penalty");
    value = ModerateRivalEngineHeatPace(Command(0.17), Sample(1.0));
    Check(value.Throttle == 0.17,
          "thermal limiter never accelerates a cautious rival");
    value = ModerateRivalEngineHeatPace(
        {0.0, 0.75, true, false}, Sample(1.0));
    Check(value.Brake && value.Throttle == 0.0 && value.Steering == 0.75,
          "prior traffic/corner brakes always take precedence");
    value = ModerateRivalEngineHeatPace(
        {0.0, 0.0, false, true}, Sample(1.0));
    Check(value.Arrived && value.Throttle == 0.0,
          "verified route arrival is unchanged");

    double lastAllowed = 1.0;
    for (int i = 0; i <= 300; ++i)
    {
        const double heat = 0.70 + static_cast<double>(i) / 1000.0;
        value = ModerateRivalEngineHeatPace(baseline, Sample(heat));
        Check(value.Throttle <= lastAllowed + 1e-12 &&
              value.Throttle >= 0.30 && !value.Brake,
              "increasing heat must monotonically reduce permitted load");
        lastAllowed = value.Throttle;
    }
    CheckFailsClosed({0.05, 2500.0, 0.8, true, false},
                     "invalid physics cannot set AI drive");
    CheckFailsClosed({0.0, 2500.0, 0.8, true, true},
                     "zero frame is invalid");
    CheckFailsClosed({0.101, 2500.0, 0.8, true, true},
                     "frame hitch fails closed");
    CheckFailsClosed({0.05, -1.0, 0.8, true, true},
                     "negative speed fails closed");
    CheckFailsClosed({0.05, 10001.0, 0.8, true, true},
                     "unqualified extreme speed fails closed");
    CheckFailsClosed({0.05, 2500.0, 0.17, true, true},
                     "invented below-ambient heat fails closed");
    CheckFailsClosed({0.05, 2500.0, 1.01, true, true},
                     "out-of-range heat fails closed");
    for (double invalid : {
             std::numeric_limits<double>::quiet_NaN(),
             std::numeric_limits<double>::infinity(),
             -std::numeric_limits<double>::infinity()})
    {
        CheckFailsClosed({invalid, 2500.0, 0.8, true, true},
                         "nonfinite frame fails closed");
        CheckFailsClosed({0.05, invalid, 0.8, true, true},
                         "nonfinite physics speed fails closed");
        CheckFailsClosed({0.05, 2500.0, invalid, true, true},
                         "nonfinite heat fails closed");
        const auto throttle = ModerateRivalEngineHeatPace(
            {invalid, 0.0, false, false}, Sample(0.8));
        Check(throttle.Brake && throttle.Throttle == 0.0,
              "nonfinite throttle fails closed");
        const auto steering = ModerateRivalEngineHeatPace(
            {0.85, invalid, false, false}, Sample(0.8));
        Check(steering.Brake && steering.Steering == 0.0,
              "nonfinite steering fails closed");
    }
    Check(ModerateRivalEngineHeatPace(
        {1.01, 0.0, false, false}, Sample(0.8)).Brake,
        "over-range drive command is rejected");

    // Integrate real thermal-state evolution, not hand-picked fractions.
    ArcadeEngineHeat physicalHeat;
    for (int i = 0; i < 1500; ++i)
    {
        physicalHeat.Observe({0.05, 3000.0, 1.0, false, true});
    }
    Check(physicalHeat.HasValidSample() &&
          physicalHeat.HeatFraction() > 0.70,
          "actual sustained rival drive produces a hot qualified sample");
    value = ModerateRivalEngineHeatPace(baseline,
        Sample(physicalHeat.HeatFraction()));
    Check(value.Throttle < baseline.Throttle,
          "controller consumes real thermal state machine output");
    const double initiallyHot = physicalHeat.HeatFraction();
    for (int i = 0; i < 1500; ++i)
    {
        physicalHeat.Observe({0.05, 5000.0, 0.0, false, true});
    }
    Check(physicalHeat.HeatFraction() < initiallyHot,
          "real coasting can cool the rival before limiter reopens");

    for (int i = 0; i < 200000; ++i)
    {
        const double fraction = 0.18 +
            0.82 * static_cast<double>(i % 1001) / 1000.0;
        const double speed = 120.0 + static_cast<double>(i % 8000);
        const double demand = static_cast<double>(i % 101) / 100.0;
        const auto input = Command(demand);
        value = ModerateRivalEngineHeatPace(input, Sample(fraction, speed));
        Check(!value.Brake && !value.Arrived &&
              value.Steering == input.Steering &&
              std::isfinite(value.Throttle) && value.Throttle >= 0.0 &&
              value.Throttle <= input.Throttle + 1e-12,
              "200k qualified samples can never steer, boost or force brake");
    }
    std::cout << "RivalEngineHeatPace: 200000 bounded samples passed (source only)\n";
    return EXIT_SUCCESS;
}
