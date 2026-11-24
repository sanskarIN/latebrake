#include "AI/LiveSixCarStartingGrid.h"
#include "AI/RivalFleetRiskSupervisor.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool pass, const char* why)
{
    if (!pass)
    {
        std::cerr << "SixCarAggregateSafety FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::array<latebrake::RecoveryObstacle,
           latebrake::StartGridCapacity> CompletePhysicalGrid()
{
    using namespace latebrake;
    std::array<RecoveryObstacle, StartGridCapacity> grid{};
    for (int slot = 0; slot < StartGridCapacity; ++slot)
    {
        grid[static_cast<std::size_t>(slot)] = {
            true, slot, static_cast<std::uint64_t>(1000 + slot),
            StartGridPoses[static_cast<std::size_t>(slot)], 0.0, 0.0};
    }
    return grid;
}

std::array<latebrake::LiveGridVerticalSample,
           latebrake::StartGridCapacity> LiveGroundReadouts()
{
    using namespace latebrake;
    std::array<LiveGridVerticalSample, StartGridCapacity> vertical{};
    for (LiveGridVerticalSample& sample : vertical)
    {
        sample.ChassisWorldZCm = LiveGridExpectedChassisCenterZCm;
        sample.Ground.PhysicsReady = true;
        sample.Ground.DeltaSeconds = 1.0 / 60.0;
        sample.Ground.ChassisVerticalSpeedCmPerSecond = 0.0;
        for (ChassisGroundProbe& probe : sample.Ground.Probes)
        {
            probe = {true, VehicleChassisHalfHeightCm, 1.0};
        }
    }
    return vertical;
}

latebrake::RivalFleetTrafficSample PhysicalTraffic(
    const std::array<latebrake::RecoveryObstacle,
                     latebrake::StartGridCapacity>& grid,
    int selfSlot)
{
    using namespace latebrake;
    RivalFleetTrafficSample result{};
    result.SessionQualified = true;
    result.BrakeObservationQualified = true;
    result.ConservativeBrakeScale = 0.85;
    result.Self = grid[static_cast<std::size_t>(selfSlot)];
    for (int slot = 0; slot < StartGridCapacity; ++slot)
    {
        if (slot == selfSlot)
        {
            continue;
        }
        result.Blockers[static_cast<std::size_t>(result.BlockerCount++)] =
            grid[static_cast<std::size_t>(slot)];
    }
    return result;
}
} // namespace

int main()
{
    using namespace latebrake;
    static_assert(StartGridCapacity == 6);
    const auto actual = CompletePhysicalGrid();
    const auto vertical = LiveGroundReadouts();

    Require(IsPlannedStartGridSafe(),
            "source-authored starting poses must remain collision-safe");
    Require(EvaluateLiveSixCarStartingGrid(actual, vertical, 6).Safe(),
            "all six actual cells have valid contact and distinct identity");

    // The independent start and post-GO stages consume the SAME actor
    // tokens, even though a full post-GO traffic sample excludes self.
    for (int selfSlot = 1; selfSlot < StartGridCapacity; ++selfSlot)
    {
        RivalFleetTrafficSample traffic = PhysicalTraffic(actual, selfSlot);
        std::array<std::uint64_t, StartGridCapacity> roster{};
        Require(ValidateSixCarFleet(traffic, roster),
                "start roster is readable as one AI plus five real cars");
        for (int slot = 0; slot < StartGridCapacity; ++slot)
        {
            Require(roster[static_cast<std::size_t>(slot)] ==
                    actual[static_cast<std::size_t>(slot)].ActorToken,
                "post-green physical token must match the launch actor");
        }

        RivalFleetRiskSupervisor supervisor;
        const PilotCommand allowed{0.8, 0.22, false, false};
        const FleetRiskSupervision decision =
            supervisor.Apply(allowed, traffic, 1.0 / 60.0);
        Require(decision.Qualified &&
                std::isfinite(decision.Command.Throttle) &&
                decision.Command.Throttle >= 0.0 &&
                decision.Command.Throttle <= allowed.Throttle &&
                decision.Command.Steering == allowed.Steering,
                "no starting slot is granted speculative motor or steering");
        if (decision.Forecast.Emergency())
        {
            Require(decision.Command.Brake &&
                    decision.Command.Throttle == 0.0,
                    "real risk after GO vetoes all AI throttle");
        }
    }

    // The GO gate and post-GO fleet brake are separate policies, both
    // of which must fail closed for a genuine missing physical actor.
    auto missing = actual;
    missing[3].Verified = false;
    Require(!EvaluateLiveSixCarStartingGrid(missing, vertical, 6).Safe(),
            "pre-green absent rival cannot authorize six-car GO");

    RivalFleetRiskSupervisor supervisor;
    RivalFleetTrafficSample traffic = PhysicalTraffic(actual, 4);
    const PilotCommand route{0.85, -0.31, false, false};
    Require(supervisor.Apply(route, traffic, 1.0 / 60.0).Qualified,
            "complete post-green roster is observed");
    traffic.BlockerCount = 4;
    const FleetRiskSupervision rejected =
        supervisor.Apply(route, traffic, 1.0 / 60.0);
    Require(!rejected.Qualified && rejected.Command.Brake &&
            rejected.Command.Throttle == 0.0 &&
            !supervisor.HasPhysicalRoster(),
            "missing post-green car destroys stale traffic authorization");

    auto unsupported = vertical;
    unsupported[1].Ground.Probes[0].Hit = false;
    unsupported[1].Ground.Probes[1].Hit = false;
    Require(EvaluateLiveSixCarStartingGrid(actual, unsupported, 6).Failure ==
            LiveGridFailure::InsufficientGroundSupport,
            "XY-correct floating chassis cannot be admitted at GO");

    // A recycled engine actor must not borrow the prior road clearance.
    traffic = PhysicalTraffic(actual, 4);
    supervisor.Reset();
    supervisor.Apply(route, traffic, 1.0 / 60.0);
    traffic.Blockers[0].ActorToken = 99001;
    const FleetRiskSupervision replaced =
        supervisor.Apply(route, traffic, 1.0 / 60.0);
    Require(replaced.Qualified && replaced.RosterChanged &&
            replaced.HoldingBrake && replaced.Command.Brake &&
            replaced.Command.Throttle == 0.0,
            "replaced physical actor requires a new stable traffic epoch");

    // Portable source simulation is NOT a UE 5.8 UHT/Chaos acceptance test.
    std::cout << "Six-car launch + physical traffic integration passed.\n";
    return EXIT_SUCCESS;
}
