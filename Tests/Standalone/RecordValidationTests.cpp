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
    Require(IsValidFinishedRaceTime(90.5),
            "ordinary positive race finish is recordable");
    Require(IsValidFinishedRaceTime(604800.0),
            "exact seven-day race boundary is recordable");
    Require(!IsValidFinishedRaceTime(604800.01),
            "above seven-day race cannot become a persisted finish");
    Require(!IsValidFinishedRaceTime(1e100),
            "finite but float-unrepresentable elapsed time rejected");
    Require(!IsValidFinishedRaceTime(std::numeric_limits<double>::infinity()),
            "infinite elapsed finish rejected");
    Require(!IsValidFinishedRaceTime(std::numeric_limits<double>::quiet_NaN()),
            "NaN elapsed finish rejected");
    Require(!IsValidFinishedRaceTime(0.0) &&
            !IsValidFinishedRaceTime(-10.0),
            "zero or negative finish cannot earn credits");
    Require(!IsValidFinishedRaceTime(1e-100),
            "subnormal time which cannot survive float storage rejected");

    Require(ImprovedBestTime(88.f, 70.5) == 70.5f,
            "valid faster finish improves personal best");
    Require(ImprovedBestTime(88.f, 95.0) == 88.f,
            "slower finish keeps previous best");
    Require(ImprovedBestTime(88.f, 604800.01) == 88.f,
            "overlong finish cannot destroy previous record");
    Require(ImprovedBestTime(88.f, 1e100) == 88.f &&
            ImprovedBestTime(88.f, std::numeric_limits<double>::infinity()) == 88.f,
            "huge or infinite candidate cannot overwrite float best");
    Require(ImprovedBestTime(0.f, 75.0) == 75.f,
            "empty record accepts first legitimate finish");
    Require(ImprovedBestTime(90.f, 0.0) == 90.f,
            "zero candidate never erases saved record");
    Require(ImprovedBestTime(std::numeric_limits<float>::infinity(), 80.0) == 80.f,
            "corrupt previous best repaired with valid result");
    Require(ImprovedBestTime(90.f, 1e-100) == 90.f,
            "unrepresentable positive candidate leaves valid record");
    Require(ImprovedBestTime(std::numeric_limits<float>::quiet_NaN(),
                             std::numeric_limits<double>::quiet_NaN()) == 0.f,
            "both invalid values resolve to no stored best");

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
