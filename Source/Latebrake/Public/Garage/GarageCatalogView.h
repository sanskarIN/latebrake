#pragma once

#include "Garage/GarageState.h"
#include "Garage/GaragePurchasePlanner.h"

#include <array>
#include <cstdint>
#include <limits>

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
    // Sticker price and individual affordability above are retained for
    // older clients. The actual Q sequence may require earlier cars.
    bool RouteValid = false;
    int RouteCars = 0;
    int RouteCost = 0;
    int RouteCreditsShort = 0;
    bool RouteBudgetReady = false;
    bool IsNextUnlock = false;
};

struct GarageCatalogView final
{
    std::array<GarageCatalogEntry, VehicleCount> Cars{};
    int SelectedVehicle = 0;
    int OwnedVehicleCount = 0;
    int Credits = 0;
    int NextUnlockId = -1;
    int EngineLevel = 0;
    int GripLevel = 0;
};

// Resolve only a coherent, fully planned row. Do not substitute the equipped
// vehicle for a missing showroom target: doing so would mislabel the preview.
inline const GarageCatalogEntry* FindGarageCatalogEntry(
    const GarageCatalogView& catalog, int vehicleId) noexcept
{
    if (!GarageState::IsVehicle(vehicleId)) return nullptr;
    const auto& entry = catalog.Cars[static_cast<std::size_t>(vehicleId)];
    if (entry.VehicleId != vehicleId || !entry.RouteValid ||
        !entry.Name || entry.Name[0] == '\0')
        return nullptr;
    return &entry;
}

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
        if (garage.Owns(id))
        {
            // Count only sanitized ownership, never unknown future SaveGame bits.
            ++view.OwnedVehicleCount;
        }
        entry.UnlockCost = spec.UnlockCost;
        entry.CreditsShort = garage.Owns(id)
            ? 0 : std::max(0, spec.UnlockCost - view.Credits);
        entry.Ownership = id == view.SelectedVehicle
            ? GarageCatalogOwnership::Equipped
            : garage.Owns(id) ? GarageCatalogOwnership::Owned
            : garage.CanUnlock(id) ? GarageCatalogOwnership::LockedAffordable
                                   : GarageCatalogOwnership::LockedInsufficientCredits;

        entry.IsNextUnlock = id == view.NextUnlockId;
        // Single authority for both cumulative costs and projected
        // performance. Do not duplicate the shared upgrade multipliers
        // or assume that Q can directly buy any locked catalogue entry.
        const auto route = PlanGaragePurchasePath(garage.Snapshot(),
            {id, view.EngineLevel, view.GripLevel, 0});
        if (route.Status != GaragePlanStatus::Invalid)
        {
            entry.RouteValid = true;
            entry.RouteCars = route.AdditionalCarsUnlocked;
            entry.RouteCost = route.UnlockCost;
            // Zero reserve, bounded car prices, and no upgrade changes.
            entry.RouteCreditsShort = static_cast<int>(route.CreditsShort);
            entry.RouteBudgetReady = route.Status == GaragePlanStatus::Affordable;
            entry.Effective = route.Projected;
        }
    }
    return view;
}

// Single shared HUD cache for the comparison, vehicle cards and textual
// catalogue. It never stores a purchase authorization or appearance state.
class GarageCatalogCache final
{
public:
    const GarageCatalogView& Read(GarageSnapshot saved) noexcept
    {
        GarageState garage;
        garage.Load(saved);
        const auto current=garage.Snapshot();
        if(!Initialized || current.Credits!=Key.Credits ||
            current.OwnedVehicleMask!=Key.OwnedVehicleMask ||
            current.SelectedVehicle!=Key.SelectedVehicle ||
            current.EngineLevel!=Key.EngineLevel || current.GripLevel!=Key.GripLevel)
        {
            Key=current;
            Cached=BuildGarageCatalogView(current);
            Initialized=true;
            if(Rebuilds!=std::numeric_limits<std::uint64_t>::max())++Rebuilds;
        }
        return Cached;
    }

    std::uint64_t RebuildCount() const noexcept {return Rebuilds;}
    void Reset() noexcept {Initialized=false;Key={};Cached={};Rebuilds=0;}

private:
    bool Initialized=false;
    GarageSnapshot Key{};
    GarageCatalogView Cached{};
    std::uint64_t Rebuilds=0;
};
} // namespace latebrake
