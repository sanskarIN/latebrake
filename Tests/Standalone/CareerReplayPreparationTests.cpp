#include "Career/CareerReplayPreparation.h"
#include "Progression/ProfileWriteCheckpoint.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool okay, const char* message)
{
    if (!okay)
    {
        std::cerr << "Career replay FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Same(const latebrake::GarageSnapshot& a, const latebrake::GarageSnapshot& b)
{
    return a.Credits == b.Credits &&
           a.OwnedVehicleMask == b.OwnedVehicleMask &&
           a.SelectedVehicle == b.SelectedVehicle &&
           a.EngineLevel == b.EngineLevel &&
           a.GripLevel == b.GripLevel;
}
}

int main()
{
    using namespace latebrake;
    CareerProgress career;
    GarageState garage;
    int races = 5;
    float bestRace = 123.0f;
    float bestLap = 40.0f;
    float bestTimeAttack = 80.0f;

    // Fresh profiles have no selected career. Replays cannot invent one.
    auto empty = StageCareerReplay(career, garage);
    Check(empty.Status == CareerLaunchStatus::LockedOrUnknownEvent &&
          !empty.NeedsSave && empty.StartingVehicle == -1 &&
          career.SelectedEvent() == -1 && garage.SelectedVehicle() == 0,
          "invalid or missing career selection fails closed");

    // Enter event 10 through its actual prerequisite medal chain.
    CareerSnapshot saved;
    saved.Medals[0] = 1;
    saved.Medals[1] = 1;
    saved.Medals[2] = 1;
    saved.SelectedEvent = 10;
    career.Load(saved);
    Check(career.SelectedEvent() == 10 &&
          career.Status(10) == CareerStatus::Available,
          "Kestrel challenge unlocked and selected");

    // If a corrupted/incompatible garage somehow loses the required car,
    // the replay must fail without spending or changing the session.
    const GarageSnapshot missing = garage.Snapshot();
    auto denied = StageCareerReplay(career, garage);
    Check(denied.Status == CareerLaunchStatus::RequiredVehicleNotOwned &&
          !denied.NeedsSave && denied.StartingVehicle == -1 &&
          Same(garage.Snapshot(), missing) && career.SelectedEvent() == 10,
          "required car missing is a side-effect-free rejection");

    GarageSnapshot owned;
    owned.Credits = 1600;
    owned.OwnedVehicleMask = 7;
    owned.SelectedVehicle = 2; // Phantom chosen in garage after finishing.
    owned.EngineLevel = 2;
    owned.GripLevel = 1;
    garage.Load(owned);
    const auto before = garage.Snapshot();
    const auto medalsBefore = career.Snapshot();

    // Simulate denied SaveGameToSlot: no Commit() means automatic rollback.
    {
        ProfileWriteCheckpoint guard(garage, career, races,
                                     bestRace, bestLap, bestTimeAttack);
        const auto prepared = StageCareerReplay(career, garage);
        Check(prepared.Status == CareerLaunchStatus::Ready &&
              prepared.NeedsSave && prepared.StartingVehicle == 1 &&
              garage.SelectedVehicle() == 1,
              "vehicle-required replay stages owned Kestrel before restart");
        // The GameMode must not reset the race until the write succeeds.
    }
    Check(Same(garage.Snapshot(), before) &&
          career.SelectedEvent() == medalsBefore.SelectedEvent &&
          career.Snapshot().Medals == medalsBefore.Medals,
          "failed replay save restores post-finish car and all medals");
    Check(races == 5 && bestRace == 123.f && bestLap == 40.f &&
          bestTimeAttack == 80.f,
          "unsaved replay never modifies award counters or best records");

    {
        ProfileWriteCheckpoint guard(garage, career, races,
                                     bestRace, bestLap, bestTimeAttack);
        const auto prepared = StageCareerReplay(career, garage);
        Check(prepared.Status == CareerLaunchStatus::Ready &&
              prepared.NeedsSave && prepared.StartingVehicle == 1,
              "replaying after a failed save can stage required vehicle again");
        guard.Commit(); // Represents a confirmed SaveGameToSlot success.
    }
    Check(garage.SelectedVehicle() == 1 &&
          garage.Owns(1) && garage.Credits() == 1600 &&
          garage.EngineLevel() == 2 && garage.GripLevel() == 1,
          "successful replay equips only required vehicle, preserving economy");
    auto unchanged = StageCareerReplay(career, garage);
    Check(unchanged.Status == CareerLaunchStatus::Ready &&
          !unchanged.NeedsSave && unchanged.StartingVehicle == 1,
          "restarting while already equipped does not write an unchanged profile");

    // Unrestricted challenge: selection after career finish becomes the
    // authoritative starting car of the next independent attempt.
    Check(career.Select(6), "Clock Hunter available");
    Check(garage.Select(2), "select Phantom after race in owned garage");
    const auto unrestricted = StageCareerReplay(career, garage);
    Check(unrestricted.Status == CareerLaunchStatus::Ready &&
          !unrestricted.NeedsSave && unrestricted.StartingVehicle == 2 &&
          career.SelectedEvent() == 6 && garage.SelectedVehicle() == 2,
          "unrestricted replay honors newly selected vehicle");

    std::cout << "Career replay selection and rollback tests passed.\n";
    return EXIT_SUCCESS;
}
