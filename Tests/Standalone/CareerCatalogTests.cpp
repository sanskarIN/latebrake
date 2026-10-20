#include "Career/CareerCatalog.h"
#include <cstdlib>
#include <iostream>

static void Check(bool ok, const char* label)
{
    if (!ok) { std::cerr << "Career catalog FAIL: " << label << '\n'; std::exit(1); }
}

int main()
{
    using namespace latebrake;
    Check(CareerEventCount >= 12, "catalog has six rounds and six challenges");
    Check(Championships.size() == 2, "two original championships");
    for (int id = 0; id < CareerEventCount; ++id)
    {
        const CareerEvent* event = GetCareerEvent(id);
        Check(event && event->Id == id, "persisted IDs match catalog indices");
        Check(event->Laps > 0 && event->Laps <= 8, "bounded configured laps");
        Check(event->Prerequisite < id && event->Prerequisite >= -1,
              "prerequisites are acyclic and defined");
        Check(event->FirstCredits > 0, "valid offline reward");
        Check(event->BronzeTarget >= event->SilverTarget &&
              event->SilverTarget >= event->GoldTarget &&
              event->GoldTarget >= 0.0, "valid ordered thresholds");
    }
    for (int series = 0; series < 2; ++series)
    {
        const auto* championship = GetChampionship(series);
        Check(championship && championship->FirstCompletionBonus > 0,
              "series with once-only completion bonus");
        for (int round = 0; round < 3; ++round)
        {
            const CareerEvent* event = GetCareerEvent(championship->Rounds[round]);
            Check(event && event->Series == series && event->Round == round,
                  "round ID, series and order agree");
        }
    }
    Check(GetCareerEvent(-1) == nullptr && GetCareerEvent(100) == nullptr,
          "unknown career IDs rejected");
    Check(GetChampionship(-1) == nullptr && GetChampionship(2) == nullptr,
          "unknown championships rejected");
    std::cout << "Career catalog passed.\n";
    return 0;
}
