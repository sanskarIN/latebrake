#include "Vehicles/RaceVehiclePhysicsReadiness.h"
#include "Racing/PlayerRaceClockReadiness.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool valid, const char* message)
{
    if (!valid)
    {
        std::cerr << "Race vehicle physics readiness FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;

    RaceVehiclePhysicsSnapshot car{
        true, true, true, true, true,
        -2550.0, -1800.0, 180.0,
        300.0, 0.0, 0.0
    };
    Require(IsRaceVehiclePhysicsReady(car),
            "original simulated collision-bearing finite car is ready");

    // Changing ANY material chassis precondition must invalidate the driver,
    // regardless of a still-possessed pawn and valid velocity coordinates.
    auto withoutChassis = car;
    withoutChassis.ChassisPresent = false;
    Require(!IsRaceVehiclePhysicsReady(withoutChassis),
            "missing physics component denies driver readiness");
    auto withoutMesh = car;
    withoutMesh.MeshPresent = false;
    Require(!IsRaceVehiclePhysicsReady(withoutMesh),
            "failed engine-provided chassis mesh denies race time");
    auto sleepingPhysicsDisabled = car;
    sleepingPhysicsDisabled.SimulatingPhysics = false;
    Require(!IsRaceVehiclePhysicsReady(sleepingPhysicsDisabled),
            "simulation disabled cannot create timed laps");
    auto queryOnly = car;
    queryOnly.QueryAndPhysicsCollision = false;
    Require(!IsRaceVehiclePhysicsReady(queryOnly),
            "query-only chassis cannot satisfy physical collision readiness");
    auto ghostBody = car;
    ghostBody.BlocksWorldStatic = false;
    Require(!IsRaceVehiclePhysicsReady(ghostBody),
            "nonblocking road/barrier collision refuses timed racing");

    // Every position and velocity component must be finite, including Z.
    // The car may have a finite location but broken Chaos physics velocity.
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (int component = 0; component < 6; ++component)
    {
        auto invalid = car;
        double* axes[6] = {
            &invalid.PositionX, &invalid.PositionY, &invalid.PositionZ,
            &invalid.VelocityX, &invalid.VelocityY, &invalid.VelocityZ
        };
        *axes[component] = nan;
        Require(!IsRaceVehiclePhysicsReady(invalid),
                "NaN in any chassis position/velocity axis invalidates driver");
        *axes[component] = inf;
        Require(!IsRaceVehiclePhysicsReady(invalid),
                "infinity in any chassis position/velocity axis invalidates driver");
        *axes[component] = -inf;
        Require(!IsRaceVehiclePhysicsReady(invalid),
                "negative infinity in any chassis axis invalidates driver");
    }

    // Unreal's transform may have finite position but corrupt orientation.
    // Never allow it to create a clock, checkpoint score or steering basis.
    for (int component = 0; component < 3; ++component)
    {
        for (double broken : {nan, inf, -inf})
        {
            auto invalid = car;
            double* rotationAxes[] = {
                &invalid.RotationPitchDegrees,
                &invalid.RotationYawDegrees,
                &invalid.RotationRollDegrees
            };
            *rotationAxes[component] = broken;
            Require(!IsRaceVehiclePhysicsReady(invalid),
                    "nonfinite pitch/yaw/roll blocks driver readiness");
        }
    }
    auto badYaw = car;
    badYaw.RotationYawDegrees = nan;
    Require(!IsRaceVehiclePhysicsReady(badYaw),
            "nonfinite actor yaw is not a ready Chaos chassis");

    RaceProgress race(1, 1);
    const auto ReadyClockStep = [&race](const RaceVehiclePhysicsSnapshot& state,
                                       double seconds) {
        return AdvancePlayerRaceClockIfReady(
            race, true, false, IsRaceVehiclePhysicsReady(state), seconds);
    };
    Require(!ReadyClockStep(withoutChassis, 20.0) &&
            !ReadyClockStep(withoutMesh, 20.0) &&
            !ReadyClockStep(sleepingPhysicsDisabled, 20.0) &&
            !ReadyClockStep(queryOnly, 20.0) &&
            !ReadyClockStep(ghostBody, 20.0) &&
            race.GetElapsedSeconds() == 0.0,
            "undrivable pawn must not generate ghost race time");
    Require(ReadyClockStep(car, 2.0) && race.GetElapsedSeconds() == 2.0,
            "valid chassis resumes clock after physics becomes available");
    Require(!CanScorePlayerCheckpoint(race, true, false,
                IsRaceVehiclePhysicsReady(ghostBody)),
            "invalid collision-bearing car cannot score an ordered gate");
    Require(CanScorePlayerCheckpoint(race, true, false,
                IsRaceVehiclePhysicsReady(car)),
            "restored physics allows legitimate checkpoint readiness");
    Require(!ReadyClockStep(queryOnly, 30.0) &&
            race.GetElapsedSeconds() == 2.0,
            "losing chassis physics during a lap freezes existing clock");
    Require(!ReadyClockStep(badYaw, 10.0) &&
            !CanScorePlayerCheckpoint(race, true, false,
                IsRaceVehiclePhysicsReady(badYaw)) &&
            race.GetElapsedSeconds() == 2.0,
            "broken orientation freezes clock and checkpoint authority");
    Require(ReadyClockStep(car, 3.0) && race.GetElapsedSeconds() == 5.0,
            "recovered chassis counts only real active frame time");
    Require(race.PassCheckpoint(0) && race.IsFinished(),
            "one legal checkpoint completes the one-gate test race");
    Require(!ReadyClockStep(car, 10.0) && race.GetElapsedSeconds() == 5.0,
            "finished race stays frozen even with healthy physics");

    std::cout << "Race vehicle physics readiness tests passed.\n";
    return EXIT_SUCCESS;
}
