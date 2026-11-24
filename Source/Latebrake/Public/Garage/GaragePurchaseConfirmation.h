#pragma once

#include "Garage/GarageState.h"

#include <cmath>

namespace latebrake
{
// Deliberately transient: no SaveGame field or reward/credit authority.
// A second identical action, within the deadline and against an unchanged
// garage snapshot, is necessary before the GameMode may call a mutator.
enum class GaragePurchaseKind
{
    None, UnlockVehicle, UpgradeEngine, UpgradeGrip
};

struct GaragePendingPurchaseView final
{
    GaragePurchaseKind Kind = GaragePurchaseKind::None;
    int Subject = -1;
    int Price = 0;
    double SecondsRemaining = 0.0;

    bool IsPending() const noexcept { return Kind != GaragePurchaseKind::None; }
};

class GaragePurchaseConfirmation final
{
public:
    static constexpr double ConfirmWindowSeconds = 8.0;

    void Reset() noexcept
    {
        Kind = GaragePurchaseKind::None;
        Subject = -1;
        Price = 0;
        OfferedAt = 0.0;
        Saved = {};
    }

    bool HasPending() const noexcept
    {
        return Kind != GaragePurchaseKind::None;
    }

    // Read-only, real-monotonic-time offer display. The UI must never obtain
    // authorization by observing an offer; only ConfirmOrStage may consume
    // matching, non-expired pending approval on a second explicit key press.
    // Snapshot edits, invalid clocks, expiry and clock rollback fail closed.
    GaragePendingPurchaseView Preview(
        GarageSnapshot current, double nowSeconds) const noexcept
    {
        if (!HasPending() || !std::isfinite(nowSeconds) ||
            !SameSnapshot(current, Saved))
        {
            return {};
        }
        const double elapsed = nowSeconds - OfferedAt;
        if (!std::isfinite(elapsed) ||
            elapsed < 0.0 || elapsed > ConfirmWindowSeconds)
        {
            return {};
        }
        return {Kind, Subject, Price, ConfirmWindowSeconds - elapsed};
    }

    // True means authorization to ATTEMPT the existing GarageState
    // transaction once. The caller must still check edit eligibility,
    // price, ownership and then apply the existing SaveGame rollback.
    bool ConfirmOrStage(GaragePurchaseKind kind, GarageSnapshot snapshot,
                        int subject, int price, double nowSeconds) noexcept
    {
        if (kind == GaragePurchaseKind::None ||
            !std::isfinite(nowSeconds) || nowSeconds < 0.0 ||
            subject < 0 || price <= 0 ||
            snapshot.Credits < price || snapshot.Credits < 0 ||
            snapshot.EngineLevel < 0 || snapshot.EngineLevel > 3 ||
            snapshot.GripLevel < 0 || snapshot.GripLevel > 3)
        {
            Reset();
            return false;
        }
        const double elapsed = nowSeconds - OfferedAt;
        if (HasPending() &&
            kind == Kind && subject == Subject && price == Price &&
            SameSnapshot(snapshot, Saved) && std::isfinite(elapsed) &&
            elapsed >= 0.0 && elapsed <= ConfirmWindowSeconds)
        {
            Reset(); // Never let a third press reuse one purchase approval.
            return true;
        }

        // A different key, cost, credits or expired offer replaces the
        // old pending approval but can never silently debit the garage.
        Kind = kind;
        Subject = subject;
        Price = price;
        Saved = snapshot;
        OfferedAt = nowSeconds;
        return false;
    }

private:
    static bool SameSnapshot(GarageSnapshot a, GarageSnapshot b) noexcept
    {
        return a.Credits == b.Credits &&
            a.OwnedVehicleMask == b.OwnedVehicleMask &&
            a.SelectedVehicle == b.SelectedVehicle &&
            a.EngineLevel == b.EngineLevel &&
            a.GripLevel == b.GripLevel;
    }

    GaragePurchaseKind Kind = GaragePurchaseKind::None;
    int Subject = -1;
    int Price = 0;
    double OfferedAt = 0.0;
    GarageSnapshot Saved{};
};
} // namespace latebrake
