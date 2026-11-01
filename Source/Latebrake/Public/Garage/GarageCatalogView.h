#pragma once

#include "Garage/GarageState.h"

#include <array>

namespace latebrake
{
// An observation-only catalogue for the actual Unreal Garage Canvas menu.
// Never selects a car, spends credits, grants ownership, or writes a save.
// Preserve the GarageState snapshot as the single economy authority.
enum class GarageCatalogOwnership
{
    Equipped,
    Owned,
    LockedAffordable,
    LockedInsufficientCredits
};

struct GarageCatalogEntry final
{
    int VehicleId = 0;
    const char* Name = "";
    GarageCatalogOwnership Ownership = GarageCatalogOwnership::LockedInsufficientCredits;
    int UnlockCost = 0;
    int CreditsShort = 0;
    // Read-only theoretical multipliers with the CURRENT shared upgrades.
    // These are not verified performance metrics or independent tuning.
    VehiclePerformance Effective{};
};

struct GarageCatalogView final
{
    std::array<GarageCatalogEntry, VehicleCount> Cars{};
    int SelectedVehicle = 0;
    int Credits = 0;
    int NextUnlockId = -1;
    int EngineLevel = 0;
    int GripLevel = 0;
};

inline GarageCatalogView BuildGarageCatalogView(GarageSnapshot persisted) noexcept
{
    // Reuse the exact sanitation rules applied during SaveGame loading.
    // Future/negative ownership masks must never advertise free cars.
    GarageState garage;
    garage.Load(persisted);

    GarageCatalogView view;
    view.SelectedVehicle = garage.SelectedVehicle();
    view.Credits = garage.Credits();
    view.NextUnlockId = garage.NextLockedVehicle();
    view.EngineLevel = garage.EngineLevel();
    view.GripLevel = garage.GripLevel();

    for (int id = 0; id < VehicleCount; ++id)
    {
        const VehicleSpec spec = GarageState::Definition(id);
        GarageCatalogEntry& entry = view.Cars[static_cast<std::size_t>(id)];
        entry.VehicleId = id;
        entry.Name = spec.Name;
        entry.UnlockCost = spec.UnlockCost;
        entry.CreditsShort = garage.Owns(id)
            ? 0 : std::max(0, spec.UnlockCost - view.Credits);
        entry.Ownership = id == view.SelectedVehicle
            ? GarageCatalogOwnership::Equipped
            : garage.Owns(id) ? GarageCatalogOwnership::Owned
            : garage.CanUnlock(id) ? GarageCatalogOwnership::LockedAffordable
                                   : GarageCatalogOwnership::LockedInsufficientCredits;

        // Match GarageState::Performance, except for the observed row's
        // vehicle rather than only the selected one. This is a preview.
        entry.Effective = {
            spec.Acceleration * (1.0 + 0.06 * view.EngineLevel),
            spec.Grip * (1.0 + 0.05 * view.GripLevel),
            spec.TopSpeed * (1.0 + 0.02 * view.EngineLevel)
        };
    }
    return view;
}
} // namespace latebrake
