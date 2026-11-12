#include "AI/LiveSixCarStartingGrid.h"

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
        std::cerr << "LiveSixCarStartingGrid FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::array<latebrake::RecoveryObstacle,
           latebrake::StartGridCapacity> GoodGrid()
{
    using namespace latebrake;
    std::array<RecoveryObstacle, StartGridCapacity> grid{};
    for (int i = 0; i < StartGridCapacity; ++i)
    {
        grid[static_cast<std::size_t>(i)] = {
            true, i, static_cast<std::uint64_t>(1001 + i),
            StartGridPoses[static_cast<std::size_t>(i)], 0.0, 0.0
        };
    }
    return grid;
}

std::array<latebrake::LiveGridVerticalSample,
           latebrake::StartGridCapacity> GoodVertical()
{
    using namespace latebrake;
    std::array<LiveGridVerticalSample, StartGridCapacity> samples{};
    for (LiveGridVerticalSample& sample : samples)
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
    return samples;
}
}

int main()
{
    using namespace latebrake;
    static_assert(StartGridCapacity == 6);
    static_assert(LiveGridMaxPoseErrorCm > 0.0);
    static_assert(LiveGridMaxPlanarSpeedCmS < 500.0);
    Require(IsPlannedStartGridSafe(), "authored grid must pass its own QA");

    const auto ready = GoodGrid();
    const auto vertical = GoodVertical();
    Require(EvaluateLiveSixCarStartingGrid(ready, vertical, 6).Safe(),
            "six distinct real bodies at audited locations admit countdown");
    Require(EvaluateLiveSixCarStartingGrid(ready, vertical, 5).Failure ==
                LiveGridFailure::MissingPhysicalCar,
            "missing actual sixth vehicle must not admit a five-car GO");
    Require(EvaluateLiveSixCarStartingGrid(ready, vertical, 0).Failure ==
                LiveGridFailure::MissingPhysicalCar &&
            EvaluateLiveSixCarStartingGrid(ready, vertical, 7).Failure ==
                LiveGridFailure::MissingPhysicalCar,
            "zero or oversized samples are always rejected");

    // The new vertical gate is mandatory: an absent sample cannot inherit
    // the previous tick's apparently safe XY position or body identity.
    auto vChanged = vertical;
    vChanged[0] = {};
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::InvalidPhysics,
            "default/missing vertical sample fails closed");
    vChanged = vertical;
    vChanged[3].ChassisWorldZCm += LiveGridMaxVerticalDisplacementCm + 0.01;
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::UnsafeVerticalPosition,
            "hovering car within its XY grid lane cannot start");
    vChanged = vertical;
    vChanged[3].ChassisWorldZCm -= LiveGridMaxVerticalDisplacementCm + 0.01;
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::UnsafeVerticalPosition,
            "sunken body cannot start even with plausible XY coordinates");
    vChanged = vertical;
    vChanged[4].Ground.ChassisVerticalSpeedCmPerSecond =
        LiveGridMaxVerticalSpeedCmS + 0.01;
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::MovingBeforeGo,
            "falling or jumping chassis cannot be admitted at GO");
    vChanged = vertical;
    vChanged[2].Ground.ChassisVerticalSpeedCmPerSecond =
        -LiveGridMaxVerticalSpeedCmS - 0.01;
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::MovingBeforeGo,
            "downward velocity also blocks GO");
    vChanged = vertical;
    vChanged[1].Ground.PhysicsReady = false;
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::InvalidPhysics,
            "fake ground traces without a verified physics frame fail closed");
    vChanged = vertical;
    vChanged[1].Ground.DeltaSeconds = 0.2;
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::InvalidPhysics,
            "hitch frame cannot claim fresh ground support");
    vChanged = vertical;
    vChanged[5].Ground.Probes[0].Hit = false;
    vChanged[5].Ground.Probes[1].Hit = false;
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::InsufficientGroundSupport,
            "two of four supports cannot prove stationary start");
    vChanged = vertical;
    vChanged[5].Ground.Probes[0].Hit = false;
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Safe(),
            "three real valid supports retain a conservative start");
    vChanged = vertical;
    vChanged[3].Ground.Probes[2].GroundNormalZ = 0.1;
    vChanged[3].Ground.Probes[1].GroundNormalZ = 0.1;
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::InsufficientGroundSupport,
            "steep wall normals never count as asphalt support");
    vChanged = vertical;
    vChanged[4].Ground.Probes[2].DistanceCm =
        ChassisContactMaximumSupportedHeightCm + 0.01;
    vChanged[4].Ground.Probes[3].DistanceCm =
        ChassisContactMaximumSupportedHeightCm + 0.01;
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::InsufficientGroundSupport,
            "distant traces cannot fabricate supported car contact");
    vChanged = vertical;
    vChanged[0].ChassisWorldZCm =
        std::numeric_limits<double>::quiet_NaN();
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::InvalidPhysics,
            "nonfinite world Z never qualifies");
    vChanged = vertical;
    vChanged[0].Ground.Probes[3].DistanceCm =
        std::numeric_limits<double>::infinity();
    Require(EvaluateLiveSixCarStartingGrid(ready, vChanged, 6).Failure ==
                LiveGridFailure::InvalidPhysics,
            "nonfinite live ground trace never qualifies");

    auto changed = ready;
    changed[5].Verified = false;
    Require(EvaluateLiveSixCarStartingGrid(changed, vertical, 6).Failure ==
                LiveGridFailure::MissingPhysicalCar,
            "pointer without verified physical chassis cannot start");
    changed = ready;
    changed[3].Slot = 4;
    Require(EvaluateLiveSixCarStartingGrid(changed, vertical, 6).Failure ==
                LiveGridFailure::MissingPhysicalCar,
            "incorrect grid actor identity slot is not genuine six-car roster");
    changed = ready;
    changed[1].ActorToken = 0;
    Require(EvaluateLiveSixCarStartingGrid(changed, vertical, 6).Failure ==
                LiveGridFailure::MissingPhysicalCar,
            "anonymous primary opponent fails closed");
    changed = ready;
    changed[5].ActorToken = changed[1].ActorToken;
    const LiveGridValidation duplicated =
        EvaluateLiveSixCarStartingGrid(changed, vertical, 6);
    Require(duplicated.Failure ==
                LiveGridFailure::DuplicateActorIdentity &&
            duplicated.Slot == 5 && duplicated.OtherSlot == 1,
            "duplicate Unreal UniqueID cannot stand for two opponents");

    changed = ready;
    changed[2].Pose.Position.X += LiveGridMaxPoseErrorCm + 0.01;
    Require(EvaluateLiveSixCarStartingGrid(changed, vertical, 6).Failure ==
                LiveGridFailure::DisplacedFromAssignedSlot,
            "car physically shifted away from authored cell denies GO");
    changed = ready;
    changed[2].Pose.HeadingDegrees +=
        LiveGridMaxHeadingErrorDegrees + 0.01;
    Require(EvaluateLiveSixCarStartingGrid(changed, vertical, 6).Failure ==
                LiveGridFailure::DisplacedFromAssignedSlot,
            "car rotated out of its grid lane denies GO");
    changed = ready;
    changed[2].Pose.HeadingDegrees += 360.0;
    Require(EvaluateLiveSixCarStartingGrid(changed, vertical, 6).Safe(),
            "equivalent yaw modulo a full revolution remains qualified");

    changed = ready;
    changed[2].VelocityX = LiveGridMaxPlanarSpeedCmS + 0.01;
    Require(EvaluateLiveSixCarStartingGrid(changed, vertical, 6).Failure ==
                LiveGridFailure::MovingBeforeGo,
            "fast moving real car invalidates otherwise static grid");
    changed = ready;
    changed[3].VelocityY = std::numeric_limits<double>::infinity();
    Require(EvaluateLiveSixCarStartingGrid(changed, vertical, 6).Failure ==
                LiveGridFailure::InvalidPhysics,
            "nonfinite Chaos velocity cannot permit start");
    changed = ready;
    changed[0].Pose.Position.X =
        std::numeric_limits<double>::quiet_NaN();
    Require(EvaluateLiveSixCarStartingGrid(changed, vertical, 6).Failure ==
                LiveGridFailure::InvalidPhysics,
            "human car missing finite pose blocks physical fleet GO");

    // Source geometry separates slot 2 from slot 4 by just 510 cm.
    // Each padded body is 470 cm long. A 50 cm collision displacement
    // may be WITHIN each individual pose tolerance, yet two bodies now
    // touch in the same lane. Only the pairwise OBB check catches it.
    changed = ready;
    changed[2].Pose.Position.X -= 50.0;
    const LiveGridValidation overlap =
        EvaluateLiveSixCarStartingGrid(changed, vertical, 6);
    Require(overlap.Failure == LiveGridFailure::CarOverlap &&
            overlap.Slot == 4 && overlap.OtherSlot == 2,
            "close but individually plausible grid poses must not intersect");

    // A bounded test sweep catches missing near-limit checks and ensures
    // a truly stationary short-lateral jitter can remain GO-qualified.
    for (int trial = 0; trial < 4096; ++trial)
    {
        changed = ready;
        const int slot = trial % StartGridCapacity;
        const double nudge = ((trial / StartGridCapacity) % 21 - 10) * 3.0;
        changed[static_cast<std::size_t>(slot)].Pose.Position.Y += nudge;
        changed[static_cast<std::size_t>(slot)].VelocityY =
            (trial & 1) == 0 ? 0.0 : 30.0;
        const LiveGridValidation result =
            EvaluateLiveSixCarStartingGrid(changed, vertical, 6);
        if (result.Safe())
        {
            Require(std::abs(nudge) <= LiveGridMaxPoseErrorCm,
                    "qualified grid remains within exact slot tolerance");
            for (int i = 0; i < StartGridCapacity; ++i)
            {
                for (int j = 0; j < i; ++j)
                {
                    Require(!StartGridFootprintsOverlap(
                        changed[static_cast<std::size_t>(i)].Pose,
                        changed[static_cast<std::size_t>(j)].Pose),
                        "qualified grid cannot contain overlapping bodies");
                }
            }
        }
    }
    std::cout << "Six real car launch alignment and 4096 jitter cases passed.\n";
    return EXIT_SUCCESS;
}
