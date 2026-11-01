#include "Garage/GarageCatalogView.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool yes, const char* message)
{
    if (!yes)
    {
        std::cerr << "Garage catalog FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Close(double a, double b)
{
    return std::abs(a - b) < 1e-10;
}
bool Same(latebrake::GarageSnapshot a, latebrake::GarageSnapshot b)
{
    return a.Credits == b.Credits &&
        a.OwnedVehicleMask == b.OwnedVehicleMask &&
        a.SelectedVehicle == b.SelectedVehicle &&
        a.EngineLevel == b.EngineLevel && a.GripLevel == b.GripLevel;
}
}

int main()
{
    using namespace latebrake;
    const GarageSnapshot fresh{};
    const GarageCatalogView initial = BuildGarageCatalogView(fresh);
    Check(initial.Cars.size() == 6 && initial.Credits == 0 &&
          initial.SelectedVehicle == 0 && initial.NextUnlockId == 1,
          "fresh profile shows six original catalog slots and free car");
    Check(initial.Cars[0].Ownership == GarageCatalogOwnership::Equipped &&
          initial.Cars[1].Ownership == GarageCatalogOwnership::LockedInsufficientCredits &&
          initial.Cars[1].CreditsShort == 600,
          "locked car is visible but never advertised as owned");
    for (int id = 0; id < VehicleCount; ++id)
    {
        const VehicleSpec spec = GarageState::Definition(id);
        const auto& row = initial.Cars[static_cast<std::size_t>(id)];
        Check(row.VehicleId == id && row.Name == spec.Name &&
              row.UnlockCost == spec.UnlockCost &&
              Close(row.Effective.Acceleration, spec.Acceleration) &&
              Close(row.Effective.Grip, spec.Grip) &&
              Close(row.Effective.TopSpeed, spec.TopSpeed),
              "all catalogue rows preserve stable specs and base tuning");
    }

    GarageState garage;
    garage.Load({1000, 1, 0, 2, 3});
    const auto before = garage.Snapshot();
    const auto affordable = BuildGarageCatalogView(before);
    Check(Same(before, garage.Snapshot()), "read-only preview never mutates garage");
    Check(affordable.Cars[1].Ownership ==
          GarageCatalogOwnership::LockedAffordable &&
          affordable.Cars[1].CreditsShort == 0 &&
          affordable.Cars[2].Ownership ==
          GarageCatalogOwnership::LockedInsufficientCredits &&
          affordable.Cars[2].CreditsShort == 200,
          "locked vehicle affordability and shortfall depend on earned credits");
    Check(Close(affordable.Cars[1].Effective.Acceleration,
                GarageState::Definition(1).Acceleration * 1.12) &&
          Close(affordable.Cars[1].Effective.Grip,
                GarageState::Definition(1).Grip * 1.15) &&
          Close(affordable.Cars[1].Effective.TopSpeed,
                GarageState::Definition(1).TopSpeed * 1.04),
          "previews apply selected shared upgrade levels to all cars");

    garage.Load({9800, 7, 2, 1, 2});
    const auto legacy = BuildGarageCatalogView(garage.Snapshot());
    Check(legacy.SelectedVehicle == 2 && legacy.NextUnlockId == 3 &&
          legacy.Cars[2].Ownership == GarageCatalogOwnership::Equipped &&
          legacy.Cars[0].Ownership == GarageCatalogOwnership::Owned &&
          legacy.Cars[1].Ownership == GarageCatalogOwnership::Owned,
          "original three-car save retains equipped/owned statuses");
    for (int id = 3; id < VehicleCount; ++id)
        Check(legacy.Cars[static_cast<std::size_t>(id)].Ownership ==
              GarageCatalogOwnership::LockedAffordable,
              "high credits never grant new car ownership automatically");

    garage.Load({900, 63, 5, 3, 3});
    const auto allOwned = BuildGarageCatalogView(garage.Snapshot());
    Check(allOwned.NextUnlockId == -1 && allOwned.SelectedVehicle == 5,
          "all-car save has no next purchase and can equip Nova");
    for (int id = 0; id < VehicleCount; ++id)
        Check(allOwned.Cars[static_cast<std::size_t>(id)].Ownership ==
              (id == 5 ? GarageCatalogOwnership::Equipped
                       : GarageCatalogOwnership::Owned) &&
              allOwned.Cars[static_cast<std::size_t>(id)].CreditsShort == 0,
              "all six owned car statuses stay deterministic");

    const auto repaired = BuildGarageCatalogView({-500, -1, 5, 99, -2});
    Check(repaired.SelectedVehicle == 0 && repaired.Credits == 0 &&
          repaired.EngineLevel == 3 && repaired.GripLevel == 0 &&
          repaired.Cars[5].Ownership ==
              GarageCatalogOwnership::LockedInsufficientCredits,
          "negative/corrupt save cannot create an unlocked catalog entry");

    const auto unknown = BuildGarageCatalogView({
        std::numeric_limits<int>::max(), 0x7FFFFFFF, 200, 0, 0});
    Check(unknown.SelectedVehicle == 0 && unknown.NextUnlockId == -1 &&
          unknown.Cars[5].Ownership == GarageCatalogOwnership::Owned,
          "future bits and invalid equipped ID are sanitized consistently");

    std::cout << "Garage catalogue view tests passed.\n";
    return EXIT_SUCCESS;
}
