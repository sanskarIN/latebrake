#pragma once

#include "Garage/GarageState.h"

namespace latebrake
{
// A read-only decision for the six physical keyboard garage shortcuts.
// Selection is ONLY committed by GameMode using the existing SaveGame
// checkpoint. This helper never grants ownership or spends credits.
enum class GarageQuickSelectStatus
{
    MenuClosed,
    WrongPage,
    CareerRaceLocked,
    InvalidVehicle,
    NotOwned,
    AlreadyEquipped,
    Ready
};

struct GarageQuickSelectDecision final
{
    GarageQuickSelectStatus Status = GarageQuickSelectStatus::MenuClosed;
    int VehicleId = -1;
};

inline GarageQuickSelectDecision PreviewGarageQuickSelect(
    GarageSnapshot saved, int requestedVehicle,
    bool menuOpen, bool garagePage, bool canEditGarage) noexcept
{
    if (!menuOpen)
        return {GarageQuickSelectStatus::MenuClosed, -1};
    if (!garagePage)
        return {GarageQuickSelectStatus::WrongPage, -1};
    if (!canEditGarage)
        return {GarageQuickSelectStatus::CareerRaceLocked, -1};
    if (!GarageState::IsVehicle(requestedVehicle))
        return {GarageQuickSelectStatus::InvalidVehicle, -1};

    // Load uses the same corruption recovery as the real saved garage.
    // This is a *copy*: no mutation, purchase, upgrade or save write.
    GarageState observed;
    observed.Load(saved);
    if (!observed.Owns(requestedVehicle))
        return {GarageQuickSelectStatus::NotOwned, requestedVehicle};
    if (observed.SelectedVehicle() == requestedVehicle)
        return {GarageQuickSelectStatus::AlreadyEquipped, requestedVehicle};
    return {GarageQuickSelectStatus::Ready, requestedVehicle};
}
} // namespace latebrake
