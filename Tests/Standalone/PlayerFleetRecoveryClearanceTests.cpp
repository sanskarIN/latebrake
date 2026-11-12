#include "AI/PlayerFleetRecoveryClearance.h"
#include "Racing/RecoveryCharge.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool pass, const char* reason)
{
    if (!pass)
    {
        std::cerr << "PlayerFleetRecoveryClearance FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    static_assert(PlayerFleetRecoveryRequiredRivals == 5);
    static_assert(MaxRecoveryObstacles == PlayerFleetRecoveryRequiredRivals);
    std::array<RecoveryObstacle, MaxRecoveryObstacles> cars{};
    for (int i = 0; i < PlayerFleetRecoveryRequiredRivals; ++i)
    {
        cars[static_cast<std::size_t>(i)] = {true, i + 1,
            static_cast<std::uint64_t>(101 + i),
            {{10000.0 + i * 1000.0, 10000.0}, 0.0}, 0.0, 0.0};
    }

    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const CircuitPose target = PlayerRecovery(gate);
        Require(EvaluatePlayerFleetRecoveryClearance(
            target, cars, 5).Safe(),
            "original human recovery physically fits asphalt and clear fleet");
    }
    const CircuitPose target = PlayerRecovery(1);
    Require(EvaluatePlayerFleetRecoveryClearance(
        target, cars, 4).Reason ==
            RecoveryClearanceReason::UnknownOccupancy &&
            EvaluatePlayerFleetRecoveryClearance(
                target, cars, 0).Reason ==
            RecoveryClearanceReason::UnknownOccupancy &&
            EvaluatePlayerFleetRecoveryClearance(
                target, cars, 6).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
            "incomplete, zero, or oversized six-car observation fails closed");

    cars[0].Pose = target;
    auto collision = EvaluatePlayerFleetRecoveryClearance(target, cars, 5);
    Require(collision.Reason == RecoveryClearanceReason::TrafficCollision &&
            collision.ObstacleSlot == 1,
            "primary rival occupied landing blocks player's paid teleport");
    cars[0].Pose = {{10000.0, 10000.0}, 0.0};
    cars[4].Pose = target;
    collision = EvaluatePlayerFleetRecoveryClearance(target, cars, 5);
    Require(collision.Reason == RecoveryClearanceReason::TrafficCollision &&
            collision.ObstacleSlot == 5,
            "last experimental rival can veto player recovery");
    cars[4].Pose = {{10000.0, 10000.0}, 0.0};

    // A high-speed rival enters and leaves the landing between 30Hz/120Hz
    // frames. Continuous swept OBB detection may not be replaced with a
    // static per-frame overlap test.
    cars[2].Pose = {{target.Position.X + 1300.0,
                     target.Position.Y}, target.HeadingDegrees};
    cars[2].VelocityX = -5000.0;
    collision = EvaluatePlayerFleetRecoveryClearance(target, cars, 5);
    Require(collision.Reason == RecoveryClearanceReason::TrafficCollision &&
            collision.ObstacleSlot == 3,
            "oncoming real rival in short horizon blocks paid recovery");
    cars[2].VelocityX = 5000.0;
    Require(EvaluatePlayerFleetRecoveryClearance(
        target, cars, 5).Safe(),
        "same car moving away from recovery does not imply phantom collision");
    cars[2].Pose = {{10000.0, 10000.0}, 0.0};
    cars[2].VelocityX = 0.0;

    cars[2].ActorToken = cars[1].ActorToken;
    Require(EvaluatePlayerFleetRecoveryClearance(
        target, cars, 5).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
            "duplicate physical identity never means safe empty street");
    cars[2].ActorToken = 103;
    cars[2].Slot = 4;
    Require(EvaluatePlayerFleetRecoveryClearance(
        target, cars, 5).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
            "missing slot three cannot be replaced by phantom duplicate four");
    cars[2].Slot = 3;
    cars[0].Verified = false;
    Require(EvaluatePlayerFleetRecoveryClearance(
        target, cars, 5).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
            "unverified primary actor fails closed");
    cars[0].Verified = true;
    cars[3].ActorToken = 0;
    Require(EvaluatePlayerFleetRecoveryClearance(
        target, cars, 5).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
            "anonymous opponent fails closed");
    cars[3].ActorToken = 104;
    cars[2].VelocityY = std::numeric_limits<double>::infinity();
    Require(EvaluatePlayerFleetRecoveryClearance(
        target, cars, 5).Reason ==
            RecoveryClearanceReason::UnknownOccupancy,
            "nonfinite Chaos velocity cannot clear a paid teleport");
    cars[2].VelocityY = 0.0;

    const CircuitPose offroad{{0.0, 0.0}, 0.0};
    Require(EvaluatePlayerFleetRecoveryClearance(
        offroad, cars, 5).Reason ==
            RecoveryClearanceReason::OffPavement,
            "dirt or full-chassis overhang cannot be a paid destination");

    RaceProgress race(4, 3);
    int recoveryCount = 0;
    const double before = race.GetTotalPenaltySeconds();
    int callbackCount = 0;
    cars[0].Pose = target;
    const bool chargedOnBlocked = TryChargePlayerRecoveryAfterVerifiedTeleport(
        race, recoveryCount, true, false, true, [&]()
        {
            ++callbackCount;
            return EvaluatePlayerFleetRecoveryClearance(
                target, cars, 5).Safe();
        });
    Require(!chargedOnBlocked && callbackCount == 1 &&
            recoveryCount == 0 &&
            race.GetTotalPenaltySeconds() == before,
            "failed physical clearance never commits five-second penalty");

    cars[0].Pose = {{10000.0, 10000.0}, 0.0};
    const bool chargedOnClear = TryChargePlayerRecoveryAfterVerifiedTeleport(
        race, recoveryCount, true, false, true, [&]()
        {
            ++callbackCount;
            return EvaluatePlayerFleetRecoveryClearance(
                target, cars, 5).Safe();
        });
    Require(chargedOnClear && recoveryCount == 1 &&
            race.GetTotalPenaltySeconds() == before + RecoveryPenaltySeconds,
            "qualified physical clearance can preserve legacy paid transaction");
    std::cout << "Player recovery fleet preflight and transactional fee passed.\n";
    return EXIT_SUCCESS;
}
