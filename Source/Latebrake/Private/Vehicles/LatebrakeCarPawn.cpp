#include "Vehicles/LatebrakeCarPawn.h"
#include "Vehicles/SteeringFilter.h"
#include "Vehicles/SpeedSensitiveSteering.h"
#include "Vehicles/CornerTractionAssist.h"
#include "Vehicles/FrameStableDamping.h"
#include "Vehicles/HandbrakeDrivePolicy.h"
#include "Vehicles/TouchCameraCyclePolicy.h"
#include "Vehicles/DriverInputSafety.h"
#include "Vehicles/DriveAxisDeadzone.h"
#include "Vehicles/RaceVehiclePhysicsReadiness.h"
#include "Vehicles/VehicleDynamicsReadiness.h"
#include "Racing/RecoveryTeleportVerification.h"
#include "Vehicles/PlayerDriveReadiness.h"
#include "UI/TouchControlLayout.h"
#include "UI/GhostTouchPolicy.h"
#include "UI/LatebrakeRaceHUD.h"
#include "UI/TouchViewportSafety.h"
#include "UI/TouchPressDispatch.h"
#include "UI/TouchResidualSafety.h"
#include "UI/FocusSuspendedTouchPolicy.h"
#include "Garage/GarageState.h"

#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include <limits>

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
    FrontLeftSpoke = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontLeftSpoke"));
    FrontRightSpoke = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontRightSpoke"));
    RearLeftSpoke = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearLeftSpoke"));
    RearRightSpoke = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearRightSpoke"));

    UStaticMeshComponent* Wheels[] = {FrontLeftWheel, FrontRightWheel, RearLeftWheel, RearRightWheel};
    UStaticMeshComponent* Spokes[] = {FrontLeftSpoke, FrontRightSpoke, RearLeftSpoke, RearRightSpoke};
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
            // Align the cylinder's original Z axis with the car's Y axis:
            // this is a *wheel spindle*, not a tire facing nose-to-tail.
            Wheels[Index]->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));
        }
        // The primitive cylinder is rotationally symmetric. A light,
        // collision-free offset spoke makes measured roll visually legible.
        Spokes[Index]->SetupAttachment(Wheels[Index]);
        Spokes[Index]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Spokes[Index]->SetSimulatePhysics(false);
        Spokes[Index]->SetCastShadow(false);
        if (Cube.Succeeded())
        {
            Spokes[Index]->SetStaticMesh(Cube.Object);
            // Position on the outward-facing cylinder cap of each side.
            Spokes[Index]->SetRelativeLocation(FVector(
                28.f, 0.f, Locations[Index].Y < 0.f ? 51.5f : -51.5f));
            Spokes[Index]->SetRelativeScale3D(FVector(0.14f, 0.24f, 0.09f));
        }
    }

    // Three low-range attached lamps, with no collision or engine-owned art.
    // Chassis uses non-uniform static-mesh scaling, so divide intended
    // world-relative rear positions by the scaled parent components.
    RearLeftSignal = CreateDefaultSubobject<UPointLightComponent>(TEXT("RearLeftSignal"));
    RearRightSignal = CreateDefaultSubobject<UPointLightComponent>(TEXT("RearRightSignal"));
    ReverseSignal = CreateDefaultSubobject<UPointLightComponent>(TEXT("ReverseSignal"));
    UPointLightComponent* Lamps[] = {RearLeftSignal, RearRightSignal, ReverseSignal};
    const FVector LampLocations[] = {
        FVector(-385.f, -115.f, 22.f),
        FVector(-385.f, 115.f, 22.f),
        FVector(-390.f, 0.f, 22.f)
    };
    const FVector ChassisScale(3.8f, 1.8f, 0.65f);
    for (int32 Index = 0; Index < 3; ++Index)
    {
        Lamps[Index]->SetupAttachment(Chassis);
        Lamps[Index]->SetRelativeLocation(LampLocations[Index] / ChassisScale);
        Lamps[Index]->SetAttenuationRadius(170.f);
        // Short-range cosmetic signals must not create costly per-frame
        // shadow passes, volumetric shadows or indirect/reflection updates.
        Lamps[Index]->SetCastShadows(false);
        Lamps[Index]->SetCastVolumetricShadow(false);
        Lamps[Index]->SetAffectGlobalIllumination(false);
        Lamps[Index]->SetAffectReflection(false);
        Lamps[Index]->SetIntensity(0.f);
        Lamps[Index]->SetLightColor(Index == 2
            ? FLinearColor(0.95f, 0.97f, 1.0f)
            : FLinearColor(1.f, 0.012f, 0.008f));
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
    ApplyCameraPreset();
}

void ALatebrakeCarPawn::BeginPlay()
{
    Super::BeginPlay();
    // OS interruptions must never leave an accelerator finger latched.
    BackgroundInputHandle = FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(
        this, &ALatebrakeCarPawn::HandleAppFocusLost);
    DeactivatedInputHandle = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(
        this, &ALatebrakeCarPawn::HandleAppFocusLost);
    ForegroundInputHandle = FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(
        this, &ALatebrakeCarPawn::HandleAppInputInterruption);
}

void ALatebrakeCarPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ApplySignalLights(false, false, 0.0);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundInputHandle);
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivatedInputHandle);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundInputHandle);
    TouchViewport.Reset();
    ClearDriverInputs();
    Super::EndPlay(EndPlayReason);
}

void ALatebrakeCarPawn::UnPossessed()
{
    ApplySignalLights(false, false, 0.0);
    TouchViewport.Reset();
    TouchPresses.Reset();
    ClearDriverInputs();
    Super::UnPossessed();
}

void ALatebrakeCarPawn::HandleAppInputInterruption()
{
    // Engine pause may suppress Tick entirely. Clear lamp output on the
    // lifecycle callback itself, not on a later optional physics tick.
    ApplySignalLights(false, false, 0.0);
    // Focus changes may discard release events; start a fresh touch session.
    TouchViewport.Reset();
    TouchPresses.Reset();
    ClearDriverInputs();
}

void ALatebrakeCarPawn::HandleAppFocusLost()
{
    // Neutralize both analog and touch commands before telling the race
    // mode about deactivation. Foreground callbacks only clear stale
    // input and never automatically unpause the race.
    HandleAppInputInterruption();
    if (UWorld* World = GetWorld())
    {
        if (ALatebrakeRaceGameMode* Mode =
                World->GetAuthGameMode<ALatebrakeRaceGameMode>())
        {
            Mode->PauseForAppInterruption();
            if (Mode->IsFocusLossGuardActive())
            {
                // Pause may have failed: stop any pre-loss momentum now.
                NeutralizeMotionWhenDrivingBlocked();
            }
        }
    }
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
    bHandbrakeHeld = false;
    TireCondition.Suspend();
    BrakeDistance.Suspend();
    SlipstreamBoost.Suspend();
    TouchDrive.Clear();
    // Recovery, restart, focus loss and interrupted input must never
    // leave the camera facing backwards after the held key is gone.
    LookBackReleased();
    ResetSpeedCameraFov();
}

void ALatebrakeCarPawn::NeutralizeMotionWhenDrivingBlocked()
{
    ApplySignalLights(false, false, 0.0);
    WheelMotion.Suspend();
    ApplyWheelVisualPose();
    DrivingStats.Suspend();
    DriftMeter.Suspend();
    GForceMeter.Suspend();
    SlipAngleMeter.Suspend();
    BrakeDistance.Suspend();
    SlipstreamBoost.Suspend();
    ClearDriverInputs();
    // A failed pause may keep Chaos simulating while race time is frozen.
    // Neutralize driver commands plus current momentum during menu/world
    // pause, OS suspension, invalid track and completed race states. A real
    // physics or input-device pause must still be qualified in Unreal.
    if (IsValid(Chassis) && Chassis->IsSimulatingPhysics())
    {
        Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
}

void ALatebrakeCarPawn::ThrottleInput(float Value)
{
    Throttle = static_cast<float>(
        latebrake::FilterHumanThrottleAxis(Value));
}

void ALatebrakeCarPawn::SteerInput(float Value)
{
    Steering = static_cast<float>(
        latebrake::FilterHumanSteeringAxis(Value));
}

void ALatebrakeCarPawn::BrakePressed()
{
    bBraking = true;
}

void ALatebrakeCarPawn::BrakeReleased()
{
    bBraking = false;
}

void ALatebrakeCarPawn::HandbrakePressed()
{
    // This input exists only for the actual human driver. A local AI
    // controller still cannot request handbrake handling on the base pawn.
    UWorld* World = GetWorld();
    const ALatebrakeRaceGameMode* Mode =
        World ? World->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    bHandbrakeHeld = IsLocallyControlled() &&
        Cast<APlayerController>(GetController()) != nullptr &&
        Mode && Mode->IsRaceStartGreen() &&
        Mode->IsCircuitReadyForGameplay() &&
        Mode->IsRestartPlacementReady() &&
        !Mode->IsMenuOpen() &&
        !UGameplayStatics::IsGamePaused(this) &&
        !Mode->IsManualRacePauseActive() &&
        !Mode->IsFocusLossGuardActive() &&
        !Mode->GetRace().IsFinished() && IsRacePhysicsReady();
}

void ALatebrakeCarPawn::HandbrakeReleased()
{
    // Release works even while the world is paused. No gameplay mutation.
    bHandbrakeHeld = false;
}

void ALatebrakeCarPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Physics may keep ticking if Unreal rejects manual SetGamePaused, and
    // OS focus loss may occur while already paused. Do not drive at all if
    // the authoritative racing clock/checkpoint gate denies progression.
    // World/GameMode absence also fails closed rather than driving a pawn
    // whose progress cannot be measured or saved.
    UWorld* World = GetWorld();
    const ALatebrakeRaceGameMode* Mode =
        World ? World->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !Mode->IsRaceStartGreen() ||
        !latebrake::CanApplyPlayerDrive({
            true,
            Mode->IsCircuitReadyForGameplay(),
            Mode->IsMenuOpen(),
            UGameplayStatics::IsGamePaused(this) ||
                Mode->IsManualRacePauseActive(),
            Mode->IsFocusLossGuardActive(),
            Mode->GetRace().IsFinished(),
            Mode->IsRestartPlacementReady()}))
    {
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }

    // The authoritative clock also requires valid Chaos angular velocity and
    // positive mass. Fail closed *before* throttle smoothing/force application,
    // not one frame after GameMode notices an invalid physics body.
    if (!IsRacePhysicsReady())
    {
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }

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
        ResetSpeedCameraFov();
        ApplySignalLights(false, false, 0.0);
        WheelMotion.Suspend();
        ApplyWheelVisualPose();
        GForceMeter.Suspend();
        SlipAngleMeter.Suspend();
        BrakeDistance.Suspend();
    SlipstreamBoost.Suspend();
        TireCondition.Suspend();
        return;
    }

    const bool bArcade = HandlingMode == ELatebrakeHandlingMode::Arcade;
    SmoothedThrottle = static_cast<float>(latebrake::SmoothThrottle(
        SmoothedThrottle, TouchDrive.ThrottleActive() ? 1.0 : static_cast<double>(Throttle),
        static_cast<double>(DeltaSeconds)));
    const float RawThrottle = SmoothedThrottle;
    const float EffectiveSteering = TouchDrive.SteeringActive()
        ? static_cast<float>(TouchDrive.Steering()) : Steering;
    SmoothedSteering = static_cast<float>(latebrake::SmoothSteering(
        SmoothedSteering, EffectiveSteering, static_cast<double>(DeltaSeconds),
        bArcade ? 3.8 : 2.4));
    const bool bServiceBraking = bBraking || TouchDrive.BrakeActive();
    // A held human handbrake lowers the prototype's lateral damping and
    // applies bounded forward resistance. It never requests new tire physics.
    const latebrake::HandbrakeDriveProfile BrakeProfile =
        latebrake::SelectHandbrakeDriveProfile(
            bHandbrakeHeld && IsLocallyControlled() &&
                Cast<APlayerController>(GetController()) != nullptr,
            bServiceBraking, bArcade,
            static_cast<double>((bArcade ? 8.5f : 4.25f) * GarageGrip),
            static_cast<double>(RawThrottle));
    const bool bEffectiveBraking = BrakeProfile.Braking;
    const float DriveForce = (bArcade ? 1500000.f : 1030000.f) * GarageAcceleration;
    const float MaxSpeed = (bArcade ? 6500.f : 5400.f) * GarageTopSpeed; // cm/s
    float GripRate = static_cast<float>(BrakeProfile.LateralGripRate);
    const float BrakeRate = static_cast<float>(BrakeProfile.ForwardResistanceRate);

    const FVector Forward = GetActorForwardVector();
    const FVector Right = GetActorRightVector();
    const FVector Velocity = Chassis->GetPhysicsLinearVelocity();
    if (!FMath::IsFinite(Velocity.X) || !FMath::IsFinite(Velocity.Y) ||
        !FMath::IsFinite(Velocity.Z))
    {
        UE_LOG(LogTemp, Warning, TEXT("Latebrake: physics velocity was nonfinite; clearing vehicle motion"));
        GForceMeter.Suspend();
        SlipAngleMeter.Suspend();
        BrakeDistance.Suspend();
    SlipstreamBoost.Suspend();
        ApplySignalLights(false, false, 0.0);
        WheelMotion.Suspend();
        ApplyWheelVisualPose();
        Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);
        ClearDriverInputs();
        return;
    }
    const FVector FlatVelocity(Velocity.X, Velocity.Y, 0.f);
    const float Speed = FlatVelocity.Size();
    const float LongitudinalSpeed = FVector::DotProduct(FlatVelocity, Forward);
    const float LateralSpeed = FVector::DotProduct(FlatVelocity, Right);
    const float Mass = Chassis->GetMass();
    // Finite velocity components alone do not ensure that derived speed
    // and directional projections stay representable in float precision.
    if (!FMath::IsFinite(Speed) || !FMath::IsFinite(LongitudinalSpeed) ||
        !FMath::IsFinite(LateralSpeed))
    {
        UE_LOG(LogTemp, Warning, TEXT("Latebrake: nonfinite derived speed; stopping chassis"));
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }

    // Purely visual: gently widen the human camera with measured forward
    // chassis speed. Rival cameras, reverse motion and rear view stay static.
    UpdateSpeedCameraFov(static_cast<double>(DeltaSeconds),
                         static_cast<double>(LongitudinalSpeed) * 0.036);

    // Observe the real chassis travel direction, not a requested steering
    // angle or decorative wheel animation. Read-only session telemetry.
    SlipAngleMeter.Observe({static_cast<double>(DeltaSeconds),
        static_cast<double>(LongitudinalSpeed),
        static_cast<double>(LateralSpeed)});
    // Chassis-derived arcade grip condition shared by human and rival.
    // The source policy rejects nonfinite physics and frame hitches.
    TireCondition.Observe({static_cast<double>(DeltaSeconds),
        static_cast<double>(Speed),
        static_cast<double>(LateralSpeed), bEffectiveBraking, true});
    // Scale the normal grip policy before frame-stable resistance, so
    // the established force-bound contract stays the final authority.
    GripRate *= static_cast<float>(TireCondition.GripMultiplier());
    // Advisory, from actual consecutive signed-forward chassis motion
    // while braking. A single speed or pressed BRAKE command is insufficient.
    BrakeDistance.Observe({static_cast<double>(DeltaSeconds),
        static_cast<double>(Speed),
        static_cast<double>(LongitudinalSpeed), bEffectiveBraking});

    // Read real valid signed chassis velocity and effective brake input,
    // never motor demand alone. Advisory decorative light intensities only.
    ApplySignalLights(true, bEffectiveBraking,
        static_cast<double>(LongitudinalSpeed));

    // Apply the same bounded corner-assist policy to controller, touch
    // and rival throttle. Brake/reverse demand is never reduced by it.
    const float EffectiveThrottle = static_cast<float>(
        latebrake::StabilizeCornerThrottle(
            BrakeProfile.Throttle, LongitudinalSpeed, LateralSpeed, bArcade));
    // Drafting must observe an actual ahead, aligned, moving rival and a
    // locally possessed human driver after all race/Chaos readiness gates.
    // Time Attack, missing/finished rival and failed physics give zero help.
    FVector RivalPosition = FVector::ZeroVector;
    FVector RivalForward = FVector::ZeroVector;
    FVector RivalVelocity = FVector::ZeroVector;
    const bool bDraftTargetReady =
        IsLocallyControlled() &&
        Cast<APlayerController>(GetController()) != nullptr &&
        Mode->HasLocalPlayerDriver() &&
        Mode->TryGetRivalDraftingState(
            RivalPosition, RivalForward, RivalVelocity);
    double DraftExtraForce = 0.0;
    if (bDraftTargetReady)
    {
        const FVector Position = GetActorLocation();
        const FVector Offset = RivalPosition - Position;
        const double RivalSignedForwardSpeed =
            FVector::DotProduct(RivalVelocity, RivalForward);
        DraftExtraForce = SlipstreamBoost.Observe({
            static_cast<double>(DeltaSeconds),
            static_cast<double>(LongitudinalSpeed),
            RivalSignedForwardSpeed,
            static_cast<double>(Forward.X), static_cast<double>(Forward.Y),
            static_cast<double>(RivalForward.X),
            static_cast<double>(RivalForward.Y),
            static_cast<double>(Offset.X), static_cast<double>(Offset.Y),
            static_cast<double>(Offset.Z),
            static_cast<double>(EffectiveThrottle),
            bEffectiveBraking, true
        });
    }
    else
    {
        SlipstreamBoost.Suspend();
    }
    // Advisory metrics only: no scoring, lap records or tire-forces use
    // these values. Invalid long frames are rejected by the policy.
    DrivingStats.Observe({static_cast<double>(DeltaSeconds),
        static_cast<double>(Speed), static_cast<double>(LateralSpeed),
        bEffectiveBraking});
    // Advisory arcade meter observes genuine same-frame chassis velocity.
    // It does not control driving forces or award saved credits.
    DriftMeter.Observe({static_cast<double>(DeltaSeconds),
        static_cast<double>(Speed), static_cast<double>(LateralSpeed),
        bEffectiveBraking});
    // Physical acceleration telemetry differentiates WORLD velocity,
    // then projects its vector on the actual car yaw. This is visual
    // feedback only: never alters forces, rewards, saves or the race clock.
    GForceMeter.Observe({static_cast<double>(DeltaSeconds),
        static_cast<double>(Velocity.X), static_cast<double>(Velocity.Y),
        static_cast<double>(GetActorRotation().Yaw), bEffectiveBraking});
    if (!bEffectiveBraking && (FMath::Abs(EffectiveThrottle) < 0.01f || FMath::Sign(LongitudinalSpeed) == FMath::Sign(EffectiveThrottle) || Speed < 100.f))
    {
        Chassis->AddForce(Forward * DriveForce * EffectiveThrottle);
        // Preserve the unmodified base acceleration contract; bonus is
        // additive and never exceeds six percent of positive motor demand.
        if (DraftExtraForce > 0.0)
        {
            Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *
                static_cast<float>(DraftExtraForce));
        }
    }
    else if (!bEffectiveBraking)
    {
        // Opposite throttle acts as engine braking until the car slows.
        Chassis->AddForce(-Forward * LongitudinalSpeed * Mass *
            static_cast<float>(latebrake::FrameStableDampingRate(3.0, DeltaSeconds)));
    }

    // Anti-slide assistance is stronger in Arcade; both presets remain tunable.
    // The shared base pawn serves human and rival drivers. On a lagging
    // frame, constrain purely resistive impulses to avoid a synthetic
    // one-frame reversal of the measured velocity component.
    const float StableGripRate = static_cast<float>(
        latebrake::FrameStableDampingRate(GripRate, DeltaSeconds));
    Chassis->AddForce(-Right * LateralSpeed * Mass * StableGripRate);
    if (bEffectiveBraking || FMath::Abs(EffectiveThrottle) < 0.02f)
    {
        const float StableBrakeRate = static_cast<float>(
            latebrake::FrameStableDampingRate(BrakeRate, DeltaSeconds));
        Chassis->AddForce(-Forward * LongitudinalSpeed * Mass * StableBrakeRate);
    }

    const float SteeringAuthority = FMath::Clamp(FMath::Abs(LongitudinalSpeed) / 900.f, 0.f, 1.f);
    const float SteerDirection = LongitudinalSpeed < -75.f ? -1.f : 1.f;
    // The same actual vehicle speed governs keyboard, gamepad, touch and
    // rival steering. Preserve low-speed agility while tempering high-speed
    // torque (Arcade stays more responsive than Simulation Inspired).
    const float AssistedSteering = static_cast<float>(
        latebrake::LimitSteeringAtSpeed(SmoothedSteering, Speed, bArcade));
    const float TurnTorque = (bArcade ? 21000000.f : 14000000.f) *
        AssistedSteering * SteeringAuthority * SteerDirection;
    // Decorative wheels track real forward chassis motion, including
    // reverse, using the same bounded speed-sensitive visual steering.
    UpdateWheelVisuals(static_cast<double>(DeltaSeconds),
        static_cast<double>(LongitudinalSpeed),
        static_cast<double>(AssistedSteering));
    Chassis->AddTorqueInRadians(FVector(0.f, 0.f, TurnTorque));

    if (Speed > MaxSpeed)
    {
        const FVector Limited = FlatVelocity.GetClampedToMaxSize(MaxSpeed);
        Chassis->SetPhysicsLinearVelocity(FVector(Limited.X, Limited.Y, Velocity.Z));
    }
}

void ALatebrakeCarPawn::ApplySignalLights(
    bool bRaceDrivingAllowed, bool bEffectiveBraking,
    double SignedForwardSpeedCmPerSecond)
{
    const latebrake::VehicleSignalLights Signals =
        latebrake::SelectVehicleSignalLights({
            bRaceDrivingAllowed, bEffectiveBraking,
            SignedForwardSpeedCmPerSecond
        });
    // No collision, race state, steering, vehicle energy or gameplay reward
    // is touched. These prototype point lights have short attenuation.
    if (IsValid(RearLeftSignal))
    {
        RearLeftSignal->SetIntensity(
            static_cast<float>(Signals.RedRearIntensity));
    }
    if (IsValid(RearRightSignal))
    {
        RearRightSignal->SetIntensity(
            static_cast<float>(Signals.RedRearIntensity));
    }
    if (IsValid(ReverseSignal))
    {
        ReverseSignal->SetIntensity(
            static_cast<float>(Signals.WhiteReverseIntensity));
    }
}

void ALatebrakeCarPawn::ApplyWheelVisualPose()
{
    const latebrake::WheelVisualPose Pose = WheelMotion.Current();
    // Cylinder spindle is local Z; first lay it along chassis Y, then
    // roll around spindle and yaw only the two front decorative wheels.
    constexpr double Pi = 3.14159265358979323846;
    const double Radians = Pi / 180.0;
    const FQuat Base(FRotator(0.f, 0.f, 90.f));
    const FQuat Rolling(FVector::UpVector, Pose.RollDegrees * Radians);
    const FQuat Steering(FVector::UpVector, Pose.FrontSteerDegrees * Radians);
    UStaticMeshComponent* Wheels[] = {
        FrontLeftWheel, FrontRightWheel, RearLeftWheel, RearRightWheel
    };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        if (IsValid(Wheels[Index]))
        {
            const FQuat Visual = (Index < 2 ? Steering : FQuat::Identity) *
                Base * Rolling;
            Wheels[Index]->SetRelativeRotation(Visual.Rotator());
        }
    }
}

void ALatebrakeCarPawn::UpdateWheelVisuals(
    double DeltaSeconds, double ForwardSpeedCmPerSecond, double VisualSteering)
{
    if (WheelMotion.Advance({
            DeltaSeconds, ForwardSpeedCmPerSecond, VisualSteering, 29.0}))
    {
        ApplyWheelVisualPose();
    }
    else
    {
        // A missing/bad physics sample cannot animate decorative tires.
        ApplyWheelVisualPose();
    }
}

bool ALatebrakeCarPawn::IsRacePhysicsReady() const
{
    if (!IsValid(Chassis))
    {
        return false;
    }
    // An actor that is possessed and positioned at a start point is not
    // necessarily drivable. A missing engine primitive, disabled Chaos
    // simulation, wrong collision mode or nonfinite state must not keep
    // accumulating race time or authorize directional gate credits.
    const FVector Position = GetActorLocation();
    const FRotator Rotation = GetActorRotation();
    const FVector Velocity = Chassis->GetPhysicsLinearVelocity();
    const FVector AngularVelocity = Chassis->GetPhysicsAngularVelocityInDegrees();
    return latebrake::IsRaceVehiclePhysicsReady({
        true,
        Chassis->GetStaticMesh() != nullptr,
        Chassis->IsSimulatingPhysics(),
        Chassis->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics,
        Chassis->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Block,
        Position.X, Position.Y, Position.Z,
        Velocity.X, Velocity.Y, Velocity.Z,
        Rotation.Pitch, Rotation.Yaw, Rotation.Roll
    }) && latebrake::AreVehicleDynamicsReady({
        Chassis->GetMass(),
        AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z
    });
}

float ALatebrakeCarPawn::GetSpeedKmH() const
{
    if (!IsValid(Chassis))
    {
        return 0.f;
    }
    const FVector V = Chassis->GetPhysicsLinearVelocity();
    if (!FMath::IsFinite(V.X) || !FMath::IsFinite(V.Y))
    {
        return 0.f;
    }
    const float SpeedKmH = FVector(V.X, V.Y, 0.f).Size() * 0.036f;
    return FMath::IsFinite(SpeedKmH) ? FMath::Max(0.f, SpeedKmH) : 0.f;
}

float ALatebrakeCarPawn::GetForwardSpeedKmH() const
{
    // Never treat malformed or missing chassis motion as a valid stopped
    // launch sample. A true reverse velocity remains signed and negative.
    if (!IsValid(Chassis))
    {
        return std::numeric_limits<float>::quiet_NaN();
    }
    const FVector Velocity = Chassis->GetPhysicsLinearVelocity();
    const FVector Forward = GetActorForwardVector();
    const double ForwardKmH = FVector::DotProduct(Velocity, Forward) * 0.036;
    if (!FMath::IsFinite(ForwardKmH) ||
        ForwardKmH > static_cast<double>(std::numeric_limits<float>::max()) ||
        ForwardKmH < -static_cast<double>(std::numeric_limits<float>::max()))
    {
        return std::numeric_limits<float>::quiet_NaN();
    }
    return static_cast<float>(ForwardKmH);
}

bool ALatebrakeCarPawn::TryRecoverTo(
    const FVector& Position, const FRotator& Rotation)
{
    // Reject invalid actor/target state before modifying physics. Do not
    // claim a paid recovery succeeded just because SetActor was called.
    if (!IsValid(Chassis) ||
        !FMath::IsFinite(Position.X) ||
        !FMath::IsFinite(Position.Y) ||
        !FMath::IsFinite(Position.Z) ||
        !FMath::IsFinite(Rotation.Pitch) ||
        !FMath::IsFinite(Rotation.Yaw) ||
        !FMath::IsFinite(Rotation.Roll))
    {
        return false;
    }

    // Even a failed Unreal transform request can disturb the body state;
    // invalidate all pre-recovery checkpoint samples BEFORE attempting it.
    ++TeleportRevision;
    DrivingStats.Suspend();
    DriftMeter.Suspend();
    GForceMeter.Suspend();
    SlipAngleMeter.Suspend();
    TireCondition.Suspend();
    BrakeDistance.Suspend();
    SlipstreamBoost.Suspend();
    ApplySignalLights(false, false, 0.0);
    WheelMotion.Reset();
    ApplyWheelVisualPose();
    ClearDriverInputs();
    if (Chassis->IsSimulatingPhysics())
    {
        Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    }
    const bool bEngineMoved = SetActorLocationAndRotation(
        Position, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
    const FVector Observed = GetActorLocation();
    const FRotator ObservedRotation = GetActorRotation();
    const bool bChassisStillValid = IsValid(Chassis);
    const FVector PostLinearVelocity = bChassisStillValid
        ? Chassis->GetPhysicsLinearVelocity() : FVector::ZeroVector;
    const FVector PostAngularVelocity = bChassisStillValid
        ? Chassis->GetPhysicsAngularVelocityInDegrees() : FVector::ZeroVector;
    // Compare full orientation and the live chassis state AFTER Unreal moves
    // the actor; the pre-move readiness gate does not prove post-move health.
    const latebrake::RecoveryTeleportDecision Result =
        latebrake::EvaluateRecoveryTeleport({
            bEngineMoved,
            Position.X, Position.Y, Position.Z, Rotation.Yaw,
            Observed.X, Observed.Y, Observed.Z, ObservedRotation.Yaw,
            Rotation.Pitch, Rotation.Roll,
            ObservedRotation.Pitch, ObservedRotation.Roll,
            bChassisStillValid && IsRacePhysicsReady(),
            PostLinearVelocity.X, PostLinearVelocity.Y, PostLinearVelocity.Z,
            PostAngularVelocity.X, PostAngularVelocity.Y, PostAngularVelocity.Z
        });
    if (Result != latebrake::RecoveryTeleportDecision::Verified)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: recovery transform or physics not verified (%s); no player time charged"),
            UTF8_TO_TCHAR(latebrake::RecoveryTeleportDecisionName(Result)));
        return false;
    }
    return true;
}

void ALatebrakeCarPawn::RecoverTo(
    const FVector& Position, const FRotator& Rotation)
{
    // Existing Blueprint, initial positioning, restart and AI paths use the
    // same checked move but have no race-time recovery penalty.
    TryRecoverTo(Position, Rotation);
}

void ALatebrakeCarPawn::RecoverPressed()
{
    HandbrakeReleased();
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->RecoverPlayer(this);
    }
}

void ALatebrakeCarPawn::RestartPressed()
{
    HandbrakeReleased();
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->RestartRace();
    }
}

void ALatebrakeCarPawn::ApplyCameraPreset()
{
    // Switching any preset or rear view starts the visual response from
    // that view's unmodified FOV; no acceleration or race state is changed.
    ReactiveFov.Reset();
    if (!IsValid(CameraBoom))
    {
        return;
    }
    const latebrake::CameraPreset Requested =
        latebrake::GetCameraPreset(CameraViews.Current());
    // Fail closed on a corrupted/future preset rather than installing
    // nonfinite camera transforms into an active physics scene.
    const latebrake::CameraPreset View =
        latebrake::IsSafeCameraPreset(Requested)
            ? Requested : latebrake::GetCameraPreset(latebrake::CameraView::Chase);
    CameraBoom->TargetArmLength = static_cast<float>(View.ArmLengthCm);
    CameraBoom->SetRelativeRotation(FRotator(
        static_cast<float>(View.PitchDegrees),
        static_cast<float>(LookBackCamera.SpringArmYawDegrees()), 0.f));
    CameraBoom->SocketOffset = FVector(
        static_cast<float>(View.ForwardOffsetCm), 0.f,
        static_cast<float>(View.HeightOffsetCm));
    CameraBoom->CameraLagSpeed = static_cast<float>(View.CameraLagSpeed);
    CameraBoom->bEnableCameraLag = View.CameraLagEnabled;
    CameraBoom->bDoCollisionTest = View.CollisionProbeEnabled;
    if (IsValid(FollowCamera))
    {
        // Distinct, bounded static lens framing. No racing-speed camera
        // pumping, reward logic, physics mutation or save format change.
        FollowCamera->SetFieldOfView(
            static_cast<float>(View.FieldOfViewDegrees));
    }
}

void ALatebrakeCarPawn::ResetSpeedCameraFov()
{
    if (!ReactiveFov.HasActiveSample())
    {
        return;
    }
    ReactiveFov.Reset();
    if (IsValid(FollowCamera))
    {
        const latebrake::CameraPreset Requested =
            latebrake::GetCameraPreset(CameraViews.Current());
        const latebrake::CameraPreset View =
            latebrake::IsSafeCameraPreset(Requested)
                ? Requested : latebrake::GetCameraPreset(latebrake::CameraView::Chase);
        FollowCamera->SetFieldOfView(static_cast<float>(View.FieldOfViewDegrees));
    }
}

void ALatebrakeCarPawn::UpdateSpeedCameraFov(
    double FrameSeconds, double SignedForwardSpeedKmH)
{
    if (!IsValid(FollowCamera))
    {
        ReactiveFov.Reset();
        return;
    }
    const latebrake::CameraPreset Requested =
        latebrake::GetCameraPreset(CameraViews.Current());
    const latebrake::CameraPreset View =
        latebrake::IsSafeCameraPreset(Requested)
            ? Requested : latebrake::GetCameraPreset(latebrake::CameraView::Chase);
    // In a standalone host an AI controller can also be local. Require
    // an actual human player controller, not merely local authority.
    const bool bHumanPlayer =
        IsLocallyControlled() && Cast<APlayerController>(GetController()) != nullptr;
    const double Fov = ReactiveFov.Sample(
        View.FieldOfViewDegrees, SignedForwardSpeedKmH, FrameSeconds,
        bHumanPlayer, LookBackCamera.IsHeld());
    FollowCamera->SetFieldOfView(static_cast<float>(Fov));
}

void ALatebrakeCarPawn::ChangeCamera()
{
    CameraViews.Next();
    ApplyCameraPreset();
}

void ALatebrakeCarPawn::LookBackPressed()
{
    UWorld* World = GetWorld();
    const ALatebrakeRaceGameMode* Mode =
        World ? World->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    // The offline opponent inherits this pawn but cannot own player-only
    // rear-view input. A denied press must not be saved for a later race.
    const bool bEligible = IsLocallyControlled() && Mode &&
        Mode->IsRaceStartGreen() &&
        Mode->IsCircuitReadyForGameplay() &&
        Mode->IsRestartPlacementReady() &&
        !Mode->IsMenuOpen() &&
        !UGameplayStatics::IsGamePaused(this) &&
        !Mode->IsManualRacePauseActive() &&
        !Mode->IsFocusLossGuardActive() &&
        !Mode->GetRace().IsFinished() &&
        IsRacePhysicsReady();
    const bool bWasHeld = LookBackCamera.IsHeld();
    const bool bNowHeld = LookBackCamera.Press(bEligible);
    if (bWasHeld != bNowHeld || bNowHeld)
    {
        ApplyCameraPreset();
    }
}

void ALatebrakeCarPawn::LookBackReleased()
{
    if (LookBackCamera.IsHeld())
    {
        LookBackCamera.Reset();
        ApplyCameraPreset();
    }
}

void ALatebrakeCarPawn::ToggleBestLapGhost()
{
    // Display-only: do not neutralize acceleration or steer inputs, and
    // never let rival pawn/AI own a human ghost visibility command.
    if (ALatebrakeRaceGameMode* Mode = GetWorld()
            ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr)
    {
        Mode->ToggleBestLapGhostView();
    }
}

void ALatebrakeCarPawn::TogglePause()
{
    // The world may stop ticking immediately: clear the held brake before
    // GameMode attempts to pause or resume engine time.
    HandbrakeReleased();
    // World pause can stop Tick; restore front view immediately, not one
    // possibly dropped frame after the menu takes input ownership.
    LookBackReleased();
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
    // A duplicated Pressed event cannot fire PAUSE/RECOVER/RESTART again.
    // Unlike an unconditional early return, the shared admission policy
    // also cancels a previously captured owner on malformed duplicate
    // coordinates. Keep all other fingers and the physical-down latch.
    const int32 FingerId = static_cast<int32>(FingerIndex);
    const bool bWasPedalOwner = TouchDrive.PedalOwner() == FingerId;
    const bool bWasSteeringOwner = TouchDrive.SteeringOwner() == FingerId;
    const latebrake::TouchPressAdmission Admission =
        latebrake::AdmitTouchPress(
            TouchPresses, TouchDrive, FingerId, Location.X, Location.Y);
    if (Admission != latebrake::TouchPressAdmission::NewPress)
    {
        if (Admission == latebrake::TouchPressAdmission::MalformedDuplicate)
        {
            // Removing ownership must also zero any still-smoothed output
            // from the canceled finger before the next physics frame.
            if (bWasPedalOwner) SmoothedThrottle = 0.f;
            if (bWasSteeringOwner) SmoothedSteering = 0.f;
        }
        return;
    }
    // Forward FIRST presses (even malformed ones) so ApplyTouch still
    // observes a changed/unavailable viewport and handles menu gating.
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
        // A corrupt held touch must not leave the already-filtered motor or
        // steering command active for further physics frames.
        latebrake::CancelTouchOwnerAndOutput(
            TouchDrive, FingerId, SmoothedThrottle, SmoothedSteering);
        return;
    }

    const latebrake::TouchControl Region =
        latebrake::HitTestTouchControl(Layout, Location.X, Location.Y);
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (Mode && Mode->IsFocusLossGuardActive())
    {
        // Even if SetGamePaused fails, a backgrounded app must not accept
        // driving fingers or recovery/career/restart commands. A NEW PAUSE
        // tap is an explicit player action allowed to resume the race.
        NeutralizeMotionWhenDrivingBlocked();
        if (latebrake::CanResumeFocusSuspendedRaceFromTouch(
                bAllowAcquire, Region))
        {
            TogglePause();
        }
        return;
    }
    if (Mode && Mode->IsMenuOpen())
    {
        // Menus own focus: never acquire or move driving fingers while paused.
        // Only a distinct new press can activate the visible navigation pads.
        TouchDrive.Clear();
        if (bAllowAcquire)
        {
            if (Region == latebrake::TouchControl::Restart) MenuDetailsPrevious();
            else if (Region == latebrake::TouchControl::Recover) MenuDetailsNext();
            else if (Region == latebrake::TouchControl::CareerNext) MenuNext();
            else if (Region == latebrake::TouchControl::CareerStart) MenuActivate();
            else if (Region == latebrake::TouchControl::MenuBrowse) CareerNext();
            else if (Region == latebrake::TouchControl::Pause) MenuToggle();
        }
        return;
    }
    if (UGameplayStatics::IsGamePaused(this) ||
        (Mode && Mode->IsManualRacePauseActive()))
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
        latebrake::MoveTouchWithResidualSafety(
            TouchDrive, Layout, FingerId, Location.X, Location.Y,
            SmoothedThrottle, SmoothedSteering);
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
    case latebrake::TouchControl::GhostView:
        // A NEW physical press in a dedicated, disjoint mobile rectangle.
        // The GameMode performs the final authoritative race/pause guard.
        if (latebrake::CanToggleGhostFromTouch({
                Region, bAllowAcquire, Mode != nullptr,
                Mode && Mode->IsCircuitReadyForGameplay(),
                Mode && Mode->HasLocalPlayerDriver(),
                Mode && Mode->IsRaceStartGreen(),
                Mode && Mode->IsMenuOpen(),
                UGameplayStatics::IsGamePaused(this) ||
                    (Mode && Mode->IsManualRacePauseActive()),
                Mode && Mode->IsFocusLossGuardActive(),
                Mode && Mode->GetRace().IsFinished()}))
        {
            ToggleBestLapGhost();
        }
        return;
    case latebrake::TouchControl::MenuBrowse:
        // The same rectangle means BROWSE only in menus and CAM in racing.
        // Explicit portable admission is a second barrier after the
        // already checked one-shot press, input owners and pause guards.
        if (latebrake::CanCycleCameraFromTouch({
                Region, bAllowAcquire, Mode != nullptr,
                Mode && Mode->IsCircuitReadyForGameplay(),
                Mode && Mode->HasLocalPlayerDriver(),
                Mode && Mode->IsMenuOpen(),
                UGameplayStatics::IsGamePaused(this) ||
                    (Mode && Mode->IsManualRacePauseActive()),
                Mode && Mode->IsFocusLossGuardActive(),
                Mode && Mode->GetRace().IsFinished()}))
        {
            ChangeCamera();
        }
        return;
    default:
        break;
    }

    TouchDrive.Press(Layout, FingerId, Location.X, Location.Y);
}

void ALatebrakeCarPawn::TouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
    (void)Location;
    // An OS finger-up is stronger than an ordinary throttle/steering
    // smoothing update: clear the released owner's residual command now.
    // Other fingers (and their smoothed output) remain untouched.
    latebrake::CancelTouchOwnerAndOutput(
        TouchDrive, static_cast<int32>(FingerIndex),
        SmoothedThrottle, SmoothedSteering);
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

void ALatebrakeCarPawn::MenuDetailsNext()
{
    if (const ALatebrakeRaceGameMode* Mode =
            GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        if (Mode->IsMenuOpen())
        {
            if (APlayerController* PC = Cast<APlayerController>(GetController()))
            {
                if (ALatebrakeRaceHUD* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
                {
                    Hud->ScrollMenuDetails(1);
                }
            }
        }
    }
}

void ALatebrakeCarPawn::MenuDetailsPrevious()
{
    if (const ALatebrakeRaceGameMode* Mode =
            GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        if (Mode->IsMenuOpen())
        {
            if (APlayerController* PC = Cast<APlayerController>(GetController()))
            {
                if (ALatebrakeRaceHUD* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
                {
                    Hud->ScrollMenuDetails(-1);
                }
            }
        }
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
    PlayerInputComponent->BindAction(TEXT("Handbrake"), IE_Pressed, this, &ALatebrakeCarPawn::HandbrakePressed);
    FInputActionBinding& HandbrakeRelease = PlayerInputComponent->BindAction(
        TEXT("Handbrake"), IE_Released, this, &ALatebrakeCarPawn::HandbrakeReleased);
    HandbrakeRelease.bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("Recover"), IE_Pressed, this, &ALatebrakeCarPawn::RecoverPressed);
    PlayerInputComponent->BindAction(TEXT("RestartRace"), IE_Pressed, this, &ALatebrakeCarPawn::RestartPressed);
    PlayerInputComponent->BindAction(TEXT("Camera"), IE_Pressed, this, &ALatebrakeCarPawn::ChangeCamera);
    PlayerInputComponent->BindAction(TEXT("LookBack"), IE_Pressed, this, &ALatebrakeCarPawn::LookBackPressed);
    FInputActionBinding& LookBackRelease = PlayerInputComponent->BindAction(
        TEXT("LookBack"), IE_Released, this, &ALatebrakeCarPawn::LookBackReleased);
    LookBackRelease.bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("GhostView"), IE_Pressed, this, &ALatebrakeCarPawn::ToggleBestLapGhost);
    FInputTouchBinding& PressedTouch = PlayerInputComponent->BindTouch(
        IE_Pressed, this, &ALatebrakeCarPawn::TouchPressed);
    PressedTouch.bExecuteWhenPaused = true; // Mobile PAUSE pad can also resume play.
    PlayerInputComponent->BindTouch(IE_Repeat, this, &ALatebrakeCarPawn::TouchMoved);
    // A finger released while the menu or standalone PAUSE is active must
    // clear its physical-down slot even when the game world stops ticking.
    // Otherwise the next real press reusing that finger index is classified
    // as a duplicate and PAUSE/STEER/GAS can appear permanently unresponsive.
    // This is cleanup only: TouchReleased never triggers gameplay actions.
    FInputTouchBinding& ReleasedTouch = PlayerInputComponent->BindTouch(
        IE_Released, this, &ALatebrakeCarPawn::TouchReleased);
    ReleasedTouch.bExecuteWhenPaused = true;
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
    FInputActionBinding& DetailsNext = PlayerInputComponent->BindAction(
        TEXT("MenuDetailsNext"), IE_Pressed, this, &ALatebrakeCarPawn::MenuDetailsNext);
    DetailsNext.bExecuteWhenPaused = true;
    FInputActionBinding& DetailsPrevious = PlayerInputComponent->BindAction(
        TEXT("MenuDetailsPrevious"), IE_Pressed, this, &ALatebrakeCarPawn::MenuDetailsPrevious);
    DetailsPrevious.bExecuteWhenPaused = true;
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
