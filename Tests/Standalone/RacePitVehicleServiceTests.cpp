#include "Vehicles/ArcadeCollisionDamage.h"
#include "Vehicles/ArcadeTireCondition.h"
#include "Vehicles/ArcadeNitroBoost.h"
#include "Racing/PitServiceJournal.h"
#include <limits>

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

    // Exact real model snapshots: the journal itself never invokes service.
    latebrake::PitServiceJournal journal;
    ArcadeCollisionDamage observedChassis;
    ArcadeTireCondition observedTires;
    ArcadeNitroBoost observedBoost;
    Check(observedChassis.Advance(0.05,true),"live impact admission");
    Check(observedChassis.ObserveImpact({1100000.0,1100.0,3800.0,0.1,true}),
        "actual damage before service");
    for(int i=0;i<100;++i)
        Check(observedTires.Observe({0.05,3800.0,1900.0,true,true}),"live wear");
    observedBoost.Press(true);
    for(int i=0;i<20;++i) observedBoost.Step(0.05,true,false,1.0,1800.0);
    const auto before=latebrake::ObservePitVehicleCondition(
        observedChassis,observedTires,observedBoost);
    observedChassis.RepairAtPit();
    observedTires.ReplaceAtPit();
    observedBoost.RefillAtPit();
    const auto after=latebrake::ObservePitVehicleCondition(
        observedChassis,observedTires,observedBoost);
    Check(journal.RecordCompleted(1,1,12.0,before,after),"record actual service");
    const auto receipt=journal.GetRecent(0);
    const auto summary=journal.Review(1);
    Check(summary.Valid && summary.Count==1 && receipt.Stop==1 &&
        receipt.Lap==1 && receipt.RaceSeconds==12.0 &&
        receipt.DamageRestored==before.Damage-after.Damage &&
        receipt.WearRemoved==before.Wear-after.Wear &&
        receipt.ChargeRestored==after.Charge-before.Charge &&
        receipt.BoostHeatRemoved==before.BoostHeat-after.BoostHeat,
        "receipt reports observed state deltas, not assumed full restoration");
    Check(!journal.Review(2).Valid,"authority count mismatch hides totals");
    Check(!journal.RecordCompleted(1,1,13.0,before,after),"duplicate receipt");
    Check(!journal.Review(1).Valid && journal.RetainedCount()==0,
        "bad receipt invalidates complete history");

    journal.Reset();
    const latebrake::PitVehicleCondition a{0.5,0.25,0.25,0.5};
    const latebrake::PitVehicleCondition b{0.25,0.0,0.5,0.25};
    for(int i=1;i<=12;++i)
        Check(journal.RecordCompleted(i,(i+1)/2,10.0*i,a,b),"ordered visits");
    const auto totals=journal.Review(12);
    Check(totals.Valid && totals.Count==12 &&
        totals.DamageRestored==3.0 && totals.WearRemoved==3.0 &&
        totals.ChargeRestored==3.0 && totals.BoostHeatRemoved==3.0,
        "bounded retention does not truncate valid cumulative totals");
    Check(journal.RetainedCount()==8 && journal.GetRecent(0).Stop==12 &&
        journal.GetRecent(7).Stop==5 && journal.GetRecent(8).Stop==0 &&
        journal.GetRecent(-1).Stop==0,"ring retains exact newest eight");
    Check(!journal.RecordCompleted(13,6,120.0,a,b),"duplicate clock");
    Check(!journal.Review(12).Valid,"clock regression invalidates report");
    journal.Reset();
    Check(!journal.RecordCompleted(2,1,10.0,a,b),"missing first receipt");
    journal.Reset();
    Check(!journal.RecordCompleted(1,1,10.0,b,a),"wrong restoration direction");
    journal.Reset();
    auto bad=a; bad.Damage=std::numeric_limits<double>::quiet_NaN();
    Check(!journal.RecordCompleted(1,1,10.0,bad,b),"nonfinite state");
    journal.Reset();
    bad=a;bad.Wear=1.1;
    Check(!journal.RecordCompleted(1,1,10.0,bad,b),"invalid fraction");
    journal.Reset();
    Check(!journal.RecordCompleted(1,0,10.0,a,b),"invalid lap");
    journal.Reset();
    Check(!journal.RecordCompleted(1,1,0.0,a,b),"invalid race clock");
    journal.Reset();
    for(int i=1;i<=latebrake::PitServiceJournal::StopLimit;++i)
        Check(journal.RecordCompleted(i,1,3.0*i,a,b),"bounded stop limit");
    Check(journal.Review(1000).Valid,"1000 genuine receipts accepted");
    Check(!journal.RecordCompleted(1000,1,3003.0,a,b),"saturated service counter");
    Check(!journal.Review(1000).Valid,"saturation cannot hide extra visit");
    journal.Reset();
    Check(!journal.Review(0).Valid && journal.RetainedCount()==0,
        "restart clears all prior service evidence");

    std::cout << "RacePitVehicleService and observed journal: all scenarios passed\n";
    return 0;
}
