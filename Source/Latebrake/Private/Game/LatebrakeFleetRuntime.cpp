#include "Game/LatebrakeRaceGameMode.h"
#include "AI/LatebrakeRivalCar.h"
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
    DestroyExtraFleet();
    if (FleetAttemptToken!=std::numeric_limits<uint64>::max())
        ++FleetAttemptToken;
    FleetFrameToken=0;
    FleetObservedEpoch=0;
    FleetBoundActors={};
    bFleetActorsBound=false;
    FleetReviewLedger.Reset();
    FleetFinishReview={};
    FleetAttempt.Begin({IsExperimentalFleetRequested(),SelectedEvent==latebrake::EventKind::Circuit,
        bCareerRace,FleetAttemptToken});
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
    if (Race.IsFinished()) return false;
    if (FleetFrameToken==std::numeric_limits<uint64>::max()) return false;
    ++FleetFrameToken;
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
    const auto Previous=FleetAttempt.Readout().Phase;
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
        latebrake::BindFleetReviewLedger(FleetReviewLedger,FleetAttempt.BoundIdentity(),
            Race.GetCheckpointCount(),Race.GetRequiredLaps());
    }
    if (Accepted) FleetObservedEpoch=CheckpointSession.Current();
    if (Current.Phase==latebrake::FleetAttemptPhase::Faulted && Previous!=Current.Phase)
    {
        UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_FAULT reason=%d slot=%d"),
            static_cast<int32>(Current.Fault),Current.FaultSlot);
    }
    return Accepted && (StartLights.IsGreen()?Current.RunningAllowed:Current.CountdownAllowed);
}

bool ALatebrakeRaceGameMode::IsExperimentalFleetDriveReady() const
{
    if (!IsExperimentalFleetRequested()) return true;
    if (!FleetAttempt.Readout().RunningAllowed || !bFleetActorsBound ||
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
    if (!FleetReviewLedger.RecordAcceptedGate(Slot,FleetToken(Actor),Ordinal,
        Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds()))
    {
        UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_GATE_RECEIPT_REFUSED slot=%d ordinal=%d"),
            Slot,Ordinal);
    }
}

void ALatebrakeRaceGameMode::ObserveFleetRivalRecovery(
    const ALatebrakeRivalCar* Actor,bool Verified)
{
    if (!IsExperimentalFleetRequested() || !IsExperimentalFleetDriveReady() ||
        !IsRegisteredRaceRival(Actor) || Race.IsFinished()) return;
    if (!FleetReviewLedger.RecordRecoveryResult(Actor->GetGridSlot(),FleetToken(Actor),
        Actor->GetPilotRecoveries(),Verified))
    {
        UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_RECOVERY_RECEIPT_REFUSED slot=%d"),
            Actor->GetGridSlot());
    }
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
    return latebrake::BuildFleetRaceReview(Frame,FleetAttempt.BoundIdentity(),
        FleetReviewLedger,Routes,Race.GetCompletedLaps(),Race.GetNextCheckpoint(),
        Race.IsFinished(),Race.GetCheckpointCount(),Race.GetRequiredLaps(),
        Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds());
}

latebrake::FleetRaceReview ALatebrakeRaceGameMode::GetFleetRaceReview() const
{
    if (!IsExperimentalFleetRequested()) return {};
    if (Race.IsFinished()) return FleetFinishReview;
    return CollectFleetRaceReview();
}
