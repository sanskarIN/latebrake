#include "Career/CareerChallenges.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

static void Check(bool ok, const char* msg)
{
    if (!ok) { std::cerr << "Career challenges FAIL: " << msg << '\n'; std::exit(1); }
}
static latebrake::CareerFinish Result(int laps, double seconds, int rec = 0,
                                      int vehicle = 0, double best = 50.0,
                                      double slowest = 55.0)
{
    return {seconds, best, slowest, laps, rec, vehicle, true};
}
int main()
{
    using namespace latebrake;
    Check(EvaluateCareerMedal(*GetCareerEvent(6), Result(1, 90)) == CareerMedal::Gold,
          "time trial gold");
    Check(EvaluateCareerMedal(*GetCareerEvent(6), Result(1, 120)) == CareerMedal::Silver,
          "time trial silver");
    Check(EvaluateCareerMedal(*GetCareerEvent(6), Result(1, 155)) == CareerMedal::Bronze,
          "time trial bronze");
    Check(EvaluateCareerMedal(*GetCareerEvent(6), Result(1, 170)) == CareerMedal::None,
          "slow time not falsely medalled");
    Check(EvaluateCareerMedal(*GetCareerEvent(7), Result(2, 180, 0)) == CareerMedal::Gold,
          "clean zero recoveries gold");
    Check(EvaluateCareerMedal(*GetCareerEvent(7), Result(2, 180, 1)) == CareerMedal::Silver,
          "clean one recovery silver");
    Check(EvaluateCareerMedal(*GetCareerEvent(7), Result(2, 180, 2)) == CareerMedal::Bronze,
          "clean two recoveries bronze");
    Check(EvaluateCareerMedal(*GetCareerEvent(7), Result(2, 180, 3)) == CareerMedal::None,
          "too many recoveries not medalled");
    auto precision = Result(2, 200);
    Check(EvaluateCareerMedal(*GetCareerEvent(8), precision) == CareerMedal::Gold,
          "strict checkpoint sequence supports precision challenge");
    precision.OrderedCheckpoints = false;
    Check(EvaluateCareerMedal(*GetCareerEvent(8), precision) == CareerMedal::None,
          "unchecked checkpoint sequence rejected");
    Check(EvaluateCareerMedal(*GetCareerEvent(9), Result(3, 260, 0, 0, 80, 86)) ==
          CareerMedal::Gold, "consistency uses lap spread");
    Check(EvaluateCareerMedal(*GetCareerEvent(9), Result(3, 260, 0, 0, 80, 90)) ==
          CareerMedal::Silver, "moderate lap spread silver");
    Check(EvaluateCareerMedal(*GetCareerEvent(9), Result(3, 260, 0, 0, 80, 100)) ==
          CareerMedal::Bronze, "wide lap spread bronze");
    Check(EvaluateCareerMedal(*GetCareerEvent(9), Result(3, 260, 0, 0, 80, 110)) ==
          CareerMedal::None, "inconsistent laps no medal");
    Check(EvaluateCareerMedal(*GetCareerEvent(10), Result(2, 200, 0, 0)) ==
          CareerMedal::None, "vehicle challenge requires Kestrel");
    Check(EvaluateCareerMedal(*GetCareerEvent(10), Result(2, 200, 0, 1)) ==
          CareerMedal::Gold, "vehicle challenge accepts Kestrel");
    Check(EvaluateCareerMedal(*GetCareerEvent(11), Result(6, 780)) ==
          CareerMedal::Gold, "endurance completes six laps");
    Check(EvaluateCareerMedal(*GetCareerEvent(11), Result(3, 780)) ==
          CareerMedal::None, "partial endurance rejected");
    auto invalid = Result(1, 90);
    invalid.ElapsedSeconds = std::numeric_limits<double>::quiet_NaN();
    Check(EvaluateCareerMedal(*GetCareerEvent(6), invalid) == CareerMedal::None,
          "NaN time rejected");
    invalid = Result(1, 90);
    invalid.SlowestLapSeconds = std::numeric_limits<double>::infinity();
    Check(EvaluateCareerMedal(*GetCareerEvent(6), invalid) == CareerMedal::None,
          "invalid lap time rejected");
    invalid = Result(1, 90, -1);
    Check(EvaluateCareerMedal(*GetCareerEvent(6), invalid) == CareerMedal::None,
          "negative recovery count rejected");
    invalid = Result(1, 90);
    invalid.BestLapSeconds = 200;
    Check(EvaluateCareerMedal(*GetCareerEvent(6), invalid) == CareerMedal::None,
          "illogical lap duration rejected");
    invalid = Result(2, 604800.01, 0);
    Check(!IsValidCareerFinish(*GetCareerEvent(7), invalid) &&
          EvaluateCareerMedal(*GetCareerEvent(7), invalid) == CareerMedal::None,
          "clean challenge cannot award gold for a seven-day-plus clock");
    invalid = Result(3, 604800.01, 0, 0, 80, 86);
    Check(!IsValidCareerFinish(*GetCareerEvent(9), invalid) &&
          EvaluateCareerMedal(*GetCareerEvent(9), invalid) == CareerMedal::None,
          "consistency medals cannot bypass invalid elapsed time");
    invalid = Result(2, 604800.0, 0, 0, 50, 55);
    Check(IsValidCareerFinish(*GetCareerEvent(7), invalid),
          "valid recordable seven-day boundary remains acceptable");
    invalid = Result(2, 180, 0, 0, 1e-100, 55);
    Check(!IsValidCareerFinish(*GetCareerEvent(7), invalid),
          "best lap which underflows persisted precision is rejected");
    invalid = Result(2, 180, 0, 0, 604800.01, 604800.01);
    Check(!IsValidCareerFinish(*GetCareerEvent(7), invalid),
          "overlong lap is invalid even before ordering validation");
    std::cout << "Career challenges passed.\n";
    return 0;
}
