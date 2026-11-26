#pragma once
#include "Garage/GarageState.h"
#include "UI/MenuNavigation.h"

namespace latebrake
{
// UI-only decision for the existing Unreal showroom's SELECT button.
// Pure and deterministic; it never equips/unlocks a car or writes SaveGame.
// The authoritative GameMode still verifies permissions and performs the
// durable, rollback-safe Garage.Select transaction.
enum class GarageEquipUiState
{
    NotInGarage, Suspended, ReadOnly, CareerLocked,
    InvalidVehicle, LockedVehicle, Equipped, Ready
};

// Shared permission for paid and equipment actions; ordinary menu pause
// is not external/manual suspension.
constexpr GarageEquipUiState EvaluateGarageMutationUi(
    bool menuOpen, MenuPage page, bool suspended,
    bool newerSave, bool careerAllowsChanges) noexcept
{
    if (!menuOpen || page != MenuPage::Garage)
        return GarageEquipUiState::NotInGarage;
    if (suspended) return GarageEquipUiState::Suspended;
    if (newerSave) return GarageEquipUiState::ReadOnly;
    if (!careerAllowsChanges) return GarageEquipUiState::CareerLocked;
    return GarageEquipUiState::Ready;
}

constexpr GarageEquipUiState EvaluateGarageEquipUi(
    bool menuOpen, MenuPage page, bool suspended,
    bool newerSave, bool careerAllowsChanges,
    bool ownsViewed, int viewedId, int equippedId) noexcept
{
    const auto access = EvaluateGarageMutationUi(
        menuOpen, page, suspended, newerSave, careerAllowsChanges);
    if (access != GarageEquipUiState::Ready) return access;
    if (!GarageState::IsVehicle(viewedId) ||
        !GarageState::IsVehicle(equippedId))
        return GarageEquipUiState::InvalidVehicle;
    if (!ownsViewed) return GarageEquipUiState::LockedVehicle;
    return viewedId == equippedId
        ? GarageEquipUiState::Equipped : GarageEquipUiState::Ready;
}

constexpr bool CanActivateGarageEquip(GarageEquipUiState state) noexcept
{
    return state == GarageEquipUiState::Ready;
}

// All captions fit the existing two-column desktop menu button. No false
// "SELECT" CTA is shown for a locked or already-equipped showroom car.
constexpr const char* GarageEquipUiCaption(
    GarageEquipUiState state) noexcept
{
    switch (state)
    {
    case GarageEquipUiState::Ready: return "EQUIP CAR";
    case GarageEquipUiState::Equipped: return "EQUIPPED";
    case GarageEquipUiState::LockedVehicle: return "LOCKED CAR";
    case GarageEquipUiState::InvalidVehicle: return "INVALID CAR";
    case GarageEquipUiState::ReadOnly: return "NEWER SAVE";
    case GarageEquipUiState::Suspended: return "SUSPENDED";
    case GarageEquipUiState::CareerLocked: return "CAREER LOCK";
    case GarageEquipUiState::NotInGarage: return "NO GARAGE";
    }
    return "NO GARAGE";
}
constexpr const char* GarageCycleUiCaption(GarageEquipUiState state) noexcept
{
    if (state == GarageEquipUiState::Ready) return "NEXT CAR";
    if (state == GarageEquipUiState::Equipped) return "NO OTHER CAR";
    return GarageEquipUiCaption(state);
}

static_assert(CanActivateGarageEquip(EvaluateGarageEquipUi(
    true, MenuPage::Garage, false, false, true, true, 2, 0)));
static_assert(!CanActivateGarageEquip(EvaluateGarageEquipUi(
    true, MenuPage::Garage, false, false, true, false, 2, 0)));
static_assert(!CanActivateGarageEquip(EvaluateGarageEquipUi(
    true, MenuPage::Garage, false, true, true, true, 2, 0)));
} // namespace latebrake
