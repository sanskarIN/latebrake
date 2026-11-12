#include "Racing/PortablePitWindowForecast.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* description)
{
    if (!condition)
    {
        std::cerr << "PortablePitWindow FAIL: " << description << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b) { return std::abs(a - b) < 1e-8; }
}

int main()
{
    using namespace latebrake;
    PortablePitWindowInput input;
    const auto forecast = ForecastPortablePitWindow(input);
    Check(forecast.Valid && forecast.EvaluatedPitWindows == 9 &&
          forecast.StopAfterUpcomingLap >= 1 &&
          forecast.StopAfterUpcomingLap <= 9 &&
          forecast.PitRecommended &&
          forecast.BenefitSeconds > 0.0,
          "actual increasing tire loss can make a stop worthwhile");
    const double expectedNoStop = 10.0 * 90.0 + 10.0 * 2.0 +
        0.5 * 10.0 * 9.0 / 2.0;
    Check(Near(forecast.NoStopProjectedSeconds, expectedNoStop),
          "closed-form wear sum matches explicit arithmetic series");

    input.PitLaneAndServiceSeconds = 600.0;
    auto result = ForecastPortablePitWindow(input);
    Check(result.Valid && !result.PitRecommended &&
          result.StopAfterUpcomingLap == -1 &&
          Near(result.BenefitSeconds, 0.0),
          "expensive optional pit must never produce fictitious savings");
    input.PitStopMandatory = true;
    result = ForecastPortablePitWindow(input);
    Check(result.Valid && result.PitRecommended &&
          result.BenefitSeconds < 0.0 &&
          result.SelectedProjectedSeconds > result.NoStopProjectedSeconds,
          "mandatory stop honestly reports its time cost");

    input.PitStopMandatory = false;
    input.PitLaneAndServiceSeconds = 0.0;
    input.CurrentTireLossSecondsPerLap = 0.0;
    input.CurrentTireLossGrowthSecondsPerLap = 0.0;
    input.NewTireLossSecondsPerLap = 0.0;
    input.NewTireLossGrowthSecondsPerLap = 0.0;
    result = ForecastPortablePitWindow(input);
    Check(result.Valid && !result.PitRecommended &&
          Near(result.BenefitSeconds, 0.0),
          "exact tie chooses no optional stop");
    input.PitStopMandatory = true;
    result = ForecastPortablePitWindow(input);
    Check(result.Valid && result.PitRecommended &&
          result.StopAfterUpcomingLap == 1,
          "mandatory equal-time pit resolves to earliest eligible lap");

    input = {};
    input.EarliestStopAfterLap = 3;
    input.LatestStopAfterLap = 4;
    result = ForecastPortablePitWindow(input);
    Check(result.Valid && result.EvaluatedPitWindows == 2 &&
          (!result.PitRecommended ||
           (result.StopAfterUpcomingLap >= 3 &&
            result.StopAfterUpcomingLap <= 4)),
          "restrict candidate window without extrapolating policy");
    input.EarliestStopAfterLap = 5;
    Check(!ForecastPortablePitWindow(input).Valid,
          "reversed pit range rejected");
    input = {};
    input.RemainingLaps = 1;
    Check(!ForecastPortablePitWindow(input).Valid,
          "no unsafe fabricated pit opportunity on final lap");
    input = {};
    input.PitLaneAndServiceSeconds =
        std::numeric_limits<double>::quiet_NaN();
    Check(!ForecastPortablePitWindow(input).Valid,
          "nonfinite stop cost rejected");
    input = {};
    input.NewTireLossGrowthSecondsPerLap = -0.001;
    Check(!ForecastPortablePitWindow(input).Valid,
          "negative wear growth refused");
    input = {};
    input.RemainingLaps = 150;
    input.LatestStopAfterLap = 149;
    input.BaselineLapSeconds = 1200.0;
    input.CurrentTireLossGrowthSecondsPerLap = 10.0;
    input.NewTireLossGrowthSecondsPerLap = 10.0;
    result = ForecastPortablePitWindow(input);
    Check(result.Valid && std::isfinite(result.SelectedProjectedSeconds) &&
          result.EvaluatedPitWindows == 149,
          "bounded full-length extreme race remains numerically safe");
    std::cout << "Portable pit window tests passed.\n";
}
