#include "Game/LatebrakeRaceGameMode.h"
#include "AI/LatebrakeRivalCar.h"
#include "Racing/LatebrakeCheckpoint.h"
#include "CoreGlobals.h"
#include "Vehicles/LatebrakeCarPawn.h"
#include "Track/CircuitStartGrid.h"
#include "Vehicles/ArcadeSkidRoadGeometry.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"
#include <limits>

namespace
{
std::uint64_t FleetToken(const ALatebrakeCarPawn* Car)
{
    return IsValid(Car)?static_cast<std::uint64_t>(Car->GetUniqueID())+1u:0u;
}
}

bool ALatebrakeRaceGameMode::IsExperimentalFleetRequested() const noexcept
{
#if UE_BUILD_SHIPPING
    return false;
#else
    return bExperimentalFleetFlag && !bCareerRace &&
        SelectedEvent==latebrake::EventKind::Circuit;
#endif
}

void ALatebrakeRaceGameMode::DestroyExtraFleet()
{
    for (auto& Car:ExtraFleet)
        if (IsValid(Car)) Car->Destroy();
    ExtraFleet.Reset();
}

void ALatebrakeRaceGameMode::BeginExperimentalFleetAttempt()
{
    ClearFleetTickPrerequisites();
    DestroyExtraFleet();
    if (FleetAttemptToken!=std::numeric_limits<uint64>::max())
        ++FleetAttemptToken;
    FleetFrameToken=0;
    FleetObservedEpoch=0;
    FleetStartup.Begin(FleetAttemptToken);
    FleetFrameAuthority.Begin(FleetAttemptToken);
    FleetBoundActors={};
    bFleetActorsBound=false;
    FleetReviewLedger.Reset();
    FleetGateTiming.Reset();
    FleetPlayerTraffic.Reset();
    FleetFinishReview={};
    if (!IsExperimentalFleetRequested() && IsValid(RivalCar))
        RivalCar->SetGarageTuning(1.f,1.f,1.f,0,latebrake::FactoryGarageLook(0));
    const bool Began=FleetAttempt.Begin({IsExperimentalFleetRequested(),
        SelectedEvent==latebrake::EventKind::Circuit,bCareerRace,FleetAttemptToken});
    if (Began && IsExperimentalFleetRequested())
    {
        RegisterFleetTickDependent(UGameplayStatics::GetPlayerPawn(this,0));
        RegisterFleetTickDependent(RivalCar.Get());
        for (const auto& Gate:FleetCheckpointActors)
            RegisterFleetTickDependent(Gate.Get());
    }
#if !UE_BUILD_SHIPPING
    if (Began && IsExperimentalFleetRequested())
        UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=begin attempt=%llu gates=%d laps=%d"),
            static_cast<unsigned long long>(FleetAttemptToken),
            Race.GetCheckpointCount(),Race.GetRequiredLaps());
#else
    static_cast<void>(Began);
#endif
}

ALatebrakeRivalCar* ALatebrakeRaceGameMode::FleetRivalAt(int32 Slot) const
{
    if (Slot==latebrake::PrimaryRivalGridSlot) return RivalCar.Get();
    return Slot>=2 && ExtraFleet.IsValidIndex(Slot-2)?ExtraFleet[Slot-2].Get():nullptr;
}

bool ALatebrakeRaceGameMode::IsRegisteredRaceRival(const ALatebrakeRivalCar* Actor) const
{
    if (!IsValid(Actor)) return false;
    if (!IsExperimentalFleetRequested()) return Actor==RivalCar.Get();
    const int32 Slot=Actor->GetGridSlot();
    return Slot>0 && Slot<latebrake::StartGridCapacity && FleetRivalAt(Slot)==Actor;
}

int32 ALatebrakeRaceGameMode::GetPhysicalFleetCount() const
{
    int32 Count=HasLocalPlayerDriver()?1:0;
    for(int32 Slot=1;Slot<latebrake::StartGridCapacity;++Slot)
    {
        const auto* Car=FleetRivalAt(Slot);
        if (IsRegisteredRaceRival(Car) && Car->IsRacePhysicsReady()) ++Count;
    }
    return Count;
}

bool ALatebrakeRaceGameMode::TryGetFleetRivalProgress(
    int32 Slot,int32& Laps,int32& NextGate,bool& Finished) const
{
    Laps=0; NextGate=-1; Finished=false;
    if (!IsExperimentalFleetRequested() || Slot<=0 || Slot>=latebrake::StartGridCapacity)
        return false;
    const auto* Car=FleetRivalAt(Slot);
    if (!IsRegisteredRaceRival(Car) || !Car->IsRacePhysicsReady() ||
        FleetAttempt.BoundIdentity()[static_cast<std::size_t>(Slot)]!=FleetToken(Car) ||
        !bFleetActorsBound || FleetBoundActors[static_cast<std::size_t>(Slot)].Get()!=Car)
        return false;
    Laps=Car->GetApproximateLaps();
    NextGate=Car->GetNextVerifiedGate();
    Finished=Car->HasCompletedRoute();
    return true;
}

void ALatebrakeRaceGameMode::SpawnExtraFleet()
{
#if !UE_BUILD_SHIPPING
    if (!IsExperimentalFleetRequested() || !FleetAttempt.CanAttemptSpawn() ||
        !ExtraFleet.IsEmpty() || !bCircuitReady || !HasLocalPlayerDriver() ||
        !IsValid(RivalCar) || !RivalCar->IsRacePhysicsReady() || !GetWorld()) return;
    if (!latebrake::IsPlannedStartGridSafe())
    {
        FleetAttempt.ReportSpawnFailure(FleetAttemptToken);
        return;
    }
    ExtraFleet.SetNum(latebrake::StartGridCapacity-2);
    for (int32 Slot=2;Slot<latebrake::StartGridCapacity;++Slot)
    {
        const auto Pose=latebrake::StartGridPoses[static_cast<std::size_t>(Slot)];
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride=
            ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
        auto* Actor=GetWorld()->SpawnActor<ALatebrakeRivalCar>(
            ALatebrakeRivalCar::StaticClass(),FVector(Pose.Position.X,Pose.Position.Y,180.0),
            FRotator(0.0,Pose.HeadingDegrees,0.0),Parameters);
        ExtraFleet[Slot-2]=Actor;
        if (!IsValid(Actor) || !Actor->AssignGridSlot(Slot) ||
            !Actor->IsRacePhysicsReady() ||
            !Actor->TryRecoverTo(FVector(Pose.Position.X,Pose.Position.Y,180.0),
                FRotator(0.0,Pose.HeadingDegrees,0.0)))
        {
            DestroyExtraFleet();
            FleetAttempt.ReportSpawnFailure(FleetAttemptToken);
            UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_SPAWN_FAILED slot=%d"),Slot);
            return;
        }
        Actor->ResetDrivingStats();
        Actor->ConfigurePilot(Race.GetCheckpointCount(),Race.GetRequiredLaps());
    }
    // Visual choices are independent from player ownership and AI forces.
    const auto Visuals=latebrake::BuildFleetVisualPlan(GetSelectedVehicleId(),GetSelectedGarageLook());
    if (latebrake::ValidFleetVisualPlan(Visuals))
    {
        for(int32 Slot=1;Slot<latebrake::StartGridCapacity;++Slot)
        {
            auto* Actor=FleetRivalAt(Slot);
            const auto& Entry=Visuals.Cars[static_cast<std::size_t>(Slot)];
            if (IsValid(Actor))
                Actor->SetGarageTuning(1.f,1.f,1.f,Entry.VehicleId,Entry.Look);
        }
    }
    UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_SPAWNED extra=%d native_qa=required"),ExtraFleet.Num());
#endif
}

bool ALatebrakeRaceGameMode::CollectFleetFrame(latebrake::FleetPhysicalFrame& Out,
    float DeltaSeconds,bool IncludeGridProbes) const
{
    Out={};
    Out.AttemptToken=FleetAttemptToken;
    Out.FrameToken=FleetFrameToken;
    const UWorld* World=GetWorld();
    if (!World || !IsExperimentalFleetRequested() || !HasLocalPlayerDriver() ||
        ExtraFleet.Num()!=latebrake::StartGridCapacity-2 ||
        !bCircuitReady) return false;
    const auto* Human=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
    for (int32 Slot=0;Slot<latebrake::StartGridCapacity;++Slot)
    {
        const auto* Rival=Slot==0?nullptr:FleetRivalAt(Slot);
        const ALatebrakeCarPawn* Car=Slot==0?Human:Rival;
        if (!IsValid(Car) || !Car->IsRacePhysicsReady() ||
            (bFleetActorsBound && FleetBoundActors[static_cast<std::size_t>(Slot)].Get()!=Car) ||
            (Slot!=0 && (!IsRegisteredRaceRival(Rival) || Rival->GetGridSlot()!=Slot)))
            return false;
        const auto Position=Car->GetActorLocation();
        const auto Velocity=Car->GetVelocity();
        const auto Heading=Car->GetActorRotation();
        if (Position.ContainsNaN() || Velocity.ContainsNaN() || Heading.ContainsNaN() ||
            FMath::Abs(Position.Z)>100000.0 || FMath::Abs(Velocity.Z)>15000.0)
            return false;
        auto& Sample=Out.Cars[static_cast<std::size_t>(Slot)];
        Sample={true,Slot,FleetToken(Car),{{Position.X,Position.Y},Heading.Yaw},Velocity.X,Velocity.Y};
        ++Out.Count;
        if (!IncludeGridProbes) continue;

        // Fresh pre-GO support, never a cached readout from last race.
        auto& Height=Out.Vertical[static_cast<std::size_t>(Slot)];
        Height.ChassisWorldZCm=Position.Z;
        Height.Ground.PhysicsReady=true;
        Height.Ground.DeltaSeconds=DeltaSeconds;
        Height.Ground.ChassisVerticalSpeedCmPerSecond=Velocity.Z;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(LatebrakeFleetGridSupport),false,Car);
        constexpr double X[]={122.0,122.0,-122.0,-122.0};
        constexpr double Y[]={-68.0,68.0,-68.0,68.0};
        for(int32 Index=0;Index<4;++Index)
        {
            const FVector Origin=Position+Car->GetActorForwardVector()*X[Index]+
                Car->GetActorRightVector()*Y[Index];
            FHitResult Hit;
            int32 Road=INDEX_NONE;
            if (World->LineTraceSingleByChannel(Hit,Origin,
                Origin-FVector(0.0,0.0,latebrake::ChassisContactProbeTraceLengthCm),
                ECC_WorldStatic,Query) && Hit.bBlockingHit &&
                TryGetSkidRoadSurface(Hit.GetComponent(),Road) &&
                latebrake::IsArcadeSkidRoadPoint(Road,
                    {Hit.ImpactPoint.X,Hit.ImpactPoint.Y,Hit.ImpactPoint.Z},
                    {Hit.ImpactNormal.X,Hit.ImpactNormal.Y,Hit.ImpactNormal.Z}))
            {
                Height.Ground.Probes[static_cast<std::size_t>(Index)]=
                    {true,Hit.Distance,Hit.ImpactNormal.Z};
            }
        }
    }
    return latebrake::InspectFleetRoster(Out).Valid();
}

bool ALatebrakeRaceGameMode::ObserveExperimentalFleet(float DeltaSeconds,bool Inactive)
{
    if (!IsExperimentalFleetRequested()) return true;
    if (Race.IsFinished() || FleetFrameToken==std::numeric_limits<uint64>::max())
    {
        FleetFrameAuthority.Revoke();
        return false;
    }
    const latebrake::FleetFrameStamp Stamp{FleetAttemptToken,FleetFrameToken+1u,
        static_cast<std::uint64_t>(GFrameCounter),CheckpointSession.Current()};
    if (!FleetFrameAuthority.BeginFrame(Stamp)) return false;
    ++FleetFrameToken;
    const auto Previous=FleetAttempt.Readout().Phase;
    if (!Inactive && !StartLights.IsGreen()) SpawnExtraFleet();
    latebrake::FleetPhysicalFrame Frame;
    Frame.AttemptToken=FleetAttemptToken;
    Frame.FrameToken=FleetFrameToken;
    Frame.Inactive=Inactive;
    if (!Inactive)
    {
        const bool Collected=CollectFleetFrame(Frame,DeltaSeconds,!StartLights.IsGreen());
        if (!Collected && FleetAttempt.CanAttemptSpawn() &&
            ExtraFleet.Num()==latebrake::StartGridCapacity-2 && HasLocalPlayerDriver())
        {
            // A completed spawn transaction with a broken roster cannot
            // wait forever as if the additional actors had not been made.
            FleetAttempt.ReportSpawnFailure(FleetAttemptToken);
        }
    }
    const bool Accepted=FleetAttempt.Observe(Frame);
    const auto Current=FleetAttempt.Readout();
    // The portable token array is necessary, not sufficient. UObject
    // UniqueIDs may be reused after destruction. Bind the actual weak
    // object generation when the first complete roster is observed,
    // even when the grid has not yet settled enough for countdown.
    if (!bFleetActorsBound && FleetAttempt.BoundIdentity()[0]!=0)
    {
        FleetBoundActors[0]=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
        for(int32 Slot=1;Slot<latebrake::StartGridCapacity;++Slot)
            FleetBoundActors[static_cast<std::size_t>(Slot)]=FleetRivalAt(Slot);
        bFleetActorsBound=true;
        for (const auto& Actor:FleetBoundActors)
            RegisterFleetTickDependent(Actor.Get());
        const bool LedgerBound=latebrake::BindFleetReviewLedger(FleetReviewLedger,
            FleetAttempt.BoundIdentity(),Race.GetCheckpointCount(),Race.GetRequiredLaps());
        if (LedgerBound && !FleetGateTiming.Configure(FleetAttemptToken,
                FleetAttempt.BoundIdentity(),Race.GetCheckpointCount(),Race.GetRequiredLaps()))
            UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_TIMING_CONFIGURATION_REFUSED"));
        if (LedgerBound && !FleetPlayerTraffic.Configure(FleetAttemptToken,FleetAttempt.BoundIdentity()))
            UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_TRAFFIC_CONFIGURATION_REFUSED"));
#if !UE_BUILD_SHIPPING
        if (LedgerBound)
        {
            const auto& Ids=FleetAttempt.BoundIdentity();
            UE_LOG(LogTemp,Display,
                TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=roster attempt=%llu frame=%llu "
                     "id0=%llu id1=%llu id2=%llu id3=%llu id4=%llu id5=%llu"),
                static_cast<unsigned long long>(FleetAttemptToken),
                static_cast<unsigned long long>(FleetFrameToken),
                static_cast<unsigned long long>(Ids[0]),static_cast<unsigned long long>(Ids[1]),
                static_cast<unsigned long long>(Ids[2]),static_cast<unsigned long long>(Ids[3]),
                static_cast<unsigned long long>(Ids[4]),static_cast<unsigned long long>(Ids[5]));
        }
#else
        static_cast<void>(LedgerBound);
#endif
    }
    if (Accepted) FleetObservedEpoch=CheckpointSession.Current();
    bool CountdownAllowed=Current.CountdownAllowed;
    if (!StartLights.IsGreen())
    {
        const auto Startup=FleetStartup.Observe({FleetAttemptToken,FleetFrameToken,
            CheckpointSession.Current(),static_cast<double>(DeltaSeconds),!Inactive,
            bCircuitReady,HasLocalPlayerDriver(),IsRestartPlacementReady(),Accepted,Current});
        if (Startup.ResetCountdown) StartLights.Reset();
        if (Startup.Phase==latebrake::FleetStartupPhase::TimedOut &&
            Current.Phase!=latebrake::FleetAttemptPhase::Faulted)
        {
            FleetAttempt.ReportGridTimeout(FleetAttemptToken);
            UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_GRID_TIMEOUT cause=%s slot=%d"),
                UTF8_TO_TCHAR(latebrake::FleetStartupCauseName(Startup.Cause)),Startup.Grid.Slot);
        }
        CountdownAllowed=Startup.CountdownAllowed &&
            FleetStartup.IsCurrentPermission(FleetAttemptToken,FleetFrameToken,CheckpointSession.Current());
    }
    const auto Final=FleetAttempt.Readout();
    if (Final.Phase==latebrake::FleetAttemptPhase::Faulted && Previous!=Final.Phase)
    {
        UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_FAULT reason=%d slot=%d"),
            static_cast<int32>(Final.Fault),Final.FaultSlot);
#if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=fault attempt=%llu reason=%d slot=%d"),
            static_cast<unsigned long long>(FleetAttemptToken),
            static_cast<int32>(Final.Fault),Final.FaultSlot);
#endif
    }
    const bool Allowed=Accepted && (StartLights.IsGreen()?Final.RunningAllowed:CountdownAllowed);
    return FleetFrameAuthority.Commit(Stamp,Allowed && Stamp.SessionEpoch==CheckpointSession.Current());
}

bool ALatebrakeRaceGameMode::IsExperimentalFleetDriveReady() const
{
    if (!IsExperimentalFleetRequested()) return true;
    if (!HasCurrentFleetFrameAuthority() || !FleetAttempt.Readout().RunningAllowed || !bFleetActorsBound ||
        FleetObservedEpoch!=CheckpointSession.Current() ||
        !GetWorld() || Menu.IsOpen() || UGameplayStatics::IsGamePaused(this) ||
        AppFocusGuard.IsSuspended() || ManualPause.IsSuspended()) return false;
    // Detect a destroyed/replaced body even between GameMode ticks.
    const auto* Human=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
    const auto& Bound=FleetAttempt.BoundIdentity();
    if (!IsValid(Human) || FleetToken(Human)!=Bound[0] || FleetBoundActors[0].Get()!=Human ||
        !Human->IsRacePhysicsReady()) return false;
    for(int32 Slot=1;Slot<latebrake::StartGridCapacity;++Slot)
    {
        const auto* Rival=FleetRivalAt(Slot);
        if (!IsRegisteredRaceRival(Rival) || !Rival->IsRacePhysicsReady() ||
            FleetToken(Rival)!=Bound[static_cast<std::size_t>(Slot)] ||
            FleetBoundActors[static_cast<std::size_t>(Slot)].Get()!=Rival) return false;
    }
    return true;
}

bool ALatebrakeRaceGameMode::TryGetFreshFleetTraffic(const ALatebrakeRivalCar* Actor,
    bool BrakeQualified,double BrakeScale,latebrake::RivalFleetTrafficSample& Out) const
{
    Out={};
    if (!IsExperimentalFleetRequested() || !IsExperimentalFleetDriveReady() ||
        !IsRegisteredRaceRival(Actor)) return false;
    latebrake::FleetPhysicalFrame Frame;
    if (!CollectFleetFrame(Frame,0.0,false)) return false;
    return latebrake::MakeFreshFleetTraffic(Frame,FleetAttempt.BoundIdentity(),
        Actor->GetGridSlot(),FleetToken(Actor),BrakeQualified,BrakeScale,Out);
}

latebrake::FleetRivalRecovery ALatebrakeRaceGameMode::GetFleetRivalRecovery(
    const ALatebrakeRivalCar* Actor,int32 Gate) const
{
    if (!IsExperimentalFleetRequested() || !IsExperimentalFleetDriveReady() ||
        !IsRegisteredRaceRival(Actor)) return {};
    latebrake::FleetPhysicalFrame Frame;
    if (!CollectFleetFrame(Frame,0.0,false)) return {};
    return latebrake::SelectFleetRivalRecovery(Gate,Actor->GetGridSlot(),FleetToken(Actor),
        Frame,FleetAttempt.BoundIdentity());
}

void ALatebrakeRaceGameMode::ObserveFleetRivalGate(const ALatebrakeRivalCar* Actor)
{
    if (!IsExperimentalFleetRequested() || !IsExperimentalFleetDriveReady() ||
        !IsRegisteredRaceRival(Actor) || Race.IsFinished() ||
        Actor->GetVerifiedGateCount()!=Race.GetCheckpointCount() ||
        Actor->GetExpectedLaps()!=Race.GetRequiredLaps())
        return;
    const int Slot=Actor->GetGridSlot();
    int Ordinal=0;
    if (!latebrake::FleetRouteOrdinal(Actor->GetApproximateLaps(),
        Actor->GetNextVerifiedGate(),Actor->HasCompletedRoute(),
        Race.GetCheckpointCount(),Race.GetRequiredLaps(),Ordinal)) return;
    const double Clock=Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds();
    if (!FleetReviewLedger.RecordAcceptedGate(Slot,FleetToken(Actor),Ordinal,Clock))
    {
        UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_GATE_RECEIPT_REFUSED slot=%d ordinal=%d"),
            Slot,Ordinal);
#if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=refused attempt=%llu kind=gate slot=%d ordinal=%d"),
            static_cast<unsigned long long>(FleetAttemptToken),Slot,Ordinal);
#endif
        return;
    }
    if (!FleetGateTiming.Record({FleetAttemptToken,FleetToken(Actor),Slot,Ordinal,Clock}))
        UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_TIMING_RECEIPT_REFUSED slot=%d ordinal=%d"),Slot,Ordinal);
#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=gate attempt=%llu slot=%d id=%llu ordinal=%d clock=%.9f"),
        static_cast<unsigned long long>(FleetAttemptToken),Slot,
        static_cast<unsigned long long>(FleetToken(Actor)),Ordinal,Clock);
#endif
}

void ALatebrakeRaceGameMode::ObserveFleetRivalRecovery(
    const ALatebrakeRivalCar* Actor,bool Verified)
{
    if (!IsExperimentalFleetRequested() || !IsExperimentalFleetDriveReady() ||
        !IsRegisteredRaceRival(Actor) || Race.IsFinished()) return;
    const int Slot=Actor->GetGridSlot();
    const int Ordinal=Actor->GetPilotRecoveries();
    if (!FleetReviewLedger.RecordRecoveryResult(Slot,FleetToken(Actor),Ordinal,Verified))
    {
        UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_RECOVERY_RECEIPT_REFUSED slot=%d"),Slot);
#if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=refused attempt=%llu kind=recovery slot=%d ordinal=%d"),
            static_cast<unsigned long long>(FleetAttemptToken),Slot,Ordinal);
#endif
        return;
    }
#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=recovery attempt=%llu slot=%d id=%llu ordinal=%d verified=%d"),
        static_cast<unsigned long long>(FleetAttemptToken),Slot,
        static_cast<unsigned long long>(FleetToken(Actor)),Ordinal,Verified?1:0);
#endif
}

bool ALatebrakeRaceGameMode::TryGetFleetDraftFrame(const ALatebrakeCarPawn* Observer,
    const latebrake::FleetDraftFollower& Follower,latebrake::FleetDraftFrame& Out) const
{
    Out={};
    static_assert(latebrake::FleetDraftCarCount==latebrake::StartGridCapacity);
    UWorld* World=GetWorld();
    if (!IsExperimentalFleetRequested() || !World || !IsValid(Observer) ||
        Observer!=FleetBoundActors[0].Get() || Observer->GetWorld()!=World ||
        !Observer->IsRacePhysicsReady() || !HasLocalPlayerDriver() ||
        Race.IsFinished() || !IsExperimentalFleetDriveReady() ||
        !Follower.EligibleHumanRace || !Follower.GroundSupported)
        return false;

    latebrake::FleetPhysicalFrame Physical;
    if (!CollectFleetFrame(Physical,0.0,false) ||
        !latebrake::MatchLiveFleetIdentity(Physical,FleetAttempt.BoundIdentity()))
        return false;

    latebrake::FleetDraftFrame CandidateFrame;
    CandidateFrame.Attempt=FleetAttemptToken;
    CandidateFrame.LocalFrame=FleetFrameToken;
    CandidateFrame.EngineFrame=static_cast<std::uint64_t>(GFrameCounter);
    CandidateFrame.SessionRevision=CheckpointSession.Current();
    CandidateFrame.BoundIdentity=FleetAttempt.BoundIdentity();
    CandidateFrame.Follower=Follower;
    CandidateFrame.Follower.Token=FleetToken(Observer);
    CandidateFrame.Follower.TeleportRevision=Observer->GetTeleportRevision();
    const FVector FollowingPosition=Observer->GetActorLocation();
    if (FollowingPosition.ContainsNaN()) return false;

    for (int32 Slot=1;Slot<latebrake::StartGridCapacity;++Slot)
    {
        const auto* Rival=FleetRivalAt(Slot);
        if (!IsValid(Rival) || !IsRegisteredRaceRival(Rival) ||
            Rival!=FleetBoundActors[static_cast<std::size_t>(Slot)].Get() ||
            !Rival->IsRacePhysicsReady())
            return false;

        const FVector Position=Rival->GetActorLocation();
        const FVector Forward=Rival->GetActorForwardVector();
        const FVector Velocity=Rival->GetVelocity();
        if (Position.ContainsNaN() || Forward.ContainsNaN() || Velocity.ContainsNaN())
            return false;

        auto& Entry=CandidateFrame.Candidates[static_cast<std::size_t>(Slot-1)];
        Entry.Slot=Slot;
        Entry.Token=FleetToken(Rival);
        Entry.TeleportRevision=Rival->GetTeleportRevision();
        Entry.PhysicalStateValid=true;
        Entry.GroundSupported=Rival->HasQualifiedDraftRoadSupport();
        Entry.RouteComplete=Rival->HasCompletedRoute();
        Entry.PositionX=Position.X;
        Entry.PositionY=Position.Y;
        Entry.PositionZ=Position.Z;
        Entry.ForwardX=Forward.X;
        Entry.ForwardY=Forward.Y;
        Entry.VelocityX=Velocity.X;
        Entry.VelocityY=Velocity.Y;

        // Trace each potentially usable real wake separately. Only the
        // human and this exact leader are ignored. Other fleet actors
        // and actual circuit barriers remain physical obstructions.
        constexpr double MaximumTraceDistanceCm=5000.0;
        if (Entry.GroundSupported && !Entry.RouteComplete &&
            FVector::DistSquared(FollowingPosition,Position)<=
                MaximumTraceDistanceCm*MaximumTraceDistanceCm)
        {
            FCollisionQueryParams Params(SCENE_QUERY_STAT(LatebrakeFleetDraftSightline),false);
            Params.AddIgnoredActor(Observer);
            Params.AddIgnoredActor(Rival);
            FHitResult Hit;
            Entry.ClearPhysicalPath=!World->LineTraceSingleByChannel(
                Hit,FollowingPosition,Position,ECC_Visibility,Params);
        }
    }

    if (!latebrake::ValidateFleetDraftFrame(CandidateFrame)) return false;
    Out=CandidateFrame;
    return true;
}

void ALatebrakeRaceGameMode::ObserveFleetPlayerTraffic(double LiveDelta,bool bClockAdvanced)
{
    // Call once after the real player clock's admission, never from DrawHUD.
    if (!IsExperimentalFleetRequested()) return;
    if (!bClockAdvanced || Race.IsFinished() || !IsExperimentalFleetDriveReady())
    {
        FleetPlayerTraffic.Suspend();
        return;
    }
    latebrake::FleetPlayerTrafficSample Sample;
    if (!CollectFleetFrame(Sample.Physical,0.0,false))
    {
        FleetPlayerTraffic.Suspend();
        return;
    }
    Sample.Identity=FleetAttempt.BoundIdentity();
    Sample.EngineFrame=static_cast<std::uint64_t>(GFrameCounter);
    Sample.SessionEpoch=CheckpointSession.Current();
    Sample.ActiveSeconds=Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds();
    Sample.AdvancedSeconds=LiveDelta;
    for (int32 Slot=0;Slot<latebrake::StartGridCapacity;++Slot)
    {
        const auto Index=static_cast<std::size_t>(Slot);
        const auto* Car=FleetBoundActors[Index].Get();
        if (!IsValid(Car))
        {
            FleetPlayerTraffic.Suspend();
            return;
        }
        Sample.TeleportRevisions[Index]=Car->GetTeleportRevision();
    }
    FleetPlayerTraffic.Observe(Sample);
}

latebrake::FleetRaceReview ALatebrakeRaceGameMode::CollectFleetRaceReview() const
{
    if (!IsExperimentalFleetRequested() || !IsExperimentalFleetDriveReady()) return {};
    latebrake::FleetPhysicalFrame Frame;
    if (!CollectFleetFrame(Frame,0.0,false)) return {};
    std::array<latebrake::FleetRouteObservation,latebrake::StartGridCapacity> Routes{};
    for(int32 Slot=1;Slot<latebrake::StartGridCapacity;++Slot)
    {
        const auto* Rival=FleetRivalAt(Slot);
        if (!IsRegisteredRaceRival(Rival) || !Rival->IsRacePhysicsReady()) return {};
        Routes[static_cast<std::size_t>(Slot)]={
            Rival->GetExpectedLaps()==Race.GetRequiredLaps() &&
            Rival->GetVerifiedGateCount()==Race.GetCheckpointCount(),
            Rival->GetApproximateLaps(),Rival->GetNextVerifiedGate(),
            Rival->GetPilotRecoveries(),Rival->GetPilotSuccessfulRecoveries(),
            Rival->GetPilotFailedRecoveries(),Rival->HasCompletedRoute()};
    }
    auto Review=latebrake::BuildFleetRaceReview(Frame,FleetAttempt.BoundIdentity(),
        FleetReviewLedger,Routes,Race.GetCompletedLaps(),Race.GetNextCheckpoint(),
        Race.IsFinished(),Race.GetCheckpointCount(),Race.GetRequiredLaps(),
        Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds());
    Review.Timing=latebrake::ReviewFleetGateTiming(Review,FleetGateTiming,
        FleetAttemptToken,FleetAttempt.BoundIdentity());
    if (Review.ValidRoster && Review.PlayerRouteValid)
        Review.TrafficHistory=FleetPlayerTraffic.Readout();
    if (Review.PlayerFinished)
        Review.Pace=latebrake::ReviewFleetWholeRunPace(Review,FleetGateTiming,
            FleetAttemptToken,FleetAttempt.BoundIdentity());
    return Review;
}

latebrake::FleetRaceReview ALatebrakeRaceGameMode::GetFleetRaceReview() const
{
    if (!IsExperimentalFleetRequested()) return {};
    if (Race.IsFinished()) return FleetFinishReview;
    return CollectFleetRaceReview();
}

bool ALatebrakeRaceGameMode::TryGetExperimentalGridSettlingSlot(
    const ALatebrakeCarPawn* Actor,int32& OutSlot) const
{
    OutSlot=INDEX_NONE;
    if (!IsExperimentalFleetRequested() || StartLights.IsGreen() ||
        !IsValid(Actor) || !Actor->IsRacePhysicsReady() ||
        !bCircuitReady || !HasLocalPlayerDriver() || Race.IsFinished() ||
        !IsRestartPlacementReady() || Menu.IsOpen() ||
        UGameplayStatics::IsGamePaused(this) ||
        AppFocusGuard.IsSuspended() || ManualPause.IsSuspended())
        return false;
    const auto Phase=FleetAttempt.Readout().Phase;
    if (Phase!=latebrake::FleetAttemptPhase::AwaitingRoster &&
        Phase!=latebrake::FleetAttemptPhase::AwaitingGrid &&
        Phase!=latebrake::FleetAttemptPhase::CountdownQualified)
        return false;
    if (Actor==UGameplayStatics::GetPlayerPawn(this,0))
    {
        if (bFleetActorsBound && FleetBoundActors[0].Get()!=Actor) return false;
        OutSlot=latebrake::HumanGridSlot;
        return true;
    }
    const auto* Rival=Cast<ALatebrakeRivalCar>(Actor);
    if (!IsRegisteredRaceRival(Rival)) return false;
    const auto Slot=Rival->GetGridSlot();
    if (bFleetActorsBound &&
        FleetBoundActors[static_cast<std::size_t>(Slot)].Get()!=Actor) return false;
    OutSlot=Slot;
    return true;
}

bool ALatebrakeRaceGameMode::IsFleetStartPresentationReady() const
{
    if (!IsExperimentalFleetRequested()) return true;
    if (StartLights.IsGreen()) return IsExperimentalFleetDriveReady();
    return HasCurrentFleetFrameAuthority() &&
        FleetStartup.IsCurrentPermission(FleetAttemptToken,FleetFrameToken,
        CheckpointSession.Current()) && FleetAttempt.Readout().CountdownAllowed &&
        !Menu.IsOpen() && !UGameplayStatics::IsGamePaused(this) &&
        !AppFocusGuard.IsSuspended() && !ManualPause.IsSuspended();
}

bool ALatebrakeRaceGameMode::HasCurrentFleetFrameAuthority() const noexcept
{
    return FleetFrameAuthority.IsCurrent({FleetAttemptToken,FleetFrameToken,
        static_cast<std::uint64_t>(GFrameCounter),CheckpointSession.Current()});
}

void ALatebrakeRaceGameMode::RegisterFleetTickDependent(AActor* Actor)
{
    if (!IsExperimentalFleetRequested() || !IsValid(Actor) ||
        Actor==this || Actor->GetWorld()!=GetWorld()) return;
    for (const auto& Existing:FleetTickDependents)
        if (Existing.Get()==Actor) return;
    // GameMode observes the roster and advances the one shared active
    // clock before the pawn, rival or checkpoint samples its frame.
    // No reverse dependency is created; this is not a second timer.
    Actor->AddTickPrerequisiteActor(this);
    FleetTickDependents.Add(Actor);
}

void ALatebrakeRaceGameMode::ClearFleetTickPrerequisites()
{
    for (const auto& WeakActor:FleetTickDependents)
        if (AActor* Actor=WeakActor.Get())
            Actor->RemoveTickPrerequisiteActor(this);
    FleetTickDependents.Reset();
}
