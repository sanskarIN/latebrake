#include "AI/LatebrakeRivalCar.h"

#include "AI/RivalPilot.h"
#include "AI/RivalGateRunThrough.h"
#include "AI/RivalCornerAnticipation.h"
#include "AI/RivalTrafficAwareness.h"
#include "AI/RivalYawRateSteering.h"
#include "AI/RivalDriveReadiness.h"
#include "Track/CircuitLayout.h"
#include "Components/StaticMeshComponent.h"
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
    RivalGateTrace.Reset();
    FollowingBrakeHold.Reset();
    GripPace.Reset();
    SurfaceDamagePace.Reset();
    SpinThrottleGuard.Reset();
    SetAutonomousControls(0.f, 0.f, false);
}

void ALatebrakeRivalCar::ConfigurePilot(int32 CheckpointCount, int32 RequiredLaps)
{
    // Do not preserve approximation/recovery history from a different
    // event or replay. The RaceProgress clock remains player-authoritative.
    Route.Configure(CheckpointCount, RequiredLaps);
    RivalGateTrace.Reset();
    FollowingBrakeHold.Reset();
    GripPace.Reset();
    SurfaceDamagePace.Reset();
    SpinThrottleGuard.Reset();
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
        RivalGateTrace.Reset();
        FollowingBrakeHold.Reset();
        GripPace.Reset();
        SurfaceDamagePace.Reset();
        SpinThrottleGuard.Reset();
        SetAutonomousControls(0.f, 0.f, true);
        Super::Tick(DeltaSeconds);
        return;
    }

    const FVector Position = GetActorLocation();
    const FVector Velocity = GetVelocity();
    const double FlatSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();
    const double Heading = FMath::DegreesToRadians(static_cast<double>(GetActorRotation().Yaw));
    const int32 Next = Route.GetNextWaypoint();
    if (!latebrake::IsCircuitGate(Next))
    {
        // An event configured with more gates than the original four-point
        // circuit must not steer the rival toward the invalid {0,0} fallback.
        RivalGateTrace.Reset();
        FollowingBrakeHold.Reset();
        GripPace.Reset();
        SurfaceDamagePace.Reset();
        SpinThrottleGuard.Reset();
        SetAutonomousControls(0.f, 0.f, true);
        Super::Tick(DeltaSeconds);
        return;
    }
    const latebrake::CircuitPoint Waypoint = latebrake::CircuitGate(Next);
    const FVector Target(Waypoint.X, Waypoint.Y, 0.0);
    const double Distance = FVector::Dist2D(Position, Target);

    // Apply the same 3D directional crossing geometry as real player
    // checkpoint actors. A car merely near a gate, reversed through it,
    // outside the lane, or teleported across it cannot earn rival laps.
    // The trace is reset on every race interruption and engine recovery.
    const bool bVerifiedGateCrossed = RivalGateTrace.Sample(
        Next, 0, {{Position.X, Position.Y}, Position.Z},
        Velocity.X, Velocity.Y);
    // The distance/speed still drive the existing stationary and
    // moving-without-approach recovery policies when no gate was crossed.
    const latebrake::RouteObservation Observation = Route.ObserveVerifiedGate(
        Distance, FlatSpeed, static_cast<double>(DeltaSeconds),
        bVerifiedGateCrossed);
    if (Observation.Advanced)
    {
        // Never bridge a frame sample across two different gate planes.
        RivalGateTrace.Reset();
    }

    if (Observation.NeedsRecovery)
    {
        RivalGateTrace.Reset();
        FollowingBrakeHold.Reset();
        GripPace.Reset();
        SurfaceDamagePace.Reset();
        SpinThrottleGuard.Reset();
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
        RivalGateTrace.Reset();
        FollowingBrakeHold.Reset();
        GripPace.Reset();
        SurfaceDamagePace.Reset();
        SpinThrottleGuard.Reset();
        SetAutonomousControls(0.f, 0.f, true);
        Super::Tick(DeltaSeconds);
        return;
    }

    // The route only advances after a verified forward gate-plane crossing,
    // not when we approach its 440cm radius. Aim 550cm THROUGH that plane
    // and disable the generic proximity-arrival stop. Otherwise the rival
    // can halt before crossing and never complete the real gated lap.
    const latebrake::CircuitPoint NextTarget =
        latebrake::RivalGateRunThroughPoint(Route.GetNextWaypoint());
    latebrake::PilotCommand Command = latebrake::AnticipateRivalCorner(
        latebrake::PlanRivalTurn(
            Position.X, Position.Y, Heading, FlatSpeed,
            NextTarget.X, NextTarget.Y, 440.0, false),
        Route.GetNextWaypoint(), {Position.X, Position.Y}, FlatSpeed);

    // Rival pace adapts to previous-tick real chassis slip, last valid
    // grip condition and current steering demand. It may only throttle
    // down or request braking; it cannot invent route or traffic state.
    Command = GripPace.Apply(Command, {
        static_cast<double>(DeltaSeconds), FlatSpeed,
        static_cast<double>(FVector::DotProduct(
            Velocity, GetActorRightVector())),
        GetTireCondition().GripMultiplier(),
        GetTireCondition().HasValidSample()
    });

    // The prior qualified chassis tick already measured actual paved
    // surface hysteresis and accepted collision damage. Use those real,
    // bounded grip effects to back off corner pace for the physical rival
    // without changing its route, player traffic or authoritative scores.
    // There is deliberately no invented terrain or damage prediction.
    const bool bOffPavement = GetSurfaceTraction().IsOffPavement();
    Command = SurfaceDamagePace.Apply(Command, {
        static_cast<double>(DeltaSeconds), FlatSpeed,
        static_cast<double>(FVector::DotProduct(
            Velocity, GetActorRightVector())),
        bOffPavement
            ? latebrake::ArcadeSurfaceTraction::OffPavementGripMultiplier
            : 1.0,
        GetCollisionDamage().LateralGripMultiplier(),
        bOffPavement,
        GetSurfaceTraction().HasValidSample(),
        GetCollisionDamage().HasEligibleFrame()
    });

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
            PlayerVelocity.X, PlayerVelocity.Y, bPlayerReady,
            Velocity.X, Velocity.Y
        });
    // A short, fail-closed hold prevents brake chatter near the
    // projected contact boundary; it cannot re-route or score a rival.
    const latebrake::PilotCommand FollowingCommand =
        FollowingBrakeHold.Apply(
            TrafficCommand, static_cast<double>(DeltaSeconds),
            bPlayerReady, TrafficCommand.Brake && !Command.Brake);

    // Read real Chaos angular velocity AFTER all route, corner, road,
    // collision and traffic decisions. Reduce same-direction steering
    // only when the chassis is already rotating quickly at speed.
    // This does not change braking, throttle, waypoint or recovery logic.
    const FVector ActualAngularVelocity =
        Chassis->GetPhysicsAngularVelocityInRadians();
    const latebrake::PilotCommand SafeTrafficCommand =
        latebrake::ModerateRivalYawRate(FollowingCommand, {
            static_cast<double>(DeltaSeconds),
            FlatSpeed,
            static_cast<double>(ActualAngularVelocity.Z),
            true
        });
    // Apply a final, bounded propulsion-only spin guard using actual
    // Chaos angular velocity and chassis lateral motion. Never steer,
    // brake, award laps or bypass existing route/traffic authority.
    const latebrake::PilotCommand SafeSpinCommand =
        SpinThrottleGuard.Apply(SafeTrafficCommand, {
            static_cast<double>(DeltaSeconds), FlatSpeed,
            static_cast<double>(FVector::DotProduct(
                Velocity, GetActorRightVector())),
            static_cast<double>(ActualAngularVelocity.Z), true
        });
    SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle),
                          static_cast<float>(SafeSpinCommand.Steering),
                          SafeSpinCommand.Brake);
    Super::Tick(DeltaSeconds);
}
