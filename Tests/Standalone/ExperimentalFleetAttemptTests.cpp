#include "AI/ExperimentalFleetAttempt.h"
#include "AI/PlayerFleetRecoveryClearance.h"
#include "AI/RivalFleetRiskSupervisor.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;

void Require(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << "ExperimentalFleetAttempt: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

FleetPhysicalFrame Grid(std::uint64_t attempt, std::uint64_t frame)
{
    FleetPhysicalFrame result;
    result.AttemptToken = attempt;
    result.FrameToken = frame;
    result.Count = StartGridCapacity;
    for (int i = 0; i < StartGridCapacity; ++i)
    {
        const auto index = static_cast<std::size_t>(i);
        result.Cars[index] = {true, i, static_cast<std::uint64_t>(1001 + i),
                            StartGridPoses[index], 0.0, 0.0};
        auto& vertical = result.Vertical[index];
        vertical.ChassisWorldZCm = LiveGridExpectedChassisCenterZCm;
        vertical.Ground.PhysicsReady = true;
        vertical.Ground.DeltaSeconds = 1.0 / 60.0;
        for (auto& probe : vertical.Ground.Probes)
        {
            probe = {true, VehicleChassisHalfHeightCm, 1.0};
        }
    }
    return result;
}

void Start(ExperimentalFleetAttempt& state, std::uint64_t attempt)
{
    Require(state.Begin({true, true, false, attempt}), "new attempt");
    Require(state.Observe(Grid(attempt, 1)), "qualified grid");
    Require(state.CommitGreen(attempt, 1), "same-frame green");
}

void ModeIsolation()
{
    for (bool requested : {false, true})
    {
        for (bool circuit : {false, true})
        {
            for (bool career : {false, true})
            {
                ExperimentalFleetAttempt state;
                Require(state.Begin({requested, circuit, career, 1}), "configure");
                const bool required = requested && circuit && !career;
                Require(state.Readout().Required == required, "event isolation");
                Require(state.CanAttemptSpawn() == required, "spawn isolation");
                if (!required)
                {
                    Require(!state.Observe(Grid(1, 1)), "disabled observe");
                    Require(!state.CommitGreen(1, 1), "disabled green");
                    Require(state.Readout().Phase == FleetAttemptPhase::Disabled,
                            "solo/career remain disabled");
                }
            }
        }
    }
}

void StartupAndFaults()
{
    ExperimentalFleetAttempt state;
    Require(!state.Begin({true, true, false, 0}), "zero attempt denied");
    Require(state.Begin({true, true, false, 1}), "initial attempt");
    auto missing = Grid(1, 1);
    missing.Count = 5;
    Require(!state.Observe(missing), "incomplete unbound grid");
    Require(state.CanAttemptSpawn(), "waiting for genuine spawn");
    state.ReportSpawnFailure(99);
    Require(state.CanAttemptSpawn(), "stale spawn callback ignored");
    state.ReportSpawnFailure(1);
    Require(state.Readout().Fault == FleetAttemptFault::SpawnFailed, "spawn fault");
    Require(!state.Observe(Grid(1, 2)), "no fallback after spawn failure");
    Require(!state.Begin({true, true, false, 1}), "same attempt cannot reset fault");
    Require(state.Begin({true, true, false, 2}), "explicit restart clears failure");
    auto bad = Grid(2, 1);
    bad.Vertical[3].Ground.PhysicsReady = false;
    Require(!state.Observe(bad), "bad physical support");
    Require(state.Readout().Phase == FleetAttemptPhase::AwaitingGrid, "await settling");
    Require(state.Observe(Grid(2, 2)), "settled");
    Require(state.Readout().CountdownAllowed, "countdown only");
    Require(!state.Readout().RunningAllowed, "pre-GO no driving");
    Require(!state.CommitGreen(2, 1), "old grid cannot authorize green");
    Require(state.Readout().Fault == FleetAttemptFault::UnexpectedGreen,
            "stale green fault");
    Require(!state.Observe(Grid(2, 3)), "fault latched");
}

void RaceAndPause()
{
    ExperimentalFleetAttempt state;
    Start(state, 3);
    auto moving = Grid(3, 2);
    for (auto& car : moving.Cars)
    {
        car.Pose.Position.X += 2000.0;
        car.VelocityX = 2000.0;
    }
    moving.Vertical = {};
    Require(state.Observe(moving), "running vehicles need not stay in grid cells");
    Require(state.Readout().RunningAllowed, "running permission");
    auto paused = FleetPhysicalFrame{};
    paused.AttemptToken = 3;
    paused.FrameToken = 3;
    paused.Inactive = true;
    Require(!state.Observe(paused), "pause no authority");
    Require(state.Readout().Phase == FleetAttemptPhase::Running, "pause preserves phase");
    Require(!state.Readout().RunningAllowed, "pause clears stale permission");
    RivalFleetTrafficSample denied{};
    Require(!state.MakeRivalTraffic(1, 1002, 3, 2, true, 0.7, denied),
            "paused frame denies old traffic");
    moving.FrameToken = 4;
    Require(state.Observe(moving), "fresh physical resume");
    auto replacement = moving;
    replacement.FrameToken = 5;
    replacement.Cars[4].ActorToken = 7000;
    Require(!state.Observe(replacement), "replacement denied");
    Require(state.Readout().Fault == FleetAttemptFault::ReplacedActor &&
            state.Readout().FaultSlot == 4, "replacement diagnosis");
    moving.FrameToken = 6;
    Require(!state.Observe(moving), "old actor cannot heal terminal fault");
}

void BlockerComposition()
{
    ExperimentalFleetAttempt state;
    Start(state, 4);
    const auto grid = Grid(4, 1);
    for (int subject = 0; subject < StartGridCapacity; ++subject)
    {
        std::array<RecoveryObstacle, MaxRecoveryObstacles> blockers{};
        int count = 99;
        const auto token = grid.Cars[static_cast<std::size_t>(subject)].ActorToken;
        Require(state.MakeRecoveryBlockers(subject, token, 4, 1, blockers, count),
                "all physical blockers supplied");
        Require(count == StartGridCapacity - 1, "exact blocker count");
        int index = 0;
        for (int slot = 0; slot < StartGridCapacity; ++slot)
        {
            if (slot == subject)
            {
                continue;
            }
            const auto& b = blockers[static_cast<std::size_t>(index++)];
            Require(b.Slot == slot && b.ActorToken ==
                    grid.Cars[static_cast<std::size_t>(slot)].ActorToken,
                    "exact ordered physical identities");
        }
        RivalFleetTrafficSample traffic{};
        const bool accepted = state.MakeRivalTraffic(subject, token, 4, 1,
                                                      true, 0.8, traffic);
        Require(accepted == (subject != HumanGridSlot), "rival-only traffic subject");
        if (accepted)
        {
            Require(traffic.SessionQualified && traffic.BlockerCount == count &&
                    traffic.Self.ActorToken == token, "traffic snapshot admitted");
            const auto command = ApplyRivalFleetTraffic({0.5, 0.0, false, false}, traffic);
            Require(command.Qualified, "existing short-horizon policy composes");
            RivalFleetRiskSupervisor supervisor;
            const auto supervised = supervisor.Apply(command.Command, traffic, 0.016);
            Require(supervised.Qualified && supervised.Command.Throttle <= 0.5,
                    "existing supervisor composes without added propulsion");
        }
        else
        {
            Require(!traffic.SessionQualified && traffic.BlockerCount == 0,
                    "failed traffic has no stale output");
        }
    }
    std::array<RecoveryObstacle, MaxRecoveryObstacles> blockers{};
    int count = 0;
    Require(state.MakeRecoveryBlockers(0, 1001, 4, 1, blockers, count),
            "player recovery list");
    Require(EvaluatePlayerFleetRecoveryClearance(StartGridPoses[1], blockers, count).Reason
                == RecoveryClearanceReason::TrafficCollision,
            "existing player preflight sees occupied physical rival slot");
    Require(!state.MakeRecoveryBlockers(0, 9999, 4, 1, blockers, count) && count == 0,
            "wrong subject clears old blockers");
    RivalFleetTrafficSample traffic{};
    Require(!state.MakeRivalTraffic(1, 1002, 4, 1, true,
                std::numeric_limits<double>::quiet_NaN(), traffic), "invalid brake scale");
    Require(state.MakeRivalTraffic(1, 1002, 4, 1, false, 0.0, traffic),
            "unavailable brake observation explicitly represented");
    Require(!traffic.BrakeObservationQualified && traffic.ConservativeBrakeScale == 0.0,
            "no invented physical brake evidence");
    state.Finish(3);
    Require(state.Readout().RunningAllowed, "stale finish ignored");
    state.Finish(4);
    Require(state.Readout().Phase == FleetAttemptPhase::Finished &&
            !state.Readout().RunningAllowed, "finish revokes permission");
    Require(!state.Observe(Grid(4, 2)), "finished attempt cannot resume");
}

void IdentityAndFrameSweep()
{
    for (int slot = 0; slot < StartGridCapacity; ++slot)
    {
        for (int kind = 0; kind < 5; ++kind)
        {
            ExperimentalFleetAttempt state;
            Start(state, 5);
            auto frame = Grid(5, 2);
            auto& car = frame.Cars[static_cast<std::size_t>(slot)];
            switch (kind)
            {
                case 0: car.Verified = false; break;
                case 1: car.ActorToken = 0; break;
                case 2: car.ActorToken = frame.Cars[static_cast<std::size_t>(
                            (slot + 1) % StartGridCapacity)].ActorToken; break;
                case 3: car.VelocityY = std::numeric_limits<double>::infinity(); break;
                case 4: car.Slot = (slot + 1) % StartGridCapacity; break;
                default: std::abort();
            }
            Require(!state.Observe(frame), "bad live observation denied");
            Require(state.Readout().Phase == FleetAttemptPhase::Faulted,
                    "bad live observation terminal for this attempt");
            Require(!state.Observe(Grid(5, 3)), "terminal fault cannot auto-recover");
        }
    }
    ExperimentalFleetAttempt stale;
    Start(stale, 6);
    Require(!stale.Observe(Grid(6, 1)), "repeated frame denied");
    Require(stale.Readout().Fault == FleetAttemptFault::InvalidFrame, "duplicate frame fault");
    Require(stale.Begin({true, true, false, 7}), "new attempt after duplicate");
    Require(!stale.Observe(Grid(6, 2)), "old attempt frame denied");
    Require(stale.Readout().Fault == FleetAttemptFault::InvalidFrame, "old attempt fault");

    ExperimentalFleetAttempt paused;
    Require(paused.Begin({true, true, false, 8}), "pre-GO attempt");
    Require(paused.Observe(Grid(8, 1)), "initial countdown sample");
    auto p = Grid(8, 2);
    p.Inactive = true;
    Require(!paused.Observe(p), "paused countdown denied");
    Require(!paused.Readout().CountdownAllowed, "no prior countdown permission");
    Require(paused.Observe(Grid(8, 3)), "fresh countdown after pause");
    Require(paused.CommitGreen(8, 3), "current resumed frame starts");
}
} // namespace

int main()
{
    ModeIsolation();
    StartupAndFaults();
    RaceAndPause();
    BlockerComposition();
    IdentityAndFrameSweep();
    std::cout << "Experimental fleet attempt lifecycle passed.\n";
    return EXIT_SUCCESS;
}
