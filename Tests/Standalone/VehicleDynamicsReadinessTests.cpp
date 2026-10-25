#include "Vehicles/VehicleDynamicsReadiness.h"
#include "Vehicles/RaceVehiclePhysicsReadiness.h"
#include "Racing/PlayerRaceClockReadiness.h"
#include "Racing/RaceProgress.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace
{
void Require(bool yes, const char* reason)
{
    if (!yes)
    {
        std::cerr << "Vehicle dynamics readiness FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    const VehicleDynamicsSample sound{1100.0, 0.0, 0.0, 42.0};
    Require(AreVehicleDynamicsReady(sound), "realistic body mass and spin pass");
    Require(EvaluateVehicleDynamics(sound) == VehicleDynamicsDecision::Ready,
            "healthy dynamics report ready");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double invalidMass : {0.0, -1.0, nan, inf, -inf})
    {
        auto sample = sound;
        sample.MassKilograms = invalidMass;
        Require(!AreVehicleDynamicsReady(sample),
                "nonpositive or nonfinite mass denied");
        Require(EvaluateVehicleDynamics(sample) ==
                    VehicleDynamicsDecision::InvalidMass,
                "mass failure returns precise diagnostic category");
    }
    for (int axis = 0; axis < 3; ++axis)
    {
        for (double broken : {nan, inf, -inf})
        {
            auto sample = sound;
            double* angles[3] = {
                &sample.AngularVelocityXDegrees,
                &sample.AngularVelocityYDegrees,
                &sample.AngularVelocityZDegrees
            };
            *angles[axis] = broken;
            Require(!AreVehicleDynamicsReady(sample),
                    "every nonfinite angular velocity axis denied");
            Require(EvaluateVehicleDynamics(sample) ==
                        VehicleDynamicsDecision::InvalidAngularVelocity,
                    "angular failure has specific reason");
        }
    }
    auto positiveTinyMass = sound;
    positiveTinyMass.MassKilograms = std::numeric_limits<double>::min();
    Require(AreVehicleDynamicsReady(positiveTinyMass),
            "positive finite mass rule remains explicit, not arbitrary clamp");
    auto finiteRapidSpin = sound;
    finiteRapidSpin.AngularVelocityZDegrees = 1000000.0;
    Require(AreVehicleDynamicsReady(finiteRapidSpin),
            "finite spin is accepted by numeric sanity (not hardware handling certification)");

    Require(std::string(VehicleDynamicsDecisionName(
                VehicleDynamicsDecision::InvalidAngularVelocity)) ==
                "invalid-angular-velocity",
            "stable angular diagnostic name");
    Require(std::string(VehicleDynamicsDecisionName(
                VehicleDynamicsDecision::InvalidMass)) == "invalid-mass",
            "stable mass diagnostic name");

    // Integrate real RaceProgress, existing physical chassis gates, and
    // readiness so rejecting angular/mass anomalies cannot earn timed laps.
    const RaceVehiclePhysicsSnapshot chassis{
        true, true, true, true, true,
        -2550.0, -1800.0, 180.0, 500.0, 0.0, 0.0
    };
    RaceProgress race(4, 1);
    auto Clock = [&race, &chassis](const VehicleDynamicsSample& state,
                                   double seconds) {
        const bool ready = IsRaceVehiclePhysicsReady(chassis) &&
                           AreVehicleDynamicsReady(state);
        return AdvancePlayerRaceClockIfReady(
            race, true, false, ready, seconds);
    };
    auto invalidAngular = sound;
    invalidAngular.AngularVelocityXDegrees = nan;
    auto invalidMass = sound;
    invalidMass.MassKilograms = 0.0;
    Require(!Clock(invalidAngular, 30.0) && !Clock(invalidMass, 30.0) &&
            race.GetElapsedSeconds() == 0.0,
            "corrupted Chaos dynamics cannot credit paused-duration race clock");
    Require(!CanScorePlayerCheckpoint(race, true, false,
                IsRaceVehiclePhysicsReady(chassis) &&
                AreVehicleDynamicsReady(invalidAngular)),
            "invalid angular dynamics block ordered checkpoint award readiness");
    Require(Clock(sound, 2.0) && race.GetElapsedSeconds() == 2.0,
            "valid restored dynamics allow normal time accrual");
    Require(!Clock(invalidMass, 3.0) &&
            race.GetElapsedSeconds() == 2.0,
            "midlap mass fault freezes time without resetting valid history");
    Require(Clock(sound, 1.0) && race.GetElapsedSeconds() == 3.0,
            "clock continues only on healthy frames");

    std::cout << "Vehicle dynamics numeric sanity and race clock tests passed (NO Unreal).\n";
    return EXIT_SUCCESS;
}
