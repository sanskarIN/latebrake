#include "Garage/GaragePurchaseConfirmation.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool yes, const char* message)
{
    if (!yes)
    {
        std::cerr << "Garage confirmation: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    GarageState garage;
    garage.EarnCredits(3000);
    GaragePurchaseConfirmation gate;
    GarageSnapshot before = garage.Snapshot();
    const int price = GarageState::Definition(1).UnlockCost;
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        before, 1, price, 10.0), "first press only offers");
    Require(gate.HasPending() && !garage.Owns(1) &&
        garage.Credits() == 3000, "preview changes no currency or ownership");
    const GaragePendingPurchaseView firstPreview = gate.Preview(before, 10.0);
    Require(firstPreview.IsPending() &&
        firstPreview.Kind == GaragePurchaseKind::UnlockVehicle &&
        firstPreview.Subject == 1 && firstPreview.Price == price &&
        firstPreview.SecondsRemaining == 8.0,
        "initial countdown reflects exactly eight seconds, without spending");
    Require(gate.Preview(before, 12.25).IsPending() &&
        gate.Preview(before, 12.25).SecondsRemaining == 5.75,
        "remaining time is read-only and uses monotonic elapsed seconds");
    Require(gate.Preview(before, 18.0).IsPending() &&
        gate.Preview(before, 18.0).SecondsRemaining == 0.0,
        "deadline is inclusive, matching ConfirmOrStage exactly");
    Require(!gate.Preview(before, 18.0001).IsPending() &&
        !gate.Preview(before, 9.999).IsPending(),
        "expired and regressing clocks hide the confirmation affordance");
    auto changedBefore = before;
    changedBefore.Credits += 1;
    Require(!gate.Preview(changedBefore, 11.0).IsPending(),
        "credit changes hide an offer that can no longer authorize");
    changedBefore = before;
    changedBefore.SelectedVehicle = 2;
    Require(!gate.Preview(changedBefore, 11.0).IsPending(),
        "selection changes hide a stale offer");
    changedBefore = before;
    changedBefore.OwnedVehicleMask |= 4;
    Require(!gate.Preview(changedBefore, 11.0).IsPending(),
        "ownership changes hide an old offer");
    changedBefore = before;
    changedBefore.EngineLevel = 1;
    Require(!gate.Preview(changedBefore, 11.0).IsPending(),
        "upgrade changes hide an offer");
    for (double badTime : {std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity(), -1.0})
    {
        Require(!gate.Preview(before, badTime).IsPending(),
            "nonfinite or negative inspection time cannot expose a purchase");
    }
    Require(gate.HasPending() && !garage.Owns(1) &&
        garage.Credits() == 3000, "observation does not consume or mutate an offer");
    Require(gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        before, 1, price, 11.0), "same second press authorizes");
    Require(!gate.HasPending(), "authorization consumed exactly once");
    Require(!gate.Preview(before, 11.0).IsPending(),
        "used authorization cannot remain visible in the HUD");
    Require(garage.Unlock(1) && garage.Owns(1) &&
        garage.Credits() == 2400, "underlying garage still spends exactly once");
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        garage.Snapshot(), 1, price, 11.1), "third click not authorized");

    gate.Reset();
    const auto fresh = garage.Snapshot();
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeEngine,
        fresh, 0, 200, 20.0), "engine first key stages");
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeGrip,
        fresh, 0, 150, 21.0), "other purchase cannot confirm pending engine");
    Require(gate.Preview(fresh, 21.0).IsPending() &&
        gate.Preview(fresh, 21.0).Kind == GaragePurchaseKind::UpgradeGrip &&
        gate.Preview(fresh, 21.0).Price == 150,
        "switching a preview key changes the displayed offer instead of charging");
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeEngine,
        fresh, 0, 200, 21.2), "switching back starts new engine offer");
    Require(gate.ConfirmOrStage(GaragePurchaseKind::UpgradeEngine,
        fresh, 0, 200, 22.0), "new engine second press confirms");
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeEngine,
        fresh, 0, 200, 22.1), "double-click not multiple purchases");
    gate.Reset();
    Require(!gate.HasPending(), "explicit cancel");

    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        fresh, 2, 1200, 100.0), "stage expensive car");
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        fresh, 2, 1200, 108.001), "expired offer never confirms");
    Require(gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        fresh, 2, 1200, 108.002), "subsequent second press works");
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        fresh, 2, 1200, 120.0), "new offer for changed time");
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        fresh, 2, 1200, 119.0), "clock moving backward cannot authorize");
    Require(gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        fresh, 2, 1200, 120.0), "fresh clock after reset can confirm");

    gate.Reset();
    auto changed = fresh;
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeGrip,
        fresh, 0, 150, 200.0), "new upgrade staged");
    changed.Credits += 10;
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeGrip,
        changed, 0, 150, 201.0), "credits changed invalidates offer");
    changed = fresh;
    changed.GripLevel = 2;
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeGrip,
        changed, 0, 150, 202.0), "tuning changed invalidates offer");
    changed = fresh;
    changed.SelectedVehicle = 2;
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeGrip,
        changed, 0, 150, 203.0), "car selection invalidates offer");
    changed = fresh;
    changed.OwnedVehicleMask = 7;
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeGrip,
        changed, 0, 150, 204.0), "ownership changed invalidates offer");
    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(), -1.0})
    {
        Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeGrip,
            fresh, 0, 150, bad) && !gate.HasPending(),
            "invalid clock fails closed");
    }
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::None,
        fresh, 0, 150, 205.0) && !gate.HasPending(),
        "none action never grants a purchase");
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeGrip,
        fresh, 0, 0, 205.0) && !gate.HasPending(),
        "sold out upgrade is not staged");
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        fresh, -1, 150, 205.0) && !gate.HasPending(),
        "invalid vehicle rejected");
    Require(!gate.ConfirmOrStage(GaragePurchaseKind::UnlockVehicle,
        fresh, 2, 9000, 205.0) && !gate.HasPending(),
        "unaffordable purchase rejected");

    // A focus interruption resets approval even if no snapshot/time changes.
    // Verify every spend kind, duplicate interruption delivery, and the
    // exact former deadline. A returning player's first press is a preview.
    struct FocusCase { GaragePurchaseKind Kind; int Subject; int Price; };
    const FocusCase focusCases[] = {
        {GaragePurchaseKind::UnlockVehicle, 2, 1200},
        {GaragePurchaseKind::UpgradeEngine, 0, 200},
        {GaragePurchaseKind::UpgradeGrip, 0, 150},
    };
    for (const auto& entry : focusCases)
    {
        for (const double returnTime : {500.0, 504.0, 508.0})
        {
            gate.Reset();
            Require(!gate.ConfirmOrStage(entry.Kind, fresh, entry.Subject,
                entry.Price, 500.0), "offer before interruption only stages");
            Require(gate.Preview(fresh, returnTime).IsPending(),
                "old offer would otherwise still be actionable");
            gate.Reset();
            gate.Reset(); // Duplicate OS deactivation must remain harmless.
            Require(!gate.HasPending() && !gate.Preview(fresh, returnTime).IsPending(),
                "interruption removes the old approval and countdown");
            Require(!gate.ConfirmOrStage(entry.Kind, fresh, entry.Subject,
                entry.Price, returnTime), "first press after interruption restages");
            Require(gate.ConfirmOrStage(entry.Kind, fresh, entry.Subject,
                entry.Price, returnTime + 0.25), "fresh second press authorizes once");
            Require(!gate.HasPending(), "new approval is consumed");
            Require(garage.Credits() == fresh.Credits &&
                garage.Snapshot().OwnedVehicleMask == fresh.OwnedVehicleMask &&
                garage.EngineLevel() == fresh.EngineLevel &&
                garage.GripLevel() == fresh.GripLevel,
                "approval policy does not mutate the garage");
        }
    }

    // Deterministic small-step state changes: no empty, old or altered
    // approval can authorize spending, even after thousands of inputs.
    for (int i = 0; i < 10000; ++i)
    {
        gate.Reset();
        const double t = 300.0 + static_cast<double>(i);
        Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeEngine,
            fresh, 0, 200, t), "new state never immediate debit");
        Require(!gate.ConfirmOrStage(GaragePurchaseKind::UpgradeEngine,
            fresh, 1, 200, t + 0.1), "wrong target cannot confirm");
        Require(gate.ConfirmOrStage(GaragePurchaseKind::UpgradeEngine,
            fresh, 1, 200, t + 0.2), "correct reoffer confirms");
    }
    std::cout << "Garage purchase confirmation passed.\n";
    return EXIT_SUCCESS;
}
