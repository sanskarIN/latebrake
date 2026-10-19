#include "Progression/RecordValidation.h"

#include <cstdlib>
#include <iostream>
#include <limits>

void Require(bool truth, const char* message)
{
    if (!truth)
    {
        std::cerr << "Record FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    Require(SanitizeBestTime(0.0) == 0.f, "new records are empty");
    Require(SanitizeBestTime(90.5) == 90.5f, "valid best time preserved");
    Require(SanitizeBestTime(-5.0) == 0.f, "negative record rejected");
    Require(SanitizeBestTime(std::numeric_limits<double>::quiet_NaN()) == 0.f,
            "NaN record rejected");
    Require(SanitizeBestTime(std::numeric_limits<double>::infinity()) == 0.f,
            "positive infinity rejected");
    Require(SanitizeBestTime(-std::numeric_limits<double>::infinity()) == 0.f,
            "negative infinity rejected");
    Require(SanitizeBestTime(604800.0) == 604800.f, "seven-day boundary valid");
    Require(SanitizeBestTime(604800.01) == 0.f, "unreasonably long record rejected");
    Require(SanitizeCompletedRaces(-100) == 0, "negative completed races repaired");
    Require(IncrementCompletedRaces(-100) == 1, "negative counter starts at one");
    Require(IncrementCompletedRaces(0) == 1, "first completed race");
    Require(IncrementCompletedRaces(std::numeric_limits<int>::max() - 1)
        == std::numeric_limits<int>::max(), "near-limit increment");
    Require(IncrementCompletedRaces(std::numeric_limits<int>::max())
        == std::numeric_limits<int>::max(), "increment saturates safely");
    std::cout << "Save record sanitation tests passed.\n";
    return EXIT_SUCCESS;
}
