#include "Career/CareerProgress.h"
#include "Garage/GarageState.h"

#include <climits>
#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool ok, const char* reason)
{
    if (!ok)
    {
        std::cerr << "CareerSixCarEligibility FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}

latebrake::CareerFinish ValidFinish(int vehicleId)
{
    // Clock Hunter: 1 measured lap, all four ordered checkpoints,
    // plausible time and split times; no special required car.
    return {90.0, 90.0, 90.0, 1, 0, vehicleId, true};
}
}

int main()
{
    using namespace latebrake;
    static_assert(VehicleCount == 6);
    static_assert(CareerEventCount == 12);
    const CareerEvent& open = *GetCareerEvent(6);
    Check(open.RequiredVehicle < 0 && open.Laps == 1,
          "Clock Hunter remains available to any owned car");
    for (int car = 0; car < VehicleCount; ++car)
    {
        const CareerFinish finish = ValidFinish(car);
        Check(GarageState::IsVehicle(car), "garage recognizes the car");
        Check(IsValidCareerFinish(open, finish),
              "all six genuine garage cars can validate a finished career event");
        Check(EvaluateCareerMedal(open, finish) == CareerMedal::Gold,
              "legacy and three newly appended garage cars earn the same clean medal");

        CareerProgress fresh;
        Check(fresh.Select(6), "first available standalone challenge selectable");
        const CareerAward award = fresh.CommitFinish(6, finish);
        Check(award.ValidFinish && award.Medal == CareerMedal::Gold &&
              award.MedalImproved && award.FirstCompletion &&
              award.Credits == open.FirstCredits,
              "each valid car earns a single uninflated first medal and credits");
        Check(fresh.Medal(6) == CareerMedal::Gold &&
              fresh.CompletedCount() == 1,
              "successful source event records a normal career medal");
        const CareerAward again = fresh.CommitFinish(6, finish);
        Check(again.ValidFinish && !again.MedalImproved &&
              !again.FirstCompletion && again.Credits == 0,
              "replaying an event cannot farm first-completion credits");

        const CareerSnapshot saved = fresh.Snapshot();
        Check(saved.Schema == CareerSaveSchema, "saved schema unchanged");
        CareerProgress restored;
        restored.Load(saved);
        Check(restored.Medal(6) == CareerMedal::Gold &&
              restored.CompletedCount() == 1,
              "stored medal remains compatible across reload");
    }

    const int invalid[] = {-100, -1, VehicleCount, VehicleCount + 1,
                           INT_MAX, INT_MIN};
    for (const int id : invalid)
    {
        Check(!GarageState::IsVehicle(id), "unknown vehicle ID is invalid");
        const CareerFinish bad = ValidFinish(id);
        Check(!IsValidCareerFinish(open, bad) &&
              EvaluateCareerMedal(open, bad) == CareerMedal::None,
              "corrupt/future vehicle IDs cannot grant a career medal");
        CareerProgress progress;
        Check(progress.Select(6), "open challenge selectable");
        const CareerAward award = progress.CommitFinish(6, bad);
        Check(!award.ValidFinish && award.Credits == 0 &&
              progress.CompletedCount() == 0,
              "bad ID never updates career save, score or credits");
    }

    const CareerEvent& mastery = *GetCareerEvent(10);
    Check(mastery.RequiredVehicle == 1,
          "existing Kestrel-only challenge requirement is unchanged");
    for (int id = 0; id < VehicleCount; ++id)
    {
        CareerFinish finish{200.0, 100.0, 100.0, 2, 0, id, true};
        const bool shouldPass = id == 1;
        Check(IsValidCareerFinish(mastery, finish) == shouldPass,
              "vehicle-specific challenge rejects every other owned car");
        Check((EvaluateCareerMedal(mastery, finish) == CareerMedal::Gold)
                == shouldPass,
              "vehicle requirement cannot be bypassed with appended car IDs");
    }

    // A player may start with any owned car, but swapping it during a
    // run must not create an award: GameMode passes VehicleId = -1 when
    // current garage selection differs from CareerStartedVehicleId.
    for (int oldId = 0; oldId < VehicleCount; ++oldId)
    {
        const auto switched = ValidFinish(-1);
        Check(!IsValidCareerFinish(open, switched),
              "swapped-car sentinel remains unconditionally rejected");
    }
    std::cout << "CareerSixCarEligibility: all six garages, medals and saves verified\n";
    return EXIT_SUCCESS;
}
