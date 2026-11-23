#pragma once

#include "Garage/GarageState.h"

namespace latebrake
{
// View-only cursor over all six original Unreal prototype cars. Browsing a
// locked profile does not select it, unlock it, charge credits, apply physics
// or mutate SaveGame. The actual selected (owned) vehicle is separate.
class GarageGalleryCursor final
{
public:
    constexpr void Reset(int currentId) noexcept
    {
        ViewedId = GarageState::IsVehicle(currentId) ? currentId : 0;
    }

    constexpr int Viewed() const noexcept { return ViewedId; }

    constexpr int Next() noexcept
    {
        ViewedId = (ViewedId + 1) % VehicleCount;
        return ViewedId;
    }

    constexpr int Previous() noexcept
    {
        ViewedId = (ViewedId + VehicleCount - 1) % VehicleCount;
        return ViewedId;
    }

    constexpr bool IsEquipped(int selectedId) const noexcept
    {
        return ViewedId == selectedId;
    }

    // Deliberately never returns permission to purchase or equip a car.
    constexpr bool IsValid() const noexcept
    {
        return GarageState::IsVehicle(ViewedId);
    }

private:
    int ViewedId = 0;
};
// Only this eligibility query can be used by the GameMode to authorize
// equipping the currently viewed car. It reads the trusted GarageState
// owned mask rather than assigning ownership from a pointer/keyboard ID.
inline bool CanEquipGalleryVehicle(
    const GarageState& garage, const GarageGalleryCursor& gallery) noexcept
{
    return gallery.IsValid() && garage.Owns(gallery.Viewed());
}
// Appearance editing can target any OWNED browsed car without equipping it.
// This is eligibility only: the caller must separately check menu focus,
// profile-schema compatibility, career restrictions and save rollback.
// An unowned showroom preview never grants the ability to edit a style.
inline bool CanStyleGalleryVehicle(
    const GarageState& garage, const GarageGalleryCursor& gallery) noexcept
{
    return gallery.IsValid() && garage.Owns(gallery.Viewed());
}
static_assert(VehicleCount == 6, "Review gallery/save ID migration");
} // namespace latebrake
