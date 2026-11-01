#include "UI/RaceGateProgressGauge.h"

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
        std::cerr << "RaceGateProgressGauge FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using latebrake::CalculateRaceGateProgressGauge;
    const auto start = CalculateRaceGateProgressGauge(0, 0, 3, 4, false);
    Check(start.Valid && start.CreditedGates == 0 &&
          start.TotalGates == 12 && start.Fraction == 0.0,
          "new race begins at zero of twelve gates");
    const auto lap = CalculateRaceGateProgressGauge(1, 0, 3, 4, false);
    Check(lap.Valid && lap.CreditedGates == 4 &&
          std::abs(lap.Fraction - 1.0 / 3.0) < 1e-12,
          "lap completion wraps gate index without resetting progress");
    const auto finish = CalculateRaceGateProgressGauge(3, 0, 3, 4, true);
    Check(finish.Valid && finish.CreditedGates == 12 &&
          finish.Fraction == 1.0, "finished race has all credited gates");
    for (int laps = 1; laps <= 8; ++laps)
    {
        for (int gates = 1; gates <= 32; ++gates)
        {
            int previous = -1;
            for (int index = 0; index <= laps * gates; ++index)
            {
                const bool finished = index == laps * gates;
                const auto value = CalculateRaceGateProgressGauge(
                    index / gates, index % gates, laps, gates, finished);
                Check(value.Valid && value.CreditedGates == index &&
                      value.CreditedGates > previous &&
                      value.TotalGates == laps * gates &&
                      value.Fraction >= 0.0 && value.Fraction <= 1.0 &&
                      std::abs(value.Fraction * value.TotalGates - index) < 1e-10,
                      "every legal ordered gate has monotonic exact fraction");
                previous = value.CreditedGates;
            }
        }
    }
    for (const auto invalid : {
        CalculateRaceGateProgressGauge(-1, 0, 3, 4, false),
        CalculateRaceGateProgressGauge(4, 0, 3, 4, true),
        CalculateRaceGateProgressGauge(1, -1, 3, 4, false),
        CalculateRaceGateProgressGauge(1, 4, 3, 4, false),
        CalculateRaceGateProgressGauge(0, 0, 0, 4, false),
        CalculateRaceGateProgressGauge(0, 0, 3, 0, false),
        CalculateRaceGateProgressGauge(0, 0, 3, 257, false),
        CalculateRaceGateProgressGauge(0, 0,
            std::numeric_limits<int>::max(), 4, false),
        CalculateRaceGateProgressGauge(3, 0, 3, 4, false),
        CalculateRaceGateProgressGauge(0, 0, 3, 4, true),
        CalculateRaceGateProgressGauge(3, 1, 3, 4, true)
    })
    {
        Check(!invalid.Valid && invalid.CreditedGates == 0 &&
              invalid.TotalGates == 0 && invalid.Fraction == 0.0,
              "corrupt/overflow snapshot fails closed instead of lying");
    }
    std::cout << "Race gate progress display policy passed\n";
    return EXIT_SUCCESS;
}
