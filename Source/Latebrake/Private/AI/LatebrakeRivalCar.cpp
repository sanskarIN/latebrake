#include "AI/LatebrakeRivalCar.h"

#include "AI/RivalPilot.h"
#include "AI/RivalGateRunThrough.h"
#include "AI/FleetLineAim.h"
#include "AI/RivalCornerAnticipation.h"
#include "AI/RivalCornerBrakeEnvelope.h"
#include "AI/RivalTrafficAwareness.h"
#include "AI/RivalYawRateSteering.h"
#include "AI/RivalPassTrafficResolution.h"
#include "AI/RivalEngineHeatPace.h"
#include "AI/RivalDriveReadiness.h"
#include "AI/SingleRivalRecoverySelection.h"
#include "Track/CircuitLayout.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "CoreGlobals.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "Kismet/GameplayStatics.h"

ALatebrakeRivalCar::ALatebrakeRivalCar()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

bool ALatebrakeRivalCar::AssignGridSlot(int32 Slot)
{
    if (Slot<latebrake::PrimaryRivalGridSlot || Slot>=latebrake::StartGridCapacity) return false;
    if (Slot!=GridSlot) { ResetPilot(); GridSlot=Slot; }
    return true;
}

void ALatebrakeRivalCar::ResetPilot()
{
    LastQualifiedSessionRevision = 0;
    bHasQualifiedSessionRevision = false;
    Route.Reset();
    RivalGateTrace.Reset();
    FollowingBrakeHold.Reset();
    GripPace.Reset();
    SurfaceDamagePace.Reset();
    SpinThrottleGuard.Reset();
    StraightPassAssist.Reset();
    FleetRisk.Reset();
    FleetQueue.Reset();
    FleetHeadway.Reset();
    FleetPassing.Reset();
    SetAutonomousControls(0.f, 0.f, false);
}

void ALatebrakeRivalCar::ConfigurePilot(int32 CheckpointCount, int32 RequiredLaps)
{
    // Do not preserve approximation/recovery history from a different
    // event or replay. The RaceProgress clock remains player-authoritative.
    LastQualifiedSessionRevision = 0;
    bHasQualifiedSessionRevision = false;
    Route.Configure(CheckpointCount, RequiredLaps);
    RivalGateTrace.Reset();
    FollowingBrakeHold.Reset();
    GripPace.Reset();
    SurfaceDamagePace.Reset();
    SpinThrottleGuard.Reset();
    StraightPassAssist.Reset();
    FleetRisk.Reset();
    FleetQueue.Reset();
    FleetHeadway.Reset();
    FleetPassing.Reset();
    SetAutonomousControls(0.f, 0.f, false);
}

void ALatebrakeRivalCar::Tick(float DeltaSeconds)
{
    ALatebrakeRaceGameMode* Mode =
        GetWorld() ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    // A malformed rival Chaos body must neither accrue approximate route
    // progress nor receive another physics control update. The base pawn
    // independently neutralizes the chassis before it can apply forces.
    if (!Mode || !Mode->IsRaceStartGreen() || !Mode->IsRegisteredRaceRival(this) ||
        !IsRacePhysicsReady() || !latebrake::CanDriveRival(
            Mode->GetRace(), Mode->GetSelectedEvent(),
            Mode->IsCircuitReadyForGameplay(),
            UGameplayStatics::IsGamePaused(this) || Mode->IsMenuOpen() ||
                Mode->IsFocusLossGuardActive() || Mode->IsManualRacePauseActive(),
            Mode->HasLocalPlayerDriver(), Route.IsFinished()))
    {
        // A paused or unready chassis may skip rival ticks entirely.
        // Never carry nearly-eight seconds of pre-pause stall into
        // an immediate post-resume recovery or teleport request.
        Route.SuspendObservation();
        RivalGateTrace.Reset();
        FollowingBrakeHold.Reset();
        GripPace.Reset();
        SurfaceDamagePace.Reset();
        SpinThrottleGuard.Reset();
        StraightPassAssist.Reset();
        FleetRisk.Reset();
        FleetQueue.Suspend();
        FleetHeadway.Suspend();
        FleetPassing.Suspend();
        SetAutonomousControls(0.f, 0.f, true);
        Super::Tick(DeltaSeconds);
        return;
    }

    // A manually ticked/unusually stalled Unreal world may deliver a
    // nonfinite, zero or >0.10-second DeltaSeconds. Reject it BEFORE
    // CheckpointMotionTrace::Sample updates its position baseline,
    // not only after the route controller has refused a gate credit.
    // Stop propulsion and rebase both physical observation windows.
    if (!latebrake::RivalRouteProgress::HasQualifiedFrameTime(
            static_cast<double>(DeltaSeconds)))
    {
        Route.SuspendObservation();
        RivalGateTrace.Reset();
        FollowingBrakeHold.Reset();
        GripPace.Reset();
        SurfaceDamagePace.Reset();
        SpinThrottleGuard.Reset();
        StraightPassAssist.Reset();
        FleetRisk.Reset();
        FleetQueue.Suspend();
        FleetHeadway.Suspend();
        FleetPassing.Suspend();
        SetAutonomousControls(0.f, 0.f, true);
        Super::Tick(DeltaSeconds);
        return;
    }

    // Unreal may skip all rival ticks during actual OS/world pause. The
    // shared GameMode epoch changes on pause/menu/focus/restart regardless,
    // so the FIRST resumed qualified frame must rebase both a possible
    // checkpoint trace and any 7.9s stationary/moving stall evidence.
    // Keep previously verified route gates and the audited teleport ledger.
    const uint64 CurrentSessionRevision = Mode->GetCheckpointSessionRevision();
    if (!bHasQualifiedSessionRevision ||
        CurrentSessionRevision != LastQualifiedSessionRevision)
    {
        Route.SuspendObservation();
        RivalGateTrace.Reset();
        FollowingBrakeHold.Reset();
        GripPace.Reset();
        SurfaceDamagePace.Reset();
        SpinThrottleGuard.Reset();
        StraightPassAssist.Reset();
        FleetRisk.Reset();
        FleetQueue.Suspend();
        FleetHeadway.Suspend();
        FleetPassing.Suspend();
        LastQualifiedSessionRevision = CurrentSessionRevision;
        bHasQualifiedSessionRevision = true;
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
        Route.SuspendObservation();
        RivalGateTrace.Reset();
        FollowingBrakeHold.Reset();
        GripPace.Reset();
        SurfaceDamagePace.Reset();
        SpinThrottleGuard.Reset();
        StraightPassAssist.Reset();
        FleetRisk.Reset();
        FleetQueue.Suspend();
        FleetHeadway.Suspend();
        FleetPassing.Suspend();
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
    bool bVerifiedTrafficWait=false;
    if (Mode->IsExperimentalFleetRequested() && !bVerifiedGateCrossed &&
        FlatSpeed<latebrake::FleetQueueMaximumSelfSpeedCmS)
    {
        latebrake::RivalFleetTrafficSample QueueSample;
        if (Mode->TryGetFreshFleetTraffic(this,false,0.0,QueueSample))
        {
            const int Ordinal=Route.GetCompletedLaps()*Route.GetWaypointCount()+Next;
            const auto FleetFrame=Mode->GetFleetAttemptReadout();
            const latebrake::FleetQueueFrameStamp QueueStamp{FleetFrame.AttemptToken,
                FleetFrame.LastFrame,static_cast<std::uint64_t>(GFrameCounter),
                CurrentSessionRevision,GetTeleportRevision()};
            bVerifiedTrafficWait=FleetQueue.Observe(Ordinal,
                static_cast<double>(DeltaSeconds),QueueSample,QueueStamp).DeferStallRecovery;
        }
        else FleetQueue.Suspend();
    }
    else FleetQueue.Suspend();
    const latebrake::RouteObservation Observation = Route.ObserveVerifiedGate(
        Distance, FlatSpeed, static_cast<double>(DeltaSeconds),
        bVerifiedGateCrossed,bVerifiedTrafficWait);
    if (Observation.Advanced)
    {
        // Never bridge a frame sample across two different gate planes.
        RivalGateTrace.Reset();
        // Only the physical directional gate trace can trigger this call.
        if (Mode->IsExperimentalFleetRequested()) Mode->ObserveFleetRivalGate(this);
        else if (GridSlot==latebrake::PrimaryRivalGridSlot) Mode->ObserveRivalVerifiedGate();
    }

    if (Observation.NeedsRecovery)
    {
        RivalGateTrace.Reset();
        FollowingBrakeHold.Reset();
        GripPace.Reset();
        SurfaceDamagePace.Reset();
        SpinThrottleGuard.Reset();
        StraightPassAssist.Reset();
        FleetRisk.Reset();
        FleetQueue.Suspend();
        FleetHeadway.Suspend();
        FleetPassing.Suspend();
        const int32 RecoveryIndex = Route.GetNextWaypoint();
        // Collect the actual human on the current game-thread snapshot.
        // No missing player or fictional grid occupant can clear a landing.
        const ALatebrakeCarPawn* RecoveryPlayer = Cast<ALatebrakeCarPawn>(
            UGameplayStatics::GetPlayerPawn(this, 0));
        latebrake::RecoveryObstacle Human{};
        if (IsValid(RecoveryPlayer) && RecoveryPlayer != this &&
            RecoveryPlayer->IsLocallyControlled() &&
            RecoveryPlayer->IsRacePhysicsReady() &&
            Mode->HasLocalPlayerDriver())
        {
            const FVector HumanPosition=RecoveryPlayer->GetActorLocation();
            const FVector HumanVelocity=RecoveryPlayer->GetVelocity();
            Human={true,latebrake::HumanGridSlot,
                static_cast<std::uint64_t>(RecoveryPlayer->GetUniqueID())+1u,
                {{HumanPosition.X,HumanPosition.Y},
                    RecoveryPlayer->GetActorRotation().Yaw},
                HumanVelocity.X,HumanVelocity.Y};
        }
        const auto Selection=latebrake::SelectSingleRivalRecovery(RecoveryIndex,Human);
        const auto FleetSelection=Mode->GetFleetRivalRecovery(this,RecoveryIndex);
        const bool bFleetMode=Mode->IsExperimentalFleetRequested();
        const latebrake::CircuitPose Pose=bFleetMode?FleetSelection.Pose:Selection.Pose;
        // A blocked preflight consumes the existing bounded attempt but
        // never invokes a transform. A clear landing still needs actual
        // engine placement and post-move physics verification.
        const bool bRecovered = (bFleetMode?FleetSelection.Clear:Selection.Safe()) && TryRecoverTo(
            FVector(Pose.Position.X, Pose.Position.Y, 180.0),
            FRotator(0.0, Pose.HeadingDegrees, 0.0));
        const bool bReported = Route.ReportRecoveryResult(bRecovered);
        if (bReported && bFleetMode) Mode->ObserveFleetRivalRecovery(this,bRecovered);
        if (!bReported || !bRecovered)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Latebrake: rival recovery not verified (preflight %d); retry after new stall"),
                static_cast<int32>(Selection.Status));
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
        StraightPassAssist.Reset();
        FleetRisk.Reset();
        FleetQueue.Suspend();
        FleetHeadway.Suspend();
        FleetPassing.Suspend();
        SetAutonomousControls(0.f, 0.f, true);
        Super::Tick(DeltaSeconds);
        return;
    }

    // The route only advances after a verified forward gate-plane crossing,
    // not when we approach its 440cm radius. Aim 550cm THROUGH that plane
    // and disable the generic proximity-arrival stop. Otherwise the rival
    // can halt before crossing and never complete the real gated lap.
    const latebrake::CircuitPoint NextTarget = Mode->IsExperimentalFleetRequested()
        ? latebrake::PlanFleetLineAim({GridSlot,Route.GetNextWaypoint(),
            {{Position.X,Position.Y},GetActorRotation().Yaw},FlatSpeed,
            IsRacePhysicsReady()}).Target
        : latebrake::RivalGateRunThroughPoint(Route.GetNextWaypoint());
    latebrake::PilotCommand Command = latebrake::AnticipateRivalCorner(
        latebrake::PlanRivalTurn(
            Position.X, Position.Y, Heading, FlatSpeed,
            NextTarget.X, NextTarget.Y, 440.0, false),
        Route.GetNextWaypoint(), {Position.X, Position.Y}, FlatSpeed);
    latebrake::FleetDecisionSample Decision;
    Decision.GateOrdinal=Route.GetCompletedLaps()*Route.GetWaypointCount()+Route.GetNextWaypoint();
    Decision.SpeedCmS=FlatSpeed;
    Decision.Set(latebrake::FleetDecisionStage::Route,Command);

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
    Decision.Set(latebrake::FleetDecisionStage::Corner,Command);

    // Apply selected Quick Race difficulty only to this already-valid
    // planner command. Existing grip/traffic/heat/spin safety limits run
    // afterward and can always reduce the requested motor demand.
    Command = latebrake::ApplyRivalDifficulty(
        Command, Mode->GetActiveRivalDifficulty(), FlatSpeed,
        IsRacePhysicsReady());
    Decision.Set(latebrake::FleetDecisionStage::Difficulty,Command);

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
    Decision.Set(latebrake::FleetDecisionStage::Grip,Command);

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
    Decision.Set(latebrake::FleetDecisionStage::SurfaceDamage,Command);
    Decision.RoadQualified=bCornerPhysicsQualified && !bOffPavement;

    // The previous qualified physical engine-heat sample can modestly
    // reduce rival throttle at road speed, letting the real shared motor
    // policy cool. Low-speed launch and prior braking stay unchanged.
    Command = latebrake::ModerateRivalEngineHeatPace(Command, {
        static_cast<double>(DeltaSeconds), FlatSpeed,
        GetEngineHeat().HeatFraction(),
        GetEngineHeat().HasValidSample(),
        IsRacePhysicsReady()});
    Decision.Set(latebrake::FleetDecisionStage::EngineHeat,Command);

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
    const latebrake::RivalPassOutput Passing = Mode->IsExperimentalFleetRequested()
        ? latebrake::RivalPassOutput{Command,false,0.0}
        : StraightPassAssist.Apply(Command, {
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
                !GetSurfaceTraction().IsOffPavement()});
    const latebrake::PilotCommand SafePassCommand = Passing.Command;
    // Reuse the previous qualified physical grip/heat braking forecast.
    // The lane policy fails closed if this becomes invalid or unavailable;
    // it must not assume perfect dry-road braking after a damaged chassis.
    const latebrake::PilotCommand TrafficCommand =
        latebrake::YieldRivalToPlayer(SafePassCommand, {
            Position.X, Position.Y, Heading, FlatSpeed,
            PlayerPosition.X, PlayerPosition.Y,
            PlayerVelocity.X, PlayerVelocity.Y, bPlayerReady,
            Velocity.X, Velocity.Y,
            BrakeForecast.SafeDecelerationCmPerSecondSquared
        });
    // A short, fail-closed hold prevents brake chatter near the
    // projected contact boundary; it cannot re-route or score a rival.
    const latebrake::PilotCommand HeldTrafficCommand =
        FollowingBrakeHold.Apply(
            TrafficCommand, static_cast<double>(DeltaSeconds),
            bPlayerReady, TrafficCommand.Brake && !SafePassCommand.Brake);
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
        // The original Reset erased the aborted lane's history, allowing
        // a fresh pass toward the same player on the very next Tick.
        // Keep this gate blocked until verified route progression; pause,
        // restart and recovery paths still call the full Reset().
        StraightPassAssist.AbortUntilGateChange();
    }
    const latebrake::PilotCommand FollowingCommand = PassResolution.Command;
    Decision.Set(latebrake::FleetDecisionStage::HumanFollowing,FollowingCommand);
    latebrake::PilotCommand FleetCommand=FollowingCommand;
    if (Mode->IsExperimentalFleetRequested())
    {
        const double BrakeScale=bCornerPhysicsQualified ?
            GetTireCondition().GripMultiplier()*GetBrakeHeat().ServiceBrakeMultiplier()*
            GetCollisionDamage().LateralGripMultiplier()*(bOffPavement?
                latebrake::ArcadeSurfaceTraction::OffPavementGripMultiplier:1.0):0.0;
        latebrake::RivalFleetTrafficSample Snapshot;
        const bool Observed=Mode->TryGetFreshFleetTraffic(this,bCornerPhysicsQualified,BrakeScale,Snapshot);
        Decision.TrafficObserved=Observed;
        if (Observed)
        {
            const int Ordinal=Route.GetCompletedLaps()*Route.GetWaypointCount()+Route.GetNextWaypoint();
            const auto FleetFrame=Mode->GetFleetAttemptReadout();
            const latebrake::FleetPassStamp PassingStamp{FleetFrame.AttemptToken,
                FleetFrame.LastFrame,static_cast<std::uint64_t>(GFrameCounter),
                CurrentSessionRevision,GetTeleportRevision()};
            const auto PassingLane=FleetPassing.Apply(FollowingCommand,{
                Ordinal,Route.GetNextWaypoint(),static_cast<double>(DeltaSeconds),
                bCornerPhysicsQualified && !bOffPavement,Snapshot,PassingStamp});
            Decision.Set(latebrake::FleetDecisionStage::Passing,PassingLane.Command);
            const auto Immediate=latebrake::ApplyRivalFleetTraffic(PassingLane.Command,Snapshot);
            Decision.Set(latebrake::FleetDecisionStage::ImmediateTraffic,Immediate.Command);
            const auto Supervised=FleetRisk.Apply(Immediate.Command,Snapshot,DeltaSeconds,
                latebrake::FleetForecastRoadPolicy::ObservedOffroadBlockers);
            FleetCommand=Supervised.Command;
            // Headway can only reduce already supervised motor demand.
            // Passing, emergency brakes and route authority stay upstream.
            latebrake::FleetHeadwayReadout Headway;
            if (Supervised.Qualified)
            {
                Headway=FleetHeadway.Apply(FleetCommand,{
                    Ordinal,Route.GetNextWaypoint(),static_cast<double>(DeltaSeconds),
                    bCornerPhysicsQualified && !bOffPavement,Snapshot,PassingStamp});
                Decision.Headway=latebrake::RecordFleetDecisionHeadway(FleetCommand,Headway);
                FleetCommand=Headway.Command;
            }
            else FleetHeadway.Suspend();
            Decision.TrafficQualified=Supervised.Qualified;
            Decision.Risk=Supervised.Forecast.Kind;
            Decision.BrakeHold=Supervised.HoldingBrake;
            Decision.OffroadBlockers=Supervised.Forecast.OffroadBlockers;
            Decision.RoadSpeedExceeded=Supervised.Forecast.RoadSpeedExceeded;
            Decision.LimitingSlot=Supervised.Forecast.LimitingSlot>=0?
                Supervised.Forecast.LimitingSlot:Immediate.LimitingSlot;
            if (FleetCommand.Brake || !Supervised.Qualified)
            {
                FleetPassing.AbortTraffic();
                FleetCommand.Steering=FollowingCommand.Steering;
            }
        }
        else
        {
            FleetRisk.Reset();
            FleetHeadway.Suspend();
            FleetPassing.Suspend();
            Decision.Set(latebrake::FleetDecisionStage::Passing,FollowingCommand);
            FleetCommand.Throttle=0.0;
            FleetCommand.Brake=true;
            Decision.Set(latebrake::FleetDecisionStage::ImmediateTraffic,FleetCommand);
        }
        Decision.Set(latebrake::FleetDecisionStage::PredictiveTraffic,FleetCommand);
        Decision.Queue=FleetQueue.Readout().Status;
        Decision.Passing=FleetPassing.Readout().Phase;
    }

    // Read real Chaos angular velocity AFTER all route, corner, road,
    // collision and traffic decisions. Reduce same-direction steering
    // only when the chassis is already rotating quickly at speed.
    // This does not change braking, throttle, waypoint or recovery logic.
    const FVector ActualAngularVelocity =
        Chassis->GetPhysicsAngularVelocityInRadians();
    const latebrake::PilotCommand SafeTrafficCommand =
        latebrake::ModerateRivalYawRate(FleetCommand, {
            static_cast<double>(DeltaSeconds),
            FlatSpeed,
            static_cast<double>(ActualAngularVelocity.Z),
            true
        });
    Decision.Set(latebrake::FleetDecisionStage::Yaw,SafeTrafficCommand);
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
    Decision.Set(latebrake::FleetDecisionStage::Spin,SafeSpinCommand);
    if (Mode->IsExperimentalFleetRequested()) Mode->ObserveFleetRivalDecision(this,Decision);
    SetAutonomousControls(static_cast<float>(SafeSpinCommand.Throttle),
                          static_cast<float>(SafeSpinCommand.Steering),
                          SafeSpinCommand.Brake);
    Super::Tick(DeltaSeconds);
}
