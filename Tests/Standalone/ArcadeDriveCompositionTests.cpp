#include "Vehicles/ArcadeBoostArbitration.h"
#include "Vehicles/ArcadeBrakeHeat.h"
#include "Vehicles/ArcadeCollisionDamage.h"
#include "Vehicles/ArcadeEngineHeat.h"
#include "Vehicles/ArcadeSurfaceTraction.h"
#include "Vehicles/GearAwareMotorPower.h"
#include "Vehicles/ArcadeTireCondition.h"
#include "Vehicles/CornerTractionAssist.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
void Check(bool condition, const char* description)
{
    if (!condition)
    {
        throw std::runtime_error(description);
    }
}

// Mirror the three real AddForce terms (base + allocated nitro + draft)
// without introducing an extra motor budget or altering production code.
double DriveFraction(double throttle,
                     latebrake::ArcadeSurfaceOutput road,
                     const latebrake::ArcadeCollisionDamage& damage,
                     latebrake::ArcadeBoostAllocation boost,
                     const latebrake::EngineHeatOutput& engine,
                     double gearFactor)
{
    if (!road.Valid || !engine.Valid ||
        !std::isfinite(gearFactor) || gearFactor < 0.0 || gearFactor > 1.0)
    {
        return 0.0;
    }
    const double scale = road.MotorMultiplier * damage.MotorMultiplier() *
        engine.MotorMultiplier * gearFactor;
    return throttle * scale
        + throttle * scale * boost.NitroExtra
        + throttle * scale * boost.DraftExtra;
}
}

int main()
{
    using namespace latebrake;
    ArcadeSurfaceTraction surface;
    ArcadeTireCondition tires;
    ArcadeCollisionDamage impacts;
    ArcadeBrakeHeat brakes;
    ArcadeEngineHeat engine;

    const auto road = surface.Observe({0.05, EastX, 0.0, true});
    Check(road.Valid && !road.OffPavement &&
          road.MotorMultiplier == 1.0 &&
          road.LateralGripMultiplier == 1.0,
          "a valid road frame retains the normal force budget");
    Check(tires.Observe({0.05, 3000.0, 500.0, false, true}),
          "tire sample accepted");
    Check(impacts.Advance(0.05, true), "impact eligibility advanced");
    const auto both = AllocateArcadeBoost(1.38, 0.06, true);
    const double throttle = StabilizeCornerThrottle(1.0, 3000.0, 500.0, true);
    Check(throttle > 0.0 && throttle <= 1.0,
          "corner assist must not invent extra throttle");
    const auto healthyEngine = engine.Observe({
        0.05, 3000.0, throttle, false, true});
    const double normalGear = GearAwareMotorPowerFactor(
        throttle, 3000.0, 6500.0, true);
    Check(healthyEngine.Valid && !healthyEngine.Derated && normalGear == 1.0,
          "ordinary race launch retains full thermal and gear authority");
    const double healthyForce = DriveFraction(
        throttle, road, impacts, both, healthyEngine, normalGear);
    Check(healthyForce <= 1.40 + 1e-10,
          "boost budget cannot exceed 140 percent on healthy road");
    Check(std::abs(healthyForce - throttle * 1.4) < 1e-10,
          "base, nitro, and draft are additive under one budget");

    // The same severe contact must reduce the BASE motor, nitro and
    // approved drafting, never simply cap one of the bonus paths.
    Check(impacts.ObserveImpact({1100000.0, 1100.0, 3800.0, 0.1, true}),
          "qualified horizontal chassis contact accepted");
    const double damagedForce = DriveFraction(throttle, road, impacts, both, healthyEngine, normalGear);
    Check(damagedForce < healthyForce &&
          damagedForce <= throttle * impacts.MotorMultiplier() * 1.4 + 1e-10,
          "real impact scales total allocated motor force");
    Check(tires.GripMultiplier() * impacts.LateralGripMultiplier() <= 1.0,
          "damage and tires cannot add lateral grip");

    // Wait for the ACTUAL on-track hysteresis policy to confirm dirt.
    ArcadeSurfaceOutput dirt;
    for (int i = 0; i < 4; ++i)
    {
        dirt = surface.Observe({0.05, 0.0, 0.0, true});
    }
    Check(dirt.Valid && dirt.OffPavement &&
          dirt.MotorMultiplier == ArcadeSurfaceTraction::OffPavementMotorMultiplier,
          "confirmed off-road surface reduces motor authority");
    Check(DriveFraction(throttle, dirt, impacts, both, healthyEngine, normalGear) < damagedForce,
          "off-road reduction composes with damage, nitro and draft");
    const double offRoadGrip =
        tires.GripMultiplier() * dirt.LateralGripMultiplier *
        impacts.LateralGripMultiplier();
    Check(offRoadGrip > 0.0 && offRoadGrip < 1.0,
          "off-road, tire and damage grip reductions compose");

    // Sustained real propulsion must fade ALL three additive motor paths,
    // not only baseline acceleration. Coasting restores power; gear taper
    // composes independently rather than bypassing the heat or damage.
    EngineHeatOutput overheatedEngine;
    for (int i = 0; i < 1200; ++i)
    {
        overheatedEngine = engine.Observe({
            0.05, 3000.0, throttle, true, true});
    }
    Check(overheatedEngine.Valid && overheatedEngine.Derated &&
          overheatedEngine.MotorMultiplier >=
              ArcadeEngineHeat::MinimumMotorMultiplier &&
          overheatedEngine.MotorMultiplier < healthyEngine.MotorMultiplier,
          "sustained qualified engine load reduces shared motor force");
    const double hotForce = DriveFraction(
        throttle, dirt, impacts, both, overheatedEngine, normalGear);
    const double coolForce = DriveFraction(
        throttle, dirt, impacts, both, healthyEngine, normalGear);
    Check(hotForce > 0.0 && hotForce < coolForce,
          "thermal fade composes with verified dirt, impact and boosts");

    EngineHeatOutput recoveredEngine;
    for (int i = 0; i < 1200; ++i)
    {
        recoveredEngine = engine.Observe({0.05, 5000.0, 0.0, false, true});
    }
    Check(recoveredEngine.Valid &&
          recoveredEngine.MotorMultiplier > overheatedEngine.MotorMultiplier,
          "coasting recovers only the thermal motor multiplier");
    const double halfGear = GearAwareMotorPowerFactor(
        throttle, 5915.0, 6500.0, true);
    Check(std::abs(halfGear - 0.5) < 1e-10,
          "production forward speed taper reaches half power");
    const double taperedForce = DriveFraction(
        throttle, dirt, impacts, both, recoveredEngine, halfGear);
    const double untaperedForce = DriveFraction(
        throttle, dirt, impacts, both, recoveredEngine, normalGear);
    Check(std::abs(taperedForce * 2.0 - untaperedForce) < 1e-10,
          "gear taper scales baseline nitro and draft together");

    // Invalid engine samples and interruptions may never be interpreted
    // as an ordinary 1.0 paved-surface multiplier.
    const auto invalid = surface.Observe({0.11, EastX, 0.0, true});
    Check(!invalid.Valid && DriveFraction(throttle, invalid, impacts, both, healthyEngine, normalGear) == 0.0,
          "long frame cannot authorize driving force");
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto broken = surface.Observe({0.05, nan, 0.0, true});
    Check(!broken.Valid && DriveFraction(throttle, broken, impacts, both, healthyEngine, normalGear) == 0.0,
          "nonfinite position fails closed");
    Check(!brakes.Observe({0.05, nan, true, true}).Valid,
          "invalid brake telemetry cannot apply heat fade");
    const auto invalidHeat = engine.Observe({0.05, nan, 1.0, true, true});
    Check(!invalidHeat.Valid && invalidHeat.MotorMultiplier == 1.0 &&
          !engine.HasValidSample(),
          "bad engine physics cannot retain a stale active derate");
    Check(DriveFraction(throttle, dirt, impacts, both, invalidHeat, normalGear) == 0.0,
          "the composition regression does not treat invalid thermal input as qualified");
    impacts.Suspend();
    Check(!impacts.ObserveImpact({1100000.0, 1100.0, 3800.0, 0.1, true}),
          "paused contact cannot add damage");

    // Multiple effects are sampled using production policies, not
    // independently invented test multipliers. Check deterministic bounds.
    for (int i = 0; i < 200000; ++i)
    {
        const double speed = 900.0 + static_cast<double>(i % 4100);
        const double lateral = static_cast<double>(i % 950);
        const bool nitro = (i % 2) == 0;
        const double nitroForce = nitro ? 1.38 : 1.0;
        const double requestedDraft = static_cast<double>(i % 61) / 1000.0;
        const auto allocation =
            AllocateArcadeBoost(nitroForce, requestedDraft, nitro);
        const double demand =
            StabilizeCornerThrottle(1.0, speed, lateral, i % 3 != 0);
        const auto location = surface.Observe({
            0.05, (i % 90 < 45) ? EastX : 0.0, 0.0, true});
        Check(tires.Observe({0.05, speed, lateral, i % 11 == 0, true}),
              "valid tire sample during stress");
        Check(impacts.Advance(0.05, true), "valid impact frame during stress");
        if (i % 137 == 0)
        {
            impacts.ObserveImpact({1500000.0, 1100.0, speed, 0.0, true});
        }
        const auto brake = brakes.Observe({
            0.05, speed, i % 11 == 0, true});
        const auto thermal = engine.Observe({
            0.05, speed, demand, nitro, true});
        const double gear = GearAwareMotorPowerFactor(
            demand, speed, 6500.0, true);
        Check(brake.Valid && brake.ServiceBrakeMultiplier >=
              ArcadeBrakeHeat::MinimumBrakeMultiplier &&
              brake.ServiceBrakeMultiplier <= 1.0,
              "brake heat cannot invert service braking");
        const double force = DriveFraction(
            demand, location, impacts, allocation, thermal, gear);
        const double limit = demand * location.MotorMultiplier *
            impacts.MotorMultiplier() * thermal.MotorMultiplier *
            gear * 1.4;
        const double grip = tires.GripMultiplier() *
            location.LateralGripMultiplier * impacts.LateralGripMultiplier();
        Check(thermal.Valid && std::isfinite(thermal.MotorMultiplier) &&
              thermal.MotorMultiplier >= ArcadeEngineHeat::MinimumMotorMultiplier &&
              thermal.MotorMultiplier <= 1.0 &&
              std::isfinite(gear) && gear >= 0.0 && gear <= 1.0,
              "thermal and gear derates remain finite and bounded");
        Check(location.Valid && std::isfinite(force) &&
              force >= 0.0 && force <= limit + 1e-10,
              "all motor forces share the same bounded composition");
        Check(std::isfinite(grip) && grip > 0.0 && grip <= 1.0,
              "all lateral grip effects remain finite and resistive");
    }

    surface.Reset();
    tires.Reset();
    impacts.Reset();
    brakes.Reset();
    engine.Reset();
    Check(!surface.HasValidSample() &&
          !tires.HasValidSample() &&
          !impacts.HasEligibleFrame() &&
          !brakes.HasValidSample() &&
          !engine.HasValidSample() &&
          engine.HeatFraction() == ArcadeEngineHeat::IdleHeat &&
          impacts.DamageFraction() == 0.0 &&
          tires.WearFraction() == 0.0,
          "a new race resets all independently owned arcade condition state");

    std::cout << "ArcadeDriveComposition: 200000 bounded policy frames passed\n";
    return 0;
}
