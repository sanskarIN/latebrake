#include "Game/LatebrakeRaceGameMode.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Game/LatebrakePlayerController.h"

#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/SkyLight.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Racing/StartGantryVisual.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Progression/LatebrakePlayerSave.h"
#include "Progression/ProfileWriteCheckpoint.h"
#include "Progression/ProfileSaveWriteGuard.h"
#include "Progression/RecordValidation.h"
#include "Career/CareerLaunchSelection.h"
#include "Career/CareerReplayPreparation.h"
#include "Garage/CareerGaragePolicy.h"
#include "Garage/GarageQuickSelect.h"
#include "Track/CircuitLightingPreview.h"
#include "UI/GarageInputFocusPolicy.h"
#include "HAL/PlatformTime.h"
#include "Racing/LatebrakeCheckpoint.h"
#include "Racing/RaceRewards.h"
#include "Racing/FinishTimingEligibility.h"
#include "Racing/RaceClockReadiness.h"
#include "Racing/PlayerRaceClockReadiness.h"
#include "Racing/RecoveryCharge.h"
#include "Racing/BestLapGhost.h"
#include "Racing/GhostSeparation.h"
#include "Track/CircuitLayout.h"
#include "Track/PitBayPaintLayout.h"
#include "Track/TrackDecorations.h"
#include "Track/DecorationBatchPlan.h"
#include "Track/BrakingReferenceBoards.h"
#include "Track/CornerApproachChevrons.h"
#include "Track/CircuitStartupAudit.h"
#include "Track/CircuitCollisionPlan.h"
#include "Vehicles/ArcadeSkidRoadGeometry.h"
#include "Track/CollisionBlockSetup.h"
#include "Track/CollisionResponsePolicy.h"
#include "Track/CircuitSpawnSafety.h"
#include <limits>
#include "UI/LatebrakeRaceHUD.h"
#include "Vehicles/LatebrakeCarPawn.h"
#include "AI/LatebrakeRivalCar.h"

DEFINE_LOG_CATEGORY_STATIC(LogLatebrakeStartup, Log, All);
#if !UE_BUILD_SHIPPING
DEFINE_LOG_CATEGORY_STATIC(LogLatebrakeRaceAudit, Log, All);
#endif

namespace
{
constexpr TCHAR SaveSlot[] = TEXT("LatebrakeProfile");
constexpr int32 SaveUser = 0;
}

ALatebrakeRaceGameMode::ALatebrakeRaceGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = ALatebrakeCarPawn::StaticClass();
    PlayerControllerClass = ALatebrakePlayerController::StaticClass();
    HUDClass = ALatebrakeRaceHUD::StaticClass();
}

FVector ALatebrakeRaceGameMode::GetStartPosition()
{
    return FVector(latebrake::PlayerStart.Position.X,
                   latebrake::PlayerStart.Position.Y, 180.0);
}

FRotator ALatebrakeRaceGameMode::GetStartRotation()
{
    return FRotator(0.0, latebrake::PlayerStart.HeadingDegrees, 0.0);
}

AActor* ALatebrakeRaceGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (!IsValid(RuntimePlayerStart))
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        RuntimePlayerStart = GetWorld()->SpawnActor<APlayerStart>(
            APlayerStart::StaticClass(), GetStartPosition(), GetStartRotation(), Parameters);
    }
    return RuntimePlayerStart;
}

bool ALatebrakeRaceGameMode::IsRestartPlacementReady() const
{
    // Initial APlayerStart spawning does not rely on an explicit restart.
    // After RESTART, eligibility is exclusive to the exact verified pawn.
    // TWeakObjectPtr rejects destroyed actors, even if Unreal reuses memory.
    const ALatebrakeCarPawn* CurrentDriver = Cast<ALatebrakeCarPawn>(
        UGameplayStatics::GetPlayerPawn(this, 0));
    const bool bSameVerifiedPawn =
        IsValid(CurrentDriver) && VerifiedRestartPawn.IsValid() &&
        VerifiedRestartPawn.Get() == CurrentDriver;
    return RestartPlacement.CanRace(bSameVerifiedPawn);
}

bool ALatebrakeRaceGameMode::HasLocalPlayerDriver() const
{
    // Possession alone is insufficient: missing chassis mesh or disabled
    // physics/collision must not accumulate ghost race time. Even a valid
    // replacement pawn cannot inherit someone else's RESTART verification.
    const ALatebrakeCarPawn* Car = Cast<ALatebrakeCarPawn>(
        UGameplayStatics::GetPlayerPawn(this, 0));
    return IsRestartPlacementReady() && IsValid(Car) && Car->IsRacePhysicsReady();
}

void ALatebrakeRaceGameMode::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    bExperimentalFleetFlag=FParse::Param(FCommandLine::Get(),TEXT("LatebrakeExperimentalSixCarGrid"));
#endif
    LoadRecords();
    AttemptPreviousBestSeconds = GetEventBestSeconds();
    BuildCircuit();
    // A newly initialized world is an actual attempt boundary too. Replay
    // and traffic adapters reserve zero for uninitialized session stamps.
    CheckpointSession.OnRaceRestart();
    Race.Reset();
    ResetPlayerSectorMotion();
    TrackLimits.Reset();
    StewardTrackedPawn.Reset();
    StewardTeleportRevision=0;
    StewardCheckpointSessionRevision=0;
    StartLights.Reset();
    BuildStartGantry();
    RivalSplits.Configure(Race.GetCheckpointCount(), Race.GetRequiredLaps());
    RivalFinishClocks.Configure(Race.GetCheckpointCount(), Race.GetRequiredLaps());
    LastRivalFinishClockComparison = {};
    LastRivalFinishObservation = {};
    LaunchMeter.Reset();
    GhostRecorder.ResetRun();
    GhostSetup.Reset();
#if !UE_BUILD_SHIPPING
    StartRaceAuditAttempt(-1); // initial pawn placement not yet observed
#endif
    RefreshRival();
    BeginExperimentalFleetAttempt();
}

#if !UE_BUILD_SHIPPING
void ALatebrakeRaceGameMode::StartRaceAuditAttempt(int32 GridAtBegin)
{
    // Instrument only distinct attempts, never individual frame ticks.
    // Saturating means an extremely long session cannot wrap identifiers
    // and make independent attempts appear to share one log stream.
    if (RaceAuditAttempt >= std::numeric_limits<int32>::max())
    {
        return;
    }
    ++RaceAuditAttempt;
    UE_LOG(LogLatebrakeRaceAudit, Display,
        TEXT("LATEBRAKE_RACE_AUDIT v=1 action=begin attempt=%d gates=%d "
             "laps=%d track=%d grid=%d"),
        RaceAuditAttempt, Race.GetCheckpointCount(), Race.GetRequiredLaps(),
        bCircuitReady ? 1 : 0, GridAtBegin);
}
#endif

void ALatebrakeRaceGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FleetFrameAuthority.Revoke();
    ClearFleetTickPrerequisites();
    FleetCheckpointActors.Reset();
    Super::EndPlay(EndPlayReason);
}

void ALatebrakeRaceGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // Do not accumulate a fictional race result if track geometry,
    // required collision responses, gate actors or safe spawns failed.
    // Explicit pause check also defends unusual manually ticked worlds.
    const bool bLocalPlayerReady = HasLocalPlayerDriver();
    const bool bInactive =
        UGameplayStatics::IsGamePaused(this) || Menu.IsOpen() ||
        AppFocusGuard.IsSuspended() || ManualPause.IsSuspended();
    const bool bWasGreen=StartLights.IsGreen();
    const bool bFleetReady=ObserveExperimentalFleet(DeltaSeconds,bInactive);
    const bool bStartQualified =
        bCircuitReady && bLocalPlayerReady && !bInactive &&
        !Race.IsFinished() && IsRestartPlacementReady() && bFleetReady;
    // The countdown consumes ready, unpaused frames only. On the first
    // GO frame it returns JUST the residual frame time after green.
    // Keep authoritative timing and checkpoint-scoring on one light gate.
    const double LiveDelta = StartLights.Advance(
        static_cast<double>(DeltaSeconds), bStartQualified);
    if (IsExperimentalFleetRequested() && !bWasGreen && StartLights.IsGreen())
    {
        const bool Committed=FleetAttempt.CommitGreen(FleetAttemptToken,FleetFrameToken);
#if !UE_BUILD_SHIPPING
        if (Committed)
        {
            const auto Startup=FleetStartup.Readout();
            UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=go attempt=%llu frame=%llu stable=%.9f startup=%.9f"),
                static_cast<unsigned long long>(FleetAttemptToken),
                static_cast<unsigned long long>(FleetFrameToken),Startup.StableSeconds,Startup.ActiveSeconds);
        }
        else
        {
            const auto Failed=FleetAttempt.Readout();
            UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=fault attempt=%llu reason=%d slot=%d"),
                static_cast<unsigned long long>(FleetAttemptToken),
                static_cast<int32>(Failed.Fault),Failed.FaultSlot);
        }
#else
        static_cast<void>(Committed);
#endif
    }
    // A 3D cosmetic reflection of this SAME authoritative light phase.
    // It is not another timer and cannot authorize GO, brakes or gates.
    UpdateStartGantryVisual();
    const bool bActualClockAdvanced=latebrake::AdvancePlayerRaceClockIfReady(
        Race, bCircuitReady, bInactive || !IsRaceStartGreen(),
        bLocalPlayerReady, LiveDelta);
    ObserveFleetPlayerTraffic(LiveDelta,bActualClockAdvanced);
    BeginFleetReplayFrame(LiveDelta,bActualClockAdvanced);
    BeginFleetDecisionFrame(LiveDelta,bActualClockAdvanced);
    ObservePlayerSectorMotion(LiveDelta,bActualClockAdvanced);
    // Unofficial reference trace observes the SAME guarded player/clock
    // frame as official lap timing, but can never modify timing or physics.
    // World pause and focus loss can produce zero gameplay ticks; the
    // session revision guard also refuses to stitch resumed frames.
    const ALatebrakeCarPawn* GhostDriver = Cast<ALatebrakeCarPawn>(
        UGameplayStatics::GetPlayerPawn(this, 0));
    // No off-pavement sample survives a replaced driver, recovery,
    // pause/menu epoch, or actual clock refusal. The same real driver
    // that supplies ghost observations supplies the projected wheel pose.
    if (IsPracticeRace() && !Race.IsFinished() && bActualClockAdvanced)
    {
        const auto PracticeKey=ReadPracticeConfiguration(GhostDriver);
        const auto PracticeActor=IsValid(GhostDriver)?
            static_cast<std::uint64_t>(GhostDriver->GetUniqueID())+1u:0u;
        PracticeHistory.ObserveConfiguration(PracticeKey,PracticeActor);
        PracticeGateHistory.ObserveConfiguration(PracticeKey,PracticeActor);
    }
    const bool bStewardDriverValid=IsValid(GhostDriver) &&
        GhostDriver->IsRacePhysicsReady() && bLocalPlayerReady;
    const bool bStewardContinuous=bStewardDriverValid &&
        StewardTrackedPawn.Get()==GhostDriver &&
        StewardTeleportRevision==GhostDriver->GetTeleportRevision() &&
        StewardCheckpointSessionRevision==CheckpointSession.Current();
    if (!bStewardContinuous)
    {
        TrackLimits.Suspend();
        StewardTrackedPawn=bStewardDriverValid
            ? const_cast<ALatebrakeCarPawn*>(GhostDriver) : nullptr;
        StewardTeleportRevision=bStewardDriverValid
            ? GhostDriver->GetTeleportRevision() : 0;
        StewardCheckpointSessionRevision=CheckpointSession.Current();
    }
    if (bStartQualified && StartLights.IsGreen() && bActualClockAdvanced &&
        bStewardDriverValid && bStewardContinuous)
    {
        const FVector P=GhostDriver->GetActorLocation();
        const FVector V=GhostDriver->GetVelocity();
        const auto StewardEvent=TrackLimits.Observe(Race,{
            {static_cast<double>(P.X),static_cast<double>(P.Y)},
            static_cast<double>(GhostDriver->GetActorRotation().Yaw),
            static_cast<double>(GhostDriver->GetSpeedKmH()),
            static_cast<double>(V.Z),LiveDelta,
            GhostDriver->GetGroundContactReadout()},
            bActualClockAdvanced,bStewardContinuous);
        if (StewardEvent==latebrake::TrackLimitsEvent::PenaltyApplied)
        {
            // A fee creates a clock discontinuity in the sampled ghost.
            SuspendBestLapGhost();
        }
#if !UE_BUILD_SHIPPING
        if (StewardEvent==latebrake::TrackLimitsEvent::ViolationRecorded ||
            StewardEvent==latebrake::TrackLimitsEvent::PenaltyApplied ||
            StewardEvent==latebrake::TrackLimitsEvent::PenaltyRefused)
        {
            const auto Snapshot=TrackLimits.Snapshot();
            UE_LOG(LogLatebrakeRaceAudit, Display,
                TEXT("LATEBRAKE_TRACK_LIMITS event=%d violations=%d charged=%d"),
                static_cast<int32>(StewardEvent),Snapshot.Violations,Snapshot.Penalties);
        }
#endif
    }
    else
    {
        TrackLimits.Suspend();
    }

    if (!Race.IsFinished()) RefreshGhostConfiguration(GhostDriver);
    if (bStartQualified && StartLights.IsGreen() && LiveDelta > 0.0 &&
        IsValid(GhostDriver) && GhostDriver->IsRacePhysicsReady() && GhostSetup.IsBound())
    {
        // A fresh lap may rebind its OWN driver/session after a previous
        // pause or recovery invalidated that previous candidate. Never
        // rebind mid-lap to stitch unrelated movement into a ghost.
        if (GhostRecorder.GetCandidateCount() == 0 &&
            GhostRecorder.IsCandidateAllowed() &&
            Race.GetCurrentLapSeconds() <= 0.45)
        {
            GhostTrackedPawn = const_cast<ALatebrakeCarPawn*>(GhostDriver);
            GhostTeleportRevision = GhostDriver->GetTeleportRevision();
            GhostSessionRevision = CheckpointSession.Current();
        }
        if (GhostTrackedPawn.Get() != GhostDriver ||
            GhostTeleportRevision != GhostDriver->GetTeleportRevision() ||
            GhostSessionRevision != CheckpointSession.Current())
        {
            GhostRecorder.InvalidateCurrentLap();
        }
        else
        {
            const FVector P = GhostDriver->GetActorLocation();
            GhostRecorder.Record({Race.GetCurrentLapSeconds(),
                static_cast<double>(P.X), static_cast<double>(P.Y),
                static_cast<double>(P.Z),
                static_cast<double>(GhostDriver->GetActorRotation().Yaw)}, true);
        }
    }
    else if (StartLights.IsGreen() && !Race.IsFinished() &&
             !bStartQualified)
    {
        GhostRecorder.InvalidateCurrentLap();
    }
    UpdateBestLapGhostMarker(bStartQualified && StartLights.IsGreen());

    // Observational only: use the already-authorized post-GO clock,
    // excluding recovery fees, and signed actual player chassis speed.
    if (bStartQualified && StartLights.IsGreen())
    {
        const ALatebrakeCarPawn* Car = Cast<ALatebrakeCarPawn>(
            UGameplayStatics::GetPlayerPawn(this, 0));
        if (IsValid(Car))
        {
            LaunchMeter.Observe(
                Race.GetElapsedSeconds() - Race.GetTotalPenaltySeconds(),
                static_cast<double>(Car->GetForwardSpeedKmH()), bStartQualified);
        }
    }
    ApplyGarageToPlayer();
}

void ALatebrakeRaceGameMode::RefreshGhostConfiguration(const ALatebrakeCarPawn* Car)
{
    latebrake::GhostSetupStamp Stamp;
    const auto Key=GetActivePracticeTrial().Key;
    if (IsValid(Car) && Car->IsLocallyControlled() && Car->IsRacePhysicsReady() &&
        LastTunedPawn.Get()==Car && LastAppliedTuning==Key && Car->GetSelectedVehicleId()==Key.Selected)
    {
        const auto Handling=Car->GetHandlingMode();
        Stamp={static_cast<std::uint64_t>(Car->GetUniqueID())+1u,Key,
            Handling==ELatebrakeHandlingMode::Arcade?0:
            Handling==ELatebrakeHandlingMode::SimulationInspired?1:-1};
    }
    const auto Change=latebrake::RefreshGhostSetup(GhostSetup,GhostRecorder,Stamp,
        Race.GetCurrentLapSeconds()==0.0 && !Race.IsFinished());
    if (Change==latebrake::GhostSetupTransition::Lost ||
        Change==latebrake::GhostSetupTransition::Rebound ||
        Change==latebrake::GhostSetupTransition::Unavailable)
    {
        GhostTrackedPawn.Reset();
        if (IsValid(BestLapGhostMarker)) BestLapGhostMarker->SetActorHiddenInGame(true);
    }
}

void ALatebrakeRaceGameMode::SuspendBestLapGhost()
{
    // Paused Unreal worlds can stop ticking before the next GameMode
    // render update. Never leave a stale physical-looking beacon visible.
    if (StartLights.IsGreen() && !Race.IsFinished())
    {
        GhostRecorder.InvalidateCurrentLap();
    }
    if (IsValid(BestLapGhostMarker))
    {
        BestLapGhostMarker->SetActorHiddenInGame(true);
    }
}

void ALatebrakeRaceGameMode::ToggleBestLapGhostView()
{
    // Press-only local input, never a pause/resume shortcut. No ghost
    // preference may alter the best-lap recorder or racing authority.
    const bool bHumanInputAllowed =
        bCircuitReady && StartLights.IsGreen() && !Race.IsFinished() &&
        !Menu.IsOpen() && !UGameplayStatics::IsGamePaused(this) &&
        !AppFocusGuard.IsSuspended() && !ManualPause.IsSuspended() &&
        HasLocalPlayerDriver();
    if (GhostDisplay.Toggle(bHumanInputAllowed) &&
        !GhostDisplay.IsEnabled() && IsValid(BestLapGhostMarker))
    {
        // Paused worlds may stop ticking. Hiding is immediate rather than
        // waiting for the next replay update to observe the new preference.
        BestLapGhostMarker->SetActorHiddenInGame(true);
    }
}

bool ALatebrakeRaceGameMode::TryGetBestLapGhostSeparationMeters(
    double& OutMeters) const
{
    OutMeters = 0.0;
    // Only the currently rendered replay reference is measured. A hidden
    // marker, failed asset spawn or invalid race cannot show stale telemetry.
    if (!bCircuitReady || !StartLights.IsGreen() || Race.IsFinished() ||
        Race.GetCompletedLaps() <= 0 || !GhostSetup.IsBound() || !GhostRecorder.HasBest() ||
        !GhostDisplay.IsEnabled() || Menu.IsOpen() ||
        UGameplayStatics::IsGamePaused(this) ||
        AppFocusGuard.IsSuspended() || ManualPause.IsSuspended() ||
        !HasLocalPlayerDriver() || !IsValid(BestLapGhostMarker) ||
        BestLapGhostMarker->IsHidden())
    {
        return false;
    }
    const ALatebrakeCarPawn* Car = Cast<ALatebrakeCarPawn>(
        UGameplayStatics::GetPlayerPawn(this, 0));
    if (!IsValid(Car) || !Car->IsRacePhysicsReady())
    {
        return false;
    }

    latebrake::LapGhostPose Sample;
    if (!GhostRecorder.Evaluate(Race.GetCurrentLapSeconds(), Sample))
    {
        return false;
    }
    const FVector PlayerPosition = Car->GetActorLocation();
    return latebrake::TryComputeGhostSeparationMeters({
        true, true,
        static_cast<double>(PlayerPosition.X),
        static_cast<double>(PlayerPosition.Y),
        static_cast<double>(PlayerPosition.Z),
        Sample.X, Sample.Y, Sample.Z
    }, OutMeters);
}

void ALatebrakeRaceGameMode::UpdateBestLapGhostMarker(bool bReplayAuthorized)
{
    // A ghost is only displayed on a subsequent lap with a legitimate
    // active running clock. Never show a preview during menus, countdown,
    // invalid track, unfinished first lap, pause, or Results.
    latebrake::LapGhostPose Sample;
    const bool bShouldShow = GhostDisplay.CanRender(bReplayAuthorized) &&
        !Race.IsFinished() &&
        Race.GetCompletedLaps() > 0 && GhostSetup.IsBound() && GhostRecorder.HasBest() &&
        GhostRecorder.Evaluate(Race.GetCurrentLapSeconds(), Sample);
    if (!bShouldShow)
    {
        if (IsValid(BestLapGhostMarker))
        {
            BestLapGhostMarker->SetActorHiddenInGame(true);
        }
        return;
    }

    if (!IsValid(BestLapGhostMarker))
    {
        UStaticMesh* Cube = LoadObject<UStaticMesh>(
            nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        UWorld* World = GetWorld();
        if (!Cube || !World)
        {
            return; // Renderer unavailable: gameplay continues unaltered.
        }
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        BestLapGhostMarker = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), FVector::ZeroVector,
            FRotator::ZeroRotator, Parameters);
        if (!IsValid(BestLapGhostMarker))
        {
            return;
        }
#if WITH_EDITOR
        BestLapGhostMarker->SetActorLabel(TEXT("RunLocalBestLapGhostBeacon"));
#endif
        UStaticMeshComponent* Mesh = BestLapGhostMarker->GetStaticMeshComponent();
        if (!IsValid(Mesh))
        {
            BestLapGhostMarker->Destroy();
            BestLapGhostMarker = nullptr;
            return;
        }
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Cube);
        Mesh->SetSimulatePhysics(false);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCastShadow(false);
        Mesh->SetCanEverAffectNavigation(false);
        BestLapGhostMarker->SetActorEnableCollision(false);
        // Floating small beacon rather than an opaque full-car imitation.
        // A genuine translucent car-shell material is future asset work.
        BestLapGhostMarker->SetActorScale3D(FVector(0.65, 0.65, 0.18));
        if (UMaterialInstanceDynamic* Material =
                Mesh->CreateAndSetMaterialInstanceDynamic(0))
        {
            Material->SetVectorParameterValue(TEXT("Color"),
                FLinearColor(0.08f, 0.75f, 0.90f));
        }
    }
    // A real ghost path is observed world motion. Raise its small beacon
    // above the prior chassis so it cannot obstruct the current car.
    const FVector MarkerPosition(Sample.X, Sample.Y, Sample.Z + 130.0);
    BestLapGhostMarker->SetActorLocationAndRotation(
        MarkerPosition, FRotator(0.0, Sample.YawDegrees, 0.0),
        false, nullptr, ETeleportType::TeleportPhysics);
    BestLapGhostMarker->SetActorHiddenInGame(false);
}

AStaticMeshActor* ALatebrakeRaceGameMode::SpawnBlock(const TCHAR* Label,
    const FVector& Position, const FVector& Scale, const FLinearColor& Color)
{
    // The portable source plan already validates its nine transforms; this
    // secondary runtime boundary also fails closed if future callers supply
    // malformed locations or dimensions.
    const latebrake::CircuitCollisionBlock Requested{
        {Position.X, Position.Y}, Position.Z, Scale.X, Scale.Y, Scale.Z,
        latebrake::CollisionBlockStyle::Ground};
    if (!latebrake::IsValidCollisionBlock(Requested))
    {
        UE_LOG(LogLatebrakeStartup, Error,
            TEXT("LATEBRAKE_COLLISION_SETUP_FAILED label=%s reason=invalid-plan"),
            Label);
        return nullptr;
    }

    UStaticMesh* Cube = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cube)
    {
        UE_LOG(LogLatebrakeStartup, Error,
            TEXT("LATEBRAKE_COLLISION_SETUP_FAILED label=%s reason=missing-cube-asset"),
            Label);
        return nullptr;
    }

    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), Position, FRotator::ZeroRotator,
        Parameters);
    if (!Block)
    {
        UE_LOG(LogLatebrakeStartup, Error,
            TEXT("LATEBRAKE_COLLISION_SETUP_FAILED label=%s reason=actor-missing"),
            Label);
        return nullptr;
    }

#if WITH_EDITOR
    Block->SetActorLabel(Label);
#endif
    UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
    if (Mesh)
    {
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Cube);
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Block->SetActorScale3D(Scale);
    }

    const latebrake::CollisionBlockSetupDecision State =
        latebrake::EvaluateCollisionBlockSetup(
            Requested, true, Mesh != nullptr,
            Mesh && Mesh->GetStaticMesh() == Cube,
            Mesh && Mesh->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics,
            Block->GetActorScale3D().Equals(Scale, 0.01f));
    if (State != latebrake::CollisionBlockSetupDecision::Ready)
    {
        UE_LOG(LogLatebrakeStartup, Error,
            TEXT("LATEBRAKE_COLLISION_SETUP_FAILED label=%s reason=%s"),
            Label, UTF8_TO_TCHAR(latebrake::CollisionBlockSetupDecisionName(State)));
        Block->Destroy();
        return nullptr;
    }

    // QueryAndPhysics can still ignore the actual vehicle or physics
    // response channels. Require the intended BlockAll contract *after*
    // applying the Unreal component profile, before the startup audit
    // treats this Actor as a complete collision-bearing road/wall.
    const latebrake::TrackCollisionResponses Responses{
        Mesh->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block,
        Mesh->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Block,
        Mesh->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block
    };
    const latebrake::TrackCollisionResponseDecision ResponseState =
        latebrake::CheckTrackCollisionResponses(Responses);
    if (ResponseState != latebrake::TrackCollisionResponseDecision::Ready)
    {
        UE_LOG(LogLatebrakeStartup, Error,
            TEXT("LATEBRAKE_COLLISION_SETUP_FAILED label=%s reason=%s"),
            Label, UTF8_TO_TCHAR(
                latebrake::TrackCollisionResponseDecisionName(ResponseState)));
        Block->Destroy();
        return nullptr;
    }

    // An appearance material is optional for this prototype. Its failure
    // does not change an already checked physical actor configuration.
    if (UMaterialInstanceDynamic* Material =
            Mesh->CreateAndSetMaterialInstanceDynamic(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), Color);
    }

    // Still only validates component configuration in the Unreal source;
    // real ground contact and barrier physics require installed playtests.
    return Block;
}

void ALatebrakeRaceGameMode::BuildStartGantry()
{
    // One original asset-free, non-blocking fixture above the south grid.
    // A cosmetic failure must not invalidate the nine mandatory collision
    // actors or the four real checkpoint actors constructed by BuildCircuit.
    StartGantryLamps.Reset();
    StartGantryBulbs.Reset();
    LastStartGantryLampMask = -1;
    if (!GetWorld()) return;

    UStaticMesh* Cube = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (!Cube || !Sphere)
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_START_GANTRY_UNAVAILABLE missing engine shape assets"));
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Gantry = GetWorld()->SpawnActor<AActor>(
        AActor::StaticClass(), FVector(-1700.0, latebrake::SouthY, 0.0),
        FRotator::ZeroRotator, Params);
    if (!IsValid(Gantry)) return;
    StartGantryActor = Gantry;
#if WITH_EDITOR
    Gantry->SetActorLabel(TEXT("LatebrakeNonCollidingStartLights"));
#endif
    Gantry->SetActorEnableCollision(false);
    USceneComponent* Root = NewObject<USceneComponent>(
        Gantry, TEXT("StartGantryRoot"));
    if (!Root)
    {
        Gantry->Destroy();
        StartGantryActor = nullptr;
        return;
    }
    Root->SetMobility(EComponentMobility::Movable);
    Gantry->SetRootComponent(Root);
    Gantry->AddInstanceComponent(Root);
    Root->RegisterComponent();

    const auto AddMesh = [Gantry, Root](
        const FName& Name, UStaticMesh* Shape, const FVector& Position,
        const FVector& Scale) -> UStaticMeshComponent*
    {
        UStaticMeshComponent* Mesh =
            NewObject<UStaticMeshComponent>(Gantry, Name);
        if (!Mesh) return nullptr;
        Mesh->SetupAttachment(Root);
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Shape);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCastShadow(false);
        Mesh->SetRelativeLocation(Position);
        Mesh->SetRelativeScale3D(Scale);
        Gantry->AddInstanceComponent(Mesh);
        Mesh->RegisterComponent();
        return Mesh;
    };

    // Two side posts and a high crossbar have zero collision: they cannot
    // obstruct a starting/recovering car even in a failed asset load.
    AddMesh(TEXT("StartGantryLeftPost"), Cube,
        FVector(0.0, -925.0, 225.0), FVector(0.7, 0.7, 4.5));
    AddMesh(TEXT("StartGantryRightPost"), Cube,
        FVector(0.0, 925.0, 225.0), FVector(0.7, 0.7, 4.5));
    AddMesh(TEXT("StartGantryCrossbar"), Cube,
        FVector(0.0, 0.0, 450.0), FVector(0.8, 19.0, 0.45));

    // Three red countdown bulbs and one green GO bulb, all on the
    // driver-facing west side of the non-blocking gantry.
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FVector BulbPosition(
            -65.0, -285.0 + static_cast<double>(Index) * 190.0, 470.0);
        UStaticMeshComponent* Bulb = AddMesh(
            FName(*FString::Printf(TEXT("StartGantryBulb%d"), Index)),
            Sphere, BulbPosition, FVector(0.55, 0.55, 0.55));
        if (Bulb)
        {
            Bulb->SetVisibility(false);
        }
        StartGantryBulbs.Add(Bulb);

        UPointLightComponent* Light = NewObject<UPointLightComponent>(
            Gantry, FName(*FString::Printf(
                TEXT("StartGantryPointLight%d"), Index)));
        if (Light)
        {
            Light->SetupAttachment(Root);
            Light->SetMobility(EComponentMobility::Movable);
            Light->SetRelativeLocation(BulbPosition + FVector(-55.0, 0.0, 0.0));
            Light->SetIntensity(3900.0f);
            Light->SetAttenuationRadius(1050.0f);
            Light->SetLightColor(Index == 3
                ? FLinearColor(0.12f, 1.0f, 0.28f)
                : FLinearColor(1.0f, 0.12f, 0.07f));
            Light->SetCastShadows(false);
            Light->SetVisibility(false);
            Gantry->AddInstanceComponent(Light);
            Light->RegisterComponent();
        }
        StartGantryLamps.Add(Light);
    }
    UE_LOG(LogLatebrakeStartup, Display,
        TEXT("LATEBRAKE_START_GANTRY_CREATED noncolliding=1 lamps=%d"),
        StartGantryLamps.Num());
    UpdateStartGantryVisual();
}

void ALatebrakeRaceGameMode::UpdateStartGantryVisual()
{
    if (!IsValid(StartGantryActor)) return;
    const bool bSuspended =
        UGameplayStatics::IsGamePaused(this) || Menu.IsOpen() ||
        AppFocusGuard.IsSuspended() || ManualPause.IsSuspended();
    const latebrake::StartGantryVisual Plan =
        latebrake::PlanStartGantryVisual(
            StartLights.Phase(), bCircuitReady, HasLocalPlayerDriver() &&
                IsRestartPlacementReady() && IsFleetStartPresentationReady(),
            bSuspended, Race.IsFinished());

    int32 NewMask = 0;
    if (Plan.Active)
    {
        for (int32 Index = 0; Index < Plan.RedBulbs; ++Index)
        {
            NewMask |= 1 << Index;
        }
        if (Plan.GreenBulb) NewMask |= 1 << 3;
    }
    if (NewMask == LastStartGantryLampMask) return;
    LastStartGantryLampMask = NewMask;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const bool bOn = (NewMask & (1 << Index)) != 0;
        if (StartGantryLamps.IsValidIndex(Index) &&
            IsValid(StartGantryLamps[Index]))
        {
            StartGantryLamps[Index]->SetVisibility(bOn);
        }
        if (StartGantryBulbs.IsValidIndex(Index) &&
            IsValid(StartGantryBulbs[Index]))
        {
            StartGantryBulbs[Index]->SetVisibility(bOn);
        }
    }
}

void ALatebrakeRaceGameMode::BuildBrakingReferenceBoards(UStaticMesh* Mesh)
{
    // One optional scene actor, one ISM component, 24 world-space signs.
    // Never call SpawnBlock (BlockAll): road and checkpoint collisions are
    // audited separately and must not depend on decorative marker geometry.
    if (!Mesh || !GetWorld())
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_BRAKING_BOARDS_INCOMPLETE reason=missing_mesh_or_world"));
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Holder = GetWorld()->SpawnActor<AActor>(
        AActor::StaticClass(), FVector::ZeroVector,
        FRotator::ZeroRotator, Params);
    if (!IsValid(Holder))
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_BRAKING_BOARDS_INCOMPLETE reason=actor_spawn"));
        return;
    }
#if WITH_EDITOR
    Holder->SetActorLabel(TEXT("LatebrakeBrakingReferenceBoards_NoCollision"));
#endif
    Holder->SetActorEnableCollision(false);
    USceneComponent* Root = NewObject<USceneComponent>(
        Holder, TEXT("BrakingReferenceRoot"));
    if (!Root)
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_BRAKING_BOARDS_INCOMPLETE reason=scene_root"));
        Holder->Destroy();
        return;
    }
    Root->SetMobility(EComponentMobility::Static);
    Holder->SetRootComponent(Root);
    Holder->AddInstanceComponent(Root);
    Root->RegisterComponent();

    UInstancedStaticMeshComponent* Boards =
        NewObject<UInstancedStaticMeshComponent>(
            Holder, TEXT("BrakingReferenceBoardInstances"));
    if (!Boards)
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_BRAKING_BOARDS_INCOMPLETE reason=instancer"));
        Holder->Destroy();
        return;
    }
    Boards->SetupAttachment(Root);
    Boards->SetMobility(EComponentMobility::Static);
    Boards->SetStaticMesh(Mesh);
    Boards->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Boards->SetGenerateOverlapEvents(false);
    Boards->SetCanEverAffectNavigation(false);
    Boards->SetCastShadow(false);
    Boards->PreAllocateInstancesMemory(
        latebrake::BrakingBoardMarkCount);
    Holder->AddInstanceComponent(Boards);
    Boards->RegisterComponent();
    // The stock cube is a readable white fallback when the default
    // engine material does not expose the optional "Color" parameter.
    if (UMaterialInstanceDynamic* Mat =
            Boards->CreateAndSetMaterialInstanceDynamic(0))
    {
        Mat->SetVectorParameterValue(
            TEXT("Color"), FLinearColor(0.95f, 0.81f, 0.16f));
    }

    int32 Added = 0;
    for (int32 Index = 0;
         Index < latebrake::BrakingBoardMarkCount; ++Index)
    {
        const latebrake::BrakingReferenceBoard Board =
            latebrake::PlannedBrakingReferenceBoard(Index);
        if (!Board.Valid) continue;
        const FTransform Pose(
            FRotator(0.0, Board.YawDegrees, 0.0),
            FVector(Board.Center.X, Board.Center.Y, Board.HeightCm),
            FVector(Board.ScaleX, Board.ScaleY, Board.ScaleZ));
        if (Boards->AddInstance(Pose, false) >= 0)
        {
            ++Added;
        }
    }

    if (Added == latebrake::BrakingBoardMarkCount)
    {
        UE_LOG(LogLatebrakeStartup, Display,
            TEXT("LATEBRAKE_BRAKING_BOARDS_READY boards=%d/%d collision=off"),
            Added, latebrake::BrakingBoardMarkCount);
    }
    else
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_BRAKING_BOARDS_INCOMPLETE boards=%d/%d collision=off"),
            Added, latebrake::BrakingBoardMarkCount);
        Holder->Destroy();
    }
}

void ALatebrakeRaceGameMode::BuildCornerApproachChevrons(UStaticMesh* Mesh)
{
    // Optional painted arrows on the left-hand approach shoulder.
    // No physics, checkpoint, track-start readiness, navigation or SaveGame
    // effects; a failed cosmetic actor must not block actual racing.
    if (!Mesh || !GetWorld())
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_CORNER_CHEVRONS_INCOMPLETE reason=missing_mesh_or_world"));
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Holder = GetWorld()->SpawnActor<AActor>(
        AActor::StaticClass(), FVector::ZeroVector,
        FRotator::ZeroRotator, Params);
    if (!IsValid(Holder))
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_CORNER_CHEVRONS_INCOMPLETE reason=actor_spawn"));
        return;
    }
#if WITH_EDITOR
    Holder->SetActorLabel(TEXT("LatebrakeCornerApproachChevrons_NoCollision"));
#endif
    Holder->SetActorEnableCollision(false);
    USceneComponent* Root = NewObject<USceneComponent>(
        Holder, TEXT("CornerChevronRoot"));
    if (!Root)
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_CORNER_CHEVRONS_INCOMPLETE reason=scene_root"));
        Holder->Destroy();
        return;
    }
    Root->SetMobility(EComponentMobility::Static);
    Holder->SetRootComponent(Root);
    Holder->AddInstanceComponent(Root);
    Root->RegisterComponent();

    UInstancedStaticMeshComponent* Marks =
        NewObject<UInstancedStaticMeshComponent>(
            Holder, TEXT("CornerChevronInstances"));
    if (!Marks)
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_CORNER_CHEVRONS_INCOMPLETE reason=instancer"));
        Holder->Destroy();
        return;
    }
    Marks->SetupAttachment(Root);
    Marks->SetMobility(EComponentMobility::Static);
    Marks->SetStaticMesh(Mesh);
    Marks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marks->SetGenerateOverlapEvents(false);
    Marks->SetCanEverAffectNavigation(false);
    Marks->SetCastShadow(false);
    Marks->PreAllocateInstancesMemory(
        latebrake::CornerChevronPieceCount);
    Holder->AddInstanceComponent(Marks);
    Marks->RegisterComponent();

    // Stock cube material stays visible even without a Color parameter.
    if (UMaterialInstanceDynamic* Mat =
            Marks->CreateAndSetMaterialInstanceDynamic(0))
    {
        Mat->SetVectorParameterValue(
            TEXT("Color"), FLinearColor(0.22f, 0.91f, 0.88f));
    }

    int32 Added = 0;
    for (int32 Index = 0;
         Index < latebrake::CornerChevronPieceCount; ++Index)
    {
        const latebrake::CornerApproachChevron Mark =
            latebrake::PlannedCornerApproachChevron(Index);
        if (!Mark.Valid) continue;
        const FTransform Pose(
            FRotator(0.0, Mark.YawDegrees, 0.0),
            FVector(Mark.Center.X, Mark.Center.Y, Mark.HeightCm),
            FVector(Mark.ScaleX, Mark.ScaleY, Mark.ScaleZ));
        if (Marks->AddInstance(Pose, false) >= 0)
        {
            ++Added;
        }
    }

    if (Added == latebrake::CornerChevronPieceCount)
    {
        UE_LOG(LogLatebrakeStartup, Display,
            TEXT("LATEBRAKE_CORNER_CHEVRONS_READY marks=%d/%d collision=off"),
            Added, latebrake::CornerChevronPieceCount);
    }
    else
    {
        UE_LOG(LogLatebrakeStartup, Warning,
            TEXT("LATEBRAKE_CORNER_CHEVRONS_INCOMPLETE marks=%d/%d collision=off"),
            Added, latebrake::CornerChevronPieceCount);
        Holder->Destroy();
    }
}

void ALatebrakeRaceGameMode::ApplyCircuitLightingPreview()
{
    // One-time and explicit Settings actions only, never regular Tick.
    // This alters render light color/intensity, not road collision or
    // timing; all real lights still need UE Editor and packaged QA.
    const auto Scene = latebrake::LightingForMood(
        bDuskLightingPreview
            ? latebrake::CircuitLightingMood::Dusk
            : latebrake::CircuitLightingMood::Day);
    if (!latebrake::ValidCircuitLightingPreset(Scene))
        return;
    if (IsValid(RuntimeSun) && IsValid(RuntimeSun->GetLightComponent()))
    {
        RuntimeSun->SetActorRotation(FRotator(
            static_cast<float>(Scene.SunPitchDeg),
            static_cast<float>(Scene.SunYawDeg),0.f));
        RuntimeSun->GetLightComponent()->SetIntensity(
            static_cast<float>(Scene.SunIntensity));
        RuntimeSun->GetLightComponent()->SetLightColor(FLinearColor(
            static_cast<float>(Scene.SunColor.R),
            static_cast<float>(Scene.SunColor.G),
            static_cast<float>(Scene.SunColor.B)));
    }
    if (IsValid(RuntimeSky) && IsValid(RuntimeSky->GetLightComponent()))
    {
        RuntimeSky->GetLightComponent()->SetIntensity(
            static_cast<float>(Scene.SkyIntensity));
        RuntimeSky->GetLightComponent()->SetLightColor(FLinearColor(
            static_cast<float>(Scene.SkyColor.R),
            static_cast<float>(Scene.SkyColor.G),
            static_cast<float>(Scene.SkyColor.B)));
    }
}

void ALatebrakeRaceGameMode::ToggleCircuitLightingPreview()
{
    if (!latebrake::CanChangeCircuitLighting(
        Menu.IsOpen(),Menu.Current()==latebrake::MenuPage::Settings,
        AppFocusGuard.IsSuspended(),ManualPause.IsSuspended(),
        IsValid(RuntimeSun) && IsValid(RuntimeSky) &&
        IsValid(RuntimeSun->GetLightComponent()) &&
        IsValid(RuntimeSky->GetLightComponent())))
        return;
    bDuskLightingPreview = !bDuskLightingPreview;
    SettingsSelection.Changed();
    ApplyCircuitLightingPreview();
    // Deliberately no SaveRecords: this is an optional current-session
    // rendering test, not a new SaveGame schema or permanent unlock.
}

void ALatebrakeRaceGameMode::BuildCircuit()
{
    latebrake::CircuitStartupAudit Audit;
    bCircuitReady = false;
    FleetCheckpointActors.Reset();
    SkidRoadComponents.Reset();
    SkidRoadComponents.SetNum(latebrake::CircuitCollisionBlockCount);
    // Spawning an Actor does not prove rendering or collision on installed
    // devices, but failed block creation must never be hidden by the
    // four-checkpoint startup success marker.
    const auto RecordBlock = [this, &Audit](
        const TCHAR* Label, const FVector& Position,
        const FVector& Scale, const FLinearColor& Color)
    {
        AStaticMeshActor* Block = SpawnBlock(Label, Position, Scale, Color);
        Audit.RecordCollisionBlock(Block != nullptr);
        return Block;
    };
    // Runtime sunlight so the starter track remains visible without map assets.
    RuntimeSun = GetWorld()->SpawnActor<ADirectionalLight>();
    RuntimeSky = GetWorld()->SpawnActor<ASkyLight>();
    ApplyCircuitLightingPreview();

    // This portable single-source-of-truth plan reproduces the prior nine
    // world-space collision actors exactly; it never adds Epic asset copies.
    // A failed spawn is tracked and prevents startup readiness and rewards.
    constexpr const TCHAR* Labels[latebrake::CircuitCollisionBlockCount] =
    {
        TEXT("Ground"), TEXT("SouthStraight"), TEXT("NorthStraight"),
        TEXT("EastStraight"), TEXT("WestStraight"),
        TEXT("NorthBarrier"), TEXT("SouthBarrier"),
        TEXT("EastBarrier"), TEXT("WestBarrier")
    };
    const FLinearColor GroundColor(0.15f, 0.30f, 0.12f);
    const FLinearColor AsphaltColor(0.09f, 0.10f, 0.13f);
    const FLinearColor BarrierColor(0.80f, 0.15f, 0.05f);
    for (int32 Index = 0; Index < latebrake::CircuitCollisionBlockCount; ++Index)
    {
        const latebrake::CircuitCollisionBlock Block =
            latebrake::PlannedCollisionBlock(Index);
        const FLinearColor Color =
            Block.Style == latebrake::CollisionBlockStyle::Ground ? GroundColor :
            Block.Style == latebrake::CollisionBlockStyle::Asphalt ? AsphaltColor :
            BarrierColor;
        AStaticMeshActor* BlockActor=RecordBlock(Labels[Index],
            FVector(Block.Center.X, Block.Center.Y, Block.Height),
            FVector(Block.ScaleX, Block.ScaleY, Block.ScaleZ), Color);
        if (Block.Style==latebrake::CollisionBlockStyle::Asphalt && IsValid(BlockActor))
        {
            SkidRoadComponents[Index]=BlockActor->GetStaticMeshComponent();
        }
    }

    // All ninety-three collision-free decorative cubes now share one scene
    // actor, with seven instanced-mesh components (one per color/material
    // style). The nine collision-bearing road/ground/barrier actors remain
    // separate so physics behavior is not changed by visual optimization.
    // Exact original transforms come from portable DecorationBatchPlan.h.
    UStaticMesh* DecorationMesh = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    int32 CreatedDecorationGroups = 0;
    int32 CreatedDecorationInstances = 0;

    if (!DecorationMesh)
    {
        UE_LOG(LogLatebrakeStartup, Error,
            TEXT("Latebrake: decoration cube unavailable; cannot batch scenery"));
    }
    else
    {
        FActorSpawnParameters DecorParameters;
        DecorParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AActor* DecorationHolder = GetWorld()->SpawnActor<AActor>(
            AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator,
            DecorParameters);
        if (DecorationHolder)
        {
#if WITH_EDITOR
            DecorationHolder->SetActorLabel(TEXT("LatebrakeInstancedScenery"));
#endif
            USceneComponent* DecorationRoot = NewObject<USceneComponent>(
                DecorationHolder, TEXT("SceneryRoot"));
            if (DecorationRoot)
            {
                DecorationRoot->SetMobility(EComponentMobility::Static);
                DecorationHolder->SetRootComponent(DecorationRoot);
                DecorationHolder->AddInstanceComponent(DecorationRoot);
                DecorationRoot->RegisterComponent();

                const FLinearColor DecorationColors[] =
                {
                    FLinearColor(0.94f, 0.94f, 0.89f), // lane dashes
                    FLinearColor(0.83f, 0.11f, 0.09f), // red curb
                    FLinearColor(0.96f, 0.95f, 0.89f), // white curb
                    FLinearColor(0.18f, 0.21f, 0.26f), // grandstand base
                    FLinearColor(0.35f, 0.43f, 0.60f), // grandstand tier light
                    FLinearColor(0.26f, 0.31f, 0.42f), // grandstand tier dark
                    FLinearColor(0.90f, 0.60f, 0.08f)  // start banner
                };
                static_assert(UE_ARRAY_COUNT(DecorationColors) ==
                              latebrake::DecorationBatchCount);

                UInstancedStaticMeshComponent*
                    Groups[latebrake::DecorationBatchCount]{};
                for (int32 Group = 0; Group < latebrake::DecorationBatchCount; ++Group)
                {
                    const FName GroupName(*FString::Printf(
                        TEXT("SceneryGroup%d"), Group));
                    UInstancedStaticMeshComponent* Instances =
                        NewObject<UInstancedStaticMeshComponent>(
                            DecorationHolder, GroupName);
                    if (!Instances) continue;

                    Instances->SetupAttachment(DecorationRoot);
                    Instances->SetMobility(EComponentMobility::Static);
                    Instances->SetStaticMesh(DecorationMesh);
                    Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Instances->SetGenerateOverlapEvents(false);
                    Instances->SetCanEverAffectNavigation(false);
                    // Small ornamental pieces need no dynamic shadow passes.
                    // Lighting and appearance still require in-engine QA.
                    Instances->SetCastShadow(false);
                    Instances->PreAllocateInstancesMemory(
                        latebrake::PlannedBatchSize(
                            static_cast<latebrake::DecorationBatch>(Group)));
                    DecorationHolder->AddInstanceComponent(Instances);
                    Instances->RegisterComponent();
                    if (UMaterialInstanceDynamic* Material =
                            Instances->CreateAndSetMaterialInstanceDynamic(0))
                    {
                        Material->SetVectorParameterValue(
                            TEXT("Color"), DecorationColors[Group]);
                    }
                    Groups[Group] = Instances;
                    ++CreatedDecorationGroups;
                }

                for (int32 Index = 0; Index < latebrake::DecorationPieceCount;
                     ++Index)
                {
                    const latebrake::DecorationPlacement Placement =
                        latebrake::PlannedDecoration(Index);
                    const int32 Group = latebrake::DecorationBatchIndex(
                        Placement.Batch);
                    if (!Placement.Valid || Group < 0 || !Groups[Group])
                    {
                        continue;
                    }
                    // The holder has identity transform at world origin, so
                    // these are the same centimetre world placements as the
                    // old one-actor-per-piece version.
                    const FTransform Transform(
                        FRotator::ZeroRotator,
                        FVector(Placement.Center.X, Placement.Center.Y,
                                Placement.Height),
                        FVector(Placement.ScaleX, Placement.ScaleY,
                                Placement.ScaleZ));
                    if (Groups[Group]->AddInstance(Transform, false) >= 0)
                    {
                        ++CreatedDecorationInstances;
                    }
                }
            }
        }
    }

    Audit.SetScenery(CreatedDecorationGroups, CreatedDecorationInstances);
    if (CreatedDecorationGroups != latebrake::DecorationBatchCount ||
        CreatedDecorationInstances != latebrake::DecorationPieceCount)
    {
        UE_LOG(LogLatebrakeStartup, Error,
            TEXT("LATEBRAKE_SCENERY_INCOMPLETE groups=%d/%d instances=%d/%d"),
            CreatedDecorationGroups, latebrake::DecorationBatchCount,
            CreatedDecorationInstances, latebrake::DecorationPieceCount);
    }
    else
    {
        UE_LOG(LogLatebrakeStartup, Display,
            TEXT("LATEBRAKE_SCENERY_READY groups=%d instances=%d collision=off"),
            CreatedDecorationGroups, CreatedDecorationInstances);
    }

    // Extra driver wayfinding furniture is optional, never part of the
    // nine-block/four-checkpoint collision and startup readiness audit.
    BuildBrakingReferenceBoards(DecorationMesh);
    BuildCornerApproachChevrons(DecorationMesh);

    // Visual-only pit beacon near the inside edge of the southern road.
    // Do NOT use SpawnBlock: its BlockAll/QueryAndPhysics configuration is
    // for the nine mandatory audited collision-bearing road/wall actors.
    // This optional beacon is never a collision, checkpoint, or race gate.
    if (DecorationMesh)
    {
        FActorSpawnParameters PitParameters;
        PitParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AStaticMeshActor* PitBeacon = GetWorld()->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(),
            FVector(latebrake::RacePitBay.X,
                    latebrake::RacePitBay.Y + 265.0, 145.0),
            FRotator::ZeroRotator, PitParameters);
        if (PitBeacon)
        {
#if WITH_EDITOR
            PitBeacon->SetActorLabel(TEXT("PIT SERVICE - inner shoulder marker"));
#endif
            UStaticMeshComponent* MarkerMesh = PitBeacon->GetStaticMeshComponent();
            if (MarkerMesh)
            {
                MarkerMesh->SetMobility(EComponentMobility::Movable);
                MarkerMesh->SetStaticMesh(DecorationMesh);
                MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                MarkerMesh->SetGenerateOverlapEvents(false);
                MarkerMesh->SetCastShadow(false);
                PitBeacon->SetActorScale3D(FVector(3.8, 0.15, 2.4));
                if (UMaterialInstanceDynamic* PitMaterial =
                        MarkerMesh->CreateAndSetMaterialInstanceDynamic(0))
                {
                    PitMaterial->SetVectorParameterValue(TEXT("Color"),
                        FLinearColor(0.15f, 0.92f, 0.74f));
                }
            }
            else
            {
                PitBeacon->Destroy();
            }
        }
    }

    // Optional visual outline for the ACTUAL physical pit service bay.
    // One instance holder with eight non-colliding pavement stripes gives
    // spatial feedback without touching the nine collision-bearing blocks,
    // the required four checkpoints, or physical service eligibility.
    if (DecorationMesh)
    {
        FActorSpawnParameters MarkParameters;
        MarkParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AActor* PaintHolder = GetWorld()->SpawnActor<AActor>(
            AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator,
            MarkParameters);
        if (PaintHolder)
        {
#if WITH_EDITOR
            PaintHolder->SetActorLabel(TEXT("LatebrakePitBayPaint"));
#endif
            USceneComponent* PaintRoot = NewObject<USceneComponent>(
                PaintHolder, TEXT("PitPaintRoot"));
            if (PaintRoot)
            {
                PaintRoot->SetMobility(EComponentMobility::Static);
                PaintHolder->SetRootComponent(PaintRoot);
                PaintHolder->AddInstanceComponent(PaintRoot);
                PaintRoot->RegisterComponent();
                UInstancedStaticMeshComponent* Marks =
                    NewObject<UInstancedStaticMeshComponent>(
                        PaintHolder, TEXT("PitBayMarks"));
                if (Marks)
                {
                    Marks->SetupAttachment(PaintRoot);
                    Marks->SetMobility(EComponentMobility::Static);
                    Marks->SetStaticMesh(DecorationMesh);
                    Marks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Marks->SetGenerateOverlapEvents(false);
                    Marks->SetCanEverAffectNavigation(false);
                    Marks->SetCastShadow(false);
                    Marks->PreAllocateInstancesMemory(
                        latebrake::PitBayPaintMarkCount);
                    PaintHolder->AddInstanceComponent(Marks);
                    Marks->RegisterComponent();
                    if (UMaterialInstanceDynamic* PaintMaterial =
                            Marks->CreateAndSetMaterialInstanceDynamic(0))
                    {
                        PaintMaterial->SetVectorParameterValue(
                            TEXT("Color"), FLinearColor(0.18f, 0.96f, 0.77f));
                    }
                    int32 Added = 0;
                    for (int32 Index = 0;
                         Index < latebrake::PitBayPaintMarkCount; ++Index)
                    {
                        const latebrake::PitBayPaintMark Mark =
                            latebrake::PlannedPitBayPaintMark(Index);
                        if (!Mark.Valid) continue;
                        const FTransform Transform(
                            FRotator(0.f,
                                static_cast<float>(Mark.YawDegrees), 0.f),
                            FVector(Mark.Center.X, Mark.Center.Y, Mark.Height),
                            FVector(Mark.ScaleX, Mark.ScaleY, Mark.ScaleZ));
                        if (Marks->AddInstance(Transform, false) >= 0)
                        {
                            ++Added;
                        }
                    }
                    // A marker creation issue must be visible in installed
                    // logs, but cosmetics never silently authorize physics.
                    if (Added == latebrake::PitBayPaintMarkCount)
                    {
                        UE_LOG(LogLatebrakeStartup, Display,
                            TEXT("LATEBRAKE_PIT_BAY_PAINT_READY marks=%d/%d collision=off"),
                            Added, latebrake::PitBayPaintMarkCount);
                    }
                    else
                    {
                        UE_LOG(LogLatebrakeStartup, Warning,
                            TEXT("LATEBRAKE_PIT_BAY_PAINT_INCOMPLETE marks=%d/%d collision=off"),
                            Added, latebrake::PitBayPaintMarkCount);
                    }
                }
            }
        }
    }

    int32 SpawnedCheckpoints = 0;
    for (int32 Index = 0; Index < latebrake::CircuitGateCount; ++Index)
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ALatebrakeCheckpoint* Checkpoint = GetWorld()->SpawnActor<ALatebrakeCheckpoint>(
            ALatebrakeCheckpoint::StaticClass(),
            FVector(latebrake::CircuitGate(Index).X,
                    latebrake::CircuitGate(Index).Y, 38.0),
            FRotator::ZeroRotator, Parameters);
        Audit.RecordCheckpoint(Index, Checkpoint != nullptr);
        if (Checkpoint)
        {
            Checkpoint->SetCheckpointIndex(Index);
            FleetCheckpointActors.Add(Checkpoint);
            ++SpawnedCheckpoints;
        }
    }
    // A real installed-package QA session can grep for this log marker.
    // It proves only that the GameMode reached BeginPlay and spawned all
    // checkpoint actors; it does NOT certify rendering, input or gameplay.
    if (Audit.CollisionReady())
    {
        UE_LOG(LogLatebrakeStartup, Display,
            TEXT("LATEBRAKE_GEOMETRY_READY collision=%d/%d"),
            Audit.CollisionBlockCount(),
            latebrake::CircuitStartupAudit::RequiredCollisionBlocks);
    }
    else
    {
        UE_LOG(LogLatebrakeStartup, Error,
            TEXT("LATEBRAKE_GEOMETRY_INCOMPLETE collision=%d/%d attempts=%d"),
            Audit.CollisionBlockCount(),
            latebrake::CircuitStartupAudit::RequiredCollisionBlocks,
            Audit.CollisionBlockAttempts());
    }
    // Every requested collision-bearing actor and every unique gate must
    // exist before the game can authorize race results. Scenery must have
    // its expected source-created instance plan as well.
    // Prevent a future route/track edit from placing player starts or
    // recovery destinations inside perimeter barriers or off the road.
    // This checks pure planned geometry only; installed vehicle contact
    // must still be tested in the actual Unreal game.
    const bool bSpawnLayoutSafe = latebrake::AreCircuitSpawnLocationsSafe();
    if (!bSpawnLayoutSafe)
    {
        UE_LOG(LogLatebrakeStartup, Error,
            TEXT("LATEBRAKE_SPAWN_LAYOUT_BLOCKED unsafe player/rival start or recovery"));
    }
    bCircuitReady = Audit.Ready() && bSpawnLayoutSafe;
    if (bCircuitReady)
    {
        UE_LOG(LogLatebrakeStartup, Display,
            TEXT("LATEBRAKE_STARTUP_READY map=%s checkpoints=%d/%d"),
            *GetWorld()->GetMapName(), SpawnedCheckpoints,
            latebrake::CircuitGateCount);
    }
    else
    {
        UE_LOG(LogLatebrakeStartup, Error,
            TEXT("LATEBRAKE_STARTUP_BLOCKED map=%s checkpoints=%d/%d"),
            *GetWorld()->GetMapName(), SpawnedCheckpoints,
            latebrake::CircuitGateCount);
    }
}

void ALatebrakeRaceGameMode::RegisterCheckpoint(int32 CheckpointIndex)
{
    // A failed ground/road/barrier spawn cannot produce legitimate racing
    // progress or a career medal, even when four gate actors exist.
    if (!IsRaceStartGreen())
    {
        return; // Gate 0 is not scoreable during the red start lights.
    }
    if (!latebrake::CanScorePlayerCheckpoint(
            Race, bCircuitReady,
            UGameplayStatics::IsGamePaused(this) || Menu.IsOpen() ||
                AppFocusGuard.IsSuspended() || ManualPause.IsSuspended(),
            HasLocalPlayerDriver()))
    {
        return;
    }
    // RaceProgress only flips to Finished on a valid ordered player gate.
    // Once finished all subsequent overlap callbacks return false.
    const int32 LapsBeforeGate = Race.GetCompletedLaps();
    if (!Race.PassCheckpoint(CheckpointIndex))
    {
        return;
    }
    // Only a successfully accepted, ordered, player-authoritative gate
    // may generate an observational split timestamp. Never time a wrong gate.
    const int64 PlayerGateOrdinal =
        static_cast<int64>(Race.GetCompletedLaps()) * Race.GetCheckpointCount() +
        Race.GetNextCheckpoint();
    if (IsPracticeRace())
    {
        const auto* PracticeDriver=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
        PracticeGateHistory.ObserveGate(ReadPracticeConfiguration(PracticeDriver),
            IsValid(PracticeDriver)?static_cast<std::uint64_t>(PracticeDriver->GetUniqueID())+1u:0u,
            Race,CheckpointIndex);
    }
    if (PlayerGateOrdinal>=1 && PlayerGateOrdinal<=latebrake::CircuitGateCount*latebrake::SectorMotionMaximumLaps)
        ObservePlayerSectorGate(CheckpointIndex,static_cast<int32>(PlayerGateOrdinal));
    if (!IsExperimentalFleetRequested() && latebrake::GetRaceEvent(SelectedEvent).SpawnRival &&
        PlayerGateOrdinal >= 1 && PlayerGateOrdinal <= 25600)
    {
        RivalSplits.RecordPlayer(static_cast<int32>(PlayerGateOrdinal),
            Race.GetElapsedSeconds() - Race.GetTotalPenaltySeconds());
    }
    if (IsExperimentalFleetRequested() && PlayerGateOrdinal >= 1 && PlayerGateOrdinal <= 25600)
    {
        if (!FleetGateTiming.Record({FleetAttemptToken,FleetAttempt.BoundIdentity()[0],0,
                static_cast<int32>(PlayerGateOrdinal),
                Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds()}))
            UE_LOG(LogTemp,Warning,TEXT("LATEBRAKE_FLEET_TIMING_RECEIPT_REFUSED slot=0 ordinal=%d"),
                static_cast<int32>(PlayerGateOrdinal));
    }
#if !UE_BUILD_SHIPPING
    if (IsExperimentalFleetRequested())
        UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=gate attempt=%llu slot=0 id=%llu ordinal=%d clock=%.9f"),
            static_cast<unsigned long long>(FleetAttemptToken),
            static_cast<unsigned long long>(FleetAttempt.BoundIdentity()[0]),
            static_cast<int32>(PlayerGateOrdinal),Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds());
    // Only genuinely accepted, ordered, player-authoritative checkpoints
    // produce this low-volume diagnostic. The audit cannot award a lap.
    if (RaceAuditAttempt > 0 && FMath::IsFinite(Race.GetElapsedSeconds()))
    {
        UE_LOG(LogLatebrakeRaceAudit, Display,
            TEXT("LATEBRAKE_RACE_AUDIT v=1 action=gate attempt=%d gate=%d "
                 "lap=%d next=%d elapsed=%.6f active=%d finished=%d"),
            RaceAuditAttempt, CheckpointIndex, Race.GetCompletedLaps(),
            Race.GetNextCheckpoint(), Race.GetElapsedSeconds(),
            Race.GetActiveTimedLapCount(), Race.IsFinished() ? 1 : 0);
        LogPlayerSectorMotionAudit(CheckpointIndex,static_cast<int32>(PlayerGateOrdinal));
    }
#endif
    if (Race.GetCompletedLaps() > LapsBeforeGate)
    {
        // Observe the authoritative, ordered-gate boundary. Never use a
        // ghost frame to accept a checkpoint or create an official time.
        const bool bRealLapHadActiveTime =
            Race.GetActiveTimedLapCount() == Race.GetCompletedLaps();
        const auto* LapDriver=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
        RefreshGhostConfiguration(LapDriver);
        GhostRecorder.FinishLap(Race.GetLastLapSeconds(),
            bRealLapHadActiveTime && GhostSetup.IsBound(),
            Race.GetLastLapPenaltySeconds() == 0.0);
    }
    if (!Race.IsFinished())
    {
        return;
    }
    // The Results page can pause the world before GameMode gets another
    // Tick. Clear the beacon synchronously with the final ordered gate.
    SuspendBestLapGhost();

    // Ordered gate completion is necessary but not sufficient for rewards.
    // Every lap must have its own successful active race-clock advancement:
    // zero-time laps and penalty-only laps cannot earn medals or be saved as
    // recordable completions. The existing seven-day float bound still applies.
    const latebrake::FinishTimingEligibility FinishTiming =
        latebrake::ValidateFinishTiming(Race);
    if (IsExperimentalFleetRequested())
    {
        // Capture actual observed results before revoking run permissions.
        FleetFinishReview=FinishTiming==latebrake::FinishTimingEligibility::Eligible
            ? CollectFleetRaceReview() : latebrake::FleetRaceReview{};
        FleetReplay.Freeze(FleetAttemptToken,FleetAttempt.BoundIdentity(),
            FinishTiming==latebrake::FinishTimingEligibility::Eligible &&
            FleetFinishReview.ValidRoster && FleetFinishReview.PlayerRouteValid);
        FleetAttempt.Finish(FleetAttemptToken);
        FleetFrameAuthority.Revoke();
        LastRaceCredits=0;
        LastCareerAward={};
        LastRivalFinishObservation={};
        LastRivalFinishClockComparison={};
        CareerFeedback=TEXT("Experimental fleet test: no persistent rewards or records.");
#if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp,Display,
            TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=finish attempt=%llu clock=%.9f fees=%.9f timing=%d review=%d completed_routes=%d saved=0"),
            static_cast<unsigned long long>(FleetAttemptToken),
            Race.GetElapsedSeconds()-Race.GetTotalPenaltySeconds(),Race.GetTotalPenaltySeconds(),
            FinishTiming==latebrake::FinishTimingEligibility::Eligible?1:0,
            FleetFinishReview.ValidRoster && FleetFinishReview.PlayerRouteValid?1:0,
            FleetFinishReview.ObservedCompletedRoutes);
#endif
        return;
    }
    if (FinishTiming != latebrake::FinishTimingEligibility::Eligible)
    {
        if (IsPracticeRace()) PracticeGateHistory.Invalidate();
        LastRivalFinishObservation = {};
        LastRivalFinishClockComparison = {};
        LastCareerAward = {};
        LastRaceCredits = 0;
        CareerFeedback = TEXT("Finish rejected: missing per-lap active timing or unrecordable clock. Restart to retry.");
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: refusing finish timing eligibility=%d; no rewards, records or SaveGame write"),
            static_cast<int32>(FinishTiming));
        return;
    }

    // Practice completes the same physical lap sequence but does not
    // participate in any profile transaction or competitive observation.
    if (IsPracticeRace())
    {
        const auto* PracticeDriver=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
        const auto PracticeResult=PracticeHistory.Finish(ReadPracticeConfiguration(PracticeDriver),
            IsValid(PracticeDriver)?static_cast<std::uint64_t>(PracticeDriver->GetUniqueID())+1u:0u,Race);
        PracticeGateHistory.Finish(PracticeResult,Race);
        LastRaceCredits = 0;
        LastCareerAward = {};
        LastRivalFinishObservation = {};
        LastRivalFinishClockComparison = {};
        CareerFeedback = TEXT("Practice complete: no saved records or rewards.");
        return;
    }

    // Read the live rival exactly when the player physically finishes. This
    // gate-order observation cannot classify winners, set medals or award
    // credits: Unreal may tick two physically crossing cars in either order.
    const bool bRivalPresent = IsValid(RivalCar);
    LastRivalFinishObservation = latebrake::ObserveRivalFinish({
        SelectedEvent, bCareerRace,
        Race.IsFinished(), true, // Validated timing above.
        Race.GetCheckpointCount(), Race.GetRequiredLaps(),
        bRivalPresent,
        bRivalPresent && RivalCar->IsRacePhysicsReady(),
        bRivalPresent ? RivalCar->GetVerifiedGateCount() : 0,
        bRivalPresent ? RivalCar->GetExpectedLaps() : 0,
        bRivalPresent ? RivalCar->GetApproximateLaps() : 0,
        bRivalPresent ? RivalCar->GetNextVerifiedGate() : -1
    });
    // One session's previously accepted final rival crossing may be sampled
    // against the player's validated finish on the same active post-GO clock.
    // Reject tight frame-order ambiguity; never award or save a race ranking.
    LastRivalFinishClockComparison = RivalFinishClocks.CompareAtPlayerFinish(
        static_cast<int32>(PlayerGateOrdinal),
        Race.GetElapsedSeconds() - Race.GetTotalPenaltySeconds(),
        !bCareerRace && SelectedEvent == latebrake::EventKind::Circuit,
        LastRivalFinishObservation.State ==
            latebrake::RivalFinishObservationState::RivalRouteAlreadyComplete);

    // Only the SaveGame write decides if a finish becomes a durable reward.
    // In-memory first-completion medals, credits and best times must not
    // survive a failed disk write as misleading spendable progress.
    latebrake::ProfileWriteCheckpoint SavedProfile(
        Garage, Career, CompletedRaceCount, PersonalBestSeconds,
        PersonalBestLapSeconds, BestTimeAttackSeconds);
    CompletedRaceCount = latebrake::IncrementCompletedRaces(CompletedRaceCount);
    if (!bCareerRace)
    {
        float& EventRecord = (SelectedEvent == latebrake::EventKind::TimeAttack)
            ? BestTimeAttackSeconds : PersonalBestSeconds;
        EventRecord = latebrake::ImprovedBestTime(
            EventRecord, Race.GetElapsedSeconds());
    }
    PersonalBestLapSeconds = latebrake::ImprovedBestTime(
        PersonalBestLapSeconds, Race.GetBestLapSeconds());

    LastCareerAward = {};
    if (bCareerRace)
    {
        // No authoritative AI finishing position exists. Only validated
        // player checkpoint/lap state is used for career scoring.
        const latebrake::CareerFinish Finish{
            Race.GetElapsedSeconds(),
            Race.GetBestLapSeconds(),
            Race.GetSlowestLapSeconds(),
            Race.GetCompletedLaps(),
            RecoveriesThisRace,
            (Garage.SelectedVehicle() == CareerStartedVehicleId)
                ? CareerStartedVehicleId : -1,
            true
        };
        LastCareerAward = Career.CommitFinish(Career.SelectedEvent(), Finish);
        LastRaceCredits = LastCareerAward.Credits;
        if (LastCareerAward.FirstCompletion)
        {
            CareerFeedback = FString::Printf(
                TEXT("Career medal %d, +%d credits; championship bonus %d."),
                static_cast<int32>(LastCareerAward.Medal),
                LastRaceCredits, LastCareerAward.ChampionshipBonus);
        }
        else if (!LastCareerAward.ValidFinish)
        {
            CareerFeedback = TEXT("Career result rejected: invalid race or vehicle data.");
        }
        else if (LastCareerAward.Medal == latebrake::CareerMedal::None)
        {
            CareerFeedback = TEXT("Career objective missed: retry for a bronze medal.");
        }
        else
        {
            CareerFeedback = TEXT("Career replay: best medal kept; no duplicate completion credits.");
        }
    }
    else
    {
        LastRaceCredits = latebrake::CalculateRaceCredits(
            SelectedEvent, Race.GetElapsedSeconds(), RecoveriesThisRace);
    }
    Garage.EarnCredits(LastRaceCredits);
    SaveRecords();
    if (bProgressSaveFailed)
    {
        SavedProfile.Rollback();
        LastRaceCredits = 0;
        LastCareerAward = {};
        CareerFeedback = TEXT("Progress was NOT saved. Rewards and records reverted; retry after storage is fixed.");
        GarageFeedback = TEXT("WARNING: Save failed; rewards were rolled back.");
        ApplyGarageToPlayer();
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: race finish was not persisted; restoring previous garage/career/records in memory"));
    }
    else
    {
        // SaveGameToSlot returned success: preserve the newly earned medal,
        // credit balance, race count and personal bests.
        SavedProfile.Commit();
    }
}

void ALatebrakeRaceGameMode::SyncDesktopMenuPointerMode()
{
#if !PLATFORM_ANDROID
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!IsValid(PC) || !PC->IsLocalController())
    {
        return;
    }
    // The Canvas menu is not a Slate widget; let the existing pawn input
    // receive unhandled mouse actions and preserve keyboard/gamepad access.
    // When the menu closes, return focus to game driving with the cursor off.
    PC->bShowMouseCursor = Menu.IsOpen();
    if (Menu.IsOpen())
    {
        FInputModeGameAndUI MouseMode;
        MouseMode.SetHideCursorDuringCapture(false);
        MouseMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(MouseMode);
    }
    else
    {
        PC->SetInputMode(FInputModeGameOnly());
    }
#endif
}

void ALatebrakeRaceGameMode::CloseMenuForRace()
{
    TrackLimits.Suspend();
    GaragePurchaseConfirm.Reset();
    DismissGaragePalette();
    if (!Menu.IsOpen())
    {
        return;
    }
    SuspendBestLapGhost();
    // A restart may have unpaused the world before the menu is closed.
    // Neither change may allow a checkpoint to reuse an older position.
    Menu.Close();
    SettingsSelection.Reset();
    SyncGarageShowroomPreview(); // Restore equipped car before unpause.
    SyncDesktopMenuPointerMode();
    AppFocusGuard.OnExplicitResume();
    ManualPause.OnExplicitResume();
    CheckpointSession.OnMenuReturnToRace();
    if (ALatebrakeCarPawn* Car =
            Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Car->ClearDriverInputs();
    }
    UGameplayStatics::SetGamePaused(this, false);
    UpdateStartGantryVisual();
}

void ALatebrakeRaceGameMode::ToggleMenu()
{
    TrackLimits.Suspend();
    GaragePurchaseConfirm.Reset();
    DismissGaragePalette();
    SuspendBestLapGhost();
    Menu.Toggle();
    MainMenuFocus=0;
    SettingsSelection.Reset();
    SyncGarageShowroomPreview();
    if (!Menu.IsOpen())
    {
        AppFocusGuard.OnExplicitResume(); // Deliberate CLOSE/return to driving.
        ManualPause.OnExplicitResume();
    }
    // Checkpoint actor ticks may stop during pause. Invalidate pre-menu
    // motion even when their next tick is only after the menu closes.
    CheckpointSession.OnPauseTransition();
    // Always neutralize held inputs before changing input focus.
    if (ALatebrakeCarPawn* Car =
        Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Car->ClearDriverInputs();
    }
    SyncDesktopMenuPointerMode();
    UGameplayStatics::SetGamePaused(this, Menu.IsOpen());
    UpdateStartGantryVisual();
}

void ALatebrakeRaceGameMode::ToggleRacePause()
{
    TrackLimits.Suspend();
    SuspendBestLapGhost();
    // Standalone keyboard/gamepad/touch PAUSE does not open the menu.
    // An engine-world pause may fail, so the *player's requested pause*
    // must independently freeze racing, checkpoints and driving.
    if (Menu.IsOpen())
    {
        ToggleMenu(); // Also synchronously hides/resets the gantry signal.
        return;
    }
    if (ALatebrakeCarPawn* Car =
            Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Car->ClearDriverInputs();
    }
    // Platforms may automatically resume the world on returning to the
    // foreground, while the focus guard correctly stays latched. One
    // deliberate PAUSE/RESUME press may unlock both fallback gates without
    // toggling an already-running Unreal world back into a paused state.
    if (AppFocusGuard.IsSuspended() && !UGameplayStatics::IsGamePaused(this))
    {
        AppFocusGuard.OnExplicitResume();
        ManualPause.OnExplicitResume();
        CheckpointSession.OnPauseTransition();
        UpdateStartGantryVisual();
        return;
    }

    const bool bWasWorldPaused = UGameplayStatics::IsGamePaused(this);
    const bool bWasManualPause = ManualPause.IsSuspended();
    // Update our fallback latch BEFORE requesting the fallible world state.
    const bool bRequestWorldPause = ManualPause.OnToggleRequested(bWasWorldPaused);
    UGameplayStatics::SetGamePaused(this, bRequestWorldPause);
    const bool bWorldPausedNow = UGameplayStatics::IsGamePaused(this);

    // When SetGamePaused(true) fails, Unreal may still tick physics, but
    // the newly latched manual pause invalidates a pre-pause gate sample.
    // Likewise an explicitly cleared fallback needs a fresh baseline.
    if (bRequestWorldPause || bWasManualPause ||
        bWorldPausedNow != bWasWorldPaused)
    {
        CheckpointSession.OnPauseTransition();
    }
    if (!bRequestWorldPause && !bWorldPausedNow)
    {
        AppFocusGuard.OnExplicitResume(); // Player explicitly resumed.
    }
    if (bRequestWorldPause && !bWorldPausedNow)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: manual world pause failed; manual race pause guard remains suspended"));
    }
    UpdateStartGantryVisual();
}

void ALatebrakeRaceGameMode::PauseForAppInterruption()
{
    TrackLimits.Suspend();
    DismissGaragePalette(); // No chooser may survive an OS focus interruption.
    // Menu focus loss may not latch race suspension; still cancel spending.
    if (GaragePurchaseConfirm.HasPending())
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = TEXT("Focus lost: offer canceled. Press Q/U/I to preview again.");
    }

    // A first Q/U/I press must never authorize a second press after the
    // user has switched apps, even when Garage is already menu-paused.
    // AppFocusRaceGuard intentionally ignores menu-open focus loss for
    // race timing; consent cancellation must happen BEFORE that early return.
    GaragePurchaseConfirm.Reset();
    // Losing focus can happen without Unreal itself pausing the game loop.
    // Keep a separate fail-closed gate even if SetGamePaused returns false.
    // A foreground notification alone must NEVER resume competition.
    const latebrake::FocusLossTransition FocusTransition =
        AppFocusGuard.OnFocusLost(
            Race, Menu.IsOpen(), UGameplayStatics::IsGamePaused(this));
    if (FocusTransition == latebrake::FocusLossTransition::Ignored)
    {
        return;
    }

    SuspendBestLapGhost();
    // A fresh focus interruption invalidates pre-loss motion regardless of
    // whether Unreal/OS already paused the world before our callback.
    // Repeated deactivation events do not bump this epoch a second time.
    CheckpointSession.OnPauseTransition();
    if (FocusTransition == latebrake::FocusLossTransition::SuspendedNeedsWorldPause &&
        (!UGameplayStatics::SetGamePaused(this, true) ||
         !UGameplayStatics::IsGamePaused(this)))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: focus-loss world pause failed; race gate remains suspended"));
    }
    UpdateStartGantryVisual();
}

latebrake::SettingsMenuSnapshot ALatebrakeRaceGameMode::GetSettingsMenuSnapshot() const
{
    const auto* Player=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
    const auto* Controller=IsValid(Player)?Cast<APlayerController>(Player->GetController()):nullptr;
    const bool Local=IsValid(Controller) && Controller->IsLocalController() &&
        Controller->GetPawn()==Player && HasLocalPlayerDriver();
    const bool Lights=IsValid(RuntimeSun) && IsValid(RuntimeSky) &&
        IsValid(RuntimeSun->GetLightComponent()) && IsValid(RuntimeSky->GetLightComponent());
    const bool CameraReady=Local && Player->CanApplyCameraMotionPreference();
    return {{Menu.IsOpen(),Menu.Current(),Local,AppFocusGuard.IsSuspended(),
        ManualPause.IsSuspended(),bFutureSaveSchema,Lights,CameraReady},SettingsSelection.Current(),
        bMilesPerHour,HudTelemetryMode,bDuskLightingPreview,SettingsSelection.Revision(),CameraMotion,HudScale};
}

void ALatebrakeRaceGameMode::ActivateSettingsControl(latebrake::SettingsControl Control)
{
    const auto Current=GetSettingsMenuSnapshot();
    if(!Current.Access.Allows(Control) || !SettingsSelection.Select(Current.Access,Control))
        return;
    switch(Control)
    {
    case latebrake::SettingsControl::SpeedUnits:
    {
        const bool PreviousMilesPerHour = bMilesPerHour;
        bMilesPerHour = !bMilesPerHour;
        SaveRecords();
        if (bProgressSaveFailed)
        {
            bMilesPerHour = PreviousMilesPerHour;
            UE_LOG(LogTemp, Warning,
                TEXT("Latebrake: speed display unit change was not saved; restoring prior setting"));
        }
        break;
    }
    case latebrake::SettingsControl::HudDetail:
        CycleHudTelemetryMode();
        break;
    case latebrake::SettingsControl::CircuitLighting:
        ToggleCircuitLightingPreview();
        break;
    case latebrake::SettingsControl::CameraMotion:
    {
        auto* Player=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
        if (!IsValid(Player) || !Player->CanApplyCameraMotionPreference()) break;
        const auto Previous=CameraMotion;
        CameraMotion=latebrake::NextCameraMotionMode(CameraMotion);
        if (!Player->RefreshCameraMotionPreference())
            CameraMotion=Previous;
        SettingsSelection.Changed();
        break;
    }
    case latebrake::SettingsControl::HudScale:
        HudScale=latebrake::NextHudScalePreference(HudScale);
        SettingsSelection.Changed();
        break;
    default:break;
    }
}

void ALatebrakeRaceGameMode::CycleHudTelemetryMode()
{
    if (!latebrake::CanCycleHudTelemetryMode(Menu.IsOpen(),
        Menu.Current()==latebrake::MenuPage::Settings,HasLocalPlayerDriver(),
        AppFocusGuard.IsSuspended()))
        return;
    HudTelemetryMode=latebrake::NextHudTelemetryMode(HudTelemetryMode);
    SettingsSelection.Changed();
}

latebrake::QuickRaceChoiceSnapshot ALatebrakeRaceGameMode::GetQuickRaceChoiceSnapshot() const
{
    const auto* Player=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
    const auto* Controller=IsValid(Player)?Cast<APlayerController>(Player->GetController()):nullptr;
    const bool Local=IsValid(Controller) && Controller->IsLocalController() &&
        Controller->GetPawn()==Player;
    return {Menu.IsOpen(),Menu.Current(),Local,AppFocusGuard.IsSuspended(),
        bFutureSaveSchema,bExperimentalFleetFlag,
        latebrake::QuickRaceChoiceIndex({QuickRaceDifficultySelection,
            bQuickRaceSoloTimeAttackSelection,QuickPracticeSelection}),
        SettingsSelection.Revision(),QuickChoiceRevision};
}

bool ALatebrakeRaceGameMode::SelectQuickRaceChoice(int32 Index)
{
    const auto Snapshot=GetQuickRaceChoiceSnapshot();
    const auto Next=latebrake::QuickRaceChoiceAt(Index);
    if(!Snapshot.CanSelect() || !latebrake::IsValidQuickRaceMenuChoice(Next))return false;
    QuickRaceDifficultySelection=Next.Difficulty;
    bQuickRaceSoloTimeAttackSelection=Next.SoloTimeAttack;
    QuickPracticeSelection=Next.Practice;
    BumpQuickChoiceRevision();
    return true; // Deliberate SELECT still starts the race separately.
}

latebrake::PracticeTrialAccess ALatebrakeRaceGameMode::GetPracticeTrialAccess() const
{
    const auto Snapshot=GetQuickRaceChoiceSnapshot();
    return {Snapshot.MenuOpen,Snapshot.Page,Snapshot.LocalPlayer,
        Snapshot.FocusSuspended || IsManualRacePauseActive(),QuickPracticeSelection};
}

latebrake::PracticeExperimentPlan ALatebrakeRaceGameMode::GetPendingPracticeExperimentPlan() const
{
    if (!GetPracticeTrialAccess().AllowsEdit())return {};
    const auto* Car=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!IsValid(Car))return {};
    const auto Handling=Car->GetHandlingMode();
    const int HandlingId=Handling==ELatebrakeHandlingMode::Arcade?0:
        Handling==ELatebrakeHandlingMode::SimulationInspired?1:-1;
    const latebrake::PracticeConfiguration Key{QuickPracticeSelection,
        GetPendingPracticeTrial().Key,HandlingId};
    return PracticeAnalysis.ReadPlan(PracticeHistory,Key);
}

latebrake::PracticeLaunchPreview ALatebrakeRaceGameMode::GetPendingPracticeLaunchPreview() const
{
    const auto Plan=GetPendingPracticeExperimentPlan();
    if (!Plan.Valid)return {};
    return latebrake::BuildPracticeLaunchPreview(Plan.Selected,GetPendingPracticeTrial(),
        PracticeHistory,PracticeGateHistory);
}

bool ALatebrakeRaceGameMode::HandlePracticeTrialInput(
    latebrake::PracticeTrialAction Action,int32 Vehicle)
{
    // Consume all trial shortcuts on the visible practice setup page,
    // including blocked edits. Never fall through to a paid garage action.
    if (!Menu.IsOpen() || Menu.Current()!=latebrake::MenuPage::QuickRace ||
        !latebrake::IsQuickPractice(QuickPracticeSelection))return false;
    bool Changed=false;
    if (Action==latebrake::PracticeTrialAction::NextTest ||
        Action==latebrake::PracticeTrialAction::RetestBest)
    {
        const auto Choices=GetPendingPracticeExperimentChoices();
        const bool Next=Action==latebrake::PracticeTrialAction::NextTest;
        if (Next?Choices.NextValid:Choices.BestValid)
            Changed=PracticeTrial.Stage(GetPracticeTrialAccess(),Next?Choices.Next:Choices.Best);
    }
    else Changed=PracticeTrial.Edit(GetPracticeTrialAccess(),Action,Garage.Snapshot(),Vehicle);
    if (Changed)
    {
        BumpQuickChoiceRevision();
        GaragePurchaseConfirm.Reset();
    }
    return true;
}

latebrake::MainMenuDashboardSnapshot ALatebrakeRaceGameMode::GetMainMenuDashboardSnapshot() const
{
    const auto* Player=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
    const auto* Controller=IsValid(Player)?Cast<APlayerController>(Player->GetController()):nullptr;
    const bool Local=IsValid(Controller) && Controller->IsLocalController() &&
        Controller->GetPawn()==Player;
    // Menu navigation remains usable when chassis physics or grid setup
    // fails. The local possession check is independent of race readiness.
    return {Menu.IsOpen(),Menu.Current(),Local,AppFocusGuard.IsSuspended(),
        SettingsSelection.Revision(),MainMenuFocus};
}

bool ALatebrakeRaceGameMode::NavigateMainMenuDashboard(latebrake::MenuPage Destination,
    const latebrake::MainMenuDashboardSnapshot& Expected)
{
    const auto Current=GetMainMenuDashboardSnapshot();
    if(!Current.AllowsNavigation() || Current!=Expected ||
        !latebrake::ValidMainMenuDestination(Destination))return false;
    // Page navigation only: do not start a race, equip, spend or unpause.
    GaragePurchaseConfirm.Reset();
    DismissGaragePalette();
    Menu.Open(Destination);
    MainMenuFocus=0;
    SettingsSelection.Reset();
    if(Destination==latebrake::MenuPage::QuickRace)BumpQuickChoiceRevision();
    SyncGarageShowroomPreview();
    return true;
}

latebrake::CareerEventCardsSnapshot ALatebrakeRaceGameMode::GetCareerEventCardsSnapshot() const
{
    const auto* Player=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
    const auto* Controller=IsValid(Player)?Cast<APlayerController>(Player->GetController()):nullptr;
    const bool Local=IsValid(Controller) && Controller->IsLocalController() &&
        Controller->GetPawn()==Player;
    return latebrake::BuildCareerEventCardsSnapshot(Menu.IsOpen(),Menu.Current(),Local,
        AppFocusGuard.IsSuspended(),bFutureSaveSchema,CareerMenuSelection,
        SettingsSelection.Revision(),Career.Snapshot(),Garage.Snapshot());
}

bool ALatebrakeRaceGameMode::SelectCareerEventCard(int32 Id,
    const latebrake::CareerEventCardsSnapshot& Expected)
{
    const auto Current=GetCareerEventCardsSnapshot();
    const auto Description=latebrake::DescribeCareerEventCard(Current,Id);
    if(Current!=Expected || !Description.Valid || !Description.Selectable)return false;
    const auto* Event=latebrake::GetCareerEvent(Id);
    if(!Event || Career.Status(Id)==latebrake::CareerStatus::Locked ||
        !latebrake::IsEventInCareerMenu(Menu.Current(),Id))return false;
    // This is the existing pending Browse cursor, never Career.Select().
    // SELECT remains the separate launch and persistence transaction.
    CareerMenuSelection=Id;
    SettingsSelection.Changed();
    GaragePurchaseConfirm.Reset();
    CareerFeedback=FString::Printf(TEXT("Career selected: %s (%d/%d). SELECT to start."),
        UTF8_TO_TCHAR(Event->Name),Id+1,latebrake::CareerEventCount);
    return true;
}

void ALatebrakeRaceGameMode::MenuNext()
{
    GaragePurchaseConfirm.Reset();
    DismissGaragePalette();
    Menu.Next();
    MainMenuFocus=0;
    SettingsSelection.Reset();
    SyncGarageShowroomPreview();
}

void ALatebrakeRaceGameMode::MenuPrevious()
{
    GaragePurchaseConfirm.Reset();
    DismissGaragePalette();
    Menu.Previous();
    MainMenuFocus=0;
    SettingsSelection.Reset();
    SyncGarageShowroomPreview();
}

void ALatebrakeRaceGameMode::MenuActivate()
{
    if (!Menu.IsOpen()) return;
    GaragePurchaseConfirm.Reset();
    DismissGaragePalette();
    const latebrake::MenuPage Page = Menu.Current();
    if (Page == latebrake::MenuPage::Main)
    {
        const auto Snapshot=GetMainMenuDashboardSnapshot();
        NavigateMainMenuDashboard(latebrake::MainMenuFocusDestination(Snapshot.Focused),Snapshot);
    }
    else if (Page == latebrake::MenuPage::QuickRace)
    {
        if (latebrake::IsQuickPractice(QuickPracticeSelection))
        {
            if (StartQuickPractice(QuickPracticeSelection)) CloseMenuForRace();
            return;
        }
        const latebrake::EventKind ChosenEvent =
            latebrake::QuickRaceMenuEvent({
                QuickRaceDifficultySelection, bQuickRaceSoloTimeAttackSelection});
        // SelectRaceEvent already restarts when changing event or leaving
        // career mode. Avoid a second immediate reset in that case.
        const bool bNeedsEventChange = bCareerRace || IsPracticeRace() || SelectedEvent != ChosenEvent;
        if (bNeedsEventChange)
        {
            SelectRaceEvent(ChosenEvent);
        }
        else
        {
            RestartRace();
        }
        CloseMenuForRace();
    }
    else if (latebrake::IsCareerMenuPage(Page))
    {
        const int32 Target = GetMenuCareerEventId();
        if (StartCareerEvent(Target)) CloseMenuForRace();
    }
    else if (Page == latebrake::MenuPage::Garage)
    {
        GarageSelectViewedVehicle();
    }
    else if (Page == latebrake::MenuPage::Results)
    {
        // A replay may be refused if a required-car selection cannot be
        // persisted. Keep RESULTS open (and paused) if that happens.
        RestartRace();
        if (!Race.IsFinished())
        {
            CloseMenuForRace();
        }
    }
    else if (Page == latebrake::MenuPage::Pause)
    {
        ToggleMenu();
    }
    else if (Page == latebrake::MenuPage::Settings)
    {
        ActivateSettingsControl(SettingsSelection.Current());
    }
    // Profile is deliberately informational; no money or unlock is created.
}

void ALatebrakeRaceGameMode::SelectRaceEvent(latebrake::EventKind Kind)
{
    if (!latebrake::IsValidEvent(Kind) || (Kind == SelectedEvent && !bCareerRace && !IsPracticeRace()))
    {
        return;
    }
    bCareerRace = false;
    PracticeTrial.EndPractice();
    ActivePractice = latebrake::QuickPracticePreset::None;
    LastCareerAward = {};
    SelectedEvent = Kind;
    const latebrake::RaceEvent Definition = latebrake::GetRaceEvent(SelectedEvent);
    Race = latebrake::RaceProgress(Definition.CheckpointCount, Definition.RequiredLaps);
    RestartRace();
    RefreshRival();
}

bool ALatebrakeRaceGameMode::StartQuickPractice(latebrake::QuickPracticePreset Preset)
{
    const auto Definition = latebrake::GetQuickPracticeDefinition(Preset);
    if (!Definition.Valid || !Menu.IsOpen() ||
        Menu.Current() != latebrake::MenuPage::QuickRace)
        return false;
    // Both active mode and physical race configuration change together.
    // Browsing later does not affect this mode, its restart or lap count.
    auto Access=GetPracticeTrialAccess();
    if (Access.Preset!=Preset || !PracticeTrial.BeginPractice(Access))return false;
    ActivePractice = Preset;
    bCareerRace = false;
    LastCareerAward = {};
    SelectedEvent = Definition.Event;
    Race = latebrake::RaceProgress(Definition.CheckpointCount, Definition.RequiredLaps);
    RestartRace();
    RefreshRival();
    CareerFeedback = TEXT("Practice: run-local lap timing only; no profile rewards.");
    return true;
}

latebrake::PracticeConfiguration ALatebrakeRaceGameMode::ReadPracticeConfiguration(
    const ALatebrakeCarPawn* Car) const
{
    if (!IsPracticeRace() || !IsValid(Car) || !Car->IsRacePhysicsReady() ||
        LastTunedPawn.Get()!=Car)
        return {};
    const auto Key=GetActivePracticeTrial().Key;
    if (Car->GetSelectedVehicleId()!=Key.Selected)return {};
    if (!(Key==LastAppliedTuning)) return {};
    const auto Handling=Car->GetHandlingMode();
    const int HandlingId=Handling==ELatebrakeHandlingMode::Arcade?0:
        Handling==ELatebrakeHandlingMode::SimulationInspired?1:-1;
    return {ActivePractice,Key,HandlingId};
}

void ALatebrakeRaceGameMode::NotifyPracticeHandlingChange(const ALatebrakeCarPawn* Car)
{
    if (!Race.IsFinished() && IsValid(Car) && Car==UGameplayStatics::GetPlayerPawn(this,0))
        RefreshGhostConfiguration(Car);
    if (IsPracticeRace() && !Race.IsFinished() && IsValid(Car) &&
        Car==UGameplayStatics::GetPlayerPawn(this,0))
    {
        PracticeHistory.InvalidateConfiguration();
        PracticeGateHistory.Invalidate();
    }
}

FString ALatebrakeRaceGameMode::GetCareerEventName() const
{
    const int32 Id = (Menu.IsOpen() || !bCareerRace)
        ? GetMenuCareerEventId() : Career.SelectedEvent();
    const latebrake::CareerEvent* Event = latebrake::GetCareerEvent(Id);
    return Event ? FString(UTF8_TO_TCHAR(Event->Name)) : TEXT("No Career Event");
}

void ALatebrakeRaceGameMode::CycleCareerEvent()
{
    if (Menu.IsOpen() && Menu.Current()==latebrake::MenuPage::Main)
    {
        const auto Snapshot=GetMainMenuDashboardSnapshot();
        if(Snapshot.AllowsNavigation())
        {
            MainMenuFocus=latebrake::AdvanceMainMenuFocus(Snapshot.Focused,1);
            SettingsSelection.Changed();
        }
        return; // Main menu owns Browse even when input is suspended.
    }
    if (Menu.IsOpen() && Menu.Current()==latebrake::MenuPage::Settings)
    {
        SettingsSelection.Browse(GetSettingsMenuSnapshot().Access);
        return; // Never change a career selection while Settings owns Browse.
    }
    // Reuse the existing Browse control across keyboard, gamepad and touch.
    // Changing a pending tier never touches the AI in the *active* race,
    // allowing a player to preview choices safely while the menu is open.
    if (Menu.IsOpen() && Menu.Current() == latebrake::MenuPage::QuickRace)
    {
        const latebrake::QuickRaceMenuChoice Next =
            latebrake::NextQuickRaceMenuChoice({
                QuickRaceDifficultySelection, bQuickRaceSoloTimeAttackSelection, QuickPracticeSelection});
        QuickRaceDifficultySelection = Next.Difficulty;
        bQuickRaceSoloTimeAttackSelection = Next.SoloTimeAttack;
        QuickPracticeSelection = Next.Practice;
        BumpQuickChoiceRevision();
        return;
    }
    const int32 Original = CareerMenuSelection;
    const auto Page = Menu.IsOpen() && latebrake::IsCareerMenuPage(Menu.Current())
        ? Menu.Current() : latebrake::MenuPage::Career;
    const auto Next = latebrake::BrowseCareerMenuTarget(Career, Page, Original);
    if (Next.Valid())
    {
        CareerMenuSelection = Next.EventId;
        SettingsSelection.Changed();
        const auto* Event = latebrake::GetCareerEvent(Next.EventId);
        CareerFeedback = FString::Printf(TEXT("Career selected: %s (%d/%d). M start."),
            UTF8_TO_TCHAR(Event->Name), Next.EventId + 1, latebrake::CareerEventCount);
    }
}

void ALatebrakeRaceGameMode::StartNextAvailableCareerEvent()
{
    const int32 Next = Career.NextAvailable();
    StartCareerEvent(Next >= 0 ? Next : CareerMenuSelection);
}

bool ALatebrakeRaceGameMode::StartCareerEvent(int32 Id)
{
    if (bFutureSaveSchema)
    {
        CareerFeedback = TEXT("Newer save version detected; career writes disabled.");
        return false;
    }
    const latebrake::CareerEvent* Event = latebrake::GetCareerEvent(Id);

    // Stage the career and required garage vehicle together. An event is not
    // considered launched until SaveGameToSlot confirms its selection; do not
    // reset the running race or switch vehicles when persistence fails.
    latebrake::ProfileWriteCheckpoint SelectionWrite(
        Garage, Career, CompletedRaceCount, PersonalBestSeconds,
        PersonalBestLapSeconds, BestTimeAttackSeconds);
    const latebrake::CareerLaunchStatus Launch =
        latebrake::StageCareerLaunch(Career, Garage, Id);
    if (Launch != latebrake::CareerLaunchStatus::Ready)
    {
        CareerFeedback = Launch == latebrake::CareerLaunchStatus::RequiredVehicleNotOwned
            ? TEXT("Unlock the required vehicle in the garage first.")
            : TEXT("Career event is locked or unknown.");
        return false;
    }

    // Only selection is staged, not a race finish/reward. A successful
    // profile write must precede any live race or pawn state transition.
    SaveRecords();
    if (bProgressSaveFailed)
    {
        SelectionWrite.Rollback();
        CareerFeedback = TEXT("Career did NOT start: event/vehicle selection could not be saved.");
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: rejected career launch because profile selection was not saved"));
        return false;
    }
    SelectionWrite.Commit();

    // A career race uses the real player checkpoint clock. Rival telemetry is
    // never interpreted as a competitive finish or championship placement.
    CareerMenuSelection = Id;
    CareerStartedVehicleId = Garage.SelectedVehicle();
    bCareerRace = true;
    PracticeTrial.EndPractice();
    ActivePractice = latebrake::QuickPracticePreset::None;
    SelectedEvent = Event->SpawnRival
        ? latebrake::EventKind::Circuit : latebrake::EventKind::TimeAttack;
    Race = latebrake::RaceProgress(4, Event->Laps);
    LastCareerAward = {};
    RestartRace(); // Applies persisted garage tuning and repositions pawn.
    RefreshRival();
    CareerFeedback = FString::Printf(TEXT("Career: %s | %d laps | N browse, M start"),
        UTF8_TO_TCHAR(Event->Name), Event->Laps);
    return true;
}

float ALatebrakeRaceGameMode::GetEventBestSeconds() const
{
    if (IsPracticeRace() || IsExperimentalFleetRequested()) return 0.f;
    return SelectedEvent == latebrake::EventKind::TimeAttack ? BestTimeAttackSeconds : PersonalBestSeconds;
}

bool ALatebrakeRaceGameMode::TryGetSkidRoadSurface(
    const UPrimitiveComponent* HitComponent,int32& OutBlockIndex) const
{
    OutBlockIndex=INDEX_NONE;
    if (!bCircuitReady || !IsValid(HitComponent) ||
        SkidRoadComponents.Num()!=latebrake::CircuitCollisionBlockCount)
        return false;
    for (int32 Index=0;Index<SkidRoadComponents.Num();++Index)
    {
        const auto* Road=SkidRoadComponents[Index].Get();
        if (!IsValid(Road) || Road!=HitComponent ||
            !latebrake::IsArcadeSkidAsphaltBlock(Index))
            continue;
        const auto Plan=latebrake::PlannedCollisionBlock(Index);
        // Do not qualify a moved/scaled or repurposed actor as the
        // original asphalt plane just because its identity is retained.
        if (Road->GetCollisionObjectType()!=ECC_WorldStatic ||
            Road->GetCollisionEnabled()!=ECollisionEnabled::QueryAndPhysics ||
            !Road->GetComponentLocation().Equals(
                FVector(Plan.Center.X,Plan.Center.Y,Plan.Height),0.01) ||
            !Road->GetComponentScale().Equals(
                FVector(Plan.ScaleX,Plan.ScaleY,Plan.ScaleZ),0.001) ||
            !Road->GetComponentQuat().Equals(FQuat::Identity,0.001))
            return false;
        OutBlockIndex=Index;
        return true;
    }
    return false;
}

bool ALatebrakeRaceGameMode::HasRival() const
{
    return IsValid(RivalCar);
}

bool ALatebrakeRaceGameMode::TryGetRivalRadarPosition(FVector& OutPosition) const
{
    // The radar is observational only. Never report a stale/destroyed
    // rival actor or claim that an unhealthy body represents live racing.
    if (!IsValid(RivalCar) || !RivalCar->IsRacePhysicsReady())
    {
        return false;
    }
    const FVector Position = RivalCar->GetActorLocation();
    if (!FMath::IsFinite(Position.X) ||
        !FMath::IsFinite(Position.Y) ||
        !FMath::IsFinite(Position.Z))
    {
        return false;
    }
    OutPosition = Position;
    return true;
}

bool ALatebrakeRaceGameMode::TryGetRivalDraftingState(
    const ALatebrakeCarPawn* Observer, FVector& OutPosition,
    FVector& OutForward, FVector& OutVelocity) const
{
    // A rival state must belong to this very live race and a distinct,
    // physics-ready following pawn. Never infer a view from the HUD.
    UWorld* World = GetWorld();
    if (IsExperimentalFleetRequested() || !World || !IsValid(Observer) || Observer == RivalCar.Get() ||
        Observer->GetWorld() != World ||
        !latebrake::GetRaceEvent(SelectedEvent).SpawnRival ||
        !IsValid(RivalCar) || !RivalCar->IsRacePhysicsReady() ||
        !RivalCar->HasQualifiedDraftRoadSupport() ||
        RivalCar->HasCompletedRoute() || Race.IsFinished())
    {
        // An airborne or barely supported physical rival cannot give a
        // human an aerodynamic bonus by leaving a stale position behind.
        return false;
    }
    const FVector Position = RivalCar->GetActorLocation();
    const FVector Forward = RivalCar->GetActorForwardVector();
    const FVector Velocity = RivalCar->GetVelocity();
    if (!FMath::IsFinite(Position.X) || !FMath::IsFinite(Position.Y) ||
        !FMath::IsFinite(Position.Z) ||
        !FMath::IsFinite(Forward.X) || !FMath::IsFinite(Forward.Y) ||
        !FMath::IsFinite(Velocity.X) || !FMath::IsFinite(Velocity.Y) ||
        !FMath::IsFinite(Velocity.Z))
    {
        return false;
    }
    const FVector FollowingPosition = Observer->GetActorLocation();
    if (!FMath::IsFinite(FollowingPosition.X) ||
        !FMath::IsFinite(FollowingPosition.Y) ||
        !FMath::IsFinite(FollowingPosition.Z))
    {
        return false;
    }
    // No drafting target more than 50m away can satisfy the portable
    // 7-38m forward / 3.8m lateral / 2.3m height window. Avoid a world
    // query for distant rivals, and reject bad/teleporting gaps early.
    constexpr double MaximumDraftTraceDistanceCm = 5000.0;
    if (FVector::DistSquared(FollowingPosition, Position) >
        MaximumDraftTraceDistanceCm * MaximumDraftTraceDistanceCm)
    {
        return false;
    }
    // Query the REAL world from physical chassis root to physical chassis
    // root, excluding only the two endpoint actors. The source circuit's
    // BlockAll WorldStatic barriers block Visibility; a decorative road
    // texture with no collision does not. No score/AI/collision edits.
    FCollisionQueryParams TraceParams(
        SCENE_QUERY_STAT(LatebrakeRivalDraftSightline), false);
    TraceParams.AddIgnoredActor(Observer);
    TraceParams.AddIgnoredActor(RivalCar.Get());
    FHitResult Obstruction;
    if (World->LineTraceSingleByChannel(
            Obstruction, FollowingPosition, Position,
            ECC_Visibility, TraceParams))
    {
        // A wall/ground/other blocking actor between the two chassis
        // revokes the entire drafting confirmation, not only its HUD.
        return false;
    }
    OutPosition = Position;
    OutForward = Forward;
    OutVelocity = Velocity;
    return true;
}

int32 ALatebrakeRaceGameMode::GetRivalApproximateLaps() const
{
    return IsValid(RivalCar) ? RivalCar->GetApproximateLaps() : 0;
}

int32 ALatebrakeRaceGameMode::GetRivalPlannedLaps() const
{
    return IsValid(RivalCar) ? RivalCar->GetExpectedLaps() : 0;
}

int32 ALatebrakeRaceGameMode::GetRivalRecoveryCount() const
{
    return IsValid(RivalCar) ? RivalCar->GetPilotRecoveries() : 0;
}

int32 ALatebrakeRaceGameMode::GetRivalSuccessfulRecoveryCount() const
{
    return IsValid(RivalCar) ? RivalCar->GetPilotSuccessfulRecoveries() : 0;
}

int32 ALatebrakeRaceGameMode::GetRivalFailedRecoveryCount() const
{
    return IsValid(RivalCar) ? RivalCar->GetPilotFailedRecoveries() : 0;
}

bool ALatebrakeRaceGameMode::HasRivalFinishedRoute() const
{
    return IsValid(RivalCar) && RivalCar->HasCompletedRoute();
}

bool ALatebrakeRaceGameMode::TryGetRivalCreditedGateGap(
    latebrake::RivalGateGap& OutGap) const
{
    OutGap = {};
    // Do not describe a rival during Time Attack, a missing/corrupt race,
    // blocked start, menu, focus loss or failed player grid placement.
    // Rival progress is independently advanced only by its true forward
    // directional gate trace; this API never grants an actual placement.
    if (IsExperimentalFleetRequested() || !latebrake::GetRaceEvent(SelectedEvent).SpawnRival ||
        !bCircuitReady || !StartLights.IsGreen() ||
        Race.IsFinished() || !IsRestartPlacementReady() ||
        !HasLocalPlayerDriver() || Menu.IsOpen() ||
        UGameplayStatics::IsGamePaused(this) ||
        AppFocusGuard.IsSuspended() || ManualPause.IsSuspended() ||
        !IsValid(RivalCar) || !RivalCar->IsRacePhysicsReady() ||
        RivalCar->GetExpectedLaps() != Race.GetRequiredLaps() ||
        RivalCar->GetVerifiedGateCount() != Race.GetCheckpointCount())
    {
        return false;
    }

    OutGap = latebrake::CompareRivalCreditedGates({
        Race.GetCheckpointCount(), Race.GetRequiredLaps(),
        Race.GetCompletedLaps(), Race.GetNextCheckpoint(),
        RivalCar->GetApproximateLaps(), RivalCar->GetNextVerifiedGate(),
        true, true, true});
    return OutGap.Valid;
}

void ALatebrakeRaceGameMode::ObserveRivalVerifiedGate()
{
    // The existing rival has already accepted its OWN forward gate.
    // Never invent a rival actor, stamp an unqualified clock, or award
    // player checkpoint credit through this observation-only path.
    latebrake::RivalGateGap Eligibility;
    if (IsExperimentalFleetRequested() || !TryGetRivalCreditedGateGap(Eligibility) || !IsValid(RivalCar))
    {
        return;
    }
    const int64 RivalGateOrdinal =
        static_cast<int64>(RivalCar->GetApproximateLaps()) * Race.GetCheckpointCount() +
        RivalCar->GetNextVerifiedGate();
    if (RivalGateOrdinal >= 1 && RivalGateOrdinal <= 25600)
    {
        const double ActiveSeconds =
            Race.GetElapsedSeconds() - Race.GetTotalPenaltySeconds();
        const bool bAcceptedSplit = RivalSplits.RecordRival(
            static_cast<int32>(RivalGateOrdinal), ActiveSeconds);
        if (bAcceptedSplit && !bCareerRace &&
            SelectedEvent == latebrake::EventKind::Circuit &&
            RivalCar->HasCompletedRoute())
        {
            RivalFinishClocks.RecordRivalAcceptedGate(
                static_cast<int32>(RivalGateOrdinal), ActiveSeconds, true);
        }
    }
}

bool ALatebrakeRaceGameMode::TryGetLatestRivalMatchedGateSplit(
    latebrake::RivalMatchedGateSplit& OutSplit) const
{
    OutSplit = {};
    latebrake::RivalGateGap Eligibility;
    if (!TryGetRivalCreditedGateGap(Eligibility))
    {
        return false;
    }
    OutSplit = RivalSplits.LatestMatched();
    return OutSplit.Valid;
}

FString ALatebrakeRaceGameMode::GetSelectedVehicleName() const
{
    return FString(UTF8_TO_TCHAR(latebrake::GarageState::Definition(Garage.SelectedVehicle()).Name));
}

latebrake::GarageEquipUiState ALatebrakeRaceGameMode::GetGarageMutationUiState() const
{
    return latebrake::EvaluateGarageMutationUi(
        Menu.IsOpen(), Menu.Current(),
        IsFocusLossGuardActive() || IsManualRacePauseActive(),
        bFutureSaveSchema, latebrake::CanEditGarageDuringRace(bCareerRace, Race));
}

bool ALatebrakeRaceGameMode::RejectGarageMutationIfBlocked()
{
    const auto Access = GetGarageMutationUiState();
    if (latebrake::CanActivateGarageEquip(Access)) return false;
    GaragePurchaseConfirm.Reset();
    switch (Access)
    {
    case latebrake::GarageEquipUiState::Suspended:
        GarageFeedback = TEXT("Resume the suspended session before changing garage.");
        break;
    case latebrake::GarageEquipUiState::ReadOnly:
        GarageFeedback = TEXT("Newer profile version: garage is read-only.");
        break;
    case latebrake::GarageEquipUiState::CareerLocked:
        GarageFeedback = TEXT("Finish or exit this career race before changing garage.");
        break;
    default:
        GarageFeedback = TEXT("Open Garage page to change garage.");
        break;
    }
    return true;
}

latebrake::GaragePurchasePanelView ALatebrakeRaceGameMode::GetGaragePurchasePanel() const
{
    const auto Snapshot = Garage.Snapshot();
    return latebrake::BuildGaragePurchasePanel(Snapshot,
        GetGarageMutationUiState(),
        GaragePurchaseConfirm.Preview(Snapshot, FPlatformTime::Seconds()));
}

FString ALatebrakeRaceGameMode::GetGaragePurchaseDisplayFeedback() const
{
    const auto Access = GetGarageMutationUiState();
    if (Access == latebrake::GarageEquipUiState::Suspended)
        return TEXT("Garage suspended: resume before changing cars or spending.");
    if (Access == latebrake::GarageEquipUiState::ReadOnly)
        return TEXT("Newer profile version: garage is read-only.");
    // HUD observation never refreshes, authorizes, or consumes an offer.
    // The authorization window uses FPlatformTime while the world is paused.
    if (!Menu.IsOpen() || Menu.Current() != latebrake::MenuPage::Garage ||
        !GaragePurchaseConfirm.HasPending())
    {
        return GarageFeedback;
    }
    if (!latebrake::CanEditGarageDuringRace(bCareerRace, Race))
    {
        return TEXT("Finish or exit the career race before spending credits.");
    }
    const latebrake::GaragePendingPurchaseView Preview =
        GaragePurchaseConfirm.Preview(Garage.Snapshot(), FPlatformTime::Seconds());
    if (!Preview.IsPending())
    {
        return TEXT("Offer expired or garage changed. Press Q/U/I to preview again.");
    }

    const TCHAR* Action = nullptr;
    const TCHAR* Key = nullptr;
    switch (Preview.Kind)
    {
    case latebrake::GaragePurchaseKind::UnlockVehicle:
        Action = TEXT("UNLOCK");
        Key = TEXT("Q");
        break;
    case latebrake::GaragePurchaseKind::UpgradeEngine:
        Action = TEXT("ENGINE");
        Key = TEXT("U");
        break;
    case latebrake::GaragePurchaseKind::UpgradeGrip:
        Action = TEXT("GRIP");
        Key = TEXT("I");
        break;
    default:
        return GarageFeedback;
    }
    // FitHudText clips from the RIGHT in narrow viewports: put the required
    // confirmation key and expiring timer FIRST, before price/detail.
    return FString::Printf(
        TEXT("PRESS %s AGAIN (%.1fs) | %s %d CR"),
        Key, Preview.SecondsRemaining, Action, Preview.Price);
}

int32 ALatebrakeRaceGameMode::GetNextLockedVehicleCost() const
{
    const int32 NextId = Garage.NextLockedVehicle();
    return NextId >= 0 ? latebrake::GarageState::Definition(NextId).UnlockCost : 0;
}

void ALatebrakeRaceGameMode::ApplyGarageToPlayer()
{
    ALatebrakeCarPawn* Car = Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!IsValid(Car))
    {
        return;
    }
    const auto Trial=GetActivePracticeTrial();
    const latebrake::GarageTuningKey Requested = Trial.Key;
    const auto Look=latebrake::PracticeTrialLook(Trial,GarageLooks);
    if (LastTunedPawn.Get() != Car || !(Requested == LastAppliedTuning))
    {
        if (IsPracticeRace() && !Race.IsFinished())
        {
            PracticeHistory.InvalidateConfiguration();
            PracticeGateHistory.Invalidate();
        }
        // Cosmetic changes must NEVER alter physics multiplier authority;
        // only actual garage tuning/selection changes call SetGarageTuning.
        const latebrake::VehiclePerformance Performance = Trial.Performance;
        Car->SetGarageTuning(
            static_cast<float>(Performance.Acceleration),
            static_cast<float>(Performance.Grip),
            static_cast<float>(Performance.TopSpeed),
            Requested.Selected, Look);
        LastTunedPawn = Car;
        LastAppliedTuning = Requested;
        if (!Race.IsFinished()) RefreshGhostConfiguration(Car);
    }
    // Apply separately, including when the same vehicle receives a new
    // free paint/stripe/wheel choice. No force or collision body changes.
    Car->SetGarageAppearance(Look);
    SyncGarageShowroomPreview();
}

void ALatebrakeRaceGameMode::DismissGaragePalette()
{
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        if (auto* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
            Hud->CloseGaragePalette();
}

void ALatebrakeRaceGameMode::SyncGarageShowroomPreview()
{
    ALatebrakeCarPawn* Car=Cast<ALatebrakeCarPawn>(
        UGameplayStatics::GetPlayerPawn(this,0));
    if (!IsValid(Car)) return;
    const bool bViewing=Menu.IsOpen() &&
        Menu.Current()==latebrake::MenuPage::Garage;
    // No work in the normal racing loop; a visual only override exists
    // while viewing the Garage menu or restoring its equipped car.
    if (!bViewing && !Car->IsGarageShowroomPreviewActive())
        return;
    const int32 Viewed=GarageGallery.Viewed();
    Car->UpdateGarageShowroomPresentation(
        bViewing,Viewed,GarageLooks.Get(Viewed));
}

latebrake::GarageVehicleCardsSnapshot ALatebrakeRaceGameMode::GetGarageVehicleCardsSnapshot() const
{
    const auto* Player=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
    const auto* Controller=IsValid(Player)?Cast<APlayerController>(Player->GetController()):nullptr;
    const bool Local=IsValid(Controller) && Controller->IsLocalController() &&
        Controller->GetPawn()==Player;
    return latebrake::BuildGarageVehicleCardsSnapshot(CanBrowseGarage(),Local,
        AppFocusGuard.IsSuspended(),bFutureSaveSchema,GarageGallery.Viewed(),
        SettingsSelection.Revision(),Garage.Snapshot(),GarageLooks);
}

bool ALatebrakeRaceGameMode::BrowseGarageVehicleCard(int32 Id,
    const latebrake::GarageVehicleCardsSnapshot& Expected)
{
    const auto Current=GetGarageVehicleCardsSnapshot();
    if(Current!=Expected || !Current.CanBrowse() || !latebrake::GarageState::IsVehicle(Id))
        return false;
    GarageBrowseVehicle(Id);
    return GarageGallery.Viewed()==Id;
}

bool ALatebrakeRaceGameMode::CanBrowseGarage() const
{
    return Menu.IsOpen() && Menu.Current() == latebrake::MenuPage::Garage;
}

latebrake::GarageEquipUiState
ALatebrakeRaceGameMode::GetGarageEquipUiState() const
{
    return latebrake::EvaluateGarageEquipUi(
        Menu.IsOpen(), Menu.Current(),
        ManualPause.IsSuspended() || AppFocusGuard.IsSuspended(),
        bFutureSaveSchema,
        latebrake::CanEditGarageDuringRace(bCareerRace, Race),
        latebrake::CanEquipGalleryVehicle(Garage, GarageGallery),
        GarageGallery.Viewed(), Garage.SelectedVehicle());
}

bool ALatebrakeRaceGameMode::CanEquipGarageViewed() const
{
    return latebrake::CanActivateGarageEquip(GetGarageEquipUiState());
}

void ALatebrakeRaceGameMode::GarageBrowseNext()
{
    if (!CanBrowseGarage()) return;
    DismissGaragePalette();
    GaragePurchaseConfirm.Reset();
    const int32 id = GarageGallery.Next();
    SettingsSelection.Changed();
    const auto car = latebrake::GarageState::Definition(id);
    GarageFeedback = FString::Printf(TEXT("SHOWROOM: %s (%s)"),
        UTF8_TO_TCHAR(car.Name),
        Garage.Owns(id) ? TEXT("OWNED") : TEXT("LOCKED / PREVIEW ONLY"));
    SyncGarageShowroomPreview();
}

void ALatebrakeRaceGameMode::GarageBrowseVehicle(int32 VehicleId)
{
    // VIEW is never EQUIP: this handler cannot spend, select or save.
    if (!CanBrowseGarage() || !latebrake::GarageState::IsVehicle(VehicleId))
        return;
    if (!GarageGallery.Browse(VehicleId)) return;
    SettingsSelection.Changed();
    DismissGaragePalette();
    GaragePurchaseConfirm.Reset();
    const auto& Car = latebrake::GarageState::Definition(VehicleId);
    GarageFeedback = FString::Printf(TEXT("SHOWROOM: %s (%s)"),
        UTF8_TO_TCHAR(Car.Name),
        Garage.Owns(VehicleId) ? TEXT("OWNED") : TEXT("LOCKED / PREVIEW ONLY"));
    SyncGarageShowroomPreview();
}

void ALatebrakeRaceGameMode::GarageBrowsePrevious()
{
    if (!CanBrowseGarage()) return;
    DismissGaragePalette();
    GaragePurchaseConfirm.Reset();
    const int32 id = GarageGallery.Previous();
    SettingsSelection.Changed();
    const auto car = latebrake::GarageState::Definition(id);
    GarageFeedback = FString::Printf(TEXT("SHOWROOM: %s (%s)"),
        UTF8_TO_TCHAR(car.Name),
        Garage.Owns(id) ? TEXT("OWNED") : TEXT("LOCKED / PREVIEW ONLY"));
    SyncGarageShowroomPreview();
}

void ALatebrakeRaceGameMode::GarageSelectViewedVehicle()
{
    if (RejectGarageMutationIfBlocked()) return;
    // All direct, next-owned and viewed-car selection uses the same
    // authoritative transaction, save rollback and visual restoration.
    GarageSelectVehicle(GarageGallery.Viewed());
}

void ALatebrakeRaceGameMode::CommitGarageVehicleSelection(int32 VehicleId)
{
    // One transaction for both E/SELECT cycling and Ctrl+1..6 selection.
    // Snapshot every persisted dependency before changing live vehicle tuning.
    latebrake::ProfileWriteCheckpoint SavedProfile(
        Garage, Career, CompletedRaceCount, PersonalBestSeconds,
        PersonalBestLapSeconds, BestTimeAttackSeconds);
    if (!Garage.Select(VehicleId))
    {
        GarageFeedback = TEXT("Vehicle is not owned. Unlock it first.");
        return;
    }
    ApplyGarageToPlayer();
    GarageFeedback = FString::Printf(TEXT("Selected %s."), *GetSelectedVehicleName());
    SaveRecords();
    if (bProgressSaveFailed)
    {
        SavedProfile.Rollback();
        ApplyGarageToPlayer();
        GarageFeedback = TEXT("Vehicle selection not saved. Previous vehicle restored.");
    }
    else
    {
        SavedProfile.Commit();
    }
    GarageGallery.Reset(Garage.SelectedVehicle());
    SyncGarageShowroomPreview();
}

void ALatebrakeRaceGameMode::GarageNextVehicle()
{
    if (RejectGarageMutationIfBlocked()) return;
    DismissGaragePalette();
    GaragePurchaseConfirm.Reset();
    // Keep E/SELECT consistent with the new Ctrl+number shortcuts.
    // Never swap a running car or change equipment on a different page.
    if (!Menu.IsOpen() || Menu.Current() != latebrake::MenuPage::Garage)
    {
        GarageFeedback = TEXT("Open Garage page to change vehicles.");
        return;
    }
    if (!latebrake::CanEditGarageDuringRace(bCareerRace, Race))
    {
        GarageFeedback = TEXT("Finish or exit this career race before changing vehicles.");
        return;
    }
    const int32 Previous = Garage.SelectedVehicle();
    const int32 Next = Garage.NextOwnedVehicle();
    if (Next == Previous)
    {
        GarageFeedback = TEXT("Unlock another vehicle with Q first.");
        return;
    }
    CommitGarageVehicleSelection(Next);
}

void ALatebrakeRaceGameMode::GarageSelectVehicle(int32 VehicleId)
{
    if (RejectGarageMutationIfBlocked()) return;
    DismissGaragePalette();
    // The shortcut cannot operate while driving, on another page, or during
    // an active career race. Never interpret Ctrl+1..6 as purchase commands.
    const bool bGaragePage = Menu.Current() == latebrake::MenuPage::Garage;
    const latebrake::GarageQuickSelectDecision Decision =
        latebrake::PreviewGarageQuickSelect(
            Garage.Snapshot(), VehicleId, Menu.IsOpen(), bGaragePage,
            latebrake::CanEditGarageDuringRace(bCareerRace, Race));
    if (Decision.Status == latebrake::GarageQuickSelectStatus::MenuClosed ||
        Decision.Status == latebrake::GarageQuickSelectStatus::WrongPage)
    {
        return;
    }
    GaragePurchaseConfirm.Reset();
    switch (Decision.Status)
    {
    case latebrake::GarageQuickSelectStatus::CareerRaceLocked:
        GarageFeedback = TEXT("Finish or exit this career race before changing vehicles.");
        return;
    case latebrake::GarageQuickSelectStatus::InvalidVehicle:
        GarageFeedback = TEXT("Unknown garage shortcut.");
        return;
    case latebrake::GarageQuickSelectStatus::NotOwned:
        GarageFeedback = FString::Printf(
            TEXT("%s is locked. Q unlocks the next car only."),
            UTF8_TO_TCHAR(latebrake::GarageState::Definition(VehicleId).Name));
        return;
    case latebrake::GarageQuickSelectStatus::AlreadyEquipped:
        GarageFeedback = FString::Printf(
            TEXT("%s is already equipped."), *GetSelectedVehicleName());
        return;
    case latebrake::GarageQuickSelectStatus::Ready:
        CommitGarageVehicleSelection(Decision.VehicleId);
        return;
    default:
        return;
    }
}

bool ALatebrakeRaceGameMode::CanCustomizeGarage() const
{
    return Menu.IsOpen() &&
        Menu.Current() == latebrake::MenuPage::Garage &&
        !ManualPause.IsSuspended() && !AppFocusGuard.IsSuspended() &&
        latebrake::CanEditGarageDuringRace(bCareerRace, Race) &&
        !bFutureSaveSchema &&
        latebrake::CanStyleGalleryVehicle(Garage, GarageGallery);
}

bool ALatebrakeRaceGameMode::BeginGarageAppearancePreview()
{
    if (!CanCustomizeGarage()) return false;
    // Opening a free chooser must never keep earlier paid consent alive.
    // This is a transient intent boundary, not a profile transaction.
    if (GaragePurchaseConfirm.HasPending())
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = TEXT("Purchase preview cancelled. Choose an appearance for free.");
    }
    return true;
}

void ALatebrakeRaceGameMode::GarageCycleAppearance(
    latebrake::GarageLookLayer layer, bool bReset,
    const latebrake::GarageDirectAppearanceRequest* Direct)
{
    if (RejectGarageMutationIfBlocked()) return;
    DismissGaragePalette();
    GaragePurchaseConfirm.Reset(); // A cosmetic click cancels pending spend.
    if (!Menu.IsOpen() || Menu.Current() != latebrake::MenuPage::Garage)
    {
        GarageFeedback = TEXT("Open the Garage menu to customize your car.");
        return;
    }
    if (ManualPause.IsSuspended() || AppFocusGuard.IsSuspended())
    {
        GarageFeedback = TEXT("Customization suspended: resume the race first.");
        return;
    }
    if (!latebrake::CanEditGarageDuringRace(bCareerRace, Race))
    {
        GarageFeedback = TEXT("Finish or exit the career race before customizing.");
        return;
    }
    if (bFutureSaveSchema)
    {
        GarageFeedback = TEXT("Newer saved profile: this version cannot edit its appearance.");
        return;
    }
    // The edited car is ALWAYS the one the player can see. An owned
    // parked car can be styled without swapping the race pawn or changing
    // saved selection/physics. Locked previews are strictly read-only.
    if (!latebrake::CanStyleGalleryVehicle(Garage, GarageGallery))
    {
        GarageFeedback = TEXT("LOCKED CAR: unlock this vehicle before styling.");
        return;
    }
    const int32 Viewed = GarageGallery.Viewed();
    const latebrake::GarageAppearanceState Before = GarageLooks;
    if (Direct)
    {
        const auto Decision = latebrake::PreviewGarageDirectAppearance(
            Viewed, GarageLooks.Get(Viewed), *Direct);
        if (Decision.Status != latebrake::GarageDirectAppearanceStatus::Ready ||
            Direct->Layer != layer || bReset)
        {
            GarageFeedback = Decision.Status == latebrake::GarageDirectAppearanceStatus::Unchanged
                ? TEXT("LOOK ALREADY ACTIVE: no new save needed.")
                : TEXT("Appearance choice expired or invalid. Open the chooser again.");
            return;
        }
    }
    const bool bChanged = Direct ? GarageLooks.Choose(Viewed, layer, Direct->Choice) :
        bReset ? GarageLooks.ResetVehicle(Viewed) : GarageLooks.Cycle(Viewed, layer);
    if (!bChanged)
    {
        GarageFeedback = bReset
            ? TEXT("FACTORY LOOK ALREADY ACTIVE: no new save needed.")
            : TEXT("Unknown appearance option; no change saved.");
        return;
    }
    // Only the currently equipped car needs a live 3D Pawn update.
    // The Canvas showroom reads GarageLooks for the viewed ID directly.
    if (Viewed == Garage.SelectedVehicle())
        ApplyGarageToPlayer();
    SaveRecords();
    if (bProgressSaveFailed)
    {
        GarageLooks = Before;
        if (Viewed == Garage.SelectedVehicle())
            ApplyGarageToPlayer();
        SyncGarageShowroomPreview();
        GarageFeedback = TEXT("Appearance NOT saved; previous car look restored.");
        return;
    }
    SyncGarageShowroomPreview();
    const latebrake::GarageLook Look = GarageLooks.Get(Viewed);
    const int32 Choice = latebrake::GarageLookChoice(Look,layer);
    if (bReset)
    {
        GarageFeedback = FString::Printf(
            TEXT("FACTORY LOOK RESTORED AND SAVED: %s"),
            UTF8_TO_TCHAR(latebrake::GarageState::Definition(Viewed).Name));
    }
    else
    {
        GarageFeedback = FString::Printf(TEXT("LOOK SAVED: %s / %s"),
            UTF8_TO_TCHAR(latebrake::GarageState::Definition(Viewed).Name),
            UTF8_TO_TCHAR(latebrake::GarageLookChoiceName(layer, Choice)));
    }
}

void ALatebrakeRaceGameMode::GarageChooseAppearance(
    latebrake::GarageDirectAppearanceRequest Request)
{
    // Reuse the shared ownership/paused/profile guards and one save rollback.
    GarageCycleAppearance(Request.Layer, false, &Request);
}

void ALatebrakeRaceGameMode::GarageCyclePaint()
{
    GarageCycleAppearance(latebrake::GarageLookLayer::Paint);
}
void ALatebrakeRaceGameMode::GarageCycleStripe()
{
    GarageCycleAppearance(latebrake::GarageLookLayer::Stripe);
}
void ALatebrakeRaceGameMode::GarageCycleWheels()
{
    GarageCycleAppearance(latebrake::GarageLookLayer::Wheels);
}
void ALatebrakeRaceGameMode::GarageResetAppearance()
{
    GarageCycleAppearance(latebrake::GarageLookLayer::Paint, true);
}

void ALatebrakeRaceGameMode::GarageUnlockNext()
{
    if (RejectGarageMutationIfBlocked()) return;
    DismissGaragePalette();
    if (!latebrake::CanEditGarageDuringRace(bCareerRace, Race))
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = TEXT("Finish or exit this career race before buying vehicles.");
        return;
    }
    // A Garage-page pause is mandatory before any two-step purchase.
    // Closed-menu keyboard input must not stage a hidden transaction.
    if (!Menu.IsOpen() || Menu.Current() != latebrake::MenuPage::Garage)
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = TEXT("Open Garage page to preview a purchase.");
        return;
    }
    const int32 Next = Garage.NextLockedVehicle();
    if (Next < 0)
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = TEXT("Every prototype profile is already unlocked.");
        return;
    }
    const int32 Cost = latebrake::GarageState::Definition(Next).UnlockCost;
    if (!Garage.CanUnlock(Next))
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = FString::Printf(TEXT("Need %d credits; currently %d."), Cost, Garage.Credits());
        return;
    }
    if (!GaragePurchaseConfirm.ConfirmOrStage(
        latebrake::GaragePurchaseKind::UnlockVehicle,
        Garage.Snapshot(), Next, Cost, FPlatformTime::Seconds()))
    {
        GarageFeedback = FString::Printf(
            TEXT("UNLOCK %s: %d credits. Press Q again within 8s to confirm."),
            UTF8_TO_TCHAR(latebrake::GarageState::Definition(Next).Name), Cost);
        return; // Preview has not mutated credits or ownership.
    }
    latebrake::ProfileWriteCheckpoint SavedProfile(
        Garage, Career, CompletedRaceCount, PersonalBestSeconds,
        PersonalBestLapSeconds, BestTimeAttackSeconds);
    if (!Garage.Unlock(Next))
    {
        GarageFeedback = TEXT("Vehicle unlock rejected.");
        return;
    }
    GarageFeedback = FString::Printf(TEXT("Unlocked %s!"), UTF8_TO_TCHAR(latebrake::GarageState::Definition(Next).Name));
    SaveRecords();
    if (bProgressSaveFailed)
    {
        SavedProfile.Rollback();
        GarageFeedback = TEXT("Vehicle purchase not saved. Credits and locked vehicle restored.");
    }
    else
    {
        SavedProfile.Commit();
    }
}

void ALatebrakeRaceGameMode::GarageUpgradeEngine()
{
    if (RejectGarageMutationIfBlocked()) return;
    DismissGaragePalette();
    if (!latebrake::CanEditGarageDuringRace(bCareerRace, Race))
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = TEXT("Finish or exit this career race before upgrading engine.");
        return;
    }
    // A Garage-page pause is mandatory before any two-step purchase.
    // Closed-menu keyboard input must not stage a hidden transaction.
    if (!Menu.IsOpen() || Menu.Current() != latebrake::MenuPage::Garage)
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = TEXT("Open Garage page to preview a purchase.");
        return;
    }
    const int32 Cost = Garage.NextUpgradeCost(latebrake::UpgradeKind::Engine);
    if (Cost <= 0 || Garage.Credits() < Cost)
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = Cost <= 0 ? TEXT("Engine is already fully upgraded.")
            : FString::Printf(TEXT("Engine needs %d credits; have %d."), Cost, Garage.Credits());
        return;
    }
    if (!GaragePurchaseConfirm.ConfirmOrStage(
        latebrake::GaragePurchaseKind::UpgradeEngine,
        Garage.Snapshot(), Garage.EngineLevel(), Cost, FPlatformTime::Seconds()))
    {
        GarageFeedback = FString::Printf(
            TEXT("ENGINE LEVEL %d: %d credits. Press U again within 8s to confirm."),
            Garage.EngineLevel() + 1, Cost);
        return;
    }
    latebrake::ProfileWriteCheckpoint SavedProfile(
        Garage, Career, CompletedRaceCount, PersonalBestSeconds,
        PersonalBestLapSeconds, BestTimeAttackSeconds);
    if (Garage.Upgrade(latebrake::UpgradeKind::Engine))
    {
        GarageFeedback = FString::Printf(TEXT("Engine upgraded to level %d."), Garage.EngineLevel());
        ApplyGarageToPlayer();
        SaveRecords();
        if (bProgressSaveFailed)
        {
            SavedProfile.Rollback();
            ApplyGarageToPlayer();
            GarageFeedback = TEXT("Engine upgrade not saved. Credits and tuning restored.");
        }
        else
        {
            SavedProfile.Commit();
        }
    }
    else
    {
        GarageFeedback = Cost <= 0 ? TEXT("Engine is already fully upgraded.")
            : FString::Printf(TEXT("Engine needs %d credits; have %d."), Cost, Garage.Credits());
    }
}

void ALatebrakeRaceGameMode::GarageUpgradeGrip()
{
    if (RejectGarageMutationIfBlocked()) return;
    DismissGaragePalette();
    if (!latebrake::CanEditGarageDuringRace(bCareerRace, Race))
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = TEXT("Finish or exit this career race before upgrading grip.");
        return;
    }
    // A Garage-page pause is mandatory before any two-step purchase.
    // Closed-menu keyboard input must not stage a hidden transaction.
    if (!Menu.IsOpen() || Menu.Current() != latebrake::MenuPage::Garage)
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = TEXT("Open Garage page to preview a purchase.");
        return;
    }
    const int32 Cost = Garage.NextUpgradeCost(latebrake::UpgradeKind::Grip);
    if (Cost <= 0 || Garage.Credits() < Cost)
    {
        GaragePurchaseConfirm.Reset();
        GarageFeedback = Cost <= 0 ? TEXT("Grip is already fully upgraded.")
            : FString::Printf(TEXT("Grip needs %d credits; have %d."), Cost, Garage.Credits());
        return;
    }
    if (!GaragePurchaseConfirm.ConfirmOrStage(
        latebrake::GaragePurchaseKind::UpgradeGrip,
        Garage.Snapshot(), Garage.GripLevel(), Cost, FPlatformTime::Seconds()))
    {
        GarageFeedback = FString::Printf(
            TEXT("GRIP LEVEL %d: %d credits. Press I again within 8s to confirm."),
            Garage.GripLevel() + 1, Cost);
        return;
    }
    latebrake::ProfileWriteCheckpoint SavedProfile(
        Garage, Career, CompletedRaceCount, PersonalBestSeconds,
        PersonalBestLapSeconds, BestTimeAttackSeconds);
    if (Garage.Upgrade(latebrake::UpgradeKind::Grip))
    {
        GarageFeedback = FString::Printf(TEXT("Grip upgraded to level %d."), Garage.GripLevel());
        ApplyGarageToPlayer();
        SaveRecords();
        if (bProgressSaveFailed)
        {
            SavedProfile.Rollback();
            ApplyGarageToPlayer();
            GarageFeedback = TEXT("Grip upgrade not saved. Credits and tuning restored.");
        }
        else
        {
            SavedProfile.Commit();
        }
    }
    else
    {
        GarageFeedback = Cost <= 0 ? TEXT("Grip is already fully upgraded.")
            : FString::Printf(TEXT("Grip needs %d credits; have %d."), Cost, Garage.Credits());
    }
}

void ALatebrakeRaceGameMode::RefreshRival()
{
    const bool bNeedsRival = latebrake::GetRaceEvent(SelectedEvent).SpawnRival;
    if (!IsExperimentalFleetRequested()) DestroyExtraFleet();
    if (!bNeedsRival)
    {
        if (IsValid(RivalCar))
        {
            RivalCar->Destroy();
            RivalCar = nullptr;
        }
        return;
    }

    if (!IsValid(RivalCar))
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        RivalCar = GetWorld()->SpawnActor<ALatebrakeRivalCar>(
            ALatebrakeRivalCar::StaticClass(), FVector(latebrake::RivalStart.Position.X,
                    latebrake::RivalStart.Position.Y, 180.0),
            FRotator::ZeroRotator, Params);
        if (!IsValid(RivalCar))
        {
            UE_LOG(LogTemp, Warning, TEXT("Latebrake: starter AI rival failed to spawn"));
        }
    }
    else
    {
        RivalCar->RecoverTo(FVector(latebrake::RivalStart.Position.X,
                    latebrake::RivalStart.Position.Y, 180.0), FRotator::ZeroRotator);
    }
    if (IsValid(RivalCar))
    {
        // Career events can require one, two, three, four or more laps.
        // Reusing the quick-race default of three laps makes the rival
        // route-completion HUD incorrect for those races.
        RivalCar->ResetDrivingStats();
        RivalCar->ConfigurePilot(Race.GetCheckpointCount(), Race.GetRequiredLaps());
    }
}

void ALatebrakeRaceGameMode::RestartRace()
{
    GaragePurchaseConfirm.Reset();
    DismissGaragePalette(); // Race transitions invalidate pending spend approvals.
    if (bCareerRace && Race.IsFinished())
    {
        // After a career finish the garage is editable. A new attempt
        // needs a NEW starting-car identity; vehicle-required challenges
        // must equip their owned required car *before* resetting the clock.
        // A failed SaveGame write must leave the finished event and current
        // garage/credits intact, including while the results menu is open.
        latebrake::ProfileWriteCheckpoint ReplayWrite(
            Garage, Career, CompletedRaceCount, PersonalBestSeconds,
            PersonalBestLapSeconds, BestTimeAttackSeconds);
        const latebrake::CareerReplayPreparation Replay =
            latebrake::StageCareerReplay(Career, Garage);
        if (Replay.Status != latebrake::CareerLaunchStatus::Ready)
        {
            CareerFeedback = Replay.Status ==
                latebrake::CareerLaunchStatus::RequiredVehicleNotOwned
                ? TEXT("Replay rejected: required vehicle not owned.")
                : TEXT("Replay rejected: career event unavailable.");
            UE_LOG(LogTemp, Warning,
                TEXT("Latebrake: refusing invalid completed career replay"));
            return;
        }
        if (Replay.NeedsSave)
        {
            SaveRecords();
            if (bProgressSaveFailed)
            {
                ReplayWrite.Rollback();
                CareerFeedback = TEXT("Replay NOT started: required vehicle selection could not be saved.");
                GarageFeedback = TEXT("WARNING: replay selection save failed; previous car restored.");
                UE_LOG(LogTemp, Warning,
                    TEXT("Latebrake: career replay denied because required vehicle selection write failed"));
                return;
            }
        }
        ReplayWrite.Commit();
        CareerStartedVehicleId = Replay.StartingVehicle;
    }

    // The new start position is not authoritative until Unreal confirms
    // the actual chassis placement. Invalidate the previous race's
    // authorization BEFORE any fallible pawn movement or world unpause.
    RestartPlacement.OnRestartRequested();
    VerifiedRestartPawn.Reset();

    // Only unpause and reset AFTER career replay preparation succeeds.
    // Restart invalidates sampled positions even if no pawn exists yet or
    // its physics recovery is not observed until the next actor tick.
    CheckpointSession.OnRaceRestart();
    PracticeHistory.CancelAttempt();
    PracticeGateHistory.CancelAttempt();
    BeginExperimentalFleetAttempt();
    // Restarting from the mobile PAUSE screen must return to a live race.
    if (UGameplayStatics::IsGamePaused(this))
    {
        UGameplayStatics::SetGamePaused(this, false);
    }
    AppFocusGuard.OnExplicitResume(); // An accepted restart is intentional gameplay.
    ManualPause.OnExplicitResume();
    Race.Reset();
    ResetPlayerSectorMotion();
    TrackLimits.Reset();
    StewardTrackedPawn.Reset();
    StewardTeleportRevision=0;
    StewardCheckpointSessionRevision=0;
    // A saved PB may change at finish; Results needs the previous record.
    AttemptPreviousBestSeconds = GetEventBestSeconds();
    StartLights.Reset();
    UpdateStartGantryVisual();
    // Only a new, accepted race attempt latches the pending preset.
    // Career medals/challenges always use the existing Standard opponent.
    ActiveRivalDifficulty = latebrake::EffectiveRivalDifficulty(
        QuickRaceDifficultySelection, bCareerRace || IsPracticeRace() ||
            SelectedEvent == latebrake::EventKind::TimeAttack);
    RivalSplits.Configure(Race.GetCheckpointCount(), Race.GetRequiredLaps());
    RivalFinishClocks.Configure(Race.GetCheckpointCount(), Race.GetRequiredLaps());
    LastRivalFinishClockComparison = {};
    LastRivalFinishObservation = {};
    LaunchMeter.Reset();
    GhostRecorder.ResetRun();
    GhostSetup.Reset();
    GhostTrackedPawn.Reset();
    GhostTeleportRevision = 0;
    GhostSessionRevision = CheckpointSession.Current();
    if (IsValid(BestLapGhostMarker))
    {
        BestLapGhostMarker->SetActorHiddenInGame(true);
    }
    LastCareerAward = {};
    ApplyGarageToPlayer();
    LastRaceCredits = 0;
    RecoveriesThisRace = 0;
    RecoveryNotice.Reset();
    ALatebrakeCarPawn* Car = Cast<ALatebrakeCarPawn>(
        UGameplayStatics::GetPlayerPawn(this, 0));
    if (IsValid(Car))
    {
        Car->ResetDrivingStats();
    }
    const bool bPlacedOnGrid =
        IsValid(Car) && Car->TryRecoverTo(GetStartPosition(), GetStartRotation());
    RestartPlacement.OnRestartPlacementResult(bPlacedOnGrid);
    VerifiedRestartPawn = bPlacedOnGrid ? Car : nullptr;
    if (bPlacedOnGrid && IsPracticeRace())
    {
        const auto PracticeKey=ReadPracticeConfiguration(Car);
        const auto PracticeActor=static_cast<std::uint64_t>(Car->GetUniqueID())+1u;
        PracticeHistory.Begin(PracticeKey,PracticeActor);
        PracticeGateHistory.Begin(PracticeKey,PracticeActor);
    }
    if (!bPlacedOnGrid)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: RESTART_GRID_UNVERIFIED - race clock, gates and driving blocked"));
    }
    UpdateStartGantryVisual();
#if !UE_BUILD_SHIPPING
    StartRaceAuditAttempt(bPlacedOnGrid ? 1 : 0);
#endif
    if (IsValid(RivalCar))
    {
        RivalCar->RecoverTo(FVector(latebrake::RivalStart.Position.X,
                    latebrake::RivalStart.Position.Y, 180.0), FRotator::ZeroRotator);
        RivalCar->ResetDrivingStats();
        RivalCar->ResetPilot();
    }
    // The direct keyboard/gamepad/mobile RESTART binding does not go through
    // MenuActivate(). Never leave an invisible open menu blocking race timing
    // after an accepted restart has already unpaused the world.
    if (Menu.IsOpen())
    {
        CloseMenuForRace();
    }
}

void ALatebrakeRaceGameMode::RecoverPlayer(ALatebrakeCarPawn* Car)
{
    // Only the possessed local player may request a paid race recovery.
    // A stale pawn or a rival is not allowed to charge the race clock.
    if (!IsValid(Car) || Car != UGameplayStatics::GetPlayerPawn(this, 0) ||
        !IsRaceStartGreen())
    {
        return; // No paid recovery or circuit jumps before GO.
    }

    // Keep the same driver/clock authority as checkpoints. No recovery
    // notice or candidate can authorize movement during a paused race.
    const bool bInactive = UGameplayStatics::IsGamePaused(this) || Menu.IsOpen() ||
        AppFocusGuard.IsSuspended() || ManualPause.IsSuspended();
    const bool bDriverReady=HasLocalPlayerDriver();
    if (!latebrake::CanScorePlayerCheckpoint(Race,bCircuitReady,bInactive,bDriverReady))
        return;

    // Time Attack is explicitly solo. A missing expected rival, an
    // unexpected actor, or an invalid physical observation is not proof
    // of empty traffic in the original single-rival circuit.
    const bool bExpectsRival=latebrake::GetRaceEvent(SelectedEvent).SpawnRival;
    const bool bObservedRival=IsValid(RivalCar);
    latebrake::RecoveryObstacle Other{};
    if (bObservedRival && RivalCar.Get()!=Car && RivalCar->IsRacePhysicsReady())
    {
        const FVector Position=RivalCar->GetActorLocation();
        const FVector Velocity=RivalCar->GetVelocity();
        Other={true,latebrake::PrimaryRivalGridSlot,
            static_cast<std::uint64_t>(RivalCar->GetUniqueID())+1u,
            {{Position.X,Position.Y},RivalCar->GetActorRotation().Yaw},
            Velocity.X,Velocity.Y};
    }
    auto Clearance=latebrake::EvaluatePlayerRecoveryClearance(
        Race.GetNextCheckpoint(),bExpectsRival,bObservedRival,Other);
    if (IsExperimentalFleetRequested())
    {
        latebrake::FleetPhysicalFrame Fresh;
        CollectFleetFrame(Fresh,0.0,false);
        Clearance=latebrake::SelectFleetPlayerRecovery(Race.GetNextCheckpoint(),
            Fresh,FleetAttempt.BoundIdentity(),
            static_cast<std::uint64_t>(Car->GetUniqueID())+1u);
    }
    // TryPaidRecoveryWithClearance composes the existing staged
    // TryChargePlayerRecoveryAfterVerifiedTeleport transaction with this
    // preflight. It never calls the engine when the landing is blocked.
    const auto Notice=latebrake::TryPaidRecoveryWithClearance(
        Race,RecoveriesThisRace,bCircuitReady,bInactive,bDriverReady,Clearance,
        [Car](latebrake::CircuitPose Pose) {
            const FVector Target(Pose.Position.X, Pose.Position.Y, 180.0);
            const FRotator Facing(0.0, Pose.HeadingDegrees, 0.0);
            return Car->TryRecoverTo(Target, Facing);
        });
    RecoveryNotice.Record(Notice,Race.GetElapsedSeconds());
    if (Notice!=latebrake::PlayerRecoveryNotice::VerifiedAndCharged)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: recovery rejected (notice %d); no fee charged"),
            static_cast<int32>(Notice));
        return;
    }
#if !UE_BUILD_SHIPPING
    if (IsExperimentalFleetRequested())
        UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_FLEET_AUDIT v=1 action=recovery attempt=%llu slot=0 id=%llu ordinal=%d verified=1"),
            static_cast<unsigned long long>(FleetAttemptToken),
            static_cast<unsigned long long>(FleetAttempt.BoundIdentity()[0]),RecoveriesThisRace);
#endif
    // A real paid teleport cannot be mistaken for a grid launch. A rejected
    // recovery above returns without erasing an otherwise valid sample.
    TrackLimits.Suspend();
    SuspendBestLapGhost();
    LaunchMeter.Invalidate();
}

void ALatebrakeRaceGameMode::LoadRecords()
{
    if (const ULatebrakePlayerSave* Save = Cast<ULatebrakePlayerSave>(
            UGameplayStatics::LoadGameFromSlot(SaveSlot, SaveUser)))
    {
        PersonalBestSeconds = latebrake::SanitizeBestTime(Save->BestRaceSeconds);
        PersonalBestLapSeconds = latebrake::SanitizeBestTime(Save->BestLapSeconds);
        BestTimeAttackSeconds = latebrake::SanitizeBestTime(Save->BestTimeAttackSeconds);
        CompletedRaceCount = latebrake::SanitizeCompletedRaces(Save->CompletedRaces);
        bMilesPerHour = Save->bSpeedInMilesPerHour;
        // Old save slots have zero for fields added after v0.2.x.
        // Garage.Load repairs the owned mask and preserves the starter car.
        Garage.Load({Save->TotalCredits, Save->OwnedVehicleMask,
                     Save->SelectedVehicleId, Save->EngineUpgradeLevel,
                     Save->GripUpgradeLevel});
        GarageGallery.Reset(Garage.SelectedVehicle());
        if (Save->GarageAppearanceSchemaVersion >
            latebrake::GarageAppearanceState::SaveVersion)
        {
            // A future appearance format may contain unknown information.
            // Do not write older saves over that newer person's choices.
            bFutureSaveSchema = true;
            bProgressSaveFailed = true;
            UE_LOG(LogTemp, Warning,
                TEXT("Latebrake: newer garage appearance schema - profile is read-only"));
        }
        else if (Save->GarageAppearanceSchemaVersion ==
                 latebrake::GarageAppearanceState::SaveVersion)
        {
            latebrake::GarageAppearanceState::Packed packed{};
            bool bValid = Save->GarageAppearanceBytes.Num() ==
                static_cast<int32>(packed.size());
            if (bValid)
            {
                for (int32 i = 0; i < static_cast<int32>(packed.size()); ++i)
                    packed[static_cast<std::size_t>(i)] =
                        Save->GarageAppearanceBytes[i];
                bValid = Save->GarageAppearanceChecksum ==
                    latebrake::GarageAppearanceState::Checksum(packed) &&
                    GarageLooks.Load(packed.data(), packed.size());
            }
            if (!bValid)
            {
                GarageLooks.Reset();
                UE_LOG(LogTemp, Warning,
                    TEXT("Latebrake: garage appearance invalid; factory cosmetics restored"));
            }
        }

        if (Save->SaveSchemaVersion > latebrake::CareerSaveSchema)
        {
            // Never overwrite an unknown future SaveGame format.
            bFutureSaveSchema = true;
            bProgressSaveFailed = true;
            UE_LOG(LogTemp, Warning, TEXT("Latebrake: future save schema - writes disabled"));
        }
        else if (Save->SaveSchemaVersion == latebrake::CareerSaveSchema &&
                 Save->CareerMedals.Num() == latebrake::CareerEventCount)
        {
            latebrake::CareerSnapshot Snapshot;
            Snapshot.SelectedEvent = Save->ActiveCareerEventId;
            for (int32 Id = 0; Id < latebrake::CareerEventCount; ++Id)
            {
                Snapshot.Medals[static_cast<std::size_t>(Id)] = Save->CareerMedals[Id];
            }
            Career.Load(Snapshot);
        }
        // A legacy/partially populated career save starts fresh without
        // resetting any of the existing times, garage or credit balance.
        CareerMenuSelection = Career.SelectedEvent() >= 0
            ? Career.SelectedEvent() : Career.NextAvailable();
        if (CareerMenuSelection < 0) CareerMenuSelection = 0;
    }
}

void ALatebrakeRaceGameMode::SaveRecords()
{
    if (bFutureSaveSchema)
    {
        bProgressSaveFailed = true;
        UE_LOG(LogTemp, Warning, TEXT("Latebrake: refusing to overwrite future save schema"));
        return;
    }
    // The profile may have changed since BeginPlay (for example, a newer
    // build was opened separately). Never write an old schema over an
    // on-disk profile we can no longer understand or even deserialize.
    // This is a best-effort preflight, NOT an interprocess atomic lock:
    // two concurrent game instances must still be avoided.
    const bool bSlotPresent =
        UGameplayStatics::DoesSaveGameExist(SaveSlot, SaveUser);
    const ULatebrakePlayerSave* DiskProfile = bSlotPresent
        ? Cast<ULatebrakePlayerSave>(
            UGameplayStatics::LoadGameFromSlot(SaveSlot, SaveUser))
        : nullptr;
    const latebrake::ProfileSaveWriteState DiskState =
        latebrake::AssessProfileSaveWrite(
            bSlotPresent, DiskProfile != nullptr,
            DiskProfile ? DiskProfile->SaveSchemaVersion : 0,
            DiskProfile ? DiskProfile->GarageAppearanceSchemaVersion : 0);
    if (!latebrake::CanWriteProfileSave(DiskState))
    {
        bProgressSaveFailed = true;
        if (DiskState == latebrake::ProfileSaveWriteState::NewerCareer ||
            DiskState == latebrake::ProfileSaveWriteState::NewerAppearance)
        {
            bFutureSaveSchema = true;
        }
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: refusing SaveGame overwrite of a newer, corrupt or unreadable on-disk profile (%d)"),
            static_cast<int32>(DiskState));
        return;
    }
    ULatebrakePlayerSave* Save = Cast<ULatebrakePlayerSave>(
        UGameplayStatics::CreateSaveGameObject(ULatebrakePlayerSave::StaticClass()));
    if (!Save)
    {
        bProgressSaveFailed = true;
        UE_LOG(LogTemp, Warning, TEXT("Latebrake: unable to create SaveGame object"));
        return;
    }
    // Defense in depth: never serialize a nonfinite or unrecordable time,
    // even if a future caller introduces invalid in-memory record state.
    Save->BestRaceSeconds = latebrake::SanitizeBestTime(PersonalBestSeconds);
    Save->BestLapSeconds = latebrake::SanitizeBestTime(PersonalBestLapSeconds);
    Save->BestTimeAttackSeconds = latebrake::SanitizeBestTime(BestTimeAttackSeconds);
    Save->CompletedRaces = CompletedRaceCount;
    const latebrake::GarageSnapshot Profile = Garage.Snapshot();
    Save->TotalCredits = Profile.Credits;
    Save->OwnedVehicleMask = Profile.OwnedVehicleMask;
    Save->SelectedVehicleId = Profile.SelectedVehicle;
    Save->EngineUpgradeLevel = Profile.EngineLevel;
    Save->GripUpgradeLevel = Profile.GripLevel;
    // Visual data is additional to the existing career schema: no car ID,
    // economy, results or medal fields are repurposed.
    const auto packedLooks = GarageLooks.Serialize();
    Save->GarageAppearanceSchemaVersion =
        latebrake::GarageAppearanceState::SaveVersion;
    Save->GarageAppearanceBytes.SetNumUninitialized(
        static_cast<int32>(packedLooks.size()));
    for (int32 i = 0; i < static_cast<int32>(packedLooks.size()); ++i)
        Save->GarageAppearanceBytes[i] =
            packedLooks[static_cast<std::size_t>(i)];
    Save->GarageAppearanceChecksum =
        latebrake::GarageAppearanceState::Checksum(packedLooks);
    const latebrake::CareerSnapshot CareerData = Career.Snapshot();
    Save->SaveSchemaVersion = latebrake::CareerSaveSchema;
    Save->bSpeedInMilesPerHour = bMilesPerHour;
    Save->ActiveCareerEventId = CareerData.SelectedEvent;
    Save->CareerMedals.SetNum(latebrake::CareerEventCount);
    for (int32 Id = 0; Id < latebrake::CareerEventCount; ++Id)
    {
        Save->CareerMedals[Id] = CareerData.Medals[static_cast<std::size_t>(Id)];
    }
    bProgressSaveFailed = !UGameplayStatics::SaveGameToSlot(Save, SaveSlot, SaveUser);
    if (bProgressSaveFailed)
    {
        UE_LOG(LogTemp, Warning, TEXT("Latebrake: progress could not be saved; verify disk permissions"));
    }
}
