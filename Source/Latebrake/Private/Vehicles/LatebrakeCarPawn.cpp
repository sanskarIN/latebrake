#include "Vehicles/LatebrakeCarPawn.h"
#include "Vehicles/SteeringFilter.h"
#include "Vehicles/DriverInputSafety.h"
#include "UI/TouchControlLayout.h"
#include "UI/TouchViewportSafety.h"
#include "Garage/GarageState.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
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

void ALatebrakeCarPawn::BeginPlay()
{
    Super::BeginPlay();
    // OS interruptions must never leave an accelerator finger latched.
    BackgroundInputHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(
        this, &ALatebrakeCarPawn::HandleAppInputInterruption);
    DeactivatedInputHandle = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(
        this, &ALatebrakeCarPawn::HandleAppInputInterruption);
    ForegroundInputHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(
        this, &ALatebrakeCarPawn::HandleAppInputInterruption);
}

void ALatebrakeCarPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundInputHandle);
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivatedInputHandle);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundInputHandle);
    TouchViewport.Reset();
    ClearDriverInputs();
    Super::EndPlay(EndPlayReason);
}

void ALatebrakeCarPawn::UnPossessed()
{
    TouchViewport.Reset();
    TouchPresses.Reset();
    ClearDriverInputs();
    Super::UnPossessed();
}

void ALatebrakeCarPawn::HandleAppInputInterruption()
{
    // Focus changes may discard release events; start a fresh touch session.
    TouchViewport.Reset();
    TouchPresses.Reset();
    ClearDriverInputs();
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
    TouchDrive.Clear();
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

#if PLATFORM_ANDROID
    // Orientation and viewport resizing can arrive without *any* subsequent
    // touch-move event. Cancel already-held driving axes in the physics tick
    // rather than waiting for the former finger to cross a resized pad.
    if (const APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        int32 Width = 0;
        int32 Height = 0;
        PC->GetViewportSize(Width, Height);
        latebrake::NeutralizeTouchOnViewportChange(
            TouchViewport, TouchDrive, SmoothedThrottle, SmoothedSteering,
            Width, Height);
    }
    else
    {
        TouchViewport.Reset();
        // Losing the controller invalidates both touch and analog input.
        // Reset the raw axes, not only their smooth filters: otherwise the
        // next Tick may rebuild throttle from a stale gamepad axis value.
        ClearDriverInputs();
    }
#endif

    if (!Chassis->IsSimulatingPhysics() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.f)
    {
        return;
    }

    const bool bArcade = HandlingMode == ELatebrakeHandlingMode::Arcade;
    SmoothedThrottle = static_cast<float>(latebrake::SmoothThrottle(
        SmoothedThrottle, TouchDrive.ThrottleActive() ? 1.0 : static_cast<double>(Throttle),
        static_cast<double>(DeltaSeconds)));
    const float EffectiveThrottle = SmoothedThrottle;
    const float EffectiveSteering = TouchDrive.SteeringActive()
        ? static_cast<float>(TouchDrive.Steering()) : Steering;
    SmoothedSteering = static_cast<float>(latebrake::SmoothSteering(
        SmoothedSteering, EffectiveSteering, static_cast<double>(DeltaSeconds),
        bArcade ? 3.8 : 2.4));
    const bool bEffectiveBraking = bBraking || TouchDrive.BrakeActive();
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
    // Notify all four checkpoint samplers of the discontinuity even when
    // its displacement is short enough to look like a valid plane crossing.
    // This applies to recovery, restart and first spawn repositioning.
    ++TeleportRevision;
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
    // Route both standalone Escape/gamepad pause and open-menu resume
    // through GameMode so every transition invalidates checkpoint motion.
    if (ALatebrakeRaceGameMode* Mode =
        GetWorld() ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr)
    {
        Mode->ToggleRacePause();
        return;
    }
    // If no authoritative GameMode exists, do not alter game pause from
    // a pawn callback that cannot update the checkpoint session revision.
    ClearDriverInputs();
}

void ALatebrakeCarPawn::TouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    // A duplicated Pressed event from any held finger (including an action
    // pad or blank UI region) must never trigger RECOVER or RESTART again.
    // Do not reset this tracker from ClearDriverInputs(): those actions
    // themselves clear driving state while the finger is still held.
    if (!TouchPresses.Press(static_cast<int>(FingerIndex))) return;
    ApplyTouch(FingerIndex, Location, true);
}

void ALatebrakeCarPawn::TouchMoved(ETouchIndex::Type FingerIndex, FVector Location)
{
    // A move never captures a finger that started outside the visible pads.
    ApplyTouch(FingerIndex, Location, false);
}

void ALatebrakeCarPawn::ApplyTouch(ETouchIndex::Type FingerIndex,
                                  const FVector& Location, bool bAllowAcquire)
{
    const int32 FingerId = static_cast<int32>(FingerIndex);
    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        TouchViewport.Reset();
        TouchDrive.Clear();
        SmoothedThrottle = 0.f;
        SmoothedSteering = 0.f;
        return;
    }
    int32 Width = 0;
    int32 Height = 0;
    PC->GetViewportSize(Width, Height);
    // The press tracker is deliberately NOT reset: a finger held before an
    // orientation change must not generate a second RECOVER/PAUSE/RESTART.
    latebrake::NeutralizeTouchOnViewportChange(
        TouchViewport, TouchDrive, SmoothedThrottle, SmoothedSteering,
        Width, Height);
    const latebrake::TouchControlLayout Layout =
        latebrake::CalculateTouchControlLayout(Width, Height);
    if (!Layout.Valid)
    {
        // Orientation or resize invalidates the geometry of every held
        // finger, not just the one that happened to move first.
        TouchDrive.Clear();
        SmoothedThrottle = 0.f;
        SmoothedSteering = 0.f;
        return;
    }
    if (!FMath::IsFinite(Location.X) || !FMath::IsFinite(Location.Y))
    {
        TouchDrive.Cancel(FingerId);
        return;
    }

    const latebrake::TouchControl Region =
        latebrake::HitTestTouchControl(Layout, Location.X, Location.Y);
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (Mode && Mode->IsMenuOpen())
    {
        // Menus own focus: never acquire or move driving fingers while paused.
        // Only a distinct new press can activate the visible navigation pads.
        TouchDrive.Clear();
        if (bAllowAcquire)
        {
            if (Region == latebrake::TouchControl::CareerNext) MenuNext();
            else if (Region == latebrake::TouchControl::CareerStart) MenuActivate();
            else if (Region == latebrake::TouchControl::MenuBrowse) CareerNext();
            else if (Region == latebrake::TouchControl::Pause) MenuToggle();
        }
        return;
    }
    if (UGameplayStatics::IsGamePaused(this))
    {
        // A standalone Escape/gamepad pause has no menu focus. Reject new
        // gas, brake and steering presses until explicit unpause; otherwise
        // a finger pressed while paused could unexpectedly accelerate later.
        TouchDrive.Clear();
        if (bAllowAcquire && Region == latebrake::TouchControl::Pause)
        {
            TogglePause();
        }
        return;
    }

    if (!bAllowAcquire)
    {
        // A move can update only a finger captured on its original press.
        TouchDrive.Move(Layout, FingerId, Location.X, Location.Y);
        return;
    }

    // Duplicate Pressed notifications from a held driving finger should not
    // cause menu or race actions when it happens to overlap an action pad.
    if (FingerId == TouchDrive.SteeringOwner() ||
        FingerId == TouchDrive.PedalOwner())
    {
        return;
    }

    // Discrete actions fire only on press. Dragging a steering/pedal finger
    // over them cannot restart, recover, open menus or collect rewards.
    switch (Region)
    {
    case latebrake::TouchControl::Pause:
        TogglePause();
        return;
    case latebrake::TouchControl::Restart:
        RestartPressed();
        return;
    case latebrake::TouchControl::Recover:
        RecoverPressed();
        return;
    case latebrake::TouchControl::CareerNext:
        CareerNext();
        return;
    case latebrake::TouchControl::CareerStart:
        CareerStart();
        return;
    default:
        break;
    }

    TouchDrive.Press(Layout, FingerId, Location.X, Location.Y);
}

void ALatebrakeCarPawn::TouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
    (void)Location;
    TouchDrive.Release(static_cast<int32>(FingerIndex));
    TouchPresses.Release(static_cast<int>(FingerIndex));
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

void ALatebrakeCarPawn::MenuToggle()
{
    ClearDriverInputs();
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->ToggleMenu();
    }
}

void ALatebrakeCarPawn::MenuNext()
{
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->MenuNext();
    }
}

void ALatebrakeCarPawn::MenuPrevious()
{
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->MenuPrevious();
    }
}

void ALatebrakeCarPawn::MenuActivate()
{
    ClearDriverInputs();
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->MenuActivate();
    }
}

void ALatebrakeCarPawn::CareerNext()
{
    ClearDriverInputs();
    if (ALatebrakeRaceGameMode* Mode =
        GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->CycleCareerEvent();
    }
}

void ALatebrakeCarPawn::CareerStart()
{
    ClearDriverInputs();
    if (ALatebrakeRaceGameMode* Mode =
        GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        if (Mode->IsMenuOpen()) Mode->MenuActivate();
        else Mode->StartCareerEvent(Mode->GetMenuCareerEventId());
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
    FInputTouchBinding& PressedTouch = PlayerInputComponent->BindTouch(
        IE_Pressed, this, &ALatebrakeCarPawn::TouchPressed);
    PressedTouch.bExecuteWhenPaused = true; // Mobile PAUSE pad can also resume play.
    PlayerInputComponent->BindTouch(IE_Repeat, this, &ALatebrakeCarPawn::TouchMoved);
    PlayerInputComponent->BindTouch(IE_Released, this, &ALatebrakeCarPawn::TouchReleased);
    FInputActionBinding& PauseAction = PlayerInputComponent->BindAction(TEXT("Pause"), IE_Pressed, this, &ALatebrakeCarPawn::TogglePause);
    PauseAction.bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("CircuitEvent"), IE_Pressed, this, &ALatebrakeCarPawn::SelectCircuit);
    PlayerInputComponent->BindAction(TEXT("TimeAttackEvent"), IE_Pressed, this, &ALatebrakeCarPawn::SelectTimeAttack);
    FInputActionBinding& BrowseCareer = PlayerInputComponent->BindAction(
        TEXT("CareerNext"), IE_Pressed, this, &ALatebrakeCarPawn::CareerNext);
    BrowseCareer.bExecuteWhenPaused = true;
    FInputActionBinding& LaunchCareer = PlayerInputComponent->BindAction(
        TEXT("CareerStart"), IE_Pressed, this, &ALatebrakeCarPawn::CareerStart);
    LaunchCareer.bExecuteWhenPaused = true;
    FInputActionBinding& OpenMenu = PlayerInputComponent->BindAction(
        TEXT("MenuToggle"), IE_Pressed, this, &ALatebrakeCarPawn::MenuToggle);
    OpenMenu.bExecuteWhenPaused = true;
    FInputActionBinding& NextMenu = PlayerInputComponent->BindAction(
        TEXT("MenuNext"), IE_Pressed, this, &ALatebrakeCarPawn::MenuNext);
    NextMenu.bExecuteWhenPaused = true;
    FInputActionBinding& PreviousMenu = PlayerInputComponent->BindAction(
        TEXT("MenuPrevious"), IE_Pressed, this, &ALatebrakeCarPawn::MenuPrevious);
    PreviousMenu.bExecuteWhenPaused = true;
    FInputActionBinding& ConfirmMenu = PlayerInputComponent->BindAction(
        TEXT("MenuActivate"), IE_Pressed, this, &ALatebrakeCarPawn::MenuActivate);
    ConfirmMenu.bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("GarageNextVehicle"), IE_Pressed, this, &ALatebrakeCarPawn::NextGarageVehicle);
    PlayerInputComponent->BindAction(TEXT("GarageUnlockVehicle"), IE_Pressed, this, &ALatebrakeCarPawn::UnlockGarageVehicle);
    PlayerInputComponent->BindAction(TEXT("GarageEngineUpgrade"), IE_Pressed, this, &ALatebrakeCarPawn::UpgradeGarageEngine);
    PlayerInputComponent->BindAction(TEXT("GarageGripUpgrade"), IE_Pressed, this, &ALatebrakeCarPawn::UpgradeGarageGrip);
    PlayerInputComponent->BindAction(TEXT("ArcadePreset"), IE_Pressed, this, &ALatebrakeCarPawn::SelectArcade);
    PlayerInputComponent->BindAction(TEXT("SimulationPreset"), IE_Pressed, this, &ALatebrakeCarPawn::SelectSimulation);
}
