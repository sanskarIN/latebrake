#include "Vehicles/LatebrakeCarPawn.h"
#include "Vehicles/SteeringFilter.h"
#include "Vehicles/DriverInputSafety.h"
#include "Garage/GarageState.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void ConfigureDecorativeMesh(UStaticMeshComponent* Component, UStaticMesh* Mesh, const FVector& RelativeLocation, const FVector& Scale)
{
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetSimulatePhysics(false);
    // Chassis is physically scaled; compensate its scale on attached meshes.
    const FVector RootScale(3.8f, 1.8f, 0.65f);
    Component->SetRelativeLocation(RelativeLocation / RootScale);
    Component->SetRelativeScale3D(Scale / RootScale);
}
}

ALatebrakeCarPawn::ALatebrakeCarPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Disabled;

    Chassis = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Chassis"));
    RootComponent = Chassis;
    Chassis->SetCollisionProfileName(TEXT("Pawn"));
    Chassis->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Chassis->SetCollisionObjectType(ECC_Pawn);
    Chassis->SetSimulatePhysics(true);
    Chassis->SetEnableGravity(true);
    Chassis->SetLinearDamping(0.5f);
    Chassis->SetAngularDamping(7.f);
    Chassis->SetMassOverrideInKg(NAME_None, 1100.f);
    // Pitch/roll constraints keep this early driving prototype accessible.
    Chassis->BodyInstance.bLockXRotation = true;
    Chassis->BodyInstance.bLockYRotation = true;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

    if (Cube.Succeeded())
    {
        Chassis->SetStaticMesh(Cube.Object);
    }
    Chassis->SetRelativeScale3D(FVector(3.8f, 1.8f, 0.65f));

    Cabin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabin"));
    Cabin->SetupAttachment(Chassis);
    if (Cube.Succeeded())
    {
        ConfigureDecorativeMesh(Cabin, Cube.Object, FVector(-20.f, 0.f, 52.f), FVector(1.75f, 1.48f, 0.55f));
    }

    FrontLeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontLeftWheel"));
    FrontRightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRightWheel"));
    RearLeftWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearLeftWheel"));
    RearRightWheel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearRightWheel"));

    UStaticMeshComponent* Wheels[] = {FrontLeftWheel, FrontRightWheel, RearLeftWheel, RearRightWheel};
    const FVector Locations[] =
    {
        FVector(122.f, -94.f, -22.f), FVector(122.f, 94.f, -22.f),
        FVector(-122.f, -94.f, -22.f), FVector(-122.f, 94.f, -22.f)
    };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Wheels[Index]->SetupAttachment(Chassis);
        if (Cylinder.Succeeded())
        {
            ConfigureDecorativeMesh(Wheels[Index], Cylinder.Object, Locations[Index], FVector(0.58f, 0.58f, 0.25f));
            Wheels[Index]->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
        }
    }

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(Chassis);
    CameraBoom->SetAbsolute(false, false, true);
    CameraBoom->SetRelativeRotation(FRotator(-12.f, 0.f, 0.f));
    CameraBoom->TargetArmLength = 560.f;
    CameraBoom->SocketOffset = FVector(0.f, 0.f, 120.f);
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 7.f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void ALatebrakeCarPawn::SetAutonomousControls(float InThrottle, float InSteering, bool bInBrake)
{
    Throttle = static_cast<float>(latebrake::SanitizeDriveAxis(InThrottle));
    Steering = static_cast<float>(latebrake::SanitizeDriveAxis(InSteering));
    bBraking = bInBrake;
}

void ALatebrakeCarPawn::SetGarageTuning(float Acceleration, float Grip, float TopSpeed,
                                     int32 VehicleId)
{
    // The garage uses bounded, engine-independent multipliers; defend the
    // physics integration from corrupt or nonfinite gameplay state anyway.
    GarageAcceleration = FMath::IsFinite(Acceleration) ? FMath::Clamp(Acceleration, 0.5f, 2.f) : 1.f;
    GarageGrip = FMath::IsFinite(Grip) ? FMath::Clamp(Grip, 0.5f, 2.f) : 1.f;
    GarageTopSpeed = FMath::IsFinite(TopSpeed) ? FMath::Clamp(TopSpeed, 0.5f, 2.f) : 1.f;
    const int32 SafeId = FMath::Clamp(VehicleId, 0, 2);
    if (SafeId != GarageVehicleId && IsValid(Cabin))
    {
        // Engine-built-in primitives only; distinguish prototype silhouettes
        // without introducing third-party vehicle meshes or other assets.
        const latebrake::VehicleSpec Spec = latebrake::GarageState::Definition(SafeId);
        Cabin->SetRelativeScale3D(FVector(
            (1.75f / 3.8f) * Spec.CabinLength,
            (1.48f / 1.8f),
            (0.55f / 0.65f) * Spec.CabinHeight));
    }
    GarageVehicleId = SafeId;
}

void ALatebrakeCarPawn::ClearDriverInputs()
{
    Throttle = 0.f;
    Steering = 0.f;
    SmoothedThrottle = 0.f;
    SmoothedSteering = 0.f;
    bBraking = false;
    SteeringFinger = INDEX_NONE;
    PedalFinger = INDEX_NONE;
    TouchSteering = 0.f;
    bTouchThrottle = false;
    bTouchBrake = false;
}

void ALatebrakeCarPawn::ThrottleInput(float Value)
{
    Throttle = static_cast<float>(latebrake::SanitizeDriveAxis(Value));
}

void ALatebrakeCarPawn::SteerInput(float Value)
{
    Steering = static_cast<float>(latebrake::SanitizeDriveAxis(Value));
}

void ALatebrakeCarPawn::BrakePressed()
{
    bBraking = true;
}

void ALatebrakeCarPawn::BrakeReleased()
{
    bBraking = false;
}

void ALatebrakeCarPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!Chassis->IsSimulatingPhysics() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.f)
    {
        return;
    }

    const bool bArcade = HandlingMode == ELatebrakeHandlingMode::Arcade;
    SmoothedThrottle = static_cast<float>(latebrake::SmoothThrottle(
        SmoothedThrottle, bTouchThrottle ? 1.0 : static_cast<double>(Throttle),
        static_cast<double>(DeltaSeconds)));
    const float EffectiveThrottle = SmoothedThrottle;
    const float EffectiveSteering = SteeringFinger != INDEX_NONE ? TouchSteering : Steering;
    SmoothedSteering = static_cast<float>(latebrake::SmoothSteering(
        SmoothedSteering, EffectiveSteering, static_cast<double>(DeltaSeconds),
        bArcade ? 3.8 : 2.4));
    const bool bEffectiveBraking = bBraking || bTouchBrake;
    const float DriveForce = (bArcade ? 1500000.f : 1030000.f) * GarageAcceleration;
    const float MaxSpeed = (bArcade ? 6500.f : 5400.f) * GarageTopSpeed; // cm/s
    const float GripRate = (bArcade ? 8.5f : 4.25f) * GarageGrip;
    const float BrakeRate = bEffectiveBraking ? 7.f : 0.55f;

    const FVector Forward = GetActorForwardVector();
    const FVector Right = GetActorRightVector();
    const FVector Velocity = Chassis->GetPhysicsLinearVelocity();
    if (!FMath::IsFinite(Velocity.X) || !FMath::IsFinite(Velocity.Y) ||
        !FMath::IsFinite(Velocity.Z))
    {
        UE_LOG(LogTemp, Warning, TEXT("Latebrake: physics velocity was nonfinite; clearing vehicle motion"));
        Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);
        ClearDriverInputs();
        return;
    }
    const FVector FlatVelocity(Velocity.X, Velocity.Y, 0.f);
    const float Speed = FlatVelocity.Size();
    const float LongitudinalSpeed = FVector::DotProduct(FlatVelocity, Forward);
    const float LateralSpeed = FVector::DotProduct(FlatVelocity, Right);
    const float Mass = Chassis->GetMass();

    if (!bEffectiveBraking && (FMath::Abs(EffectiveThrottle) < 0.01f || FMath::Sign(LongitudinalSpeed) == FMath::Sign(EffectiveThrottle) || Speed < 100.f))
    {
        Chassis->AddForce(Forward * DriveForce * EffectiveThrottle);
    }
    else if (!bEffectiveBraking)
    {
        // Opposite throttle acts as engine braking until the car slows.
        Chassis->AddForce(-Forward * LongitudinalSpeed * Mass * 3.f);
    }

    // Anti-slide assistance is stronger in Arcade; both presets remain tunable.
    Chassis->AddForce(-Right * LateralSpeed * Mass * GripRate);
    if (bEffectiveBraking || FMath::Abs(EffectiveThrottle) < 0.02f)
    {
        Chassis->AddForce(-Forward * LongitudinalSpeed * Mass * BrakeRate);
    }

    const float SteeringAuthority = FMath::Clamp(FMath::Abs(LongitudinalSpeed) / 900.f, 0.f, 1.f);
    const float SteerDirection = LongitudinalSpeed < -75.f ? -1.f : 1.f;
    const float TurnTorque = (bArcade ? 21000000.f : 14000000.f) * SmoothedSteering * SteeringAuthority * SteerDirection;
    Chassis->AddTorqueInRadians(FVector(0.f, 0.f, TurnTorque));

    if (Speed > MaxSpeed)
    {
        const FVector Limited = FlatVelocity.GetClampedToMaxSize(MaxSpeed);
        Chassis->SetPhysicsLinearVelocity(FVector(Limited.X, Limited.Y, Velocity.Z));
    }
}

float ALatebrakeCarPawn::GetSpeedKmH() const
{
    const FVector V = Chassis->GetPhysicsLinearVelocity();
    return FVector(V.X, V.Y, 0.f).Size() * 0.036f;
}

void ALatebrakeCarPawn::RecoverTo(const FVector& Position, const FRotator& Rotation)
{
    Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    SetActorLocationAndRotation(Position, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
    ClearDriverInputs();
}

void ALatebrakeCarPawn::RecoverPressed()
{
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->RecoverPlayer(this);
    }
}

void ALatebrakeCarPawn::RestartPressed()
{
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->RestartRace();
    }
}

void ALatebrakeCarPawn::ChangeCamera()
{
    bNearCamera = !bNearCamera;
    CameraBoom->TargetArmLength = bNearCamera ? 270.f : 560.f;
}

void ALatebrakeCarPawn::TogglePause()
{
    // Drop held analog/touch input on both edges of a pause transition.
    ClearDriverInputs();
    UGameplayStatics::SetGamePaused(this, !UGameplayStatics::IsGamePaused(this));
}

void ALatebrakeCarPawn::TouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    ApplyTouch(FingerIndex, Location);
}

void ALatebrakeCarPawn::TouchMoved(ETouchIndex::Type FingerIndex, FVector Location)
{
    ApplyTouch(FingerIndex, Location);
}

void ALatebrakeCarPawn::ApplyTouch(ETouchIndex::Type FingerIndex, const FVector& Location)
{
    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }
    int32 Width = 0;
    int32 Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0)
    {
        return;
    }

    if (!FMath::IsFinite(Location.X) || !FMath::IsFinite(Location.Y))
    {
        return;
    }
    const int32 FingerId = static_cast<int32>(FingerIndex);
    if (SteeringFinger == FingerId ||
        (SteeringFinger == INDEX_NONE && PedalFinger != FingerId &&
         Location.X < static_cast<float>(Width) * 0.5f))
    {
        SteeringFinger = FingerId;
        TouchSteering = static_cast<float>(latebrake::SanitizeTouchSteering(Location.X, Width));
    }
    else if (PedalFinger == FingerId ||
        (PedalFinger == INDEX_NONE && SteeringFinger != FingerId &&
         Location.X >= static_cast<float>(Width) * 0.5f))
    {
        PedalFinger = FingerId;
        bTouchBrake = Location.Y > static_cast<float>(Height) * 0.68f;
        bTouchThrottle = !bTouchBrake;
    }
}

void ALatebrakeCarPawn::TouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
    (void)Location;
    const int32 FingerId = static_cast<int32>(FingerIndex);
    if (SteeringFinger == FingerId)
    {
        SteeringFinger = INDEX_NONE;
        TouchSteering = 0.f;
    }
    if (PedalFinger == FingerId)
    {
        PedalFinger = INDEX_NONE;
        bTouchThrottle = false;
        bTouchBrake = false;
    }
}

void ALatebrakeCarPawn::SelectCircuit()
{
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->SelectRaceEvent(latebrake::EventKind::Circuit);
    }
}

void ALatebrakeCarPawn::SelectTimeAttack()
{
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->SelectRaceEvent(latebrake::EventKind::TimeAttack);
    }
}

void ALatebrakeCarPawn::NextGarageVehicle()
{
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->GarageNextVehicle();
    }
}

void ALatebrakeCarPawn::UnlockGarageVehicle()
{
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->GarageUnlockNext();
    }
}

void ALatebrakeCarPawn::UpgradeGarageEngine()
{
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->GarageUpgradeEngine();
    }
}

void ALatebrakeCarPawn::UpgradeGarageGrip()
{
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->GarageUpgradeGrip();
    }
}

void ALatebrakeCarPawn::SelectArcade()
{
    HandlingMode = ELatebrakeHandlingMode::Arcade;
}

void ALatebrakeCarPawn::SelectSimulation()
{
    HandlingMode = ELatebrakeHandlingMode::SimulationInspired;
}

void ALatebrakeCarPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("Throttle"), this, &ALatebrakeCarPawn::ThrottleInput);
    PlayerInputComponent->BindAxis(TEXT("Steer"), this, &ALatebrakeCarPawn::SteerInput);
    PlayerInputComponent->BindAction(TEXT("Brake"), IE_Pressed, this, &ALatebrakeCarPawn::BrakePressed);
    PlayerInputComponent->BindAction(TEXT("Brake"), IE_Released, this, &ALatebrakeCarPawn::BrakeReleased);
    PlayerInputComponent->BindAction(TEXT("Recover"), IE_Pressed, this, &ALatebrakeCarPawn::RecoverPressed);
    PlayerInputComponent->BindAction(TEXT("RestartRace"), IE_Pressed, this, &ALatebrakeCarPawn::RestartPressed);
    PlayerInputComponent->BindAction(TEXT("Camera"), IE_Pressed, this, &ALatebrakeCarPawn::ChangeCamera);
    PlayerInputComponent->BindTouch(IE_Pressed, this, &ALatebrakeCarPawn::TouchPressed);
    PlayerInputComponent->BindTouch(IE_Repeat, this, &ALatebrakeCarPawn::TouchMoved);
    PlayerInputComponent->BindTouch(IE_Released, this, &ALatebrakeCarPawn::TouchReleased);
    FInputActionBinding& PauseAction = PlayerInputComponent->BindAction(TEXT("Pause"), IE_Pressed, this, &ALatebrakeCarPawn::TogglePause);
    PauseAction.bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("CircuitEvent"), IE_Pressed, this, &ALatebrakeCarPawn::SelectCircuit);
    PlayerInputComponent->BindAction(TEXT("TimeAttackEvent"), IE_Pressed, this, &ALatebrakeCarPawn::SelectTimeAttack);
    PlayerInputComponent->BindAction(TEXT("GarageNextVehicle"), IE_Pressed, this, &ALatebrakeCarPawn::NextGarageVehicle);
    PlayerInputComponent->BindAction(TEXT("GarageUnlockVehicle"), IE_Pressed, this, &ALatebrakeCarPawn::UnlockGarageVehicle);
    PlayerInputComponent->BindAction(TEXT("GarageEngineUpgrade"), IE_Pressed, this, &ALatebrakeCarPawn::UpgradeGarageEngine);
    PlayerInputComponent->BindAction(TEXT("GarageGripUpgrade"), IE_Pressed, this, &ALatebrakeCarPawn::UpgradeGarageGrip);
    PlayerInputComponent->BindAction(TEXT("ArcadePreset"), IE_Pressed, this, &ALatebrakeCarPawn::SelectArcade);
    PlayerInputComponent->BindAction(TEXT("SimulationPreset"), IE_Pressed, this, &ALatebrakeCarPawn::SelectSimulation);
}
