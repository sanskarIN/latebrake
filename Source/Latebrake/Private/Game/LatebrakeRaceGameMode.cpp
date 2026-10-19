#include "Game/LatebrakeRaceGameMode.h"

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
#include "Progression/RecordValidation.h"
#include "Racing/LatebrakeCheckpoint.h"
#include "Racing/RaceRewards.h"
#include <limits>
#include "UI/LatebrakeRaceHUD.h"
#include "Vehicles/LatebrakeCarPawn.h"
#include "AI/LatebrakeRivalCar.h"

namespace
{
constexpr TCHAR SaveSlot[] = TEXT("LatebrakeProfile");
constexpr int32 SaveUser = 0;
}

ALatebrakeRaceGameMode::ALatebrakeRaceGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = ALatebrakeCarPawn::StaticClass();
    HUDClass = ALatebrakeRaceHUD::StaticClass();
}

FVector ALatebrakeRaceGameMode::GetStartPosition()
{
    return FVector(-2550.f, -1800.f, 180.f);
}

FRotator ALatebrakeRaceGameMode::GetStartRotation()
{
    return FRotator(0.f, 0.f, 0.f);
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
    SpawnBlock(TEXT("SouthStraight"), FVector(0.f, -1800.f, -22.f), FVector(80.f, 16.f, 0.42f), Asphalt);
    SpawnBlock(TEXT("NorthStraight"), FVector(0.f, 1800.f, -22.f), FVector(80.f, 16.f, 0.42f), Asphalt);
    SpawnBlock(TEXT("EastStraight"), FVector(2800.f, 0.f, -22.f), FVector(16.f, 52.f, 0.42f), Asphalt);
    SpawnBlock(TEXT("WestStraight"), FVector(-2800.f, 0.f, -22.f), FVector(16.f, 52.f, 0.42f), Asphalt);

    const FLinearColor BarrierColor(0.80f, 0.15f, 0.05f);
    SpawnBlock(TEXT("NorthBarrier"), FVector(0.f, 2750.f, 65.f), FVector(85.f, 1.0f, 2.0f), BarrierColor);
    SpawnBlock(TEXT("SouthBarrier"), FVector(0.f, -2750.f, 65.f), FVector(85.f, 1.0f, 2.0f), BarrierColor);
    SpawnBlock(TEXT("EastBarrier"), FVector(3750.f, 0.f, 65.f), FVector(1.0f, 60.f, 2.0f), BarrierColor);
    SpawnBlock(TEXT("WestBarrier"), FVector(-3750.f, 0.f, 65.f), FVector(1.0f, 60.f, 2.0f), BarrierColor);

    const FVector Gates[] =
    {
        FVector(2800.f, -1800.f, 38.f),
        FVector(2800.f, 1800.f, 38.f),
        FVector(-2800.f, 1800.f, 38.f),
        FVector(-2800.f, -1800.f, 38.f)
    };

    for (int32 Index = 0; Index < 4; ++Index)
    {
        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        if (ALatebrakeCheckpoint* Checkpoint = GetWorld()->SpawnActor<ALatebrakeCheckpoint>(
                ALatebrakeCheckpoint::StaticClass(), Gates[Index], FRotator::ZeroRotator, Parameters))
        {
            Checkpoint->SetCheckpointIndex(Index);
        }
    }
}

void ALatebrakeRaceGameMode::RegisterCheckpoint(int32 CheckpointIndex)
{
    if (Race.PassCheckpoint(CheckpointIndex) && Race.IsFinished())
    {
        CompletedRaceCount = latebrake::IncrementCompletedRaces(CompletedRaceCount);
        const float FinishedSeconds = static_cast<float>(Race.GetElapsedSeconds());
        float& EventRecord = (SelectedEvent == latebrake::EventKind::TimeAttack)
            ? BestTimeAttackSeconds : PersonalBestSeconds;
        if (EventRecord <= 0.f || FinishedSeconds < EventRecord)
        {
            EventRecord = FinishedSeconds;
        }
        const float FinishedBestLap = static_cast<float>(Race.GetBestLapSeconds());
        if (FinishedBestLap > 0.f &&
            (PersonalBestLapSeconds <= 0.f || FinishedBestLap < PersonalBestLapSeconds))
        {
            PersonalBestLapSeconds = FinishedBestLap;
        }
        LastRaceCredits = latebrake::CalculateRaceCredits(
            SelectedEvent, Race.GetElapsedSeconds(), RecoveriesThisRace);
        Garage.EarnCredits(LastRaceCredits);
        SaveRecords();
    }
}

void ALatebrakeRaceGameMode::SelectRaceEvent(latebrake::EventKind Kind)
{
    if (!latebrake::IsValidEvent(Kind) || Kind == SelectedEvent)
    {
        return;
    }
    SelectedEvent = Kind;
    const latebrake::RaceEvent Definition = latebrake::GetRaceEvent(SelectedEvent);
    Race = latebrake::RaceProgress(Definition.CheckpointCount, Definition.RequiredLaps);
    RestartRace();
    RefreshRival();
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
    const latebrake::VehiclePerformance Performance = Garage.Performance();
    Car->SetGarageTuning(
        static_cast<float>(Performance.Acceleration),
        static_cast<float>(Performance.Grip),
        static_cast<float>(Performance.TopSpeed),
        Garage.SelectedVehicle());
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
            ALatebrakeRivalCar::StaticClass(), FVector(-2250.f, -1450.f, 180.f),
            FRotator::ZeroRotator, Params);
        if (!IsValid(RivalCar))
        {
            UE_LOG(LogTemp, Warning, TEXT("Latebrake: starter AI rival failed to spawn"));
        }
    }
    else
    {
        RivalCar->RecoverTo(FVector(-2250.f, -1450.f, 180.f), FRotator::ZeroRotator);
        RivalCar->ResetPilot();
    }
}

void ALatebrakeRaceGameMode::RestartRace()
{
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
        RivalCar->RecoverTo(FVector(-2250.f, -1450.f, 180.f), FRotator::ZeroRotator);
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

    // Reorient behind the next expected gate, keeping prior lap progress.
    const FVector RecoveryPoints[] =
    {
        FVector(-2550.f, -1800.f, 180.f),
        FVector(2800.f, -1300.f, 180.f),
        FVector(2300.f, 1800.f, 180.f),
        FVector(-2800.f, 1300.f, 180.f)
    };
    const FRotator RecoveryRotations[] =
    {
        FRotator(0.f, 0.f, 0.f),
        FRotator(0.f, 90.f, 0.f),
        FRotator(0.f, 180.f, 0.f),
        FRotator(0.f, -90.f, 0.f)
    };
    // A five-second penalty discourages skipping sectors by teleporting.
    if (Race.AddTimePenalty(5.0) &&
        RecoveriesThisRace < std::numeric_limits<int32>::max())
    {
        ++RecoveriesThisRace;
    }
    const int Index = FMath::Clamp(Race.GetNextCheckpoint(), 0, 3);
    Car->RecoverTo(RecoveryPoints[Index], RecoveryRotations[Index]);
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
        // Old save slots have zero for fields added after v0.2.x.
        // Garage.Load repairs the owned mask and preserves the starter car.
        Garage.Load({Save->TotalCredits, Save->OwnedVehicleMask,
                     Save->SelectedVehicleId, Save->EngineUpgradeLevel,
                     Save->GripUpgradeLevel});
    }
}

void ALatebrakeRaceGameMode::SaveRecords()
{
    ULatebrakePlayerSave* Save = Cast<ULatebrakePlayerSave>(
        UGameplayStatics::CreateSaveGameObject(ULatebrakePlayerSave::StaticClass()));
    if (!Save)
    {
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
    if (!UGameplayStatics::SaveGameToSlot(Save, SaveSlot, SaveUser))
    {
        UE_LOG(LogTemp, Warning, TEXT("Latebrake: failed to persist race results"));
    }
}
