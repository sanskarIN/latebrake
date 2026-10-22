#include "Game/LatebrakeRaceGameMode.h"
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
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Progression/LatebrakePlayerSave.h"
#include "Progression/ProfileWriteCheckpoint.h"
#include "Progression/RecordValidation.h"
#include "Career/CareerLaunchSelection.h"
#include "Career/CareerReplayPreparation.h"
#include "Garage/CareerGaragePolicy.h"
#include "Racing/LatebrakeCheckpoint.h"
#include "Racing/RaceRewards.h"
#include "Racing/RecoveryCharge.h"
#include "Track/CircuitLayout.h"
#include "Track/TrackDecorations.h"
#include "Track/DecorationBatchPlan.h"
#include "Track/CircuitStartupAudit.h"
#include "Track/CircuitCollisionPlan.h"
#include "Track/CollisionBlockSetup.h"
#include "Track/CollisionResponsePolicy.h"
#include "Track/CircuitSpawnSafety.h"
#include <limits>
#include "UI/LatebrakeRaceHUD.h"
#include "Vehicles/LatebrakeCarPawn.h"
#include "AI/LatebrakeRivalCar.h"

DEFINE_LOG_CATEGORY_STATIC(LogLatebrakeStartup, Log, All);

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

void ALatebrakeRaceGameMode::BeginPlay()
{
    Super::BeginPlay();
    LoadRecords();
    BuildCircuit();
    Race.Reset();
    RefreshRival();
}

void ALatebrakeRaceGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Race.Advance(static_cast<double>(DeltaSeconds));
    ApplyGarageToPlayer();
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

void ALatebrakeRaceGameMode::BuildCircuit()
{
    latebrake::CircuitStartupAudit Audit;
    bCircuitReady = false;
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
    if (ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>())
    {
        Sun->SetActorRotation(FRotator(-55.f, -30.f, 0.f));
        Sun->GetLightComponent()->SetIntensity(6.f);
    }
    if (ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>())
    {
        Sky->GetLightComponent()->SetIntensity(1.f);
    }

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
        RecordBlock(Labels[Index],
            FVector(Block.Center.X, Block.Center.Y, Block.Height),
            FVector(Block.ScaleX, Block.ScaleY, Block.ScaleZ), Color);
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
    if (!bCircuitReady)
    {
        return;
    }
    // RaceProgress only flips to Finished on a valid ordered player gate.
    // Once finished all subsequent overlap callbacks return false.
    if (!Race.PassCheckpoint(CheckpointIndex) || !Race.IsFinished())
    {
        return;
    }

    // The SaveGame schema records best times as floats with an existing
    // seven-day bound. Do not grant career medals, completion counts or
    // quick-race credits for a finish which this schema cannot represent
    // reliably, e.g. elapsed doubles outside that bound or a zero lap.
    if (!latebrake::IsValidFinishedRaceTime(Race.GetElapsedSeconds()) ||
        !latebrake::IsValidFinishedRaceTime(Race.GetBestLapSeconds()))
    {
        LastCareerAward = {};
        LastRaceCredits = 0;
        CareerFeedback = TEXT("Finish rejected: race/lap clock is outside supported record range. Restart to retry.");
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: refusing non-recordable finish times; no rewards, records or SaveGame write"));
        return;
    }

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

void ALatebrakeRaceGameMode::ToggleMenu()
{
    Menu.Toggle();
    // Always neutralize held inputs before changing input focus.
    if (ALatebrakeCarPawn* Car =
        Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Car->ClearDriverInputs();
    }
    UGameplayStatics::SetGamePaused(this, Menu.IsOpen());
}

void ALatebrakeRaceGameMode::MenuNext()
{
    Menu.Next();
}

void ALatebrakeRaceGameMode::MenuPrevious()
{
    Menu.Previous();
}

void ALatebrakeRaceGameMode::MenuActivate()
{
    if (!Menu.IsOpen()) return;
    const latebrake::MenuPage Page = Menu.Current();
    if (Page == latebrake::MenuPage::Main)
    {
        Menu.Open(latebrake::MenuPage::QuickRace);
    }
    else if (Page == latebrake::MenuPage::QuickRace)
    {
        Menu.Close();
        UGameplayStatics::SetGamePaused(this, false);
        SelectRaceEvent(latebrake::EventKind::Circuit);
        RestartRace();
    }
    else if (Page == latebrake::MenuPage::Career)
    {
        if (StartCareerEvent(CareerMenuSelection)) Menu.Close();
    }
    else if (Page == latebrake::MenuPage::Championships)
    {
        int32 Target = CareerMenuSelection >= 0 && CareerMenuSelection < 6 &&
            Career.Status(CareerMenuSelection) != latebrake::CareerStatus::Locked
            ? CareerMenuSelection : -1;
        for (int32 Id = 0; Id < 6 && Target < 0; ++Id)
        {
            if (Career.Status(Id) == latebrake::CareerStatus::Available)
            {
                Target = Id;
                break;
            }
        }
        if (Target < 0) Target = 0; // Replay an already accessible round.
        if (StartCareerEvent(Target)) Menu.Close();
    }
    else if (Page == latebrake::MenuPage::Challenges)
    {
        int32 Target = CareerMenuSelection >= 6 &&
            CareerMenuSelection < latebrake::CareerEventCount &&
            Career.Status(CareerMenuSelection) != latebrake::CareerStatus::Locked
            ? CareerMenuSelection : -1;
        for (int32 Id = 6; Id < latebrake::CareerEventCount && Target < 0; ++Id)
        {
            if (Career.Status(Id) == latebrake::CareerStatus::Available)
            {
                Target = Id;
                break;
            }
        }
        if (Target < 0) Target = 6;
        if (StartCareerEvent(Target)) Menu.Close();
    }
    else if (Page == latebrake::MenuPage::Garage)
    {
        GarageNextVehicle();
    }
    else if (Page == latebrake::MenuPage::Results)
    {
        // A replay may be refused if a required-car selection cannot be
        // persisted. Keep RESULTS open (and paused) if that happens.
        RestartRace();
        if (!Race.IsFinished())
        {
            Menu.Close();
        }
    }
    else if (Page == latebrake::MenuPage::Pause)
    {
        ToggleMenu();
    }
    else if (Page == latebrake::MenuPage::Settings)
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
    }
    // Profile is deliberately informational; no money or unlock is created.
}

void ALatebrakeRaceGameMode::SelectRaceEvent(latebrake::EventKind Kind)
{
    if (!latebrake::IsValidEvent(Kind) || (Kind == SelectedEvent && !bCareerRace))
    {
        return;
    }
    bCareerRace = false;
    LastCareerAward = {};
    SelectedEvent = Kind;
    const latebrake::RaceEvent Definition = latebrake::GetRaceEvent(SelectedEvent);
    Race = latebrake::RaceProgress(Definition.CheckpointCount, Definition.RequiredLaps);
    RestartRace();
    RefreshRival();
}

FString ALatebrakeRaceGameMode::GetCareerEventName() const
{
    const int32 Id = (Menu.IsOpen() || !bCareerRace)
        ? CareerMenuSelection : Career.SelectedEvent();
    const latebrake::CareerEvent* Event = latebrake::GetCareerEvent(Id);
    return Event ? FString(UTF8_TO_TCHAR(Event->Name)) : TEXT("No Career Event");
}

void ALatebrakeRaceGameMode::CycleCareerEvent()
{
    const int32 Original = CareerMenuSelection;
    for (int32 Delta = 1; Delta <= latebrake::CareerEventCount; ++Delta)
    {
        const int32 Id = (Original + Delta) % latebrake::CareerEventCount;
        if (Menu.IsOpen() &&
            Menu.Current() == latebrake::MenuPage::Championships && Id >= 6) continue;
        if (Menu.IsOpen() &&
            Menu.Current() == latebrake::MenuPage::Challenges && Id < 6) continue;
        if (Career.Status(Id) != latebrake::CareerStatus::Locked)
        {
            CareerMenuSelection = Id;
            const latebrake::CareerEvent* Event = latebrake::GetCareerEvent(Id);
            CareerFeedback = FString::Printf(TEXT("Career selected: %s (%d/%d). M start."),
                UTF8_TO_TCHAR(Event->Name), Id + 1, latebrake::CareerEventCount);
            return;
        }
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
    return SelectedEvent == latebrake::EventKind::TimeAttack ? BestTimeAttackSeconds : PersonalBestSeconds;
}

bool ALatebrakeRaceGameMode::HasRival() const
{
    return IsValid(RivalCar);
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

bool ALatebrakeRaceGameMode::HasRivalFinishedRoute() const
{
    return IsValid(RivalCar) && RivalCar->HasCompletedRoute();
}

FString ALatebrakeRaceGameMode::GetSelectedVehicleName() const
{
    return FString(UTF8_TO_TCHAR(latebrake::GarageState::Definition(Garage.SelectedVehicle()).Name));
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
    const latebrake::GarageTuningKey Requested = latebrake::MakeGarageTuningKey(Garage.Snapshot());
    if (LastTunedPawn.Get() == Car && Requested == LastAppliedTuning)
    {
        // Credits and garage ownership alone do not affect live handling.
        return;
    }
    const latebrake::VehiclePerformance Performance = Garage.Performance();
    Car->SetGarageTuning(
        static_cast<float>(Performance.Acceleration),
        static_cast<float>(Performance.Grip),
        static_cast<float>(Performance.TopSpeed),
        Garage.SelectedVehicle());
    LastTunedPawn = Car;
    LastAppliedTuning = Requested;
}

void ALatebrakeRaceGameMode::GarageNextVehicle()
{
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
    latebrake::ProfileWriteCheckpoint SavedProfile(
        Garage, Career, CompletedRaceCount, PersonalBestSeconds,
        PersonalBestLapSeconds, BestTimeAttackSeconds);
    if (Garage.Select(Next))
    {
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
    }
}

void ALatebrakeRaceGameMode::GarageUnlockNext()
{
    if (!latebrake::CanEditGarageDuringRace(bCareerRace, Race))
    {
        GarageFeedback = TEXT("Finish or exit this career race before buying vehicles.");
        return;
    }
    const int32 Next = Garage.NextLockedVehicle();
    if (Next < 0)
    {
        GarageFeedback = TEXT("Every prototype profile is already unlocked.");
        return;
    }
    const int32 Cost = latebrake::GarageState::Definition(Next).UnlockCost;
    if (!Garage.CanUnlock(Next))
    {
        GarageFeedback = FString::Printf(TEXT("Need %d credits; currently %d."), Cost, Garage.Credits());
        return;
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
    if (!latebrake::CanEditGarageDuringRace(bCareerRace, Race))
    {
        GarageFeedback = TEXT("Finish or exit this career race before upgrading engine.");
        return;
    }
    const int32 Cost = Garage.NextUpgradeCost(latebrake::UpgradeKind::Engine);
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
    if (!latebrake::CanEditGarageDuringRace(bCareerRace, Race))
    {
        GarageFeedback = TEXT("Finish or exit this career race before upgrading grip.");
        return;
    }
    const int32 Cost = Garage.NextUpgradeCost(latebrake::UpgradeKind::Grip);
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
        RivalCar->ConfigurePilot(Race.GetCheckpointCount(), Race.GetRequiredLaps());
    }
}

void ALatebrakeRaceGameMode::RestartRace()
{
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

    // Only unpause and reset AFTER career replay preparation succeeds.
    // Restarting from the mobile PAUSE screen must return to a live race.
    if (UGameplayStatics::IsGamePaused(this))
    {
        UGameplayStatics::SetGamePaused(this, false);
    }
    Race.Reset();
    LastCareerAward = {};
    ApplyGarageToPlayer();
    LastRaceCredits = 0;
    RecoveriesThisRace = 0;
    if (ALatebrakeCarPawn* Car = Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Car->RecoverTo(GetStartPosition(), GetStartRotation());
    }
    if (IsValid(RivalCar))
    {
        RivalCar->RecoverTo(FVector(latebrake::RivalStart.Position.X,
                    latebrake::RivalStart.Position.Y, 180.0), FRotator::ZeroRotator);
        RivalCar->ResetPilot();
    }
}

void ALatebrakeRaceGameMode::RecoverPlayer(ALatebrakeCarPawn* Car)
{
    if (!IsValid(Car) || Car != UGameplayStatics::GetPlayerPawn(this, 0) ||
        Race.IsFinished())
    {
        return;
    }

    // Player recovery and AI recovery consume the same course definition.
    const latebrake::CircuitPose Pose =
        latebrake::PlayerRecovery(Race.GetNextCheckpoint());
    // Fail closed: never teleport if the recovery counter or race timer
    // cannot accept the full penalty. This also prevents uncharged free
    // repositioning after a numerical overflow or saturated recovery count.
    if (!latebrake::TryChargePlayerRecovery(Race, RecoveriesThisRace))
    {
        UE_LOG(LogTemp, Warning, TEXT("Latebrake: recovery rejected; penalty could not be charged"));
        return;
    }
    Car->RecoverTo(
        FVector(Pose.Position.X, Pose.Position.Y, 180.0),
        FRotator(0.0, Pose.HeadingDegrees, 0.0));
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
