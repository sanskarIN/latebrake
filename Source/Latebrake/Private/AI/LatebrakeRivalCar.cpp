#include "AI/LatebrakeRivalCar.h"

#include "AI/RivalPilot.h"
#include "Engine/World.h"
#include "Game/LatebrakeRaceGameMode.h"

namespace
{
const FVector RivalStart(-2250.f, -1450.f, 180.f);
const FVector CircuitWaypoints[] =
{
    FVector(2800.f, -1800.f, 0.f),
    FVector(2800.f, 1800.f, 0.f),
    FVector(-2800.f, 1800.f, 0.f),
    FVector(-2800.f, -1800.f, 0.f)
};
const FVector RecoveryPositions[] =
{
    RivalStart,
    FVector(2800.f, -1300.f, 180.f),
    FVector(2300.f, 1800.f, 180.f),
    FVector(-2800.f, 1300.f, 180.f)
};
const FRotator RecoveryRotations[] =
{
    FRotator::ZeroRotator,
    FRotator(0.f, 90.f, 0.f),
    FRotator(0.f, 180.f, 0.f),
    FRotator(0.f, -90.f, 0.f)
};
static_assert(UE_ARRAY_COUNT(CircuitWaypoints) == UE_ARRAY_COUNT(RecoveryPositions));
}

ALatebrakeRivalCar::ALatebrakeRivalCar()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void ALatebrakeRivalCar::ResetPilot()
{
    Route.Reset();
    SetAutonomousControls(0.f, 0.f, false);
}

void ALatebrakeRivalCar::Tick(float DeltaSeconds)
{
    const ALatebrakeRaceGameMode* Mode =
        GetWorld() ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || Mode->GetRace().IsFinished() ||
        Mode->GetSelectedEvent() != latebrake::EventKind::Circuit ||
        Route.IsFinished())
    {
        SetAutonomousControls(0.f, 0.f, true);
        Super::Tick(DeltaSeconds);
        return;
    }

    const FVector Position = GetActorLocation();
    const FVector Velocity = GetVelocity();
    const double FlatSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();
    const double Heading = FMath::DegreesToRadians(static_cast<double>(GetActorRotation().Yaw));
    const int32 Next = Route.GetNextWaypoint();
    const FVector Target = CircuitWaypoints[Next];
    const double Distance = FVector::Dist2D(Position, Target);

    const latebrake::RouteObservation Observation = Route.Observe(
        Distance, FlatSpeed, static_cast<double>(DeltaSeconds));

    if (Observation.NeedsRecovery)
    {
        const int32 RecoveryIndex = Route.GetNextWaypoint();
        RecoverTo(RecoveryPositions[RecoveryIndex], RecoveryRotations[RecoveryIndex]);
        // Reset controls but keep waypoint/lap count and recovery history.
        SetAutonomousControls(0.f, 0.f, false);
        Super::Tick(DeltaSeconds);
        return;
    }

    if (Observation.Completed)
    {
        SetAutonomousControls(0.f, 0.f, true);
        Super::Tick(DeltaSeconds);
        return;
    }

    const FVector NextTarget = CircuitWaypoints[Route.GetNextWaypoint()];
    const latebrake::PilotCommand Command = latebrake::PlanRivalTurn(
        Position.X, Position.Y, Heading, FlatSpeed,
        NextTarget.X, NextTarget.Y);

    // Plan the control for this physics tick, not one frame after applying
    // the previous command.
    SetAutonomousControls(static_cast<float>(Command.Throttle),
                          static_cast<float>(Command.Steering),
                          Command.Brake);
    Super::Tick(DeltaSeconds);
}
