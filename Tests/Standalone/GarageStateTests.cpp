#include "Garage/GarageState.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

void Require(bool good, const char* msg)
{
    if (!good)
    {
        std::cerr << "Garage FAIL: " << msg << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    GarageState garage;
    Require(garage.Owns(0) && !garage.Owns(1), "original vehicle is free and owned");
    Require(!garage.Owns(-1) && !garage.Owns(VehicleCount), "reject unknown vehicle");
    Require(!garage.Select(1), "locked vehicle cannot be selected");
    Require(!garage.Unlock(1), "cannot unlock without earned credits");
    Require(garage.Credits() == 0, "initial balance empty");
    garage.EarnCredits(-50);
    garage.EarnCredits(0);
    Require(garage.Credits() == 0, "negative credit reward rejected");
    garage.EarnCredits(900);
    Require(garage.Unlock(1) && garage.Credits() == 300, "first vehicle unlock debits credits");
    Require(!garage.Unlock(1) && garage.Credits() == 300, "no repeated purchase charge");
    Require(garage.Select(1) && garage.SelectedVehicle() == 1, "select owned vehicle");
    Require(garage.Definition(garage.SelectedVehicle()).Id == 1, "selected spec correct");

    Require(garage.Upgrade(UpgradeKind::Engine) && garage.Credits() == 100,
            "engine level one costs 200");
    Require(!garage.Upgrade(UpgradeKind::Engine), "insufficient engine funds rejected");
    Require(!garage.Upgrade(UpgradeKind::Grip), "insufficient grip funds rejected");
    garage.EarnCredits(1200);
    Require(garage.Upgrade(UpgradeKind::Grip), "buy initial grip upgrade");
    Require(garage.GripLevel() == 1, "grip level increases");
    const VehiclePerformance tuned = garage.Performance();
    Require(tuned.Acceleration > garage.Definition(1).Acceleration, "engine upgrade changes acceleration");
    Require(tuned.Grip > garage.Definition(1).Grip, "grip upgrade improves grip");
    Require(tuned.TopSpeed > garage.Definition(1).TopSpeed, "engine upgrade improves speed");

    const auto saved = garage.Snapshot();
    GarageState restored;
    restored.Load(saved);
    Require(restored.SelectedVehicle() == 1 && restored.Credits() == garage.Credits(),
            "garage snapshot can be restored");
    Require(restored.EngineLevel() == 1 && restored.GripLevel() == 1,
            "upgrade levels restored");
    restored.Load({-99, 0, 2, 50, -2});
    Require(restored.Credits() == 0 && restored.SelectedVehicle() == 0 &&
            restored.EngineLevel() == 3 && restored.GripLevel() == 0,
            "corrupt save clamps credits, selection and upgrades");
    Require(restored.Owns(0), "starter stays owned after invalid save");

    GarageState maxed;
    maxed.Load({std::numeric_limits<int>::max() - 1, 7, 2, 3, 3});
    maxed.EarnCredits(10000);
    Require(maxed.Credits() == std::numeric_limits<int>::max(), "credit overflow saturates");
    Require(!maxed.Upgrade(UpgradeKind::Engine), "engine capped at level three");
    Require(!maxed.Upgrade(UpgradeKind::Grip), "grip capped at level three");
    Require(!maxed.Unlock(2), "owned vehicle cannot be repurchased");
    Require(!maxed.Unlock(VehicleCount) && !maxed.Select(-1), "invalid id operations rejected");

    GarageState upgrades;
    upgrades.EarnCredits(3000);
    Require(upgrades.Upgrade(UpgradeKind::Engine), "engine level one");
    Require(upgrades.Upgrade(UpgradeKind::Engine), "engine level two");
    Require(upgrades.Upgrade(UpgradeKind::Engine), "engine level three");
    Require(!upgrades.Upgrade(UpgradeKind::Engine), "engine max enforced");
    Require(upgrades.Upgrade(UpgradeKind::Grip), "grip level one");
    Require(upgrades.Upgrade(UpgradeKind::Grip), "grip level two");
    Require(upgrades.Upgrade(UpgradeKind::Grip), "grip level three");
    Require(!upgrades.Upgrade(UpgradeKind::Grip), "grip max enforced");
    Require(GarageState::UpgradeCost(UpgradeKind::Engine, 3) == 0,
            "max upgrade price is zero");
    Require(GarageState::Definition(999).Id == 0, "invalid definition falls back safely");

    // Fictional prototype silhouettes remain bounded and have distinct cabins.
    const auto arrow = GarageState::Definition(0);
    const auto kestrel = GarageState::Definition(1);
    const auto phantom = GarageState::Definition(2);
    Require(arrow.CabinLength > 0.5 && arrow.CabinHeight > 0.5,
            "starter cabin dimensions positive");
    Require(kestrel.CabinLength > arrow.CabinLength, "kestrel has a longer canopy");
    Require(phantom.CabinHeight > arrow.CabinHeight, "phantom has a higher canopy");
    Require(phantom.CabinLength < arrow.CabinLength, "phantom has shorter canopy");
    // v0.3.1: the UI and game-mode share portable transaction selection
    // rules; invalid masks can no longer accidentally unlock every vehicle.
    GarageState chooser;
    Require(chooser.NextLockedVehicle() == 1, "first locked car is Kestrel");
    Require(chooser.NextOwnedVehicle() == 0, "single owned car cycles to itself");
    Require(!chooser.CanUnlock(1) && !chooser.CanUnlock(-1), "affordability validated");
    chooser.EarnCredits(600);
    Require(chooser.CanUnlock(1), "enough credits unlocks first vehicle");
    Require(chooser.Unlock(1), "unlock first vehicle");
    Require(chooser.NextLockedVehicle() == 2, "second unlock is Phantom");
    Require(chooser.NextOwnedVehicle() == 1, "next owned car cycles predictably");
    Require(chooser.Select(1), "select Kestrel for cycling");
    Require(chooser.NextOwnedVehicle() == 0, "cycle wraps to Arrow");
    Require(chooser.NextUpgradeCost(UpgradeKind::Engine) == 200, "engine preview cost");
    Require(chooser.NextUpgradeCost(UpgradeKind::Grip) == 150, "grip preview cost");
    Require(chooser.NextUpgradeCost(static_cast<UpgradeKind>(99)) == 0,
            "unknown upgrade cannot be quoted");
    chooser.Load({700, -1, 2, 1, 2});
    Require(!chooser.Owns(1) && !chooser.Owns(2),
            "negative ownership mask never grants free unlocks");
    Require(chooser.SelectedVehicle() == 0 && chooser.Credits() == 700,
            "corrupt ownership restores free car while preserving credits");
    // Backward-compatible 3-car save masks must NOT auto-grant appended cars.
    chooser.Load({10000, 7, 2, 3, 3});
    Require(chooser.Owns(0) && chooser.Owns(1) && chooser.Owns(2) &&
            !chooser.Owns(3) && !chooser.Owns(4) && !chooser.Owns(5),
            "legacy three-car ownership survives without free new unlocks");
    Require(chooser.NextLockedVehicle() == 3, "legacy mask sees Tempest next");
    Require(chooser.Unlock(3) && chooser.Credits() == 8200,
            "Tempest costs 1800 earned credits");
    Require(chooser.NextLockedVehicle() == 4 && chooser.Unlock(4) &&
            chooser.Credits() == 5700, "Glider costs 2500 earned credits");
    Require(chooser.NextLockedVehicle() == 5 && chooser.Unlock(5) &&
            chooser.Credits() == 2300, "Nova costs 3400 earned credits");
    Require(chooser.NextLockedVehicle() == -1, "all six vehicles now unlocked");
    Require(chooser.Select(5) && chooser.NextOwnedVehicle() == 0,
            "six-car selection wraps from Nova to starter");
    GarageState allRestored;
    allRestored.Load(chooser.Snapshot());
    Require(allRestored.Owns(5) && allRestored.SelectedVehicle() == 5 &&
            allRestored.Credits() == 2300,
            "all new owned vehicle bits and selected ID survive restore");
    Require(chooser.NextUpgradeCost(UpgradeKind::Engine) == 0,
            "maxed engine has no upgrade price");
    Require(chooser.NextUpgradeCost(UpgradeKind::Grip) == 0,
            "maxed grip has no upgrade price");
    GarageState future;
    future.Load({1500, 0x7FFFFFFF, 99, 0, 0});
    Require(future.Snapshot().OwnedVehicleMask == KnownVehicleMask &&
            future.SelectedVehicle() == 0,
            "future/unknown ownership bits removed and invalid selection rejected");
    future.Load({1500, -1, 5, 0, 0});
    Require(future.Snapshot().OwnedVehicleMask == 1 && !future.Owns(5) &&
            future.SelectedVehicle() == 0,
            "negative mask never becomes six free purchased cars");
    for (int id = 0; id < VehicleCount; ++id)
    {
        const auto spec = GarageState::Definition(id);
        Require(spec.Id == id && spec.UnlockCost >= 0 &&
                std::isfinite(spec.Acceleration) && spec.Acceleration > 0.0 &&
                std::isfinite(spec.Grip) && spec.Grip > 0.0 &&
                std::isfinite(spec.TopSpeed) && spec.TopSpeed > 0.0,
                "all six specs are finite and addressable by stable IDs");
    }
    Require(GarageState::Definition(3).UnlockCost == 1800 &&
            GarageState::Definition(4).UnlockCost == 2500 &&
            GarageState::Definition(5).UnlockCost == 3400,
            "new appended unlock costs stable");
    std::cout << "Garage economy and profile tests passed.\n";
    return EXIT_SUCCESS;
}
