#include "Vehicles/ArcadeCollisionDamage.h"
#include "Vehicles/ArcadeTireCondition.h"
#include "Vehicles/ArcadeNitroBoost.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
void Check(bool ok, const char* reason)
{
    if (!ok) throw std::runtime_error(reason);
}
}

int main()
{
    using latebrake::ArcadeCollisionDamage;
    using latebrake::ArcadeTireCondition;
    using latebrake::ArcadeNitroBoost;

    ArcadeCollisionDamage chassis;
    Check(chassis.Advance(0.05, true), "arm real collision policy");
    Check(chassis.ObserveImpact({1100000.0, 1100.0, 3800.0, 0.1, true}),
          "genuine hard collision earns finite damage");
    const double beforeRepair = chassis.DamageFraction();
    const unsigned impacts = chassis.ImpactEventCount();
    Check(beforeRepair > 0.0, "must start damaged");
    chassis.RepairAtPit();
    Check(chassis.DamageFraction() < beforeRepair, "pit reduces chassis damage");
    Check(chassis.DamageFraction() >= 0.0, "repair cannot make negative damage");
    Check(chassis.ImpactEventCount() == impacts,
          "repair does not erase recorded incident history");
    Check(!chassis.HasEligibleFrame(), "post-service contact requires new race tick");
    chassis.RepairAtPit();
    Check(chassis.DamageFraction() == 0.0, "repair cannot over-heal");
    Check(chassis.MotorMultiplier() == 1.0, "healthy motor restored");

    ArcadeTireCondition tires;
    for (int i = 0; i < 250; ++i)
        Check(tires.Observe({0.05, 3800.0, 1900.0, true, true}),
              "finite chassis slip can wear tires");
    Check(tires.WearFraction() > 0.03, "tires accumulated nontrivial wear");
    tires.ReplaceAtPit();
    Check(tires.WearFraction() == 0.0,
          "pit restores race-local synthetic tire condition");
    Check(std::abs(tires.TemperatureFraction() - 0.42) < 1e-12,
          "pit installs baseline tire-temperature policy");
    Check(!tires.HasValidSample(), "post-service condition needs live reading");
    Check(tires.GripMultiplier() == 1.0, "new tire baseline grip");

    ArcadeNitroBoost nitro;
    nitro.Press(true);
    for (int i = 0; i < 20; ++i)
        nitro.Step(0.05, true, false, 1.0, 1800.0);
    const double beforeRefill = nitro.ChargeFraction();
    Check(beforeRefill < 1.0 && beforeRefill > 0.0, "live boost has consumed charge");
    nitro.RefillAtPit();
    Check(nitro.ChargeFraction() >= beforeRefill, "pit never drains charge");
    Check(nitro.ChargeFraction() <= 1.0, "charge cannot exceed full");
    Check(nitro.ChargeFraction() - beforeRefill <= 0.400000001,
          "pit replenishment is bounded to forty percent");
    Check(!nitro.IsHeld() && !nitro.IsActive(),
          "held nitro input is cleared after service");
    Check(nitro.CooldownRemainingSeconds() == 0.0, "pit clears nitro cooldown");
    Check(nitro.HeatFraction() >= 0.0 && nitro.HeatFraction() <= 1.0,
          "pit cooling is bounded");
    for (int i = 0; i < 100; ++i) nitro.RefillAtPit();
    Check(nitro.ChargeFraction() == 1.0, "no charge overflow on repeated service");

    std::cout << "RacePitVehicleService: all scenarios passed\n";
    return 0;
}
