#include "Vehicles/LatebrakeCarPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
void ConfigureDecorativeMesh(UStaticMeshComponent* Component, UStaticMesh* Mesh, const FVector& RelativeLocation, const FVector& Scale)
{
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetSimulatePhysics(false);
    Component->SetRelativeLocation(RelativeLocation);
    Component->SetRelativeScale3D(Scale);
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
    CameraBoom->TargetArmLength = 560.f;
    CameraBoom->SocketOffset = FVector(0.f, 0.f, 120.f);
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 7.f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void ALatebrakeCarPawn::ThrottleInput(float Value)
{
    Throttle = FMath::Clamp(Value, -1.f, 1.f);
}

void ALatebrakeCarPawn::SteerInput(float Value)
{
    Steering = FMath::Clamp(Value, -1.f, 1.f);
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

    if (!Chassis->IsSimulatingPhysics() || DeltaSeconds <= 0.f)
    {
        return;
    }

    const bool bArcade = HandlingMode == ELatebrakeHandlingMode::Arcade;
    const float DriveForce = bArcade ? 1500000.f : 1030000.f;
    const float MaxSpeed = bArcade ? 6500.f : 5400.f; // cm/s
    const float GripRate = bArcade ? 8.5f : 4.25f;
    const float BrakeRate = bBraking ? 7.f : 0.55f;

    const FVector Forward = GetActorForwardVector();
    const FVector Right = GetActorRightVector();
    const FVector Velocity = Chassis->GetPhysicsLinearVelocity();
    const FVector FlatVelocity(Velocity.X, Velocity.Y, 0.f);
    const float Speed = FlatVelocity.Size();
    const float LongitudinalSpeed = FVector::DotProduct(FlatVelocity, Forward);
    const float LateralSpeed = FVector::DotProduct(FlatVelocity, Right);
    const float Mass = Chassis->GetMass();

    if (!bBraking && (FMath::Abs(Throttle) < 0.01f || FMath::Sign(LongitudinalSpeed) == FMath::Sign(Throttle) || Speed < 100.f))
    {
        Chassis->AddForce(Forward * DriveForce * Throttle);
    }
    else if (!bBraking)
    {
        // Opposite throttle acts as engine braking until the car slows.
        Chassis->AddForce(-Forward * LongitudinalSpeed * Mass * 3.f);
    }

    // Anti-slide assistance is stronger in Arcade; both presets remain tunable.
    Chassis->AddForce(-Right * LateralSpeed * Mass * GripRate);
    if (bBraking || FMath::Abs(Throttle) < 0.02f)
    {
        Chassis->AddForce(-Forward * LongitudinalSpeed * Mass * BrakeRate);
    }

    const float SteeringAuthority = FMath::Clamp(FMath::Abs(LongitudinalSpeed) / 900.f, 0.f, 1.f);
    const float SteerDirection = LongitudinalSpeed < -75.f ? -1.f : 1.f;
    const float TurnTorque = (bArcade ? 21000000.f : 14000000.f) * Steering * SteeringAuthority * SteerDirection;
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
    Throttle = 0.f;
    Steering = 0.f;
    bBraking = false;
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
    PlayerInputComponent->BindAction(TEXT("ArcadePreset"), IE_Pressed, this, &ALatebrakeCarPawn::SelectArcade);
    PlayerInputComponent->BindAction(TEXT("SimulationPreset"), IE_Pressed, this, &ALatebrakeCarPawn::SelectSimulation);
}
