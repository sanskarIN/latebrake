#include "AI/LatebrakeRivalCar.h"

#include "AI/RivalPilot.h"
#include "AI/RivalGateRunThrough.h"
#include "AI/RivalCornerAnticipation.h"
#include "AI/RivalCornerBrakeEnvelope.h"
#include "AI/RivalTrafficAwareness.h"
#include "AI/RivalYawRateSteering.h"
#include "AI/RivalPassTrafficResolution.h"
#include "AI/RivalEngineHeatPace.h"
#include "AI/RivalDriveReadiness.h"
#include "AI/RivalRecoveryOccupancy.h"
#include "AI/RivalFleetTraffic.h"
#include "AI/RivalRecoveryGrid.h"
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

bool ALatebrakeRivalCar::AssignGridSlot(int32 NewSlot)
{
    if (NewSlot < latebrake::PrimaryRivalGridSlot ||
        NewSlot >= latebrake::StartGridCapacity)
    {
        return false;
    }
    if (NewSlot != GridSlot)
    {
        ResetPilot(); // A different actor identity starts from no prior route.
        GridSlot = NewSlot;
    }
    return true;
}

void ALatebrakeRivalCar::ResetPilot()
{
    Route.Reset();
    RivalGateTrace.Reset();
    FollowingBrakeHold.Reset();
    GripPace.Reset();
    SurfaceDamagePace.Reset();
    SpinThrottleGuard.Reset();
    FleetRiskSupervisor.Reset();
        StraightPassAssist.Reset();
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
    FleetRiskSupervisor.Reset();
        StraightPassAssist.Reset();
    SetAutonomousControls(0.f, 0.f, false);
}

void ALatebrakeRivalCar::Tick(float DeltaSeconds)
{
    ALatebrakeRaceGameMode* Mode =
        GetWorld() ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    // A malformed rival Chaos body must neither accrue approximate route
    // progress nor receive another physics control update. The base pawn
    // independently neutralizes the chassis before it can apply forces.
    if (!Mode || !Mode->IsRaceStartGreen() ||
        !Mode->IsRegisteredRivalActor(this) ||
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
        FleetRiskSupervisor.Reset();
        StraightPassAssist.Reset();
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
        FleetRiskSupervisor.Reset();
        StraightPassAssist.Reset();
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
        // Only the primary physical rival owns the live one-rival HUD
        // observations. Other future rival actors cannot impersonate it.
        if (Mode->IsPrimaryRivalActor(this))
        {
            Mode->ObserveRivalVerifiedGate();
        }
        else
        {
            Mode->ObserveAdditionalRivalVerifiedGate(this);
        }
    }

    if (Observation.NeedsRecovery)
    {
        RivalGateTrace.Reset();
        FollowingBrakeHold.Reset();
        GripPace.Reset();
        SurfaceDamagePace.Reset();
        SpinThrottleGuard.Reset();
        FleetRiskSupervisor.Reset();
        StraightPassAssist.Reset();
        const int32 RecoveryIndex = Route.GetNextWaypoint();
        const latebrake::CircuitPose Pose = latebrake::RivalRecovery(RecoveryIndex);
        // The legacy primary uses the original next-gate recovery pose.
        // Additional test cars return to their own already-audited grid
        // lane, never all teleport onto the same rival recovery point.
        const latebrake::CircuitPose LandingPose =
            GridSlot == latebrake::PrimaryRivalGridSlot
                ? Pose
                : latebrake::PlannedRivalRecoveryPose(RecoveryIndex, GridSlot);
        // Collect ACTUAL positions/velocities of the human and every
        // remaining physical AI actor. An absent or corrupt participant
        // cannot be silently removed from the candidate collision set.
        // Only a fully observed fleet may authorize a physical teleport.
        std::array<latebrake::RecoveryObstacle,
                   latebrake::MaxRecoveryObstacles> Blockers{};
        int32 BlockerCount = 0;
        const bool bFleetObserved = Mode->TryCollectPhysicalTraffic(
            this, Blockers, BlockerCount);
        const latebrake::RecoveryClearanceResult Clearance =
            latebrake::EvaluateRivalRecoveryClearance(
                LandingPose, Blockers, bFleetObserved ? BlockerCount : 0);
        if (!Clearance.Safe())
        {
            // A rejected occupancy check is a FAILED recovery attempt,
            // not a successful (or free) teleport. Honor the route budget.
            const bool bReported = Route.ReportRecoveryResult(false);
            if (bReported)
            {
                if (Mode->IsPrimaryRivalActor(this))
                {
                    Mode->ObserveRivalRecoveryResult(this, false);
                }
                else
                {
                    Mode->ObserveAdditionalRivalRecoveryResult(this, false);
                }
            }
            SetAutonomousControls(0.f, 0.f, true);
            Super::Tick(DeltaSeconds);
            return;
        }
        // Actual pawn recovery remains the final collision/physics gate.
        // A rejected/misplaced/unstable teleport is a failed ATTEMPT, not a
        // successful recovery. The finite attempt budget is never refunded.
        const bool bRecovered = TryRecoverTo(
            FVector(LandingPose.Position.X, LandingPose.Position.Y, 180.0),
            FRotator(0.0, LandingPose.HeadingDegrees, 0.0));
        const bool bReported = Route.ReportRecoveryResult(bRecovered);
        if (bReported)
        {
            if (Mode->IsPrimaryRivalActor(this))
            {
                Mode->ObserveRivalRecoveryResult(this, bRecovered);
            }
            else
            {
                Mode->ObserveAdditionalRivalRecoveryResult(this, bRecovered);
            }
        }
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
        FleetRiskSupervisor.Reset();
        StraightPassAssist.Reset();
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

    // The original anticipator uses a fixed approach window; the newly
    // bounded physical brake forces need an earlier, distance-based forecast.
    // Measure the previous QUALIFIED pawn tick's actual brake heat, tire
    // condition and contact-surface state. Do not predict imaginary grip
    // after focus loss, a recovery teleport or malformed Chaos physics.
    const bool bCornerPhysicsQualified =
        GetSurfaceTraction().HasValidSample() &&
        GetTireCondition().HasValidSample() &&
        GetCollisionDamage().HasEligibleFrame() &&
        GetBrakeHeat().HasValidSample();
    const latebrake::RivalCornerBrakeResult BrakeForecast =
        latebrake::ApplyRivalCornerBrakeEnvelope(Command, {
            Route.GetNextWaypoint(),
            {Position.X, Position.Y}, FlatSpeed,
            GetSurfaceTraction().IsOffPavement()
                ? latebrake::ArcadeSurfaceTraction::OffPavementGripMultiplier
                : 1.0,
            GetTireCondition().GripMultiplier(),
            GetCollisionDamage().LateralGripMultiplier(),
            GetBrakeHeat().ServiceBrakeMultiplier(),
            GetHandlingMode() == ELatebrakeHandlingMode::Arcade,
            bCornerPhysicsQualified});
    Command = BrakeForecast.Command;

    // Apply selected Quick Race difficulty only to this already-valid
    // planner command. Existing grip/traffic/heat/spin safety limits run
    // afterward and can always reduce the requested motor demand.
    Command = latebrake::ApplyRivalDifficulty(
        Command, Mode->GetActiveRivalDifficulty(), FlatSpeed,
        IsRacePhysicsReady());

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

    // The previous qualified physical engine-heat sample can modestly
    // reduce rival throttle at road speed, letting the real shared motor
    // policy cool. Low-speed launch and prior braking stay unchanged.
    Command = latebrake::ModerateRivalEngineHeatPace(Command, {
        static_cast<double>(DeltaSeconds), FlatSpeed,
        GetEngineHeat().HeatFraction(),
        GetEngineHeat().HasValidSample(),
        IsRacePhysicsReady()});

    // Plan the control for this physics tick, not one frame after applying
    // the previous command. A fully qualified, same-straight slow player
    // permits a limited passing lane steering target; otherwise preserve
    // the real route's steering and use the existing emergency following
    // and corner-brake systems. No passing command can award a checkpoint.
    const ALatebrakeCarPawn* Player = Cast<ALatebrakeCarPawn>(
        UGameplayStatics::GetPlayerPawn(this, 0));
    const bool bPlayerReady = IsValid(Player) && Player->IsRacePhysicsReady();
    const FVector PlayerPosition = bPlayerReady
        ? Player->GetActorLocation() : FVector::ZeroVector;
    const FVector PlayerVelocity = bPlayerReady
        ? Player->GetVelocity() : FVector::ZeroVector;
    // A genuine physical single-rival passing decision uses the actual
    // licensed-engine positions/speeds, not a scripted opponent phantom.
    // Passing only changes bounded STEERING on a paved, away-from-corners
    // straight. The independent traffic-brake guard still runs afterward.
    const latebrake::RivalPassOutput Passing =
        Mode->IsPrimaryRivalActor(this)
            ? StraightPassAssist.Apply(Command, {
            static_cast<double>(DeltaSeconds),
            Route.GetNextWaypoint(),
            {Position.X, Position.Y},
            {PlayerPosition.X, PlayerPosition.Y},
            Heading, Velocity.X, Velocity.Y,
            PlayerVelocity.X, PlayerVelocity.Y,
            bPlayerReady && IsRacePhysicsReady() &&
                GetSurfaceTraction().HasValidSample() &&
                GetTireCondition().HasValidSample() &&
                GetCollisionDamage().HasEligibleFrame(),
            GetSurfaceTraction().HasValidSample() &&
                !GetSurfaceTraction().IsOffPavement()})
            : latebrake::RivalPassOutput{Command, false, 0.0};
    const latebrake::PilotCommand SafePassCommand = Passing.Command;
    const latebrake::PilotCommand TrafficCommand =
        latebrake::YieldRivalToPlayer(SafePassCommand, {
            Position.X, Position.Y, Heading, FlatSpeed,
            PlayerPosition.X, PlayerPosition.Y,
            PlayerVelocity.X, PlayerVelocity.Y, bPlayerReady,
            Velocity.X, Velocity.Y
        });
    // Existing player courtesy-braking remains, but an independent
    // finite multi-car forecast now observes every *physically verified*
    // rival as well. It may only remove motor demand or add braking;
    // an unsafe/missing opponent sample is a safe hard brake.
    // These extra sweeps are only run during a completely established
    // opt-in physical fleet; default gameplay retains its old CPU path.
    const bool bUseFleetTraffic =
        Mode->IsExperimentalSixCarGridEnabled();
    std::array<latebrake::RecoveryObstacle,
               latebrake::MaxRecoveryObstacles> TrafficBlockers{};
    int32 TrafficBlockerCount = 0;
    const bool bTrafficQualified = bUseFleetTraffic &&
        Mode->TryCollectPhysicalTraffic(
            this, TrafficBlockers, TrafficBlockerCount);
    // Real previous accepted chassis observations constrain additional
    // predictive stopping distance; unknown physics cannot grant throttle.
    const bool bPredictiveBrakeQualified =
        bUseFleetTraffic && bCornerPhysicsQualified;
    const double BrakeCapabilityScale = bPredictiveBrakeQualified
        ? GetTireCondition().GripMultiplier() *
          GetBrakeHeat().ServiceBrakeMultiplier() *
          GetCollisionDamage().LateralGripMultiplier() *
          (bOffPavement
              ? latebrake::ArcadeSurfaceTraction::OffPavementGripMultiplier
              : 1.0)
        : 0.0;
    const latebrake::RivalFleetTrafficSample FleetSnapshot{
        {true, GridSlot, static_cast<uint64>(GetUniqueID()),
            {{Position.X, Position.Y},
             static_cast<double>(GetActorRotation().Yaw)},
            Velocity.X, Velocity.Y},
        TrafficBlockers,
        bTrafficQualified ? TrafficBlockerCount : 0,
        bTrafficQualified,
        bPredictiveBrakeQualified,
        BrakeCapabilityScale};
    const latebrake::RivalFleetTrafficResult FleetTraffic =
        bUseFleetTraffic
            ? latebrake::ApplyRivalFleetTraffic(TrafficCommand, FleetSnapshot)
            : latebrake::RivalFleetTrafficResult{
                TrafficCommand, true, false, -1};
    // Preserve the established one-rival handling in normal launches.
    // The newly conservative fleet braking is dev-opt-in only and does
    // not silently change retail pace or primary rival difficulty.
    const latebrake::PilotCommand FleetTrafficCommand =
        bUseFleetTraffic ? FleetTraffic.Command : TrafficCommand;
    // Additional longer-lookahead, six-actor identity-checked predictive
    // braking runs only in the explicit non-shipping fleet. It may LOWER
    // motor demand or ADD a brake, but never steer, move or score a car.
    // Independent existing short-horizon fleet and corner limits win.
    if (!bUseFleetTraffic && FleetRiskSupervisor.HasPhysicalRoster())
    {
        FleetRiskSupervisor.Reset();
    }
    const latebrake::FleetRiskSupervision RiskSupervision =
        bUseFleetTraffic
            ? FleetRiskSupervisor.Apply(
                FleetTrafficCommand, FleetSnapshot,
                static_cast<double>(DeltaSeconds))
            : latebrake::FleetRiskSupervision{
                FleetTrafficCommand, {}, true, false, false, 0.0, 1.0};
    // A short, fail-closed hold prevents brake chatter near the
    // projected contact boundary; it cannot re-route or score a rival.
    const latebrake::PilotCommand HeldTrafficCommand =
        FollowingBrakeHold.Apply(
            RiskSupervision.Command, static_cast<double>(DeltaSeconds),
            bUseFleetTraffic
                ? (bTrafficQualified && RiskSupervision.Qualified)
                : bPlayerReady,
            RiskSupervision.Command.Brake && !SafePassCommand.Brake);
    // An emergency following-brake decision may legitimately arrive
    // before the rival has physically completed its passing lane shift.
    // Braking with the passing steer still active can direct the car
    // into the player. Abort, restore original route steering, and keep
    // the real traffic brake and 0.18-second hold authoritative.
    const latebrake::RivalPassTrafficResolution PassResolution =
        latebrake::ResolveRivalPassTraffic(
            Command, Passing, HeldTrafficCommand);
    if (PassResolution.AbortPassing)
    {
        StraightPassAssist.Reset();
    }
    const latebrake::PilotCommand FollowingCommand = PassResolution.Command;

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
