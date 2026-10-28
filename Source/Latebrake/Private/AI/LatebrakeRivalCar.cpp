#include "AI/LatebrakeRivalCar.h"

#include "AI/RivalPilot.h"
#include "AI/RivalCornerAnticipation.h"
#include "AI/RivalTrafficAwareness.h"
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
    FollowingBrakeHold.Reset();
    SetAutonomousControls(0.f, 0.f, false);
}

void ALatebrakeRivalCar::ConfigurePilot(int32 CheckpointCount, int32 RequiredLaps)
{
    // Do not preserve approximation/recovery history from a different
    // event or replay. The RaceProgress clock remains player-authoritative.
    Route.Configure(CheckpointCount, RequiredLaps);
    FollowingBrakeHold.Reset();
    SetAutonomousControls(0.f, 0.f, false);
}

void ALatebrakeRivalCar::Tick(float DeltaSeconds)
{
    const ALatebrakeRaceGameMode* Mode =
        GetWorld() ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    // A malformed rival Chaos body must neither accrue approximate route
    // progress nor receive another physics control update. The base pawn
    // independently neutralizes the chassis before it can apply forces.
    if (!Mode || !Mode->IsRaceStartGreen() ||
        !IsRacePhysicsReady() || !latebrake::CanDriveRival(
            Mode->GetRace(), Mode->GetSelectedEvent(),
            Mode->IsCircuitReadyForGameplay(),
            UGameplayStatics::IsGamePaused(this) || Mode->IsMenuOpen() ||
                Mode->IsFocusLossGuardActive() || Mode->IsManualRacePauseActive(),
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
        // Record a recovery outcome only after the same verified engine
        // placement policy used by the local player's paid RECOVER runs.
        // A rejected/misplaced/unstable teleport is a failed ATTEMPT, not a
        // successful recovery. The finite attempt budget is never refunded.
        const bool bRecovered = TryRecoverTo(
            FVector(Pose.Position.X, Pose.Position.Y, 180.0),
            FRotator(0.0, Pose.HeadingDegrees, 0.0));
        const bool bReported = Route.ReportRecoveryResult(bRecovered);
        if (!bReported || !bRecovered)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Latebrake: rival recovery placement not verified; retry after new stall"));
        }
        // A failed placement must not reuse the old acceleration command.
        // Keep the waypoint/lap history even when a recovery fails.
        SetAutonomousControls(0.f, 0.f, !bRecovered);
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
    const latebrake::PilotCommand Command = latebrake::AnticipateRivalCorner(
        latebrake::PlanRivalTurn(
            Position.X, Position.Y, Heading, FlatSpeed,
            NextTarget.X, NextTarget.Y),
        Route.GetNextWaypoint(), {Position.X, Position.Y}, FlatSpeed);

    // Plan the control for this physics tick, not one frame after applying
    // the previous command.
    // A real possessed player can be driving just ahead of the rival on
    // the same circuit lane. Do not change route steering or invent an
    // overtaking path: only reduce throttle or request ordinary braking
    // when the rival is actually closing on an observed in-lane car.
    const ALatebrakeCarPawn* Player = Cast<ALatebrakeCarPawn>(
        UGameplayStatics::GetPlayerPawn(this, 0));
    const bool bPlayerReady = IsValid(Player) && Player->IsRacePhysicsReady();
    const FVector PlayerPosition = bPlayerReady
        ? Player->GetActorLocation() : FVector::ZeroVector;
    const FVector PlayerVelocity = bPlayerReady
        ? Player->GetVelocity() : FVector::ZeroVector;
    const latebrake::PilotCommand TrafficCommand =
        latebrake::YieldRivalToPlayer(Command, {
            Position.X, Position.Y, Heading, FlatSpeed,
            PlayerPosition.X, PlayerPosition.Y,
            PlayerVelocity.X, PlayerVelocity.Y, bPlayerReady
        });
    SetAutonomousControls(static_cast<float>(TrafficCommand.Throttle),
                          static_cast<float>(TrafficCommand.Steering),
                          TrafficCommand.Brake);
    Super::Tick(DeltaSeconds);
}
