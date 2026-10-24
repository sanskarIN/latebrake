#include "AI/LatebrakeRivalCar.h"

#include "AI/RivalPilot.h"
#include "AI/RivalDriveReadiness.h"
#include "Track/CircuitLayout.h"
#include "Engine/World.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "Kismet/GameplayStatics.h"

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

void ALatebrakeRivalCar::ConfigurePilot(int32 CheckpointCount, int32 RequiredLaps)
{
    // Do not preserve approximation/recovery history from a different
    // event or replay. The RaceProgress clock remains player-authoritative.
    Route.Configure(CheckpointCount, RequiredLaps);
    SetAutonomousControls(0.f, 0.f, false);
}

void ALatebrakeRivalCar::Tick(float DeltaSeconds)
{
    const ALatebrakeRaceGameMode* Mode =
        GetWorld() ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !latebrake::CanDriveRival(
            Mode->GetRace(), Mode->GetSelectedEvent(),
            Mode->IsCircuitReadyForGameplay(),
            UGameplayStatics::IsGamePaused(this) || Mode->IsMenuOpen() ||
                Mode->IsFocusLossGuardActive(),
            Mode->HasLocalPlayerDriver(), Route.IsFinished()))
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
    const latebrake::CircuitPoint Waypoint = latebrake::CircuitGate(Next);
    const FVector Target(Waypoint.X, Waypoint.Y, 0.0);
    const double Distance = FVector::Dist2D(Position, Target);

    const latebrake::RouteObservation Observation = Route.Observe(
        Distance, FlatSpeed, static_cast<double>(DeltaSeconds));

    if (Observation.NeedsRecovery)
    {
        const int32 RecoveryIndex = Route.GetNextWaypoint();
        const latebrake::CircuitPose Pose = latebrake::RivalRecovery(RecoveryIndex);
        RecoverTo(FVector(Pose.Position.X, Pose.Position.Y, 180.0),
                  FRotator(0.0, Pose.HeadingDegrees, 0.0));
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

    const latebrake::CircuitPoint NextTarget =
        latebrake::CircuitGate(Route.GetNextWaypoint());
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
