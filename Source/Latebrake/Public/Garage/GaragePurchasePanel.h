#pragma once

#include "Garage/GarageEquipUiPolicy.h"
#include "Garage/GaragePurchaseConfirmation.h"
#include <array>
#include <cmath>

namespace latebrake
{
// An observation of in-game earned credit offers. Only the existing
// GameMode handlers can stage or consume consent and persist a change.
enum class GarageOfferStatus
{
    Blocked, Complete, NeedCredits, Preview, Confirm
};

struct GaragePurchasePanelOffer final
{
    GaragePurchaseKind Kind = GaragePurchaseKind::None;
    GarageOfferStatus Status = GarageOfferStatus::Blocked;
    int Subject = -1; // next vehicle ID, or current upgrade level
    int Price = 0;
    int Shortfall = 0;
    double SecondsRemaining = 0.0;

    constexpr bool Enabled() const noexcept
    {
        return Status == GarageOfferStatus::Preview ||
               Status == GarageOfferStatus::Confirm;
    }
};

inline constexpr std::size_t GaragePurchaseOfferCount = 3;
using GaragePurchasePanelView = std::array<GaragePurchasePanelOffer,
    GaragePurchaseOfferCount>;

inline GaragePurchasePanelView BuildGaragePurchasePanel(
    GarageSnapshot saved, GarageEquipUiState access,
    GaragePendingPurchaseView pending) noexcept
{
    GarageState garage;
    garage.Load(saved);
    const int next = garage.NextLockedVehicle();
    GaragePurchasePanelView result{{
        {GaragePurchaseKind::UnlockVehicle, GarageOfferStatus::Blocked,
            next, next < 0 ? 0 : GarageState::Definition(next).UnlockCost},
        {GaragePurchaseKind::UpgradeEngine, GarageOfferStatus::Blocked,
            garage.EngineLevel(), garage.NextUpgradeCost(UpgradeKind::Engine)},
        {GaragePurchaseKind::UpgradeGrip, GarageOfferStatus::Blocked,
            garage.GripLevel(), garage.NextUpgradeCost(UpgradeKind::Grip)}
    }};
    for (auto& offer : result)
    {
        offer.Shortfall = std::max(0, offer.Price - garage.Credits());
        if (access != GarageEquipUiState::Ready) continue;
        if (offer.Price <= 0)
            offer.Status = GarageOfferStatus::Complete;
        else if (offer.Shortfall > 0)
            offer.Status = GarageOfferStatus::NeedCredits;
        else if (pending.Kind == offer.Kind &&
                 pending.Subject == offer.Subject &&
                 pending.Price == offer.Price &&
                 std::isfinite(pending.SecondsRemaining) &&
                 pending.SecondsRemaining >= 0.0 &&
                 pending.SecondsRemaining <= GaragePurchaseConfirmation::ConfirmWindowSeconds)
        {
            offer.Status = GarageOfferStatus::Confirm;
            offer.SecondsRemaining = pending.SecondsRemaining;
        }
        else
            offer.Status = GarageOfferStatus::Preview;
    }
    return result;
}
} // namespace latebrake
