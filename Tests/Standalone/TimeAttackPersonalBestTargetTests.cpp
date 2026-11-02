#include "Racing/TimeAttackPersonalBestTarget.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool ok, const char* label)
{
    if (!ok)
    {
        std::cerr << "TimeAttackPersonalBestTarget FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b) { return std::abs(a - b) < 1e-5; }
}
int main()
{
    using namespace latebrake;
    const auto Measure = [](double elapsed, double saved)
    {
        return MeasureTimeAttackPersonalBestTarget(
            EventKind::TimeAttack, false, true, false, elapsed, saved);
    };
    const auto start = Measure(0.0, 90.5);
    Require(start.Valid && !start.DeadlineReached &&
            Near(start.SecondsRemaining, 90.5), "full initial PB budget");
    const auto halfway = Measure(40.25, 90.5);
    Require(halfway.Valid && Near(halfway.SavedBestSeconds, 90.5) &&
            Near(halfway.SecondsRemaining, 50.25) &&
            Near(halfway.SecondsOver, 0.0), "partial authoritative run clock");
    const auto tied = Measure(90.5, 90.5);
    Require(tied.Valid && tied.DeadlineReached &&
            Near(tied.SecondsOver, 0.0) &&
            Near(tied.SecondsRemaining, 0.0),
            "record tie never represents time left to improve");
    const auto missed = Measure(94.0, 90.5);
    Require(missed.Valid && missed.DeadlineReached &&
            Near(missed.SecondsOver, 3.5) &&
            Near(missed.SecondsRemaining, 0.0), "overdue clock is nonnegative");
    Require(Near(Measure(55.0, 60.0).SecondsRemaining, 5.0) &&
            Near(Measure(60.5, 60.0).SecondsOver, 0.5),
            "clock budget counts recovery penalties as actual race time");
    Require(!Measure(5.0, 0.0).Valid && !Measure(5.0, -2.0).Valid &&
            !Measure(5.0, 1e-100).Valid &&
            !Measure(5.0, 604800.01).Valid &&
            !Measure(5.0, std::numeric_limits<double>::infinity()).Valid &&
            !Measure(5.0, std::numeric_limits<double>::quiet_NaN()).Valid,
            "missing or corrupted saved PB cannot display a target");
    Require(!Measure(-1.0, 60.0).Valid &&
            !Measure(604800.01, 60.0).Valid &&
            !Measure(std::numeric_limits<double>::infinity(), 60.0).Valid &&
            !Measure(std::numeric_limits<double>::quiet_NaN(), 60.0).Valid,
            "invalid elapsed clock refuses a target");
    Require(Measure(604800.0, 604800.0).Valid,
            "record schema seven-day boundary is consistent");
    for (EventKind event : {EventKind::Circuit, EventKind::TimeAttack})
    for (int career = 0; career < 2; ++career)
    for (int running = 0; running < 2; ++running)
    for (int finished = 0; finished < 2; ++finished)
    {
        const auto state = MeasureTimeAttackPersonalBestTarget(event,
            career != 0, running != 0, finished != 0, 30.0, 60.0);
        Require(state.Valid == (event == EventKind::TimeAttack &&
            career == 0 && running == 1 && finished == 0),
            "circuit/career/countdown/paused/results states gated");
    }
    std::cout << "Time Attack personal-best clock target passed.\n";
    return EXIT_SUCCESS;
}
