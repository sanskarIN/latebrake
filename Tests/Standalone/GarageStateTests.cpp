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
    Require(!garage.Owns(-1) && !garage.Owns(3), "reject unknown vehicle");
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
    Require(!maxed.Unlock(3) && !maxed.Select(-1), "invalid id operations rejected");

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

    std::cout << "Garage economy and profile tests passed.\n";
    return EXIT_SUCCESS;
}
