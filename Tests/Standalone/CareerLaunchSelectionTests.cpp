#include "Career/CareerLaunchSelection.h"
#include "Progression/ProfileWriteCheckpoint.h"

#include <cstdlib>
#include <iostream>

static void Check(bool ok, const char* why)
{
    if (!ok)
    {
        std::cerr << "Career launch FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    CareerProgress career;
    GarageState garage;
    int completedRaces = 2;
    float bestRace = 120.f;
    float bestLap = 40.f;
    float bestAttack = 90.f;

    Check(StageCareerLaunch(career, garage, -1) ==
          CareerLaunchStatus::LockedOrUnknownEvent,
          "negative career event rejected");
    Check(StageCareerLaunch(career, garage, CareerEventCount) ==
          CareerLaunchStatus::LockedOrUnknownEvent,
          "out-of-range career event rejected");
    Check(StageCareerLaunch(career, garage, 1) ==
          CareerLaunchStatus::LockedOrUnknownEvent &&
          career.SelectedEvent() == -1, "locked round does not alter selection");

    // Calling code stages the requested selection only; without an
    // acknowledged disk write the guard must restore both profile models.
    {
        ProfileWriteCheckpoint guard(garage, career, completedRaces,
                                     bestRace, bestLap, bestAttack);
        Check(StageCareerLaunch(career, garage, 0) == CareerLaunchStatus::Ready &&
              career.SelectedEvent() == 0, "valid first event staged");
        // Simulate SaveGameToSlot returning false by not committing.
        guard.Rollback();
    }
    Check(career.SelectedEvent() == -1 && garage.SelectedVehicle() == 0,
          "failed save restores career and garage selection");

    {
        ProfileWriteCheckpoint guard(garage, career, completedRaces,
                                     bestRace, bestLap, bestAttack);
        Check(StageCareerLaunch(career, garage, 0) == CareerLaunchStatus::Ready,
              "retry after failed save may stage same event");
        guard.Commit(); // Represents a successful SaveGameToSlot only.
    }
    Check(career.SelectedEvent() == 0, "successful save commits staged event");

    // Kestrel Mastery (event 10) unlocks after Rookie Finale (event 2),
    // but requires vehicle 1 to be owned. That requirement must not
    // implicitly purchase/unlock a car.
    CareerSnapshot medals;
    medals.Medals[0] = 1;
    medals.Medals[1] = 1;
    medals.Medals[2] = 1;
    medals.SelectedEvent = 0;
    career.Load(medals);
    Check(career.Status(10) == CareerStatus::Available,
          "Kestrel Mastery unlocked by valid prerequisite medals");
    Check(StageCareerLaunch(career, garage, 10) ==
          CareerLaunchStatus::RequiredVehicleNotOwned,
          "required car not owned is a hard rejection");
    Check(career.SelectedEvent() == 0 && garage.SelectedVehicle() == 0,
          "missing car rejection leaves both models intact");

    GarageSnapshot owned = garage.Snapshot();
    owned.OwnedVehicleMask |= 1 << 1;
    garage.Load(owned);
    {
        ProfileWriteCheckpoint guard(garage, career, completedRaces,
                                     bestRace, bestLap, bestAttack);
        Check(StageCareerLaunch(career, garage, 10) == CareerLaunchStatus::Ready,
              "owned Kestrel permits event launch");
        Check(career.SelectedEvent() == 10 && garage.SelectedVehicle() == 1,
              "required vehicle is selected together with career event");
        // Abandoned scope: OS/SaveGame failure rolls back both.
    }
    Check(career.SelectedEvent() == 0 && garage.SelectedVehicle() == 0,
          "automatic rollback restores active career and previous car");

    {
        ProfileWriteCheckpoint guard(garage, career, completedRaces,
                                     bestRace, bestLap, bestAttack);
        Check(StageCareerLaunch(career, garage, 10) == CareerLaunchStatus::Ready,
              "successful retry stages correct car/event");
        guard.Commit();
    }
    Check(career.SelectedEvent() == 10 && garage.SelectedVehicle() == 1 &&
          garage.Owns(1), "committed launch preserves selected owned car");
    Check(completedRaces == 2 && bestRace == 120.f && bestLap == 40.f &&
          bestAttack == 90.f, "selection never fakes finish or reward records");

    std::cout << "Career launch selection tests passed.\n";
    return EXIT_SUCCESS;
}
