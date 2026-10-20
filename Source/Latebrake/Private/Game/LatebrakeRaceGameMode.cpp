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
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Progression/LatebrakePlayerSave.h"
#include "Progression/ProfileWriteCheckpoint.h"
#include "Progression/RecordValidation.h"
#include "Racing/LatebrakeCheckpoint.h"
#include "Racing/RaceRewards.h"
#include "Track/CircuitLayout.h"
#include "Track/TrackDecorations.h"
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

AStaticMeshActor* ALatebrakeRaceGameMode::SpawnBlock(const TCHAR* Label, const FVector& Position,
    const FVector& Scale, const FLinearColor& Color)
{
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cube)
    {
        UE_LOG(LogTemp, Error, TEXT("Latebrake: built-in Cube mesh not found, cannot build track"));
        return nullptr;
    }

    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), Position, FRotator::ZeroRotator, Parameters);
    if (!Block)
    {
        return nullptr;
    }

#if WITH_EDITOR
    Block->SetActorLabel(Label);
#else
    (void)Label;
#endif
    UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(Cube);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Block->SetActorScale3D(Scale);

    // BasicShapeMaterial is part of the editor's engine content; do not copy it.
    if (UMaterialInstanceDynamic* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), Color);
    }

    return Block;
}

void ALatebrakeRaceGameMode::BuildCircuit()
{
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

    // World-space units are centimetres. All geometry references stock engine
    // primitives; this repository does not ship or redistribute Epic assets.
    SpawnBlock(TEXT("Ground"), FVector(0.f, 0.f, -100.f),
        FVector(110.f, 85.f, 1.25f), FLinearColor(0.15f, 0.30f, 0.12f));

    const FLinearColor Asphalt(0.09f, 0.10f, 0.13f);
    SpawnBlock(TEXT("SouthStraight"), FVector(0.0, latebrake::SouthY, -22.0), FVector(80.f, 16.f, 0.42f), Asphalt);
    SpawnBlock(TEXT("NorthStraight"), FVector(0.0, latebrake::NorthY, -22.0), FVector(80.f, 16.f, 0.42f), Asphalt);
    SpawnBlock(TEXT("EastStraight"), FVector(latebrake::EastX, 0.0, -22.0), FVector(16.f, 52.f, 0.42f), Asphalt);
    SpawnBlock(TEXT("WestStraight"), FVector(latebrake::WestX, 0.0, -22.0), FVector(16.f, 52.f, 0.42f), Asphalt);

    const FLinearColor BarrierColor(0.80f, 0.15f, 0.05f);
    SpawnBlock(TEXT("NorthBarrier"), FVector(0.f, 2750.f, 65.f), FVector(85.f, 1.0f, 2.0f), BarrierColor);
    SpawnBlock(TEXT("SouthBarrier"), FVector(0.f, -2750.f, 65.f), FVector(85.f, 1.0f, 2.0f), BarrierColor);
    SpawnBlock(TEXT("EastBarrier"), FVector(3750.f, 0.f, 65.f), FVector(1.0f, 60.f, 2.0f), BarrierColor);
    SpawnBlock(TEXT("WestBarrier"), FVector(-3750.f, 0.f, 65.f), FVector(1.0f, 60.f, 2.0f), BarrierColor);

    // Track furnishings are lightweight Unreal built-in cubes with collision
    // disabled, so painted curbs/stripes never become invisible tire barriers.
    // The current source prototype still uses separate actors; a production
    // pass will batch these as instanced meshes to reduce mobile draw calls.
    const auto Decoration = [this](const TCHAR* Name, const FVector& Pos,
                                   const FVector& Dimensions, const FLinearColor& Color)
    {
        if (AStaticMeshActor* Actor = SpawnBlock(Name, Pos, Dimensions, Color))
        {
            Actor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    };
    const FLinearColor Stripe(0.94f, 0.94f, 0.89f);
    const FLinearColor CurbRed(0.83f, 0.11f, 0.09f);
    const FLinearColor CurbWhite(0.96f, 0.95f, 0.89f);
    for (int32 Straight = 0; Straight < latebrake::StraightCount; ++Straight)
    {
        for (int32 Segment = 0; Segment < latebrake::MarkersPerStraight; ++Segment)
        {
            const latebrake::TrackMarker Dash = latebrake::LaneStripe(Straight, Segment);
            if (Dash.Valid)
            {
                Decoration(TEXT("LaneStripe"),
                    FVector(Dash.Center.X, Dash.Center.Y, 4.0),
                    FVector(Dash.ScaleX, Dash.ScaleY, 0.025), Stripe);
            }
            for (int32 Edge = 0; Edge < 2; ++Edge)
            {
                const latebrake::TrackMarker Curb =
                    latebrake::TrackCurb(Straight, Segment, Edge != 0);
                if (Curb.Valid)
                {
                    Decoration(TEXT("Curbstone"),
                        FVector(Curb.Center.X, Curb.Center.Y, 6.0),
                        FVector(Curb.ScaleX, Curb.ScaleY, 0.08),
                        ((Segment + Edge) % 2 == 0) ? CurbRed : CurbWhite);
                }
            }
        }
    }

    // Primitive-only grandstand terraces outside the drivable asphalt.
    // Their presentation and clipping are still pending Unreal playtesting.
    for (int32 Stand = 0; Stand < latebrake::GrandstandCount; ++Stand)
    {
        const latebrake::CircuitPoint Center = latebrake::GrandstandCenter(Stand);
        Decoration(TEXT("GrandstandBase"),
            FVector(Center.X, Center.Y, 5.0),
            FVector(15.0, 5.5, 0.9), FLinearColor(0.18f, 0.21f, 0.26f));
        for (int32 Tier = 0; Tier < 3; ++Tier)
        {
            const double Direction = Stand == 0 ? 1.0 : -1.0;
            Decoration(TEXT("GrandstandTier"),
                FVector(Center.X, Center.Y + Direction * (Tier - 1) * 155.0,
                        62.0 + Tier * 32.0),
                FVector(13.0, 1.25, 0.48), 
                (Tier % 2 == 0) ? FLinearColor(0.35f, 0.43f, 0.60f)
                                : FLinearColor(0.26f, 0.31f, 0.42f));
        }
    }

    Decoration(TEXT("StartFinishBanner"),
        FVector(-2150.0, latebrake::SouthY, 390.0),
        FVector(8.0, 0.32, 1.5), FLinearColor(0.90f, 0.60f, 0.08f));

    int32 SpawnedCheckpoints = 0;
    for (int32 Index = 0; Index < latebrake::CircuitGateCount; ++Index)
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (ALatebrakeCheckpoint* Checkpoint = GetWorld()->SpawnActor<ALatebrakeCheckpoint>(
                ALatebrakeCheckpoint::StaticClass(),
                FVector(latebrake::CircuitGate(Index).X,
                        latebrake::CircuitGate(Index).Y, 38.0),
                FRotator::ZeroRotator, Parameters))
        {
            Checkpoint->SetCheckpointIndex(Index);
            ++SpawnedCheckpoints;
        }
    }
    // A real installed-package QA session can grep for this log marker.
    // It proves only that the GameMode reached BeginPlay and spawned all
    // checkpoint actors; it does NOT certify rendering, input or gameplay.
    if (SpawnedCheckpoints == latebrake::CircuitGateCount)
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
    // RaceProgress only flips to Finished on a valid ordered player gate.
    // Once finished all subsequent overlap callbacks return false.
    if (!Race.PassCheckpoint(CheckpointIndex) || !Race.IsFinished())
    {
        return;
    }

    // Only the SaveGame write decides if a finish becomes a durable reward.
    // In-memory first-completion medals, credits and best times must not
    // survive a failed disk write as misleading spendable progress.
    latebrake::ProfileWriteCheckpoint SavedProfile(
        Garage, Career, CompletedRaceCount, PersonalBestSeconds,
        PersonalBestLapSeconds, BestTimeAttackSeconds);
    CompletedRaceCount = latebrake::IncrementCompletedRaces(CompletedRaceCount);
    const float FinishedSeconds = static_cast<float>(Race.GetElapsedSeconds());
    if (!bCareerRace)
    {
        float& EventRecord = (SelectedEvent == latebrake::EventKind::TimeAttack)
            ? BestTimeAttackSeconds : PersonalBestSeconds;
        if (EventRecord <= 0.f || FinishedSeconds < EventRecord)
        {
            EventRecord = FinishedSeconds;
        }
    }
    const float FinishedBestLap = static_cast<float>(Race.GetBestLapSeconds());
    if (FinishedBestLap > 0.f &&
        (PersonalBestLapSeconds <= 0.f || FinishedBestLap < PersonalBestLapSeconds))
    {
        PersonalBestLapSeconds = FinishedBestLap;
    }

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
        Menu.Close();
        UGameplayStatics::SetGamePaused(this, false);
        RestartRace();
    }
    else if (Page == latebrake::MenuPage::Pause)
    {
        ToggleMenu();
    }
    else if (Page == latebrake::MenuPage::Settings)
    {
        bMilesPerHour = !bMilesPerHour;
        SaveRecords();
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
    if (!Event || Career.Status(Id) == latebrake::CareerStatus::Locked)
    {
        CareerFeedback = TEXT("Career event is locked or unknown.");
        return false;
    }
    if (Event->RequiredVehicle >= 0 && !Garage.Owns(Event->RequiredVehicle))
    {
        CareerFeedback = TEXT("Unlock the required vehicle in the garage first.");
        return false;
    }
    // A career race uses the real player checkpoint clock. Rival telemetry is
    // never interpreted as a competitive finish or championship placement.
    if (!Career.Select(Id))
    {
        CareerFeedback = TEXT("Career selection was rejected.");
        return false;
    }
    CareerMenuSelection = Id;
    if (Event->RequiredVehicle >= 0)
    {
        Garage.Select(Event->RequiredVehicle);
        ApplyGarageToPlayer();
    }
    CareerStartedVehicleId = Garage.SelectedVehicle();
    bCareerRace = true;
    SelectedEvent = Event->SpawnRival
        ? latebrake::EventKind::Circuit : latebrake::EventKind::TimeAttack;
    Race = latebrake::RaceProgress(4, Event->Laps);
    LastCareerAward = {};
    RestartRace();
    RefreshRival();
    // Persist the active event so exits/restarts can recover the selection.
    // Only the final validated checkpoint commits a medal and credits.
    SaveRecords();
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
    const int32 Previous = Garage.SelectedVehicle();
    const int32 Next = Garage.NextOwnedVehicle();
    if (Next == Previous)
    {
        GarageFeedback = TEXT("Unlock another vehicle with Q first.");
        return;
    }
    if (Garage.Select(Next))
    {
        ApplyGarageToPlayer();
        GarageFeedback = FString::Printf(TEXT("Selected %s."), *GetSelectedVehicleName());
        SaveRecords();
    }
}

void ALatebrakeRaceGameMode::GarageUnlockNext()
{
    const int32 Next = Garage.NextLockedVehicle();
    if (Next < 0)
    {
        GarageFeedback = TEXT("Every prototype profile is already unlocked.");
        return;
    }
    const int32 Cost = latebrake::GarageState::Definition(Next).UnlockCost;
    if (!Garage.Unlock(Next))
    {
        GarageFeedback = FString::Printf(TEXT("Need %d credits; currently %d."), Cost, Garage.Credits());
        return;
    }
    GarageFeedback = FString::Printf(TEXT("Unlocked %s!"), UTF8_TO_TCHAR(latebrake::GarageState::Definition(Next).Name));
    SaveRecords();
}

void ALatebrakeRaceGameMode::GarageUpgradeEngine()
{
    const int32 Cost = Garage.NextUpgradeCost(latebrake::UpgradeKind::Engine);
    if (Garage.Upgrade(latebrake::UpgradeKind::Engine))
    {
        GarageFeedback = FString::Printf(TEXT("Engine upgraded to level %d."), Garage.EngineLevel());
        ApplyGarageToPlayer();
        SaveRecords();
    }
    else
    {
        GarageFeedback = Cost <= 0 ? TEXT("Engine is already fully upgraded.")
            : FString::Printf(TEXT("Engine needs %d credits; have %d."), Cost, Garage.Credits());
    }
}

void ALatebrakeRaceGameMode::GarageUpgradeGrip()
{
    const int32 Cost = Garage.NextUpgradeCost(latebrake::UpgradeKind::Grip);
    if (Garage.Upgrade(latebrake::UpgradeKind::Grip))
    {
        GarageFeedback = FString::Printf(TEXT("Grip upgraded to level %d."), Garage.GripLevel());
        ApplyGarageToPlayer();
        SaveRecords();
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
        RivalCar->ResetPilot();
    }
}

void ALatebrakeRaceGameMode::RestartRace()
{
    // Restarting from the mobile PAUSE screen must return to a live race.
    if (UGameplayStatics::IsGamePaused(this))
    {
        UGameplayStatics::SetGamePaused(this, false);
    }
    Race.Reset();
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
    // A five-second penalty discourages skipping sectors by teleporting.
    if (Race.AddTimePenalty(5.0) &&
        RecoveriesThisRace < std::numeric_limits<int32>::max())
    {
        ++RecoveriesThisRace;
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
    Save->BestRaceSeconds = PersonalBestSeconds;
    Save->BestLapSeconds = PersonalBestLapSeconds;
    Save->BestTimeAttackSeconds = BestTimeAttackSeconds;
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
