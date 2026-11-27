#include "UI/GarageShowroomLane.h"
#include "Vehicles/LatebrakeCarPawn.h"
#include "Vehicles/LatebrakeSkidTrailActor.h"
#include "Vehicles/ArcadeSkidProbeObservation.h"
#include "Vehicles/SteeringFilter.h"
#include "Vehicles/VehicleVisualRecipe.h"
#include "Vehicles/VehicleBodyKitRecipe.h"
#include "Vehicles/VehiclePaintRecipe.h"
#include "Vehicles/VehicleRearLampLayout.h"
#include "Vehicles/VehicleFrontLampRig.h"
#include "Vehicles/VehicleRearSignalRenderRig.h"
#include "Vehicles/SpeedSensitiveSteering.h"
#include "Vehicles/CornerTractionAssist.h"
#include "Vehicles/HumanSpinTractionControl.h"
#include "Vehicles/FrameStableDamping.h"
#include "Vehicles/GripLimitedBraking.h"
#include "Vehicles/LateralGripBudget.h"
#include "Vehicles/ChassisGroundContactTraction.h"
#include "Vehicles/ProgressiveRoadLoad.h"
#include "Vehicles/SpeedAwareYawTorque.h"
#include "Vehicles/HandbrakeDrivePolicy.h"
#include "Vehicles/ArcadeBoostArbitration.h"
#include "Vehicles/ArcadeReverseSpeedLimit.h"
#include "Vehicles/GearAwareMotorPower.h"
#include "Vehicles/AutomaticTransmission.h"
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
#include "UI/GarageInputFocusPolicy.h"
#include "UI/TouchViewportSafety.h"
#include "UI/TouchPressDispatch.h"
#include "UI/TouchResidualSafety.h"
#include "UI/FocusSuspendedTouchPolicy.h"
#include "Garage/GarageState.h"
#include "Vehicles/OriginalVehicleRenderSections.h"
#include "Vehicles/OriginalVehicleSurfaceShading.h"
#include "Vehicles/OriginalVehicleIndexedRender.h"
#include "Vehicles/OriginalVehicleAnimatedWheels.h"
#include "Vehicles/GarageVisualTransition.h"
#include "Vehicles/GarageShowroomPresentation.h"
#include "Vehicles/OriginalVehicleTintMaterialPolicy.h"

#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Math/RotationMatrix.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Vehicles/OriginalVehicleRoofStripe.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameters.h"
#include "UObject/UObjectGlobals.h"
#include "Game/LatebrakeRaceGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
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
    const FVector RootScale(
        static_cast<float>(latebrake::VehicleRootScaleX),
        static_cast<float>(latebrake::VehicleRootScaleY),
        static_cast<float>(latebrake::VehicleRootScaleZ));
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
    // Actual Chaos chassis contacts drive the bounded race-local damage
    // policy; decorative meshes cannot report collision damage.
    Chassis->SetNotifyRigidBodyCollision(true);
    Chassis->SetEnableGravity(true);
    // Low baseline damping preserves coasting; progressive speed-squared
    // drag/rolling load is applied during qualified driving frames below.
    Chassis->SetLinearDamping(
        static_cast<float>(latebrake::RoadLoadChassisLinearDamping));
    Chassis->SetAngularDamping(
        static_cast<float>(latebrake::SteeringChassisAngularDamping));
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
    const FVector RootScale(
        static_cast<float>(latebrake::VehicleRootScaleX),
        static_cast<float>(latebrake::VehicleRootScaleY),
        static_cast<float>(latebrake::VehicleRootScaleZ));
    Chassis->SetRelativeScale3D(RootScale);

    OriginalVehicleVisual =
        CreateDefaultSubobject<UProceduralMeshComponent>(
            TEXT("OriginalVehicleVisual"));
    OriginalVehicleVisual->SetupAttachment(Chassis);
    OriginalVehicleVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OriginalVehicleVisual->SetGenerateOverlapEvents(false);
    OriginalVehicleVisual->SetSimulatePhysics(false);
    OriginalVehicleVisual->SetVisibility(false, false);
    OriginalVehicleVisual->SetCastShadow(true);
    // Real source mesh uses centimetres in the car's original local space,
    // wheel centres at Z=40. Existing prototype wheel centres are Z=-22.
    // Cancel the chassis cube's nonuniform physics-root scale; do not
    // replace or resize that authoritative Chaos collision shape.
    OriginalVehicleVisual->SetRelativeLocation(
        FVector(0.f, 0.f,
            static_cast<float>(
                latebrake::vehiclemesh::OriginalVehicleVisualOffsetZCm))
            / RootScale);
    OriginalVehicleVisual->SetRelativeScale3D(
        FVector(1.f / RootScale.X, 1.f / RootScale.Y,
                1.f / RootScale.Z));

    OriginalRoofStripes = CreateDefaultSubobject<UProceduralMeshComponent>(
        TEXT("OriginalRoofStripes"));
    OriginalRoofStripes->SetupAttachment(OriginalVehicleVisual);
    OriginalRoofStripes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OriginalRoofStripes->SetGenerateOverlapEvents(false);
    OriginalRoofStripes->SetSimulatePhysics(false);
    OriginalRoofStripes->SetCastShadow(false);
    OriginalRoofStripes->SetVisibility(false, false);

    // Each source wheel is a distinct child of the inverse-scaled body
    // render root. Its relative location is the true mesh wheel center,
    // and uploaded triangle positions are centered on that pivot. This
    // enables rolling around local Y + front steering around local Z
    // with NO expensive geometry uploads during vehicle Tick.
    OriginalFrontLeftWheel = CreateDefaultSubobject<UProceduralMeshComponent>(
        TEXT("OriginalFrontLeftWheel"));
    OriginalFrontRightWheel = CreateDefaultSubobject<UProceduralMeshComponent>(
        TEXT("OriginalFrontRightWheel"));
    OriginalRearLeftWheel = CreateDefaultSubobject<UProceduralMeshComponent>(
        TEXT("OriginalRearLeftWheel"));
    OriginalRearRightWheel = CreateDefaultSubobject<UProceduralMeshComponent>(
        TEXT("OriginalRearRightWheel"));
    UProceduralMeshComponent* AnimatedWheels[] = {
        OriginalFrontLeftWheel.Get(), OriginalFrontRightWheel.Get(),
        OriginalRearLeftWheel.Get(), OriginalRearRightWheel.Get()};
    for (int32 i=0;i<4;++i)
    {
        auto* Wheel = AnimatedWheels[i];
        Wheel->SetupAttachment(OriginalVehicleVisual);
        const auto Pivot = latebrake::vehiclemesh::OriginalWheelPivot(i);
        Wheel->SetRelativeLocation(FVector(
            static_cast<float>(Pivot.X),
            static_cast<float>(Pivot.Y),
            static_cast<float>(Pivot.Z)));
        Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Wheel->SetGenerateOverlapEvents(false);
        Wheel->SetSimulatePhysics(false);
        Wheel->SetVisibility(false, false);
        Wheel->SetCastShadow(true);
    }

    Cabin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cabin"));
    Cabin->SetupAttachment(Chassis);
    if (Cube.Succeeded())
    {
        ConfigureDecorativeMesh(Cabin, Cube.Object, FVector(-20.f, 0.f, 52.f), FVector(1.75f, 1.48f, 0.55f));
    }

    // Two lightweight visible roof stripes, attached to the chassis.
    // Neither uses overlap/collision, body simulation, or gameplay forces.
    RoofStripeLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoofStripeLeft"));
    RoofStripeRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoofStripeRight"));
    for (UStaticMeshComponent* Stripe :
         {RoofStripeLeft.Get(), RoofStripeRight.Get()})
    {
        Stripe->SetupAttachment(Chassis);
        Stripe->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Stripe->SetGenerateOverlapEvents(false);
        Stripe->SetSimulatePhysics(false);
        Stripe->SetCastShadow(false);
        Stripe->SetVisibility(false, true);
        if (Cube.Succeeded())
        {
            ConfigureDecorativeMesh(Stripe, Cube.Object,
                FVector(-20.f, 0.f, 83.f), FVector(1.2f, 0.05f, 0.035f));
        }
    }

    // Original, collision-free procedural body accents. These shapes do not
    // replace the physically simulated chassis or change its contact body.
    Hood = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hood"));
    RearDeck = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearDeck"));
    SpoilerWing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpoilerWing"));
    SpoilerSupportLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpoilerSupportLeft"));
    SpoilerSupportRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpoilerSupportRight"));
    Hood->SetupAttachment(Chassis);
    RearDeck->SetupAttachment(Chassis);
    SpoilerWing->SetupAttachment(Chassis);
    SpoilerSupportLeft->SetupAttachment(Chassis);
    SpoilerSupportRight->SetupAttachment(Chassis);
    // Explicitly stay non-colliding even if the built-in mesh is missing.
    for (UStaticMeshComponent* Cosmetic : {
             Hood.Get(), RearDeck.Get(), SpoilerWing.Get(),
             SpoilerSupportLeft.Get(), SpoilerSupportRight.Get()})
    {
        Cosmetic->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Cosmetic->SetSimulatePhysics(false);
    }
    if (Cube.Succeeded())
    {
        ConfigureDecorativeMesh(Hood, Cube.Object,
            FVector(120.f, 0.f, 39.f), FVector(1.20f, 1.50f, 0.26f));
        ConfigureDecorativeMesh(RearDeck, Cube.Object,
            FVector(-125.f, 0.f, 38.f), FVector(1.12f, 1.42f, 0.25f));
        ConfigureDecorativeMesh(SpoilerWing, Cube.Object,
            FVector(-158.f, 0.f, 102.f), FVector(0.18f, 1.60f, 0.13f));
        ConfigureDecorativeMesh(SpoilerSupportLeft, Cube.Object,
            FVector(-158.f, -58.f, 74.f), FVector(0.12f, 0.12f, 0.50f));
        ConfigureDecorativeMesh(SpoilerSupportRight, Cube.Object,
            FVector(-158.f, 58.f, 74.f), FVector(0.12f, 0.12f, 0.50f));
    }

    // Seven additional original geometric details distinguish the six
    // garage silhouettes. They use only engine-provided primitive meshes,
    // never own contact physics, and cannot affect race timing or rewards.
    FrontSplitter = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontSplitter"));
    LeftSideSkirt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftSideSkirt"));
    RightSideSkirt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightSideSkirt"));
    RearDiffuser = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearDiffuser"));
    HoodScoop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HoodScoop"));
    LeftRearFin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRearFin"));
    RightRearFin = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRearFin"));
    for (UStaticMeshComponent* Trim : {
             FrontSplitter.Get(), LeftSideSkirt.Get(),
             RightSideSkirt.Get(), RearDiffuser.Get(), HoodScoop.Get(),
             LeftRearFin.Get(), RightRearFin.Get()})
    {
        Trim->SetupAttachment(Chassis);
        Trim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Trim->SetGenerateOverlapEvents(false);
        Trim->SetSimulatePhysics(false);
        // Dynamic shadow cost for a seven-piece decorative kit is not
        // justified by this prototype's visual fidelity.
        Trim->SetCastShadow(false);
        if (Cube.Succeeded())
        {
            ConfigureDecorativeMesh(Trim, Cube.Object,
                FVector::ZeroVector, FVector(0.10f, 0.10f, 0.10f));
        }
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
    // The actual stock Cube chassis has a 190-cm rear half-length and
    // 90-cm half-width. The old X=-385/-390cm origins were 195-200cm
    // behind the bumper: beyond these lamps' 170cm attenuation radius.
    // Keep all three points 4-6cm behind the real physics shell instead.
    const auto& Rear = latebrake::VehicleRearLampOffsets;
    const FVector LampLocations[] = {
        FVector(Rear[0].X, Rear[0].Y, Rear[0].Z),
        FVector(Rear[1].X, Rear[1].Y, Rear[1].Z),
        FVector(Rear[2].X, Rear[2].Y, Rear[2].Z)
    };
    const FVector ChassisScale(
        static_cast<float>(latebrake::VehicleRootScaleX),
        static_cast<float>(latebrake::VehicleRootScaleY),
        static_cast<float>(latebrake::VehicleRootScaleZ));
    for (int32 Index = 0; Index < 3; ++Index)
    {
        Lamps[Index]->SetupAttachment(Chassis);
        Lamps[Index]->SetRelativeLocation(LampLocations[Index] / ChassisScale);
        Lamps[Index]->SetAttenuationRadius(
            static_cast<float>(latebrake::VehicleRearLampRadiusCm));
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

    // Real movable Unreal SpotLight components: two road-facing lamps
    // follow the owned car's visible front bumper. Cosmetic light only.
    // No new actors, collision components, dynamic shadows or tick loops.
    FrontLeftLamp = CreateDefaultSubobject<USpotLightComponent>(
        TEXT("FrontLeftLamp"));
    FrontRightLamp = CreateDefaultSubobject<USpotLightComponent>(
        TEXT("FrontRightLamp"));
    const auto FrontPlan = latebrake::PlanFrontLampRig(false,false);
    USpotLightComponent* FrontLamps[] = {
        FrontLeftLamp.Get(), FrontRightLamp.Get()};
    for (int32 i=0;i<2;++i)
    {
        USpotLightComponent* Lamp=FrontLamps[i];
        Lamp->SetupAttachment(Chassis);
        Lamp->SetMobility(EComponentMobility::Movable);
        const auto& P=FrontPlan.Mounts[static_cast<std::size_t>(i)];
        Lamp->SetRelativeLocation(FVector(P.Xcm,P.Ycm,P.Zcm)/ChassisScale);
        Lamp->SetRelativeRotation(FRotator(
            static_cast<float>(FrontPlan.PitchDegrees),0.f,0.f));
        Lamp->SetInnerConeAngle(
            static_cast<float>(FrontPlan.BeamInnerDegrees));
        Lamp->SetOuterConeAngle(
            static_cast<float>(FrontPlan.BeamOuterDegrees));
        Lamp->SetAttenuationRadius(
            static_cast<float>(FrontPlan.AttenuationRadiusCm));
        Lamp->SetCastShadows(false);
        Lamp->SetCastVolumetricShadow(false);
        Lamp->SetAffectGlobalIllumination(false);
        Lamp->SetAffectReflection(false);
        Lamp->SetLightColor(FLinearColor(0.90f,0.95f,1.f));
        Lamp->SetIntensity(0.f);
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
    ApplyVehicleVisualRecipe(0);
}

void ALatebrakeCarPawn::BeginPlay()
{
    Super::BeginPlay();
    // Dynamic material instances belong to these mesh components, and are
    // initialized only when the actor is live (not in the CDO constructor).
    // A showroom request can arrive before BeginPlay. The portable
    // transition records its transient identity but deliberately defers
    // material/mesh allocation. Use the SAME displayed identity for
    // shape, geometry and palette, not a mixture of equipped and viewed.
    const int32 DisplayVehicleId = GetPresentedGarageVehicleId();
    ApplyVehicleVisualRecipe(DisplayVehicleId);
    RebuildOriginalVehicleVisual(DisplayVehicleId);
    ApplyVehiclePaintRecipe(DisplayVehicleId);
    Chassis->OnComponentHit.AddDynamic(this, &ALatebrakeCarPawn::HandleChassisHit);
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
    if (IsValid(Chassis))
    {
        Chassis->OnComponentHit.RemoveDynamic(this, &ALatebrakeCarPawn::HandleChassisHit);
    }
    ApplySignalLights(false, false, 0.0);
    FCoreDelegates::ApplicationWillEnterBackgroundDelegate.Remove(BackgroundInputHandle);
    FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivatedInputHandle);
    FCoreDelegates::ApplicationHasEnteredForegroundDelegate.Remove(ForegroundInputHandle);
    TouchViewport.Reset();
    ClearDriverInputs();
    if (IsValid(SkidTrailActor)) SkidTrailActor->Destroy();
    SkidTrailActor=nullptr;
    OriginalRenderCache.Reset();
    GarageCameraGeometry.Reset();
    GarageCameraFit.Reset();
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

void ALatebrakeCarPawn::HandleChassisHit(
    UPrimitiveComponent* HitComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, FVector NormalImpulse,
    const FHitResult& Hit)
{
    // Collision notifications during pre-GO, pauses, failed restarts,
    // spawned overlaps or unhealthy Chaos states are NOT race impacts.
    UWorld* World = GetWorld();
    const ALatebrakeRaceGameMode* Mode =
        World ? World->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !Mode->IsRaceStartGreen() ||
        !Mode->IsCircuitReadyForGameplay() || !Mode->IsRestartPlacementReady() ||
        Mode->IsMenuOpen() || Mode->IsFocusLossGuardActive() ||
        Mode->IsManualRacePauseActive() || Mode->GetRace().IsFinished() ||
        UGameplayStatics::IsGamePaused(this) ||
        HitComponent != Chassis.Get() || !IsValid(OtherActor) ||
        OtherActor == this || OtherComp == Chassis.Get() ||
        !IsRacePhysicsReady() || !CollisionDamage.HasEligibleFrame())
    {
        return;
    }

    // The latest eligible pre-contact chassis speed protects against an
    // impulse callback arriving after the physics body lost momentum.
    // It is explicitly cleared by interruption, recovery and invalid frames.
    const FVector Velocity = Chassis->GetPhysicsLinearVelocity();
    const double LiveSpeed = FVector(Velocity.X, Velocity.Y, 0.0).Size();
    if (!FMath::IsFinite(LiveSpeed))
    {
        return;
    }
    const double Speed = FMath::Max(
        LiveSpeed, LastQualifiedPlanarSpeedCmPerSecond);
    CollisionDamage.ObserveImpact({
        static_cast<double>(NormalImpulse.Size()),
        static_cast<double>(Chassis->GetMass()), Speed,
        static_cast<double>(Hit.ImpactNormal.Z), true});
}

void ALatebrakeCarPawn::SetAutonomousControls(float InThrottle, float InSteering, bool bInBrake)
{
    Throttle = static_cast<float>(latebrake::SanitizeDriveAxis(InThrottle));
    Steering = static_cast<float>(latebrake::SanitizeDriveAxis(InSteering));
    bBraking = bInBrake;
}

void ALatebrakeCarPawn::ApplyVehicleVisualRecipe(int32 VehicleId)
{
    const latebrake::VehicleSpec Spec =
        latebrake::GarageState::Definition(VehicleId);
    const latebrake::VehicleVisualRecipe Recipe =
        latebrake::VisualRecipeForVehicle(VehicleId);
    const latebrake::VehicleBodyKitRecipe Kit =
        latebrake::BodyKitForVehicle(VehicleId);
    // Chassis primitive uses a 3.8 x 1.8 x 0.65 non-uniform scale.
    // Compensate its parent transform, just like ConfigureDecorativeMesh.
    const FVector RootScale(
        static_cast<float>(latebrake::VehicleRootScaleX),
        static_cast<float>(latebrake::VehicleRootScaleY),
        static_cast<float>(latebrake::VehicleRootScaleZ));
    if (IsValid(Cabin))
    {
        Cabin->SetRelativeScale3D(FVector(
            1.75 * Spec.CabinLength / RootScale.X,
            1.48 * Recipe.RoofWidthScale / RootScale.Y,
            0.55 * Spec.CabinHeight / RootScale.Z));
    }
    if (IsValid(Hood))
    {
        Hood->SetRelativeScale3D(FVector(
            1.20 * Recipe.HoodLengthScale / RootScale.X,
            1.50 / RootScale.Y, 0.26 / RootScale.Z));
    }
    if (IsValid(RearDeck))
    {
        RearDeck->SetRelativeScale3D(FVector(
            1.12 * Recipe.RearDeckLengthScale / RootScale.X,
            1.42 / RootScale.Y, 0.25 / RootScale.Z));
    }

    // The wing and both supports are a single cosmetic on/off group.
    // Set visibility even when engine primitive loading was unavailable.
    UStaticMeshComponent* WingParts[] = {
        SpoilerWing, SpoilerSupportLeft, SpoilerSupportRight};
    for (UStaticMeshComponent* Part : WingParts)
    {
        if (IsValid(Part))
        {
            Part->SetVisibility(Recipe.HasRearWing, true);
            Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
    if (Recipe.HasRearWing && IsValid(SpoilerWing))
    {
        SpoilerWing->SetRelativeLocation(
            FVector(-158.0, 0.0, Recipe.WingHeightCm) / RootScale);
        SpoilerWing->SetRelativeScale3D(FVector(
            0.18 / RootScale.X,
            1.60 * Recipe.WingSpanScale / RootScale.Y,
            0.13 / RootScale.Z));
    }
    if (Recipe.HasRearWing)
    {
        for (UStaticMeshComponent* Part :
             {SpoilerSupportLeft.Get(), SpoilerSupportRight.Get()})
        {
            if (IsValid(Part))
            {
                const double Side = (Part == SpoilerSupportLeft.Get()
                    ? -58.0 : 58.0) * Recipe.WingSpanScale;
                Part->SetRelativeLocation(FVector(
                    -158.0, Side, Recipe.WingHeightCm - 28.0) / RootScale);
            }
        }
    }

    // Apply a complete deterministic visual recipe whenever the selected
    // owned car changes, including switching FROM a fin/scoop-equipped car
    // back to Arrow. Every primitive stays attached and non-colliding.
    const auto SetTrim = [&RootScale](UStaticMeshComponent* Part,
        const FVector& CenterCm, const FVector& Scale, bool bVisible)
    {
        if (!IsValid(Part)) return;
        Part->SetRelativeLocation(CenterCm / RootScale);
        Part->SetRelativeScale3D(Scale / RootScale);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetVisibility(bVisible, true);
    };
    SetTrim(FrontSplitter, FVector(183.f, 0.f, -27.f),
        FVector(0.42, 1.48 * Kit.SplitterWidthScale, 0.09), true);
    SetTrim(LeftSideSkirt, FVector(0.f, -89.f, -27.f),
        FVector(1.90 * Kit.SideSkirtLengthScale, 0.10, 0.13), true);
    SetTrim(RightSideSkirt, FVector(0.f, 89.f, -27.f),
        FVector(1.90 * Kit.SideSkirtLengthScale, 0.10, 0.13), true);
    SetTrim(RearDiffuser, FVector(-183.f, 0.f, -27.f),
        FVector(0.37, 1.41 * Kit.DiffuserWidthScale, 0.11), true);
    SetTrim(HoodScoop, FVector(90.f, 0.f, 55.f + Kit.ScoopHeightCm * 0.5),
        FVector(0.52, 0.48, FMath::Max(1.0, Kit.ScoopHeightCm) / 100.0),
        Kit.HasHoodScoop);
    SetTrim(LeftRearFin, FVector(-166.f, -66.f,
            55.f + Kit.RearFinHeightCm * 0.5),
        FVector(0.20, 0.11,
            FMath::Max(1.0, Kit.RearFinHeightCm) / 100.0),
        Kit.HasRearFins);
    SetTrim(RightRearFin, FVector(-166.f, 66.f,
            55.f + Kit.RearFinHeightCm * 0.5),
        FVector(0.20, 0.11,
            FMath::Max(1.0, Kit.RearFinHeightCm) / 100.0),
        Kit.HasRearFins);
}

void ALatebrakeCarPawn::HideLegacyVisualPrimitives(bool bHide)
{
    // Do NOT propagate hiding from physics root to procedural child.
    // HiddenInGame affects presentation, not collision, force, speed or
    // authoritative vehicle/garage state. Every fallback restores it.
    for (UStaticMeshComponent* Part : {
        Chassis.Get(), Cabin.Get(), Hood.Get(), RearDeck.Get(),
        SpoilerWing.Get(), SpoilerSupportLeft.Get(),
        SpoilerSupportRight.Get(), FrontSplitter.Get(),
        LeftSideSkirt.Get(), RightSideSkirt.Get(), RearDiffuser.Get(),
        HoodScoop.Get(), LeftRearFin.Get(), RightRearFin.Get(),
        FrontLeftWheel.Get(), FrontRightWheel.Get(),
        RearLeftWheel.Get(), RearRightWheel.Get(),
        FrontLeftSpoke.Get(), FrontRightSpoke.Get(),
        RearLeftSpoke.Get(), RearRightSpoke.Get()
    })
    {
        if (IsValid(Part)) Part->SetHiddenInGame(bHide, false);
    }
}

namespace
{
uint32 DiscoverOriginalVehicleFinishParameters(UMaterialInterface* Material)
{
    if (!IsValid(Material)) return 0u;
    TArray<FMaterialParameterInfo> ScalarInfo;
    TArray<FGuid> ScalarGuids;
    Material->GetAllScalarParameterInfo(ScalarInfo,ScalarGuids);
    uint32 Mask=0u;
    for (std::size_t i=0;i<latebrake::vehiclemesh::VehicleFinishParameterNames.size();++i)
    {
        const FName Name(UTF8_TO_TCHAR(
            latebrake::vehiclemesh::VehicleFinishParameterNames[i]));
        if (ScalarInfo.ContainsByPredicate([&Name](const FMaterialParameterInfo& Info)
            {
                return Info.Name==Name &&
                    Info.Association==EMaterialParameterAssociation::GlobalParameter;
            }))
            Mask|=1u<<i;
    }
    return Mask;
}

void ApplyOriginalVehicleFinish(UMaterialInstanceDynamic* Material,uint32 Exposed,
    latebrake::vehiclemesh::VehicleFinishScalars Finish)
{
    if (!IsValid(Material)) return;
    const auto Plan=latebrake::vehiclemesh::PlanVehicleFinishWrites(Exposed,Finish);
    for (std::size_t i=0;i<Plan.Count;++i)
    {
        const auto& Write=Plan.Writes[i];
        const FName Name(UTF8_TO_TCHAR(latebrake::vehiclemesh::VehicleFinishParameterNames[
            static_cast<std::size_t>(Write.Parameter)]));
        Material->SetScalarParameterValue(Name,static_cast<float>(Write.Value));
    }
}
}

void ALatebrakeCarPawn::ClearOriginalRoofStripes()
{
    if (!IsValid(OriginalRoofStripes)) return;
    OriginalRoofStripes->SetVisibility(false, false);
    OriginalRoofStripes->ClearAllMeshSections();
    OriginalRoofStripes->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OriginalRoofStripes->SetGenerateOverlapEvents(false);
    OriginalRoofStripes->SetSimulatePhysics(false);
}

bool ALatebrakeCarPawn::PrepareOriginalRoofStripes(
    int32 VehicleId, UMaterialInterface* Material)
{
    if (!IsValid(OriginalRoofStripes) || !IsValid(Material)) return false;
    const auto Plan=latebrake::vehiclemesh::BuildOriginalRoofStripePlan(VehicleId);
    if (!Plan.Valid) return false;
    // All three bands are prepared while hidden. Appearance updates
    // only switch section visibility and tint, never recompose geometry.
    for (std::size_t i=0; i<Plan.Bands.size(); ++i)
    {
        const auto& Band=Plan.Bands[i];
        TArray<FVector> Positions, Normals;
        TArray<int32> Indices;
        TArray<FVector2D> UV0;
        TArray<FColor> Colors;
        TArray<FProcMeshTangent> Tangents;
        const int32 VertexCount=static_cast<int32>(Band.Count*4u);
        Positions.Reserve(VertexCount);
        Normals.Reserve(VertexCount);
        UV0.Reserve(VertexCount);
        Colors.Reserve(VertexCount);
        Tangents.Reserve(VertexCount);
        Indices.Reserve(static_cast<int32>(Band.Count*6u));
        for (std::size_t j=0;j<Band.Count;++j)
        {
            const auto& Quad=Band.Quads[j];
            const int32 Base=Positions.Num();
            for (std::size_t k=0;k<Quad.Points.size();++k)
            {
                const auto& P=Quad.Points[k];
                Positions.Add(FVector(P.X,P.Y,P.Z));
                Normals.Add(FVector(Quad.Normal.X,Quad.Normal.Y,Quad.Normal.Z));
                Tangents.Add(FProcMeshTangent(
                    FVector(Quad.Tangent.X,Quad.Tangent.Y,Quad.Tangent.Z),false));
                UV0.Add(FVector2D((P.X+70.0)/140.0,k<2u?0.0:1.0));
                Colors.Add(FColor::White);
            }
            for (int32 Index:{0,1,2,0,2,3}) Indices.Add(Base+Index);
        }
        const int32 Section=static_cast<int32>(i);
        OriginalRoofStripes->CreateMeshSection(Section,Positions,Indices,
            Normals,UV0,Colors,Tangents,false /* visual only */);
        if (!IsValid(OriginalRoofStripes->CreateDynamicMaterialInstance(Section,Material)))
        {
            ClearOriginalRoofStripes();
            return false;
        }
        OriginalRoofStripes->SetMeshSectionVisible(Section,false);
    }
    return true;
}

void ALatebrakeCarPawn::ApplyOriginalRoofStripeLook(latebrake::GarageLook Look)
{
    if (!IsValid(OriginalRoofStripes)) return;
    if (!bOriginalVehicleVisualReady || OriginalVehicleTintParameter.IsNone() ||
        !latebrake::ValidGarageLook(Look))
    {
        OriginalRoofStripes->SetVisibility(false,false);
        return;
    }
    const auto Rgb=latebrake::StripeTintForGarageLook(Look);
    const FLinearColor Tint(static_cast<float>(Rgb.R),static_cast<float>(Rgb.G),
        static_cast<float>(Rgb.B),1.f);
    for (std::size_t i=0;i<latebrake::vehiclemesh::OriginalRoofStripeBandCount;++i)
    {
        const int32 Section=static_cast<int32>(i);
        const bool bVisible=latebrake::vehiclemesh::OriginalRoofStripeVisible(Look.Stripe,i);
        OriginalRoofStripes->SetMeshSectionVisible(Section,bVisible);
        if (auto* Instance=Cast<UMaterialInstanceDynamic>(OriginalRoofStripes->GetMaterial(Section)))
        {
            Instance->SetVectorParameterValue(OriginalVehicleTintParameter,Tint);
            ApplyOriginalVehicleFinish(Instance,OriginalVehicleFinishMask,
                latebrake::vehiclemesh::OriginalStripeFinish(Look));
        }
    }
    OriginalRoofStripes->SetVisibility(Look.Stripe!=0u,false);
}

void ALatebrakeCarPawn::RebuildOriginalVehicleVisual(int32 VehicleId)
{
    // Actual 3D source rebuild attempts are counted once at entry.
    // Failure paths never increment the committed metric.
    SourceMeshBuildTelemetry =
        latebrake::OriginalMeshBuildAttempt(SourceMeshBuildTelemetry);
    bOriginalVehicleVisualReady = false;
    ClearOriginalRoofStripes();
    OriginalRenderedRimStyle = -1;
    OriginalVehicleTintParameter = NAME_None;
    OriginalVehicleFinishMask = 0u;
    GarageCameraGeometry.Reset();
    GarageCameraFit.Reset();
    bGarageCameraViewPresented = false;
    UProceduralMeshComponent* RenderParts[] = {
        OriginalVehicleVisual.Get(), OriginalFrontLeftWheel.Get(),
        OriginalFrontRightWheel.Get(), OriginalRearLeftWheel.Get(),
        OriginalRearRightWheel.Get()};
    bool bAllRenderPartsPresent = true;
    for (auto* Part : RenderParts)
    {
        if (!IsValid(Part))
        {
            // Do not return here. A missing front wheel must not leave
            // the remaining old rear wheels (or body) visible over the
            // legacy fallback. Clean EVERY surviving component first.
            bAllRenderPartsPresent = false;
            continue;
        }
        Part->SetVisibility(false, false);
        Part->ClearAllMeshSections();
        // This is a visual adapter, including all four articulated
        // wheels: defensively re-apply NO collision/overlap/physics even
        // if an Editor component override was changed on this Pawn.
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetSimulatePhysics(false);
    }
    HideLegacyVisualPrimitives(false);
    if (!bAllRenderPartsPresent || !latebrake::GarageState::IsVehicle(VehicleId))
        return;

    // Use the exact engine material that the starter Cube expects, but
    // never claim this is a professionally authored PBR material. If
    // missing in a target UE build, preserve the old visible fallback.
    UMaterialInterface* BaseMaterial =
        LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!IsValid(BaseMaterial)) return;

    // UE material asset paths do not guarantee a particular vector
    // parameter name. In some engine installations the BasicShape
    // tint is named "Colour", not "Color". Only render the new 3D
    // mesh if the actual loaded material reports a supported tint.
    TArray<FMaterialParameterInfo> VectorInfo;
    TArray<FGuid> VectorGuids;
    BaseMaterial->GetAllVectorParameterInfo(VectorInfo, VectorGuids);
    std::uint32_t AvailableTintMask = 0u;
    for (std::size_t i=0;
         i<latebrake::vehiclemesh::SupportedVehicleTintNames.size();
         ++i)
    {
        const FName Candidate(UTF8_TO_TCHAR(
            latebrake::vehiclemesh::SupportedVehicleTintNames[i]));
        if (VectorInfo.ContainsByPredicate(
            [&Candidate](const FMaterialParameterInfo& Info)
            {
                return Info.Name == Candidate;
            }))
        {
            AvailableTintMask |= 1u << i;
        }
    }
    const int TintIndex =
        latebrake::vehiclemesh::SelectVehicleTintCandidate(
            AvailableTintMask);
    if (TintIndex < 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Latebrake: original vehicle material has no supported color parameter; keeping visible legacy car"));
        return;
    }
    const FName ResolvedTintName(UTF8_TO_TCHAR(
        latebrake::vehiclemesh::SupportedVehicleTintNames[
            static_cast<std::size_t>(TintIndex)]));

    // This key contains only the authored geometry identity. Cosmetic
    // palette edits still use dynamic materials, and every actual upload
    // attempt (including a CPU cache hit) is counted by the older metric.
    const auto* Prepared = OriginalRenderCache.Acquire(
        {VehicleId,static_cast<int>(GetPresentedGarageLook().Wheels)},
        [](latebrake::vehiclemesh::OriginalRenderKey Key)
            -> latebrake::vehiclemesh::OriginalPreparedVehicleRender
        {
            const auto generated =
                latebrake::vehiclemesh::OriginalVehicleSurfaceComposer{}.Build(
                    Key.VehicleId,Key.RimStyle);
            if (!generated.Valid || !generated.Mesh.Ready ||
                generated.Mesh.ChangesChaosContact)
                return {};
            const auto shading =
                latebrake::vehiclemesh::BuildOriginalSurfaceShading(generated.Mesh);
            if (!shading.Valid || shading.Frames.size() != generated.Mesh.Vertices.size())
                return {};
            const auto sections =
                latebrake::vehiclemesh::MakeOriginalVehicleRenderSections(
                    generated.Mesh);
            if (!sections.Valid ||
                sections.TotalTriangles != generated.Mesh.Triangles.size())
                return {};
            const auto animated =
                latebrake::vehiclemesh::MakeOriginalVehicleAnimatedParts(
                    generated.Mesh);
            if (!animated.Valid ||
                animated.TotalTriangles != generated.Mesh.Triangles.size())
                return {};
            return latebrake::vehiclemesh::OriginalPreparedVehicleRender::Prepare(
                generated.Mesh, shading, animated);
        });
    if (!Prepared || !Prepared->Valid()) return;
    const auto& Packed = Prepared->Packed();
    if (!Packed.Valid || Packed.IndexCount == 0u) return;

    // All 5 components are invisible until EVERY live part has complete
    // material sections. On an allocation failure, clear the entire
    // attempted group and retain the previously working primitive car.
    bool bAnySection = false;
    bool bAllMaterialsReady = true;
    for (std::size_t partIndex=0;
         partIndex<latebrake::vehiclemesh::OriginalVehicleAnimatedPartCount;
         ++partIndex)
    {
        UProceduralMeshComponent* Part = RenderParts[partIndex];
        const auto& part=Packed.Parts[partIndex];
        for (std::size_t section=0;
             section<latebrake::vehiclemesh::OriginalVehicleSectionCount;
             ++section)
        {
            const auto& Data=part.Sections[section];
            if (Data.Indices.empty()) continue;
            TArray<FVector> Positions;
            TArray<int32> Indices;
            TArray<FVector> Normals;
            TArray<FVector2D> UV0;
            TArray<FColor> VertexColors;
            TArray<FProcMeshTangent> Tangents;
            const int32 Capacity=static_cast<int32>(Data.Vertices.size());
            Positions.Reserve(Capacity);
            Indices.Reserve(static_cast<int32>(Data.Indices.size()));
            Normals.Reserve(Capacity);
            UV0.Reserve(Capacity);
            VertexColors.Reserve(Capacity);
            Tangents.Reserve(Capacity);
            // The portable preflight already subtracted each wheel's
            // exact source pivot and proved the expanded triangle stream.
            for (const auto& vertex:Data.Vertices)
            {
                Positions.Add(FVector(vertex.Position.X,
                    vertex.Position.Y,vertex.Position.Z));
                const auto& Surface = vertex.Surface;
                Normals.Add(FVector(Surface.Normal.X,
                                    Surface.Normal.Y,
                                    Surface.Normal.Z));
                Tangents.Add(FProcMeshTangent(
                    FVector(Surface.Tangent.X,Surface.Tangent.Y,Surface.Tangent.Z),
                    Surface.FlipTangentY));
                UV0.Add(FVector2D(vertex.TexCoord.U,vertex.TexCoord.V));
                VertexColors.Add(FColor::White);
            }
            for (const auto Index:Data.Indices)
                Indices.Add(static_cast<int32>(Index));
            Part->CreateMeshSection(
                static_cast<int32>(section),
                Positions, Indices, Normals, UV0, VertexColors,
                Tangents, false /* no collision, ever */);
            UMaterialInstanceDynamic* Instance =
                Part->CreateDynamicMaterialInstance(
                    static_cast<int32>(section), BaseMaterial);
            if (!IsValid(Instance))
            {
                bAllMaterialsReady = false;
                break;
            }
            bAnySection = true;
        }
        if (!bAllMaterialsReady) break;
    }
    if (!bAllMaterialsReady)
    {
        for (auto* Part : RenderParts)
            Part->ClearAllMeshSections();
        HideLegacyVisualPrimitives(false);
        return;
    }
    if (!latebrake::vehiclemesh::CanPresentOriginalVehicleVisual(
        bAnySection, true /* each material allocation succeeded */,
        !ResolvedTintName.IsNone()))
    {
        for (auto* Part : RenderParts)
            Part->ClearAllMeshSections();
        HideLegacyVisualPrimitives(false);
        return;
    }
    if (!PrepareOriginalRoofStripes(VehicleId,BaseMaterial))
    {
        for (auto* Part : RenderParts)
            Part->ClearAllMeshSections();
        ClearOriginalRoofStripes();
        HideLegacyVisualPrimitives(false);
        return;
    }
    OriginalVehicleTintParameter = ResolvedTintName;
    OriginalVehicleFinishMask = DiscoverOriginalVehicleFinishParameters(BaseMaterial);

    // Actor-owned render component follows the real car rigid body, but
    // cannot add a Chaos mesh/collision volume or modify its physics mass.
    OriginalVehicleVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OriginalVehicleVisual->SetGenerateOverlapEvents(false);
    OriginalVehicleVisual->SetSimulatePhysics(false);
    bOriginalVehicleVisualReady = true;
    HideLegacyVisualPrimitives(true);
    OriginalVehicleVisual->SetVisibility(true, false);
    for (std::size_t i=1;i<
         latebrake::vehiclemesh::OriginalVehicleAnimatedPartCount;++i)
        RenderParts[i]->SetVisibility(true, false);
    OriginalRenderedRimStyle = Packed.RimStyle;
    // Copy only the cached validated camera points after all five native
    // parts commit. Never keep a pointer into an evictable cache entry.
    GarageCameraGeometry = Prepared->CameraGeometry();
    SourceMeshBuildTelemetry =
        latebrake::OriginalMeshBuildCommit(SourceMeshBuildTelemetry);
    ApplyWheelVisualPose();
}

int32 ALatebrakeCarPawn::GetCommittedOriginalWheelParts() const noexcept
{
    if (!bOriginalVehicleVisualReady) return 0;
    const UProceduralMeshComponent* Wheels[] = {
        OriginalFrontLeftWheel.Get(), OriginalFrontRightWheel.Get(),
        OriginalRearLeftWheel.Get(), OriginalRearRightWheel.Get()};
    int32 count=0;
    for (const auto* Part : Wheels)
        if (IsValid(Part) && Part->IsVisible() &&
            Part->GetNumSections()>0)
            ++count;
    return count;
}

void ALatebrakeCarPawn::ApplyOriginalVehiclePalette(
    int32 VehicleId, latebrake::GarageLook Look)
{
    if (!bOriginalVehicleVisualReady ||
        OriginalVehicleTintParameter.IsNone() ||
        !IsValid(OriginalVehicleVisual)) return;
    const auto paint=latebrake::PaintForGarageLook(VehicleId, Look);
    const auto rim=latebrake::RimTintForGarageLook(Look);
    const latebrake::VehiclePaintRgb tones[] = {
        paint.Body, paint.Accent, paint.Canopy,
        {0.030,0.033,0.038}, rim,
        {0.30,0.32,0.36}, {0.95,0.95,0.81},
        {0.90,0.06,0.045}, {0.04,0.06,0.07},
        {0.12,0.14,0.17}
    };
    static_assert(sizeof(tones)/sizeof(tones[0]) ==
        latebrake::vehiclemesh::OriginalVehicleSectionCount,
        "Material palette and original surface groups must match");
    UProceduralMeshComponent* RenderParts[] = {
        OriginalVehicleVisual.Get(), OriginalFrontLeftWheel.Get(),
        OriginalFrontRightWheel.Get(), OriginalRearLeftWheel.Get(),
        OriginalRearRightWheel.Get()};
    // Body and independently animated wheel sections use the SAME
    // ten-slot paint contract; F5/F7 update every material exactly once
    // with no new triangles/mesh allocation on paint-only changes.
    for (auto* Part : RenderParts)
    {
        if (!IsValid(Part)) continue;
        for (int32 section=0;
             section<static_cast<int32>(
                 latebrake::vehiclemesh::OriginalVehicleSectionCount);
             ++section)
        {
            UMaterialInstanceDynamic* material =
                Cast<UMaterialInstanceDynamic>(Part->GetMaterial(section));
            if (!IsValid(material)) continue;
            const auto& rgb=tones[section];
            material->SetVectorParameterValue(
                OriginalVehicleTintParameter, FLinearColor(
                    static_cast<float>(rgb.R),
                    static_cast<float>(rgb.G),
                    static_cast<float>(rgb.B), 1.f));
            ApplyOriginalVehicleFinish(material,OriginalVehicleFinishMask,
                latebrake::vehiclemesh::OriginalSurfaceFinish(
                    static_cast<latebrake::vehiclemesh::OriginalVehicleMaterial>(section),Look));
        }
    }
}

void ALatebrakeCarPawn::ApplyVehiclePaintRecipe(int32 VehicleId)
{
    // These are cosmetic colors only. Retain each built-in mesh's original
    // material as the parent: its physical-material/contact settings and
    // Chaos collision body are not changed. Material parameter availability
    // must still be visually verified in the licensed Unreal Editor.
    const latebrake::GarageLook RequestedLook = GetPresentedGarageLook();
    const latebrake::GarageLook Look =
        latebrake::ValidGarageLook(RequestedLook)
            ? RequestedLook : latebrake::FactoryGarageLook(VehicleId);
    const latebrake::VehiclePaintRecipe Paint =
        latebrake::PaintForGarageLook(VehicleId, Look);
    const auto Tint = [&](UStaticMeshComponent* Part,
                          latebrake::VehiclePaintRgb Rgb)
    {
        if (!IsValid(Part) || !Part->GetStaticMesh())
        {
            return;
        }
        UMaterialInstanceDynamic* Instance =
            Cast<UMaterialInstanceDynamic>(Part->GetMaterial(0));
        if (!Instance)
        {
            // Create once on first live use; switching garage vehicles
            // reuses the component-owned dynamic material, not new actors.
            Instance = Part->CreateDynamicMaterialInstance(0);
        }
        if (Instance)
        {
            Instance->SetVectorParameterValue(
                TEXT("Color"),
                FLinearColor(static_cast<float>(Rgb.R),
                             static_cast<float>(Rgb.G),
                             static_cast<float>(Rgb.B), 1.f));
        }
    };
    Tint(Chassis, Paint.Body);
    Tint(Cabin, Paint.Canopy);
    Tint(Hood, Paint.Body);
    Tint(RearDeck, Paint.Body);
    Tint(FrontSplitter, Paint.Accent);
    Tint(LeftSideSkirt, Paint.Accent);
    Tint(RightSideSkirt, Paint.Accent);
    Tint(RearDiffuser, Paint.Accent);
    Tint(HoodScoop, Paint.Accent);
    Tint(LeftRearFin, Paint.Accent);
    Tint(RightRearFin, Paint.Accent);
    Tint(SpoilerWing, Paint.Accent);
    Tint(SpoilerSupportLeft, Paint.Accent);
    Tint(SpoilerSupportRight, Paint.Accent);

    // Actual attached visible stripe geometry: center/twin/clean.
    // Its top surface follows the selected car's roof height and length.
    const latebrake::VehicleSpec Spec =
        latebrake::GarageState::Definition(VehicleId);
    // Use the chassis's actual installed scale for purely decorative
    // roof marks; no new scale constants or physics-body mutations.
    const FVector ChassisScale = Chassis->GetRelativeScale3D();
    // The legacy cube roof uses its own fallback stripe dimensions.
    // Generated cars use the prepared surface-following bands instead.
    const float StripeHeight = 52.f + 27.5f * static_cast<float>(Spec.CabinHeight) + 2.0f;
    const latebrake::VehiclePaintRgb StripeRgb = latebrake::StripeTintForGarageLook(Look);
    const auto ApplyStripe = [&](UStaticMeshComponent* Part, bool Visible,
                                  float SideCm)
    {
        if (!IsValid(Part)) return;
        Visible = Visible && !bOriginalVehicleVisualReady;
        Part->SetVisibility(Visible && Part->GetStaticMesh() != nullptr, true);
        if (!Visible) return;
        Part->SetRelativeLocation(
            FVector(-20.f, SideCm, StripeHeight) / ChassisScale);
        Part->SetRelativeScale3D(
            FVector(1.2 * Spec.CabinLength, 0.045, 0.035) / ChassisScale);
        Tint(Part, StripeRgb);
    };
    ApplyStripe(RoofStripeLeft, Look.Stripe != 0u,
        Look.Stripe == 1u ? 0.f : -21.f);
    ApplyStripe(RoofStripeRight, Look.Stripe == 2u, 21.f);
    ApplyOriginalRoofStripeLook(Look);

    // Trim/rim finishes change ONLY the four decorative spoke meshes,
    // not wheel contacts, tire grip, physics, or the actual Chaos chassis.
    const latebrake::VehiclePaintRgb RimRgb = latebrake::RimTintForGarageLook(Look);
    for (UStaticMeshComponent* Rim :
         {FrontLeftSpoke.Get(), FrontRightSpoke.Get(),
          RearLeftSpoke.Get(), RearRightSpoke.Get()})
    {
        if (!IsValid(Rim)) continue;
        const float Width = Look.Wheels == 2u ? 0.24f :
            Look.Wheels == 1u ? 0.19f : 0.14f;
        Rim->SetRelativeScale3D(FVector(Width, 0.24f, 0.09f));
        Tint(Rim, RimRgb);
    }
    ApplyOriginalVehiclePalette(VehicleId, Look);
    RefreshFrontLampPositions();
    RefreshRearSignalPositions();
}

void ALatebrakeCarPawn::RefreshFrontLampPositions()
{
    // Reposition only when the actual vehicle mesh is rebuilt/equipped
    // or when its look changes; never allocate/update transforms on Tick.
    if (!IsValid(Chassis)) return;
    const FVector Scale=Chassis->GetRelativeScale3D();
    if (FMath::Abs(Scale.X)<0.001f ||
        FMath::Abs(Scale.Y)<0.001f ||
        FMath::Abs(Scale.Z)<0.001f)
        return;
    const auto Plan=latebrake::PlanFrontLampRig(
        bOriginalVehicleVisualReady,false);
    USpotLightComponent* Lamps[] = {
        FrontLeftLamp.Get(), FrontRightLamp.Get()};
    for (int32 i=0;i<2;++i)
    {
        if (!IsValid(Lamps[i])) continue;
        const auto& p=Plan.Mounts[static_cast<std::size_t>(i)];
        Lamps[i]->SetRelativeLocation(
            FVector(p.Xcm,p.Ycm,p.Zcm)/Scale);
    }
}

void ALatebrakeCarPawn::RefreshRearSignalPositions()
{
    // The old 190cm physics Cube is shorter than the original 222cm
    // 3D rear bumper. Reposition all three existing render-only point
    // lights when visual shape/fallback changes, never in Tick.
    if (!IsValid(Chassis)) return;
    const FVector Scale=Chassis->GetRelativeScale3D();
    if (FMath::Abs(Scale.X)<0.001f ||
        FMath::Abs(Scale.Y)<0.001f ||
        FMath::Abs(Scale.Z)<0.001f)
        return;
    const auto Positions=latebrake::PlanRearSignalVisualOffsets(
        bOriginalVehicleVisualReady);
    UPointLightComponent* Lamps[] = {
        RearLeftSignal.Get(), RearRightSignal.Get(),
        ReverseSignal.Get()};
    for(int32 i=0;i<3;++i)
    {
        if (!IsValid(Lamps[i]) ||
            !latebrake::RearSignalNearVisibleBumper(
                bOriginalVehicleVisualReady,
                Positions[static_cast<std::size_t>(i)]))
            continue;
        const auto& p=Positions[static_cast<std::size_t>(i)];
        Lamps[i]->SetRelativeLocation(
            FVector(p.X,p.Y,p.Z)/Scale);
    }
}

void ALatebrakeCarPawn::SetGarageTuning(float Acceleration, float Grip, float TopSpeed,
                                     int32 VehicleId, latebrake::GarageLook Look)
{
    // This method is called only on an actual vehicle/tuning selection,
    // not on a paint-only change. Keep physics multipliers independent of
    // the free cosmetic options, and preserve finite value guards.
    GarageAcceleration = FMath::IsFinite(Acceleration)
        ? FMath::Clamp(Acceleration, 0.5f, 2.f) : 1.f;
    GarageGrip = FMath::IsFinite(Grip)
        ? FMath::Clamp(Grip, 0.5f, 2.f) : 1.f;
    GarageTopSpeed = FMath::IsFinite(TopSpeed)
        ? FMath::Clamp(TopSpeed, 0.5f, 2.f) : 1.f;
    const auto Transition = latebrake::PlanGarageVisualTransition(
        GarageVehicleId, CurrentGarageLook, VehicleId, Look,
        HasActorBegunPlay());
    const int32 SafeId = Transition.VehicleId;
    if (SafeId != GarageVehicleId)
    {
        // Selection changes authority, not the parked showroom view.
        if (!bGarageShowroomPreviewActive)
            ApplyVehicleVisualRecipe(SafeId);
    }

    // Commit identity and appearance atomically BEFORE the expensive 3D
    // renderer. It must never build new-car geometry in the old car's
    // saved rim style or allocate the same mesh twice on one equip action.
    GarageVehicleId = SafeId;
    CurrentGarageLook = Transition.Look;
    if (!bGarageShowroomPreviewActive)
    {
        if (Transition.RebuildSourceMesh)
            RebuildOriginalVehicleVisual(SafeId);
        if (Transition.RefreshMaterials)
            ApplyVehiclePaintRecipe(SafeId);
    }
}

void ALatebrakeCarPawn::SetGarageAppearance(latebrake::GarageLook look)
{
    if (!latebrake::ValidGarageLook(look)) return;
    if (look.Paint == CurrentGarageLook.Paint &&
        look.Stripe == CurrentGarageLook.Stripe &&
        look.Wheels == CurrentGarageLook.Wheels)
        return;
    const bool bRimsChanged = look.Wheels != CurrentGarageLook.Wheels;
    CurrentGarageLook = look;
    // Changing rim STYLE updates actual Alloy triangles once. Colors and
    // stripes reuse materials; neither change can rebuild the geometry.
    // No mesh composition is allowed during a physics frame Tick.
    if (HasActorBegunPlay())
    {
        if (bGarageShowroomPreviewActive) return;
        if (bRimsChanged) RebuildOriginalVehicleVisual(GarageVehicleId);
        ApplyVehiclePaintRecipe(GarageVehicleId);
    }
}

void ALatebrakeCarPawn::UpdateGarageShowroomPresentation(
    bool bGarageMenuVisible, int32 ViewedVehicleId,
    latebrake::GarageLook ViewedLook)
{
    const auto Plan=latebrake::PlanGarageShowroomPresentation({
        bGarageMenuVisible, ViewedVehicleId, ViewedLook,
        GarageVehicleId, CurrentGarageLook,
        bGarageShowroomPreviewActive, GarageShowroomVehicleId,
        GarageShowroomLook, HasActorBegunPlay()
    });
    // Visual identity is transient. No modifications to the selected
    // vehicle, saved appearance, chassis, engine/grip or race authority.
    bGarageShowroomPreviewActive=Plan.PreviewActive;
    GarageShowroomVehicleId=Plan.DisplayVehicleId;
    GarageShowroomLook=Plan.DisplayLook;
    if (!Plan.PreviewActive)
    {
        GarageCameraOrbitIndex = 0;
        GarageCameraFit.Reset();
        bGarageCameraViewPresented = false;
    }
    // Navigate while paused: update only on actual cosmetic differences.
    if (Plan.ShapeChanged)
        ApplyVehicleVisualRecipe(Plan.DisplayVehicleId);
    if (Plan.RebuildSourceMesh)
        RebuildOriginalVehicleVisual(Plan.DisplayVehicleId);
    if (Plan.RefreshMaterials)
        ApplyVehiclePaintRecipe(Plan.DisplayVehicleId);
}

void ALatebrakeCarPawn::ClearDriverInputs()
{
    BreakSkidTrailContinuity();
    Throttle = 0.f;
    Steering = 0.f;
    SmoothedThrottle = 0.f;
    SmoothedSteering = 0.f;
    bBraking = false;
    bHandbrakeHeld = false;
    bPitServiceHeld = false;
    TouchPit.Clear();
    PitService.Suspend();
    SpinAssist.Suspend();
    TireCondition.Suspend();
    BrakeHeat.Suspend();
    EngineHeat.Suspend();
    Transmission.Suspend();
    LastGroundContact = {};
    CollisionDamage.Suspend();
    LastQualifiedPlanarSpeedCmPerSecond = 0.0;
    SurfaceTraction.Suspend();
    NitroBoost.Suspend();
    CurrentNitroSpeedCapScale = 1.f;
    CurrentDraftAppliedFraction = 0.f;
    BrakeDistance.Suspend();
    SlipstreamBoost.Suspend();
    FleetSlipstreamBoost.Suspend();
    TouchDrive.Clear();
    // Recovery, restart, focus loss and interrupted input must never
    // leave the camera facing backwards after the held key is gone.
    LookBackReleased();
    ResetSpeedCameraFov();
    ResetCornerCameraYaw();
}

double ALatebrakeCarPawn::ReadExperimentalGridSettlingVelocity(float DeltaSeconds) const
{
    const UWorld* World=GetWorld();
    const auto* Mode=World?World->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    int32 Slot=INDEX_NONE;
    if (!Mode || !Mode->TryGetExperimentalGridSettlingSlot(this,Slot) ||
        !IsRacePhysicsReady() || !IsValid(Chassis) || !Chassis->IsSimulatingPhysics())
        return 0.0;
    const FVector Position=GetActorLocation();
    const FVector Velocity=Chassis->GetPhysicsLinearVelocity();
    const auto Settling=latebrake::PlanFleetGridSettling({true,Slot,
        {{Position.X,Position.Y},GetActorRotation().Yaw},Position.Z,
        Velocity.X,Velocity.Y,Velocity.Z,static_cast<double>(DeltaSeconds)});
    return Settling.PreserveDownward?Settling.VerticalVelocityCmS:0.0;
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
    FleetSlipstreamBoost.Suspend();
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

void ALatebrakeCarPawn::NitroPressed()
{
    // Only an actual, valid local human race driver may arm nitro.
    UWorld* World = GetWorld();
    const ALatebrakeRaceGameMode* Mode =
        World ? World->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    const bool bEligible = IsLocallyControlled() &&
        Cast<APlayerController>(GetController()) != nullptr &&
        Mode && Mode->IsRaceStartGreen() &&
        Mode->IsCircuitReadyForGameplay() &&
        Mode->IsRestartPlacementReady() &&
        !Mode->IsMenuOpen() &&
        !UGameplayStatics::IsGamePaused(this) &&
        !Mode->IsManualRacePauseActive() &&
        !Mode->IsFocusLossGuardActive() &&
        !Mode->GetRace().IsFinished() && IsRacePhysicsReady();
    NitroBoost.Press(bEligible);
}

void ALatebrakeCarPawn::NitroReleased()
{
    // Released callbacks execute during pause so input cannot stay stuck.
    NitroBoost.Release();
}

void ALatebrakeCarPawn::PitServicePressed()
{
    // Button state alone cannot repair: the physical human chassis, actual
    // position, clock and scene readiness are rechecked in the next Tick.
    bPitServiceHeld = true;
}

void ALatebrakeCarPawn::PitServiceReleased()
{
    bPitServiceHeld = false;
    PitService.Suspend();
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
    ALatebrakeRaceGameMode* Mode =
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
        // The developer-only grid may be waiting for the actual bodies to
        // settle from the authored elevated spawn. Preserve only their
        // already-measured downward component, never a commanded force.
        // All ordinary blocked states still use the exact zero reset.
        const double SettlingZ=ReadExperimentalGridSettlingVelocity(DeltaSeconds);
        NeutralizeMotionWhenDrivingBlocked();
        if (SettlingZ<0.0 && IsValid(Chassis) && Chassis->IsSimulatingPhysics())
        {
            Chassis->SetPhysicsLinearVelocity(FVector(0.0,0.0,SettlingZ));
        }
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
        if (latebrake::NeutralizeTouchOnViewportChange(
                TouchViewport, TouchDrive, SmoothedThrottle, SmoothedSteering,
                Width, Height))
        {
            // A rotate/resize may arrive without any further finger event.
            // Never allow a stale PIT hold to finish on the new viewport.
            TouchPit.Clear();
            PitService.Suspend();
        }
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
        // A rejected physics frame must not leave last tick's full road
        // contact displayed after simulation stops or frame validity fails.
        BreakSkidTrailContinuity();
        LastGroundContact = {};
        ResetSpeedCameraFov();
        ResetCornerCameraYaw();
        ApplySignalLights(false, false, 0.0);
        SpinAssist.Suspend();
        WheelMotion.Suspend();
        ApplyWheelVisualPose();
        GForceMeter.Suspend();
        SlipAngleMeter.Suspend();
        BrakeDistance.Suspend();
        SlipstreamBoost.Suspend();
        FleetSlipstreamBoost.Suspend();
        TireCondition.Suspend();
        BrakeHeat.Suspend();
        EngineHeat.Suspend();
        CollisionDamage.Suspend();
        LastQualifiedPlanarSpeedCmPerSecond = 0.0;
        SurfaceTraction.Suspend();
        NitroBoost.Suspend();
        CurrentDraftAppliedFraction = 0.f;
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
    // A held human handbrake reduces bounded lateral grip, preserving
    // a controllable physical slide instead of snapping velocity sideways.
    const latebrake::HandbrakeDriveProfile BrakeProfile =
        latebrake::SelectHandbrakeDriveProfile(
            bHandbrakeHeld && IsLocallyControlled() &&
                Cast<APlayerController>(GetController()) != nullptr,
            bServiceBraking, bArcade,
            static_cast<double>(bArcade ? 8.5f : 4.25f),
            static_cast<double>(RawThrottle));
    const bool bEffectiveBraking = BrakeProfile.Braking;
    const float DriveForce = (bArcade ? 1500000.f : 1030000.f) * GarageAcceleration;
    const float MaxSpeed = (bArcade ? 6500.f : 5400.f) * GarageTopSpeed; // cm/s

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
        NitroBoost.Suspend();
    SlipstreamBoost.Suspend();
    FleetSlipstreamBoost.Suspend();
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
    // REAL Unreal WorldStatic ground queries near the four measured chassis
    // corners, not the procedural XY road mask alone. The vehicle remains
    // a single pitch/roll-locked Chaos rigid body: these are limited
    // contact observations, NOT sprung wheels or suspension physics.
    // Ignoring this actor prevents a ray from treating its own chassis
    // as a supporting road surface.
    latebrake::ChassisGroundContactSample GroundSample{};
    latebrake::SurfaceSkidFrame SkidFrame{};
    SkidFrame.DeltaSeconds=static_cast<double>(DeltaSeconds);
    SkidFrame.TeleportRevision=GetTeleportRevision();
    SkidFrame.SessionRevision=Mode->GetCheckpointSessionRevision();
    GroundSample.DeltaSeconds = static_cast<double>(DeltaSeconds);
    GroundSample.ChassisVerticalSpeedCmPerSecond =
        static_cast<double>(Velocity.Z);
    GroundSample.PhysicsReady = World != nullptr && IsRacePhysicsReady();
    const FVector GroundOrigin = Chassis->GetComponentLocation();
    FCollisionQueryParams GroundQuery(
        SCENE_QUERY_STAT(LatebrakeChassisGroundContact), false);
    GroundQuery.AddIgnoredActor(this);
    constexpr float ProbeForwardCm = 122.f;
    constexpr float ProbeSideCm = 68.f;
    const float ProbeX[] = {
        ProbeForwardCm, ProbeForwardCm,
        -ProbeForwardCm, -ProbeForwardCm
    };
    const float ProbeY[] = {
        -ProbeSideCm, ProbeSideCm, -ProbeSideCm, ProbeSideCm
    };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FVector Origin = GroundOrigin +
            Forward * ProbeX[Index] + Right * ProbeY[Index];
        const FVector End = Origin -
            FVector(0.f, 0.f,
                static_cast<float>(
                    latebrake::ChassisContactProbeTraceLengthCm));
        FHitResult Hit;
        const bool bGroundHit = World &&
            World->LineTraceSingleByChannel(
                Hit, Origin, End, ECC_WorldStatic, GroundQuery) &&
            Hit.bBlockingHit &&
            IsValid(Hit.GetComponent()) &&
            Hit.GetComponent()->GetCollisionObjectType() == ECC_WorldStatic;
        if (bGroundHit)
        {
            GroundSample.Probes[static_cast<unsigned>(Index)] = {
                true, static_cast<double>(Hit.Distance),
                static_cast<double>(Hit.ImpactNormal.Z)
            };
            int32 AsphaltBlock=INDEX_NONE;
            if (Mode->TryGetSkidRoadSurface(Hit.GetComponent(),AsphaltBlock))
            {
                const FVector PointVelocity=
                    Chassis->GetPhysicsLinearVelocityAtPoint(Hit.ImpactPoint);
                SkidFrame.Contacts[static_cast<unsigned>(Index)]=
                    latebrake::MakeArcadeSkidProbeContact({
                        true,AsphaltBlock,
                        static_cast<std::uint64_t>(Hit.GetComponent()->GetUniqueID())+1u,
                        GroundSample.Probes[static_cast<unsigned>(Index)],
                        {Hit.ImpactPoint.X,Hit.ImpactPoint.Y,Hit.ImpactPoint.Z},
                        {Hit.ImpactNormal.X,Hit.ImpactNormal.Y,Hit.ImpactNormal.Z},
                        FVector::DotProduct(PointVelocity,Forward),
                        FVector::DotProduct(PointVelocity,Right)});
            }
        }
    }
    const latebrake::ChassisGroundContactOutput GroundContact =
        latebrake::EvaluateChassisGroundContact(GroundSample);
    if (!GroundContact.Valid)
    {
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }
    // One accepted physics frame only: the HUD never repeats a stale
    // ground observation after pause, lost input or failed recovery.
    LastGroundContact = GroundContact;
    const float ContactTractionScale =
        static_cast<float>(GroundContact.ContactTractionScale);
    // Finite velocity components alone do not ensure that derived speed
    // and directional projections stay representable in float precision.
    if (!FMath::IsFinite(Speed) || !FMath::IsFinite(LongitudinalSpeed) ||
        !FMath::IsFinite(LateralSpeed))
    {
        UE_LOG(LogTemp, Warning, TEXT("Latebrake: nonfinite derived speed; stopping chassis"));
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }

    // Both driver types use the ACTUAL chassis position against the
    // authored paved circuit footprint. Short entry/exit hysteresis avoids
    // force flicker at road boundaries. Untrusted positions and frame hitches
    // must fail closed before this frame's motor or lateral forces.
    const FVector SurfacePosition = GetActorLocation();
    const latebrake::ArcadeSurfaceOutput Surface = SurfaceTraction.Observe({
        static_cast<double>(DeltaSeconds),
        static_cast<double>(SurfacePosition.X),
        static_cast<double>(SurfacePosition.Y), true});
    if (!Surface.Valid)
    {
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }
    // Resist actual planar motion, including reverse/sliding, rather than
    // permanently damping all speeds by the same strong Chaos constant.
    // Validate the proposed load BEFORE any motor/nitro/AI force this tick.
    const latebrake::ProgressiveRoadLoadOutput RoadLoad =
        latebrake::CalculateProgressiveRoadLoad({
            static_cast<double>(DeltaSeconds),
            static_cast<double>(Velocity.X),
            static_cast<double>(Velocity.Y),
            static_cast<double>(Mass),
            bArcade, Surface.OffPavement});
    if (!RoadLoad.Valid)
    {
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }
    // Only a finite, source-authorized live chassis tick can arm actual
    // component-hit callbacks; a hitch or pause cannot claim new damage.
    CollisionDamage.Advance(static_cast<double>(DeltaSeconds), true);
    LastQualifiedPlanarSpeedCmPerSecond = CollisionDamage.HasEligibleFrame()
        ? static_cast<double>(Speed) : 0.0;

    // Source-authorized arcade pit: only a real possessed human chassis
    // almost stopped on the inside south shoulder with neutral throttle.
    // The official race clock/rivals continue while servicing; no teleport,
    // lap credit, garage purchase or saved profile changes are made here.
    const bool bPitNeedsService =
        CollisionDamage.DamageFraction() > 0.001 ||
        TireCondition.WearFraction() > 0.03 ||
        NitroBoost.ChargeFraction() < 0.99;
    if (PitService.Observe({
        static_cast<double>(DeltaSeconds),
        static_cast<double>(SurfacePosition.X),
        static_cast<double>(SurfacePosition.Y),
        static_cast<double>(Speed),
        static_cast<double>(TouchDrive.ThrottleActive() ? 1.f : Throttle),
        (bPitServiceHeld || TouchPit.IsHeld()), NitroBoost.IsHeld(),
        IsLocallyControlled() &&
            Cast<APlayerController>(GetController()) != nullptr &&
            Mode->HasLocalPlayerDriver(),
        bPitNeedsService,
        latebrake::HasStablePitGroundSupport(
            GroundContact, static_cast<double>(Velocity.Z))}))
    {
        const auto Before = latebrake::ObservePitVehicleCondition(
            CollisionDamage, TireCondition, NitroBoost);
        CollisionDamage.RepairAtPit();
        TireCondition.ReplaceAtPit();
        NitroBoost.RefillAtPit();
        CurrentNitroSpeedCapScale = 1.f;
        const auto After = latebrake::ObservePitVehicleCondition(
            CollisionDamage, TireCondition, NitroBoost);
        const auto& PitRace = Mode->GetRace();
        // Observe actual effects once, only after the existing completed
        // service latch. Receipt failure cannot grant or deny repairs.
        PitJournal.RecordCompleted(PitService.StopCount(),
            PitRace.GetCompletedLaps() + 1, PitRace.GetElapsedSeconds(),
            Before, After);
    }

    // Purely visual: gently widen the human camera with measured forward
    // chassis speed. Rival cameras, reverse motion and rear view stay static.
    UpdateSpeedCameraFov(static_cast<double>(DeltaSeconds),
                         static_cast<double>(LongitudinalSpeed) * 0.036);
    // The same measured signed-forward speed and player's actual filtered
    // steering give a small visual-only lead into the turn. Hood and rear
    // views, invalid frames and AI opponents cannot acquire this effect.
    UpdateCornerCameraYaw(static_cast<double>(DeltaSeconds),
        static_cast<double>(LongitudinalSpeed),
        static_cast<double>(SmoothedSteering));

    // Observe the real chassis travel direction, not a requested steering
    // angle or decorative wheel animation. Read-only session telemetry.
    SlipAngleMeter.Observe({static_cast<double>(DeltaSeconds),
        static_cast<double>(LongitudinalSpeed),
        static_cast<double>(LateralSpeed)});
    // Chassis-derived arcade grip condition shared by human and rival.
    // The source policy rejects nonfinite physics and frame hitches.
    TireCondition.Observe({static_cast<double>(DeltaSeconds),
        static_cast<double>(Speed),
        static_cast<double>(LateralSpeed), bEffectiveBraking,
        GroundContact.HasDriveContact});
    // Shared human/rival service-brake heating from real signed chassis
    // speed. Handbrake-only slides do not raise service-brake heat.
    const latebrake::BrakeHeatOutput Thermal = BrakeHeat.Observe({
        static_cast<double>(DeltaSeconds),
        static_cast<double>(LongitudinalSpeed),
        bServiceBraking && GroundContact.HasDriveContact, true});
    // Ordinary service/handbrake braking now uses an explicit finite
    // longitudinal deceleration budget, not a huge speed-proportional
    // resistance that stops a racing car almost instantaneously.
    // Reuse previously qualified live tire/surface/damage/heat samples.
    const latebrake::GripLimitedBrakingOutput PhysicalBraking =
        latebrake::CalculateGripLimitedBraking({
            static_cast<double>(DeltaSeconds),
            static_cast<double>(LongitudinalSpeed),
            static_cast<double>(Mass),
            bServiceBraking, BrakeProfile.HandbrakeActive, bArcade,
            Surface.LateralGripMultiplier,
            TireCondition.GripMultiplier() * GarageGrip,
            CollisionDamage.LateralGripMultiplier(),
            Thermal.Valid ? Thermal.ServiceBrakeMultiplier : 1.0});
    if (!PhysicalBraking.Valid)
    {
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }
    // A shared friction-circle budget prevents arbitrary 20g+ lateral
    // corrections. True physics chassis slip and the SAME-FRAME braking
    // request compete for finite road/tire grip. Garage upgrades modify
    // this budget once, without multiplying a tuned rate twice.
    const latebrake::LateralGripOutput LateralForce =
        latebrake::CalculateLateralGripForce({
            static_cast<double>(DeltaSeconds),
            static_cast<double>(LateralSpeed),
            static_cast<double>(Mass),
            BrakeProfile.LateralGripRate,
            static_cast<double>(GarageGrip) *
                TireCondition.GripMultiplier() *
                Surface.LateralGripMultiplier *
                CollisionDamage.LateralGripMultiplier(),
            PhysicalBraking.DecelerationCmPerSecondSquared,
            bArcade, BrakeProfile.HandbrakeActive});
    if (!LateralForce.Valid)
    {
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }
    // Advisory, from actual consecutive signed-forward chassis motion
    // while braking. A single speed or pressed BRAKE command is insufficient.
    BrakeDistance.Observe({static_cast<double>(DeltaSeconds),
        static_cast<double>(Speed),
        static_cast<double>(LongitudinalSpeed),
        bEffectiveBraking && GroundContact.HasDriveContact});

    // Read real valid signed chassis velocity and effective brake input,
    // never motor demand alone. Advisory decorative light intensities only.
    ApplySignalLights(true, bEffectiveBraking,
        static_cast<double>(LongitudinalSpeed));

    // Apply the same bounded corner-assist policy to controller, touch
    // and rival throttle. Brake/reverse demand is never reduced by it.
    const float CornerThrottle = static_cast<float>(
        latebrake::StabilizeCornerThrottle(
            BrakeProfile.Throttle, LongitudinalSpeed, LateralSpeed, bArcade));
    // This is a *second* Arcade-only human safeguard. It observes actual
    // Chaos yaw rate and signed chassis slip after the existing lateral
    // traction policy. The rival keeps its independent AI spin guard;
    // Simulation Inspired and an intentional handbrake drift are unchanged.
    const bool bHumanDriver = IsLocallyControlled() &&
        Cast<APlayerController>(GetController()) != nullptr;
    const FVector AngularVelocity = Chassis->GetPhysicsAngularVelocityInRadians();
    const float AssistedSteering = static_cast<float>(
        latebrake::LimitSteeringAtSpeed(SmoothedSteering, Speed, bArcade));
    // Replace the rigid fixed steering impulse with a measured-yaw
    // target. Both player and AI use exactly the same bounded controller.
    // Handbrake slides retain their original, intentionally loose torque.
    const latebrake::SpeedAwareYawOutput PlannedYaw =
        latebrake::CalculateSpeedAwareYawTorque({
            static_cast<double>(DeltaSeconds),
            static_cast<double>(LongitudinalSpeed),
            static_cast<double>(Speed),
            static_cast<double>(AngularVelocity.Z),
            static_cast<double>(AssistedSteering),
            static_cast<double>(Mass),
            bArcade, Surface.OffPavement});
    if (!PlannedYaw.Valid)
    {
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }
    const float EffectiveThrottle = static_cast<float>(
        latebrake::ModerateHumanSpinThrottle(
            CornerThrottle, static_cast<double>(LongitudinalSpeed),
            static_cast<double>(LateralSpeed),
            static_cast<double>(AngularVelocity.Z),
            bArcade, bHumanDriver, BrakeProfile.HandbrakeActive));
    // Observational only: classify the reduction ACTUALLY applied to the
    // shared base/nitro/draft throttle, never the driver's requested axis.
    // This meter cannot alter physics, race progress or stored rewards.
    SpinAssist.Observe({static_cast<double>(DeltaSeconds),
        static_cast<double>(CornerThrottle),
        static_cast<double>(EffectiveThrottle),
        bArcade && bHumanDriver && !BrakeProfile.HandbrakeActive &&
            !bEffectiveBraking});
    // A finite, player-only boost modifies existing acceleration and speed
    // cap; it never modifies checkpoints, rivals, rewards or race records.
    // Steady-speed cap ramps avoid a velocity snap when a held boost ends.
    const latebrake::NitroDriveOutput NitroDrive = NitroBoost.Step(
        static_cast<double>(DeltaSeconds),
        bHumanDriver && GroundContact.HasDriveContact,
        bEffectiveBraking, static_cast<double>(EffectiveThrottle),
        static_cast<double>(LongitudinalSpeed));
    CurrentNitroSpeedCapScale = FMath::FInterpConstantTo(
        CurrentNitroSpeedCapScale,
        static_cast<float>(NitroDrive.SpeedCapMultiplier),
        FMath::Min(DeltaSeconds, 0.1f), 0.24f);
    // Race-local heat uses real signed chassis speed, effective propulsion
    // after spin moderation, and qualified nitro; service braking does not
    // generate motor heat. Player and rival share the same policy.
    const latebrake::EngineHeatOutput EngineThermal = EngineHeat.Observe({
        static_cast<double>(DeltaSeconds),
        static_cast<double>(LongitudinalSpeed),
        bEffectiveBraking || !GroundContact.HasDriveContact
            ? 0.0 : static_cast<double>(EffectiveThrottle),
        NitroDrive.Active, true});
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
            this, RivalPosition, RivalForward, RivalVelocity);
    double DraftExtraForce = 0.0;
    // The follower and the leader must each have independently
    // qualified at least three real WorldStatic road probes. Mere
    // chassis speed or an airborne rival's last location is not draft.
    if (bDraftTargetReady &&
        latebrake::HasQualifiedDraftGroundSupport(GroundContact))
    {
        const FVector Position = GetActorLocation();
        const FVector Offset = RivalPosition - Position;
        const double RivalSignedForwardSpeed =
            FVector::DotProduct(RivalVelocity, RivalForward);
        // The rival's true world-velocity right-axis component is just
        // as important as the player's observed chassis lateral slip.
        // Two similarly facing cars can still be sliding across the
        // circuit: those frames must never accumulate drafting credit.
        const double RivalLateralSpeed = FVector::DotProduct(
            RivalVelocity,
            FVector(-RivalForward.Y, RivalForward.X, 0.f));
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
            bEffectiveBraking, true,
            static_cast<double>(LateralSpeed),
            RivalLateralSpeed,
            true // GameMode just verified an unobstructed physical sightline.
        });
    }
    else
    {
        SlipstreamBoost.Suspend();
    }
    if (Mode->IsExperimentalFleetRequested() && IsLocallyControlled() &&
        Cast<APlayerController>(GetController()) != nullptr && Mode->HasLocalPlayerDriver())
    {
        const FVector Position=GetActorLocation();
        latebrake::FleetDraftFollower Follower;
        Follower.FrameSeconds=static_cast<double>(DeltaSeconds);
        Follower.PositionX=Position.X;
        Follower.PositionY=Position.Y;
        Follower.PositionZ=Position.Z;
        Follower.ForwardX=Forward.X;
        Follower.ForwardY=Forward.Y;
        Follower.ForwardSpeed=static_cast<double>(LongitudinalSpeed);
        Follower.LateralSpeed=static_cast<double>(LateralSpeed);
        Follower.Throttle=static_cast<double>(EffectiveThrottle);
        Follower.Braking=bEffectiveBraking;
        Follower.GroundSupported=latebrake::HasQualifiedDraftGroundSupport(GroundContact);
        Follower.EligibleHumanRace=true;
        latebrake::FleetDraftFrame DraftFrame;
        if (Mode->TryGetFleetDraftFrame(this,Follower,DraftFrame))
            DraftExtraForce=FleetSlipstreamBoost.Observe(DraftFrame).ExtraForce;
        else
            FleetSlipstreamBoost.Suspend();
    }
    else
    {
        FleetSlipstreamBoost.Suspend();
    }
    // Both independent arcade effects share one final bounded motor-force
    // budget. Charge-consuming nitro gets first priority.
    const latebrake::ArcadeBoostAllocation CombinedBoost =
        latebrake::AllocateArcadeBoost(
            NitroDrive.ForceMultiplier, DraftExtraForce, NitroDrive.Active);
    DraftExtraForce = CombinedBoost.DraftExtra;
    CurrentDraftAppliedFraction = static_cast<float>(DraftExtraForce);
    if (Mode->IsExperimentalFleetRequested())
    {
        // The four original asphalt hits come from this exact physics
        // tick. WorldStatic ground support alone is not road proof.
        bool bOriginalRoad=latebrake::HasQualifiedDraftGroundSupport(GroundContact);
        for (const auto& Contact:SkidFrame.Contacts)
            bOriginalRoad=bOriginalRoad && Contact.Supported && Contact.PavedRoad &&
                Contact.Evidence==latebrake::SkidContactEvidence::ArcadeGroundProbe;
        const FVector ReplayPosition=GetActorLocation();
        latebrake::PhysicalReplayCarSample Sample;
        Sample.WorldXcm=ReplayPosition.X;
        Sample.WorldYcm=ReplayPosition.Y;
        Sample.WorldZcm=ReplayPosition.Z;
        Sample.VelocityXcmps=Velocity.X;
        Sample.VelocityYcmps=Velocity.Y;
        Sample.VelocityZcmps=Velocity.Z;
        Sample.HeadingRadians=FMath::DegreesToRadians(static_cast<double>(GetActorRotation().Yaw));
        Sample.SteeringNormalized=AssistedSteering;
        // The replay schema records positive propulsion demand only.
        Sample.ThrottleNormalized=FMath::Max(0.0,static_cast<double>(EffectiveThrottle));
        Sample.BrakeNormalized=bEffectiveBraking?1.0:0.0;
        Sample.ActualChaosBodyObserved=IsRacePhysicsReady();
        Sample.OriginalRoadPhysicalContact=bOriginalRoad;
        Sample.RoadContactConfidence=bOriginalRoad?1.0:0.0;
        Mode->ObserveFleetReplayCar(this,Sample);
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
    // The same conservative impact-damage reduction applies to normal
    // motor, human-only nitro and qualified drafting. No separate boost
    // path may bypass it or exceed the existing final boost budget.
    const float ImpactMotorScale = static_cast<float>(
        CollisionDamage.MotorMultiplier());
    const float EngineMotorScale = static_cast<float>(
        EngineThermal.MotorMultiplier);
    // Ease positive propulsion into the currently qualified forward (nitro)
    // or reverse gear ceiling. Baseline, nitro and drafting share this
    // single factor; opposing-throttle engine braking is not weakened.
    // Real signed chassis speed drives the policy, not commanded speed.
    // A real per-car automatic gear state is updated from measured,
    // signed forward movement, not a decorative speedometer animation.
    // An active shift briefly reduces physical motor thrust, but NEVER
    // rewrites reverse braking, physical velocity, checkpoint or scoring.
    const latebrake::AutomaticTransmissionState TransmissionFrame =
        Transmission.Advance({
            static_cast<double>(DeltaSeconds),
            static_cast<double>(LongitudinalSpeed),
            static_cast<double>(EffectiveThrottle),
            static_cast<double>(MaxSpeed),
            bArcade, true});
    if (!TransmissionFrame.Valid)
    {
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }
    const float GearPowerFactor = static_cast<float>(
        latebrake::GearAwareMotorPowerFactor(
            static_cast<double>(EffectiveThrottle),
            static_cast<double>(LongitudinalSpeed),
            static_cast<double>(MaxSpeed * CurrentNitroSpeedCapScale),
            bArcade) * TransmissionFrame.PropulsionFactor *
            ContactTractionScale);
    if (GroundContact.HasDriveContact &&
        !bEffectiveBraking &&
        (FMath::Abs(EffectiveThrottle) < 0.01f ||
         FMath::Sign(LongitudinalSpeed) == FMath::Sign(EffectiveThrottle) ||
         Speed < 100.f))
    {
        Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *
            static_cast<float>(Surface.MotorMultiplier) * ImpactMotorScale * EngineMotorScale * GearPowerFactor);
        // Preserve the baseline motor path; an active, human-only nitro
        // adds at most 38% bounded arcade acceleration on this same frame.
        if (NitroDrive.Active)
        {
            Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *
                static_cast<float>(CombinedBoost.NitroExtra) *
                static_cast<float>(Surface.MotorMultiplier) * ImpactMotorScale * EngineMotorScale * GearPowerFactor);
        }
        // Preserve the unmodified base acceleration contract; bonus is
        // additive and never exceeds six percent of positive motor demand.
        if (DraftExtraForce > 0.0)
        {
            Chassis->AddForce(Forward * DriveForce * EffectiveThrottle *
                static_cast<float>(DraftExtraForce) *
                static_cast<float>(Surface.MotorMultiplier) * ImpactMotorScale * EngineMotorScale * GearPowerFactor);
        }
    }
    else if (GroundContact.HasDriveContact && !bEffectiveBraking)
    {
        // Opposite throttle acts as engine braking until the car slows.
        Chassis->AddForce(-Forward * LongitudinalSpeed * Mass *
            static_cast<float>(latebrake::FrameStableDampingRate(3.0, DeltaSeconds)) *
            ContactTractionScale);
    }

    // Real planar resistance: low-speed coasting, increasing aero drag
    // toward top speed, and additional grass/shoulder rolling loss. Applied
    // as a mass-scaled world-space force so it never alters Chaos vertical
    // gravity or directly overwrites actual chassis velocity.
    Chassis->AddForce(FVector(
        static_cast<float>(RoadLoad.ForceX),
        static_cast<float>(RoadLoad.ForceY), 0.f));

    // Contact-grip-limited correction of actual sideways momentum.
    // This preserves a sustained skid when grip is exhausted, while
    // remaining opposite measured slip and never rewriting Chaos velocity.
    Chassis->AddForce(Right *
        static_cast<float>(LateralForce.RightForce) *
        ContactTractionScale);
    if (bEffectiveBraking && GroundContact.HasDriveContact)
    {
        // Physically sampled brake force, bounded to a credible approximate
        // contact-grip budget. This retains reverse braking and handbrake
        // slide handling but does not apply a speed-proportional hard stop.
        Chassis->AddForce(Forward *
            static_cast<float>(PhysicalBraking.ForwardForce) *
            ContactTractionScale);
    }
    // Neutral throttle COASTS under the progressive road load and Chaos
    // baseline damping. It is not an implicit service-brake command.

    const float SteeringAuthority = FMath::Clamp(FMath::Abs(LongitudinalSpeed) / 900.f, 0.f, 1.f);
    const float SteerDirection = LongitudinalSpeed < -75.f ? -1.f : 1.f;
    // Speed-sensitive steering is already evaluated before the motor
    // forces. Prefer a real measured yaw-rate response with lateral
    // acceleration limits; preserve the original yaw authority only
    // during deliberate human handbrake slides.
    const float TurnTorque = BrakeProfile.HandbrakeActive
        ? (bArcade ? 21000000.f : 14000000.f) *
              AssistedSteering * SteeringAuthority * SteerDirection
        : static_cast<float>(PlannedYaw.TorqueZ);
    // Decorative wheels track real forward chassis motion, including
    // reverse, using the same bounded speed-sensitive visual steering.
    UpdateWheelVisuals(static_cast<double>(DeltaSeconds),
        static_cast<double>(LongitudinalSpeed),
        static_cast<double>(AssistedSteering));
    Chassis->AddTorqueInRadians(FVector(
        0.f, 0.f, TurnTorque * ContactTractionScale));

    const float ActiveSpeedCap = MaxSpeed * CurrentNitroSpeedCapScale;
    if (Speed > ActiveSpeedCap)
    {
        const FVector Limited = FlatVelocity.GetClampedToMaxSize(ActiveSpeedCap);
        Chassis->SetPhysicsLinearVelocity(FVector(Limited.X, Limited.Y, Velocity.Z));
    }

    // Real signed chassis motion, not requested reverse throttle, drives
    // the backward gear limit for player and autonomous rival alike. Leave
    // lateral slides and Chaos vertical velocity untouched, including after
    // the existing forward/nitro maximum-speed budget has been enforced.
    const FVector PostCapVelocity = Chassis->GetPhysicsLinearVelocity();
    const latebrake::ReverseSpeedLimitOutput ReverseLimit =
        latebrake::LimitPlanarReverseSpeed(
            static_cast<double>(PostCapVelocity.X),
            static_cast<double>(PostCapVelocity.Y),
            static_cast<double>(Forward.X),
            static_cast<double>(Forward.Y), bArcade);
    if (!ReverseLimit.Valid)
    {
        NeutralizeMotionWhenDrivingBlocked();
        return;
    }
    if (ReverseLimit.Limited)
    {
        Chassis->SetPhysicsLinearVelocity(FVector(
            static_cast<float>(ReverseLimit.VelocityX),
            static_cast<float>(ReverseLimit.VelocityY),
            PostCapVelocity.Z));
    }
    // Optional marks consume only the already admitted physics frame.
    // Never synthesize an individual wheel load or a braking slip value.
    if (!GroundContact.HasDriveContact) SkidFrame.Contacts={};
    SkidFrame.DrivingAllowed=true;
    SubmitSkidTrailFrame(SkidFrame);
}

void ALatebrakeCarPawn::SubmitSkidTrailFrame(const latebrake::SurfaceSkidFrame& Frame)
{
    if (!bSkidRendererAttempted)
    {
        // One lazy, cosmetic world-space holder per accepted live car.
        // Do not respawn every physics frame on optional asset failure.
        bSkidRendererAttempted=true;
        if (UWorld* World=GetWorld())
        {
            FActorSpawnParameters Parameters;
            Parameters.Owner=this;
            Parameters.ObjectFlags|=RF_Transient;
            Parameters.SpawnCollisionHandlingOverride=
                ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            SkidTrailActor=World->SpawnActor<ALatebrakeSkidTrailActor>(
                ALatebrakeSkidTrailActor::StaticClass(),FVector::ZeroVector,
                FRotator::ZeroRotator,Parameters);
        }
    }
    if (IsValid(SkidTrailActor)) SkidTrailActor->Submit(Frame);
}

void ALatebrakeCarPawn::BreakSkidTrailContinuity()
{
    if (IsValid(SkidTrailActor)) SkidTrailActor->BreakContinuity();
}

void ALatebrakeCarPawn::ResetSkidTrails()
{
    if (IsValid(SkidTrailActor)) SkidTrailActor->ResetTrails();
    else bSkidRendererAttempted=false;
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

    // A real pair of short, fixed-budget road-facing spotlights. The
    // existing authoritative race/focus/pause guard controls both light
    // outputs: no light remains latched during OS suspension or menu pause.
    const auto FrontPlan = latebrake::PlanFrontLampRig(
        bOriginalVehicleVisualReady,
        Signals.RedRearIntensity > 0.0);
    bFrontLampOutputRequested = FrontPlan.OutputIntensity > 0.0 &&
        IsValid(FrontLeftLamp) && IsValid(FrontRightLamp);
    if (IsValid(FrontLeftLamp))
        FrontLeftLamp->SetIntensity(
            static_cast<float>(FrontPlan.OutputIntensity));
    if (IsValid(FrontRightLamp))
        FrontRightLamp->SetIntensity(
            static_cast<float>(FrontPlan.OutputIntensity));
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

    if (!bOriginalVehicleVisualReady) return;
    UProceduralMeshComponent* OriginalWheels[] = {
        OriginalFrontLeftWheel.Get(), OriginalFrontRightWheel.Get(),
        OriginalRearLeftWheel.Get(), OriginalRearRightWheel.Get()};
    // Source mesh wheels are authored in X/Z circular planes with axle Y,
    // not the legacy cylinder's local-Z spindle. A +Y roll gives the
    // negative contact velocity needed for physically forward +X motion.
    const FQuat SourceRoll(FVector::RightVector,
        Pose.RollDegrees * Radians);
    for (int32 Index=0;Index<4;++Index)
    {
        if (IsValid(OriginalWheels[Index]))
        {
            const FQuat Visual =
                (Index<2 ? Steering : FQuat::Identity)*SourceRoll;
            OriginalWheels[Index]->SetRelativeRotation(Visual);
        }
    }
}

void ALatebrakeCarPawn::UpdateWheelVisuals(
    double DeltaSeconds, double ForwardSpeedCmPerSecond, double VisualSteering)
{
    if (WheelMotion.Advance({
            DeltaSeconds, ForwardSpeedCmPerSecond, VisualSteering,
            bOriginalVehicleVisualReady
                ? latebrake::vehiclemesh::OriginalVehicleMeshSettings{}
                    .WheelRadiusCm
                : 29.0}))
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
    BrakeHeat.Suspend();
    EngineHeat.Suspend();
    BrakeDistance.Suspend();
    SlipstreamBoost.Suspend();
    FleetSlipstreamBoost.Suspend();
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
    NitroReleased();
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->RecoverPlayer(this);
    }
}

void ALatebrakeCarPawn::RestartPressed()
{
    HandbrakeReleased();
    NitroReleased();
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
    CornerLookAhead.Reset();
    const bool bBlendChaseRig = bBlendNextCameraPreset;
    bBlendNextCameraPreset = false; // A look-back/recovery cannot inherit this.
    if (!IsValid(CameraBoom))
    {
        ChaseRigTransition.Snap(latebrake::GetCameraPreset(
            latebrake::CameraView::Chase));
        return;
    }
    const latebrake::CameraPreset Requested =
        latebrake::GetCameraPreset(CameraViews.Current());
    // Fail closed on a corrupted/future preset rather than installing
    // nonfinite camera transforms into an active physics scene.
    const latebrake::CameraPreset View =
        latebrake::IsSafeCameraPreset(Requested)
            ? Requested : latebrake::GetCameraPreset(latebrake::CameraView::Chase);
    // Only deliberate human chase-to-chase switches animate. Read the
    // actually displayed spring-arm pose, including interrupted blends.
    // Hood and held rear view always snap to their complete safe preset.
    const latebrake::ChaseCameraPose DisplayedPose{
        static_cast<double>(CameraBoom->TargetArmLength),
        static_cast<double>(CameraBoom->GetRelativeRotation().Pitch),
        static_cast<double>(CameraBoom->SocketOffset.X),
        static_cast<double>(CameraBoom->SocketOffset.Z)};
    const bool bTransitioning = bBlendChaseRig && IsLocallyControlled() &&
        Cast<APlayerController>(GetController()) != nullptr &&
        ChaseRigTransition.Begin(DisplayedPose, View);
    if (!bTransitioning)
    {
        ChaseRigTransition.Snap(View);
    }
    const latebrake::ChaseCameraPose Pose = ChaseRigTransition.Pose();
    CameraBoom->TargetArmLength = static_cast<float>(Pose.ArmLengthCm);
    CameraBoom->SetRelativeRotation(FRotator(
        static_cast<float>(Pose.PitchDegrees),
        static_cast<float>(LookBackCamera.SpringArmYawDegrees()), 0.f));
    CameraBoom->SocketOffset = FVector(
        static_cast<float>(Pose.ForwardOffsetCm), 0.f,
        static_cast<float>(Pose.HeightOffsetCm));
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

void ALatebrakeCarPawn::ResetCornerCameraYaw()
{
    // Pause, recovery, focus loss and invalid Chaos frames stop both
    // animations immediately. Never leave an in-between camera rig frozen.
    const bool bHadTransition = ChaseRigTransition.IsActive();
    ChaseRigTransition.Cancel();
    if (!CornerLookAhead.HasActiveSample() && !bHadTransition)
    {
        return;
    }
    CornerLookAhead.Reset();
    if (!IsValid(CameraBoom))
    {
        return;
    }
    // Restore the preset immediately when the race loses input ownership,
    // even if Chaos or the paused world never sends another Tick.
    const latebrake::ChaseCameraPose Pose = ChaseRigTransition.Pose();
    CameraBoom->TargetArmLength = static_cast<float>(Pose.ArmLengthCm);
    CameraBoom->SocketOffset = FVector(
        static_cast<float>(Pose.ForwardOffsetCm), 0.f,
        static_cast<float>(Pose.HeightOffsetCm));
    CameraBoom->SetRelativeRotation(FRotator(
        static_cast<float>(Pose.PitchDegrees),
        static_cast<float>(LookBackCamera.SpringArmYawDegrees()), 0.f));
}

void ALatebrakeCarPawn::UpdateCornerCameraYaw(
    double FrameSeconds, double ForwardCmPerSecond, double VisualSteering)
{
    if (!IsValid(CameraBoom))
    {
        CornerLookAhead.Reset();
        // A temporarily missing arm may never resume a stale animation.
        ChaseRigTransition.Snap(latebrake::GetCameraPreset(CameraViews.Current()));
        return;
    }
    const bool bHumanPlayer = IsLocallyControlled() &&
        Cast<APlayerController>(GetController()) != nullptr;
    const double YawLead = CornerLookAhead.Sample(
        ForwardCmPerSecond, VisualSteering, FrameSeconds,
        bHumanPlayer, LookBackCamera.IsHeld(), CameraViews.Current());
    if (!bHumanPlayer)
    {
        // Avoid an interrupted human camera animation surviving controller
        // handoff to a now-unpossessed pawn or a locally hosted AI rival.
        ChaseRigTransition.Cancel();
        return;
    }
    // Update only the local camera spring arm. A smooth chase framing
    // change is layered underneath the existing steering yaw lead.
    // Failed/hitched frames snap to their safe target (portable policy).
    const latebrake::ChaseCameraPose Pose =
        ChaseRigTransition.Advance(FrameSeconds);
    CameraBoom->TargetArmLength = static_cast<float>(Pose.ArmLengthCm);
    CameraBoom->SocketOffset = FVector(
        static_cast<float>(Pose.ForwardOffsetCm), 0.f,
        static_cast<float>(Pose.HeightOffsetCm));
    CameraBoom->SetRelativeRotation(FRotator(
        static_cast<float>(Pose.PitchDegrees),
        static_cast<float>(LookBackCamera.SpringArmYawDegrees() + YawLead),
        0.f));
}

void ALatebrakeCarPawn::ChangeCamera()
{
    const latebrake::CameraView Previous = CameraViews.Current();
    const latebrake::CameraView Next = CameraViews.Next();
    UWorld* World = GetWorld();
    const ALatebrakeRaceGameMode* Mode =
        World ? World->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    // Preset switching remains available outside a race, but transition
    // animation needs the exact same live human/physical driver authority.
    // A menu or paused-world press snaps rather than freezing midway.
    const bool bLiveHumanRace = IsLocallyControlled() &&
        Cast<APlayerController>(GetController()) != nullptr &&
        Mode && Mode->IsRaceStartGreen() &&
        Mode->IsCircuitReadyForGameplay() &&
        Mode->IsRestartPlacementReady() &&
        !Mode->IsMenuOpen() &&
        !Mode->IsFocusLossGuardActive() &&
        !Mode->IsManualRacePauseActive() &&
        !Mode->GetRace().IsFinished() &&
        !UGameplayStatics::IsGamePaused(this) &&
        IsRacePhysicsReady();
    // The hood's zero-length spring arm must NEVER pass through the car
    // on a blended camera path; rear view retains immediate priority.
    bBlendNextCameraPreset =
        bLiveHumanRace &&
        Previous != latebrake::CameraView::Hood &&
        Next != latebrake::CameraView::Hood &&
        !LookBackCamera.IsHeld();
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
    NitroReleased();
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
            if (TouchPit.FingerOwner() == FingerId)
            {
                TouchPit.Clear();
                PitService.Suspend();
            }
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
        TouchPit.Clear();
        PitService.Suspend();
        SmoothedThrottle = 0.f;
        SmoothedSteering = 0.f;
        return;
    }
    int32 Width = 0;
    int32 Height = 0;
    PC->GetViewportSize(Width, Height);
    // The press tracker is deliberately NOT reset: a finger held before an
    // orientation change must not generate a second RECOVER/PAUSE/RESTART.
    const bool bViewportInvalidated =
        latebrake::NeutralizeTouchOnViewportChange(
            TouchViewport, TouchDrive, SmoothedThrottle, SmoothedSteering,
            Width, Height);
    if (bViewportInvalidated)
    {
        TouchPit.Clear();
        PitService.Suspend();
    }
    const latebrake::TouchControlLayout Layout =
        latebrake::CalculateTouchControlLayout(Width, Height);
    if (!Layout.Valid)
    {
        // Orientation or resize invalidates the geometry of every held
        // finger, not just the one that happened to move first.
        TouchDrive.Clear();
        TouchPit.Clear();
        PitService.Suspend();
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
        if (TouchPit.FingerOwner() == FingerId)
        {
            TouchPit.Clear();
            PitService.Suspend();
        }
        return;
    }

    // A fresh press may be the FIRST event delivered after a rotate or
    // viewport resize. The old finger coordinates must not trigger
    // PAUSE, RESTART, RECOVER, career/menu or GHOST commands against newly
    // moved rectangles. AdmitTouchPress has already marked this physical
    // finger down: do not reset or release that one-shot action latch.
    // A real lift and next press on a stable viewport are still allowed.
    if (latebrake::SuppressFreshTouchOnInvalidatedViewport(
            bAllowAcquire, bViewportInvalidated))
    {
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
        TouchPit.Clear();
        PitService.Suspend();
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
        TouchPit.Clear();
        PitService.Suspend();
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
        TouchPit.Clear();
        PitService.Suspend();
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
        // The independent PIT finger must stay inside the original pad.
        // A movement outside cancels, without re-acquiring on drag back.
        const bool bWasPitOwner = TouchPit.FingerOwner() == FingerId;
        TouchPit.Move(FingerId, Region, !bViewportInvalidated);
        if (bWasPitOwner && !TouchPit.IsHeld())
        {
            PitService.Suspend();
        }
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
    case latebrake::TouchControl::PitService:
        // Only a new physical press can start this held action.
        // The real pit state machine verifies all chassis, speed,
        // throttle, nitro and race conditions every physics tick.
        if (!bViewportInvalidated && Mode &&
            Mode->IsCircuitReadyForGameplay() &&
            Mode->HasLocalPlayerDriver() && Mode->IsRaceStartGreen() &&
            !Mode->GetRace().IsFinished() && IsRacePhysicsReady())
        {
            TouchPit.Begin(FingerId, Region, true);
        }
        return;
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
    const int32 FingerId = static_cast<int32>(FingerIndex);
    latebrake::CancelTouchOwnerAndOutput(
        TouchDrive, static_cast<int32>(FingerIndex),
        SmoothedThrottle, SmoothedSteering);
    if (TouchPit.FingerOwner() == FingerId)
    {
        TouchPit.Release(FingerId);
        PitService.Suspend();
    }
    TouchPresses.Release(FingerId);
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
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
        if (auto* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
            if (Hud->HandleGaragePaletteNavigation(latebrake::GaragePaletteNavigationInput::Next))
            {
                ClearDriverInputs();
                return;
            }
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->MenuNext();
    }
}

void ALatebrakeCarPawn::MenuPrevious()
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
        if (auto* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
            if (Hud->HandleGaragePaletteNavigation(latebrake::GaragePaletteNavigationInput::Previous))
            {
                ClearDriverInputs();
                return;
            }
    if (ALatebrakeRaceGameMode* Mode = GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->MenuPrevious();
    }
}

void ALatebrakeCarPawn::MenuDetailsNext()
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
        if (auto* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
            if (Hud->HandleGaragePaletteNavigation(latebrake::GaragePaletteNavigationInput::Next))
            {
                ClearDriverInputs();
                return;
            }
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
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
        if (auto* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
            if (Hud->HandleGaragePaletteNavigation(latebrake::GaragePaletteNavigationInput::Previous))
            {
                ClearDriverInputs();
                return;
            }
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
    // Controller Y is contextual: do not clear live driving or a held pit
    // service request when no menu owns the input. Confirming an open menu
    // still cancels all previous human drive commands before navigation.
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !Mode->IsMenuOpen()) return;
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
        if (auto* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
            if (Hud->HandleGaragePaletteNavigation(latebrake::GaragePaletteNavigationInput::Confirm))
            {
                ClearDriverInputs();
                return;
            }
    ClearDriverInputs();
    Mode->MenuActivate();
}

void ALatebrakeCarPawn::MenuMouseWheelUp()
{
    ScrollMenuMouseWheel(-1);
}

void ALatebrakeCarPawn::MenuMouseWheelDown()
{
    ScrollMenuMouseWheel(1);
}

void ALatebrakeCarPawn::ScrollMenuMouseWheel(int direction)
{
#if !PLATFORM_ANDROID
    // One wheel notch only affects details in an open local desktop menu.
    // Never transfer it into steering, racing, garage, or menu selection.
    const ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!Mode || !Mode->IsMenuOpen() || !PC ||
        !PC->IsLocalController())
    {
        return;
    }
    float MouseX = 0.f;
    float MouseY = 0.f;
    if (!PC->GetMousePosition(MouseX, MouseY))
    {
        return;
    }
    if (ALatebrakeRaceHUD* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
    {
        if (Hud->TryScrollMenuMouse(MouseX, MouseY, direction))
        {
            ClearDriverInputs();
        }
    }
#else
    (void)direction;
#endif
}

void ALatebrakeCarPawn::DispatchFleetReplayAction(int32 Action)
{
    const auto* Mode=GetWorld()?GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>():nullptr;
    auto* PC=Cast<APlayerController>(GetController());
    if(!Mode || !Mode->IsMenuOpen() || Mode->GetMenuPage()!=latebrake::MenuPage::Results ||
        !Mode->GetRace().IsFinished() || !PC || !PC->IsLocalController()) return;
    if(auto* Hud=Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
        if(Hud->HandleFleetReplayAction(static_cast<latebrake::FleetReplayAction>(Action)))
            ClearDriverInputs();
}

void ALatebrakeCarPawn::FleetReplayPlayPause()
{
    DispatchFleetReplayAction(static_cast<int32>(latebrake::FleetReplayAction::PlayPause));
}

void ALatebrakeCarPawn::FleetReplayPrevious()
{
    DispatchFleetReplayAction(static_cast<int32>(latebrake::FleetReplayAction::Previous));
}

void ALatebrakeCarPawn::FleetReplayNext()
{
    DispatchFleetReplayAction(static_cast<int32>(latebrake::FleetReplayAction::Next));
}

void ALatebrakeCarPawn::FleetReplayHighlight()
{
    DispatchFleetReplayAction(static_cast<int32>(latebrake::FleetReplayAction::NextHighlight));
}

void ALatebrakeCarPawn::MenuMousePressed()
{
#if !PLATFORM_ANDROID
    // Only the paused, open, local-player menu accepts mouse input.
    // Hits use the HUD's actual Canvas dimensions, not a guessed DPI
    // scale; no game-world click can drive or teleport. Paid menu intents reuse
    // the keyboard's authoritative two-step purchase handlers.
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!Mode || !Mode->IsMenuOpen() || !PC ||
        !PC->IsLocalController())
    {
        return;
    }
    ALatebrakeRaceHUD* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD());
    float MouseX = 0.f;
    float MouseY = 0.f;
    if (!Hud || !PC->GetMousePosition(MouseX, MouseY))
    {
        return;
    }
    if(Hud->TryHandleFleetReplayPointer(MouseX,MouseY) ||
        Hud->TryHandleSectorMotionPointer(MouseX,MouseY))
    {
        ClearDriverInputs();
        return;
    }
    const latebrake::DesktopMenuMouseAction Action =
        Hud->HitTestMenuMouse(MouseX, MouseY);
    if (Action == latebrake::DesktopMenuMouseAction::None)
    {
        return;
    }
    // Do not allow a click to inherit any held throttle or steering.
    ClearDriverInputs();
    if (latebrake::IsGaragePaletteAction(Action))
    {
        if (Action == latebrake::DesktopMenuMouseAction::GaragePaletteClose)
            Hud->CloseGaragePalette();
        else
            Hud->ApplyGaragePaletteChoice(Action);
        return;
    }
    if (!latebrake::IsGaragePaletteTab(Action))
        Hud->CloseGaragePalette();
    switch (Action)
    {
    case latebrake::DesktopMenuMouseAction::PreviousPage:
        Mode->MenuPrevious(); break;
    case latebrake::DesktopMenuMouseAction::NextPage:
        Mode->MenuNext(); break;
    case latebrake::DesktopMenuMouseAction::DetailsUp:
        Hud->ScrollMenuDetails(-1); break;
    case latebrake::DesktopMenuMouseAction::DetailsDown:
        Hud->ScrollMenuDetails(1); break;
    case latebrake::DesktopMenuMouseAction::Activate:
        Mode->MenuActivate(); break;
    case latebrake::DesktopMenuMouseAction::Close:
        Mode->ToggleMenu(); break;
    case latebrake::DesktopMenuMouseAction::GarageUnlockOffer:
        Mode->GarageUnlockNext(); break;
    case latebrake::DesktopMenuMouseAction::GarageEngineOffer:
        Mode->GarageUpgradeEngine(); break;
    case latebrake::DesktopMenuMouseAction::GarageGripOffer:
        Mode->GarageUpgradeGrip(); break;
    case latebrake::DesktopMenuMouseAction::GaragePaint:
        if (!Hud->ToggleGaragePalette(latebrake::GarageLookLayer::Paint))
            Mode->GarageCyclePaint();
        break;
    case latebrake::DesktopMenuMouseAction::GarageStripe:
        if (!Hud->ToggleGaragePalette(latebrake::GarageLookLayer::Stripe))
            Mode->GarageCycleStripe();
        break;
    case latebrake::DesktopMenuMouseAction::GarageWheels:
        if (!Hud->ToggleGaragePalette(latebrake::GarageLookLayer::Wheels))
            Mode->GarageCycleWheels();
        break;
    case latebrake::DesktopMenuMouseAction::GarageReset:
        Mode->GarageResetAppearance(); break;
    case latebrake::DesktopMenuMouseAction::GarageBrowsePrevious:
        Mode->GarageBrowsePrevious(); break;
    case latebrake::DesktopMenuMouseAction::GarageBrowseNext:
        Mode->GarageBrowseNext(); break;
    case latebrake::DesktopMenuMouseAction::GarageOrbit0:
    case latebrake::DesktopMenuMouseAction::GarageOrbit1:
    case latebrake::DesktopMenuMouseAction::GarageOrbit2:
    case latebrake::DesktopMenuMouseAction::GarageOrbit3:
        SelectGarageCameraOrbit(latebrake::GarageOrbitIndex(Action)); break;
    case latebrake::DesktopMenuMouseAction::GarageQuickView0:
    case latebrake::DesktopMenuMouseAction::GarageQuickView1:
    case latebrake::DesktopMenuMouseAction::GarageQuickView2:
    case latebrake::DesktopMenuMouseAction::GarageQuickView3:
    case latebrake::DesktopMenuMouseAction::GarageQuickView4:
    case latebrake::DesktopMenuMouseAction::GarageQuickView5:
        Mode->GarageBrowseVehicle(
            latebrake::GarageQuickViewIndex(Action)); break;
    default: break;
    }
#endif
}

void ALatebrakeCarPawn::CareerNext()
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
        if (auto* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
            if (Hud->AdvanceGaragePaletteLayer())
            {
                ClearDriverInputs();
                return;
            }
    ClearDriverInputs();
    if (ALatebrakeRaceGameMode* Mode =
        GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        Mode->CycleCareerEvent();
    }
}

void ALatebrakeCarPawn::CareerStart()
{
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
        if (auto* Hud = Cast<ALatebrakeRaceHUD>(PC->GetHUD()))
            if (Hud->HandleGaragePaletteNavigation(latebrake::GaragePaletteNavigationInput::Cancel))
            {
                ClearDriverInputs();
                return;
            }
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

void ALatebrakeCarPawn::SelectGarageSlot(int32 VehicleId)
{
    if (ALatebrakeRaceGameMode* Mode =
        GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>())
    {
        if (Mode->IsMenuOpen() &&
            Mode->GetMenuPage() == latebrake::MenuPage::Garage)
        {
            ClearDriverInputs();
            Mode->GarageSelectVehicle(VehicleId);
        }
    }
}

void ALatebrakeCarPawn::GarageSlot1() { SelectGarageSlot(0); }
void ALatebrakeCarPawn::GarageSlot2() { SelectGarageSlot(1); }
void ALatebrakeCarPawn::GarageSlot3() { SelectGarageSlot(2); }
void ALatebrakeCarPawn::GarageSlot4() { SelectGarageSlot(3); }
void ALatebrakeCarPawn::GarageSlot5() { SelectGarageSlot(4); }
void ALatebrakeCarPawn::GarageSlot6() { SelectGarageSlot(5); }

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

void ALatebrakeCarPawn::CycleGaragePaint()
{
    // Several gamepad buttons double as live driving controls (R3
    // handbrake, L3 nitro, LB look-back, RB camera, B recovery).
    // Reject outside the *visible* Garage BEFORE clearing any inputs.
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !latebrake::CanConsumeGarageShortcut(
        Mode->IsMenuOpen(), Mode->GetMenuPage(),
        Mode->IsFocusLossGuardActive() ||
        Mode->IsManualRacePauseActive()))
        return;
    ClearDriverInputs();
    Mode->GarageCyclePaint();
}
void ALatebrakeCarPawn::CycleGarageStripe()
{
    // Several gamepad buttons double as live driving controls (R3
    // handbrake, L3 nitro, LB look-back, RB camera, B recovery).
    // Reject outside the *visible* Garage BEFORE clearing any inputs.
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !latebrake::CanConsumeGarageShortcut(
        Mode->IsMenuOpen(), Mode->GetMenuPage(),
        Mode->IsFocusLossGuardActive() ||
        Mode->IsManualRacePauseActive()))
        return;
    ClearDriverInputs();
    Mode->GarageCycleStripe();
}
void ALatebrakeCarPawn::CycleGarageWheels()
{
    // Several gamepad buttons double as live driving controls (R3
    // handbrake, L3 nitro, LB look-back, RB camera, B recovery).
    // Reject outside the *visible* Garage BEFORE clearing any inputs.
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !latebrake::CanConsumeGarageShortcut(
        Mode->IsMenuOpen(), Mode->GetMenuPage(),
        Mode->IsFocusLossGuardActive() ||
        Mode->IsManualRacePauseActive()))
        return;
    ClearDriverInputs();
    Mode->GarageCycleWheels();
}

void ALatebrakeCarPawn::ResetGarageLook()
{
    // Several gamepad buttons double as live driving controls (R3
    // handbrake, L3 nitro, LB look-back, RB camera, B recovery).
    // Reject outside the *visible* Garage BEFORE clearing any inputs.
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !latebrake::CanConsumeGarageShortcut(
        Mode->IsMenuOpen(), Mode->GetMenuPage(),
        Mode->IsFocusLossGuardActive() ||
        Mode->IsManualRacePauseActive()))
        return;
    ClearDriverInputs();
    Mode->GarageResetAppearance();
}

void ALatebrakeCarPawn::BrowseGarageNext()
{
    // Several gamepad buttons double as live driving controls (R3
    // handbrake, L3 nitro, LB look-back, RB camera, B recovery).
    // Reject outside the *visible* Garage BEFORE clearing any inputs.
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !latebrake::CanConsumeGarageShortcut(
        Mode->IsMenuOpen(), Mode->GetMenuPage(),
        Mode->IsFocusLossGuardActive() ||
        Mode->IsManualRacePauseActive()))
        return;
    ClearDriverInputs();
    Mode->GarageBrowseNext();
}
void ALatebrakeCarPawn::BrowseGaragePrevious()
{
    // Several gamepad buttons double as live driving controls (R3
    // handbrake, L3 nitro, LB look-back, RB camera, B recovery).
    // Reject outside the *visible* Garage BEFORE clearing any inputs.
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || !latebrake::CanConsumeGarageShortcut(
        Mode->IsMenuOpen(), Mode->GetMenuPage(),
        Mode->IsFocusLossGuardActive() ||
        Mode->IsManualRacePauseActive()))
        return;
    ClearDriverInputs();
    Mode->GarageBrowsePrevious();
}

void ALatebrakeCarPawn::ToggleCircuitLighting()
{
    ALatebrakeRaceGameMode* Mode = GetWorld()
        ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    // This paused input belongs only to Settings. No driving action
    // should run while another UI page or OS focus guard is active.
    if (!Mode || !Mode->IsMenuOpen() ||
        Mode->GetMenuPage()!=latebrake::MenuPage::Settings ||
        Mode->IsFocusLossGuardActive() ||
        Mode->IsManualRacePauseActive())
        return;
    ClearDriverInputs();
    Mode->ToggleCircuitLightingPreview();
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
    PlayerInputComponent->BindAction(TEXT("PitService"), IE_Pressed, this, &ALatebrakeCarPawn::PitServicePressed);
    FInputActionBinding& PitRelease = PlayerInputComponent->BindAction(
        TEXT("PitService"), IE_Released, this, &ALatebrakeCarPawn::PitServiceReleased);
    PitRelease.bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("Nitro"), IE_Pressed, this, &ALatebrakeCarPawn::NitroPressed);
    FInputActionBinding& NitroRelease = PlayerInputComponent->BindAction(
        TEXT("Nitro"), IE_Released, this, &ALatebrakeCarPawn::NitroReleased);
    NitroRelease.bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("Recover"), IE_Pressed, this, &ALatebrakeCarPawn::RecoverPressed);
    PlayerInputComponent->BindAction(TEXT("RestartRace"), IE_Pressed, this, &ALatebrakeCarPawn::RestartPressed);
    PlayerInputComponent->BindAction(TEXT("Camera"), IE_Pressed, this, &ALatebrakeCarPawn::ChangeCamera);
    FInputActionBinding& GarageOrbitKey = PlayerInputComponent->BindAction(
        TEXT("GarageOrbit"), IE_Pressed, this, &ALatebrakeCarPawn::OrbitGarageCamera);
    GarageOrbitKey.bExecuteWhenPaused = true;
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
    PlayerInputComponent->BindAction(TEXT("FleetReplayPlayPause"),IE_Pressed,this,
        &ALatebrakeCarPawn::FleetReplayPlayPause).bExecuteWhenPaused=true;
    PlayerInputComponent->BindAction(TEXT("FleetReplayPrevious"),IE_Pressed,this,
        &ALatebrakeCarPawn::FleetReplayPrevious).bExecuteWhenPaused=true;
    PlayerInputComponent->BindAction(TEXT("FleetReplayNext"),IE_Pressed,this,
        &ALatebrakeCarPawn::FleetReplayNext).bExecuteWhenPaused=true;
    PlayerInputComponent->BindAction(TEXT("FleetReplayHighlight"),IE_Pressed,this,
        &ALatebrakeCarPawn::FleetReplayHighlight).bExecuteWhenPaused=true;
    FInputActionBinding& DuskView = PlayerInputComponent->BindAction(
        TEXT("CircuitLightingPreview"), IE_Pressed,
        this, &ALatebrakeCarPawn::ToggleCircuitLighting);
    DuskView.bExecuteWhenPaused = true;
    FInputActionBinding& MouseMenu = PlayerInputComponent->BindAction(
        TEXT("MenuMouseClick"), IE_Pressed,
        this, &ALatebrakeCarPawn::MenuMousePressed);
    MouseMenu.bExecuteWhenPaused = true;
    FInputActionBinding& WheelUp = PlayerInputComponent->BindAction(
        TEXT("MenuMouseWheelUp"), IE_Pressed,
        this, &ALatebrakeCarPawn::MenuMouseWheelUp);
    WheelUp.bExecuteWhenPaused = true;
    FInputActionBinding& WheelDown = PlayerInputComponent->BindAction(
        TEXT("MenuMouseWheelDown"), IE_Pressed,
        this, &ALatebrakeCarPawn::MenuMouseWheelDown);
    WheelDown.bExecuteWhenPaused = true;
    // These are paused-menu actions: Unreal normally stops gameplay Tick
    // while the Canvas menu is open. Never bind as race-driving controls.
    FInputActionBinding& Slot1 = PlayerInputComponent->BindAction(
        TEXT("GarageSlot1"), IE_Pressed, this, &ALatebrakeCarPawn::GarageSlot1);
    Slot1.bExecuteWhenPaused = true;
    FInputActionBinding& Slot2 = PlayerInputComponent->BindAction(
        TEXT("GarageSlot2"), IE_Pressed, this, &ALatebrakeCarPawn::GarageSlot2);
    Slot2.bExecuteWhenPaused = true;
    FInputActionBinding& Slot3 = PlayerInputComponent->BindAction(
        TEXT("GarageSlot3"), IE_Pressed, this, &ALatebrakeCarPawn::GarageSlot3);
    Slot3.bExecuteWhenPaused = true;
    FInputActionBinding& Slot4 = PlayerInputComponent->BindAction(
        TEXT("GarageSlot4"), IE_Pressed, this, &ALatebrakeCarPawn::GarageSlot4);
    Slot4.bExecuteWhenPaused = true;
    FInputActionBinding& Slot5 = PlayerInputComponent->BindAction(
        TEXT("GarageSlot5"), IE_Pressed, this, &ALatebrakeCarPawn::GarageSlot5);
    Slot5.bExecuteWhenPaused = true;
    FInputActionBinding& Slot6 = PlayerInputComponent->BindAction(
        TEXT("GarageSlot6"), IE_Pressed, this, &ALatebrakeCarPawn::GarageSlot6);
    Slot6.bExecuteWhenPaused = true;
    // E/Q/U/I are controls on the *paused* Garage page, just like Ctrl+1..6.
    // Without these flags Unreal drops the key press while the menu pauses play.
    FInputActionBinding& GarageNext = PlayerInputComponent->BindAction(
        TEXT("GarageNextVehicle"), IE_Pressed, this, &ALatebrakeCarPawn::NextGarageVehicle);
    GarageNext.bExecuteWhenPaused = true;
    FInputActionBinding& GarageUnlock = PlayerInputComponent->BindAction(
        TEXT("GarageUnlockVehicle"), IE_Pressed, this, &ALatebrakeCarPawn::UnlockGarageVehicle);
    GarageUnlock.bExecuteWhenPaused = true;
    FInputActionBinding& GarageEngine = PlayerInputComponent->BindAction(
        TEXT("GarageEngineUpgrade"), IE_Pressed, this, &ALatebrakeCarPawn::UpgradeGarageEngine);
    GarageEngine.bExecuteWhenPaused = true;
    FInputActionBinding& GarageGrip = PlayerInputComponent->BindAction(
        TEXT("GarageGripUpgrade"), IE_Pressed, this, &ALatebrakeCarPawn::UpgradeGarageGrip);
    GarageGrip.bExecuteWhenPaused = true;
    FInputActionBinding& GaragePaintKey = PlayerInputComponent->BindAction(
        TEXT("GarageCyclePaint"), IE_Pressed, this, &ALatebrakeCarPawn::CycleGaragePaint);
    GaragePaintKey.bExecuteWhenPaused = true;
    FInputActionBinding& GarageStripeKey = PlayerInputComponent->BindAction(
        TEXT("GarageCycleStripe"), IE_Pressed, this, &ALatebrakeCarPawn::CycleGarageStripe);
    GarageStripeKey.bExecuteWhenPaused = true;
    FInputActionBinding& GarageWheelsKey = PlayerInputComponent->BindAction(
        TEXT("GarageCycleWheels"), IE_Pressed, this, &ALatebrakeCarPawn::CycleGarageWheels);
    GarageWheelsKey.bExecuteWhenPaused = true;
    FInputActionBinding& GarageResetKey = PlayerInputComponent->BindAction(
        TEXT("GarageResetLook"), IE_Pressed, this, &ALatebrakeCarPawn::ResetGarageLook);
    GarageResetKey.bExecuteWhenPaused = true;
    FInputActionBinding& BrowseNextKey = PlayerInputComponent->BindAction(
        TEXT("GarageBrowseNext"), IE_Pressed, this, &ALatebrakeCarPawn::BrowseGarageNext);
    BrowseNextKey.bExecuteWhenPaused = true;
    FInputActionBinding& BrowsePreviousKey = PlayerInputComponent->BindAction(
        TEXT("GarageBrowsePrevious"), IE_Pressed, this, &ALatebrakeCarPawn::BrowseGaragePrevious);
    BrowsePreviousKey.bExecuteWhenPaused = true;
    PlayerInputComponent->BindAction(TEXT("ArcadePreset"), IE_Pressed, this, &ALatebrakeCarPawn::SelectArcade);
    PlayerInputComponent->BindAction(TEXT("SimulationPreset"), IE_Pressed, this, &ALatebrakeCarPawn::SelectSimulation);
}


bool ALatebrakeCarPawn::CanSelectGarageCameraOrbit() const
{
    UWorld* World = GetWorld();
    const ALatebrakeRaceGameMode* Mode =
        World ? World->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    const APlayerController* PC = Cast<APlayerController>(GetController());
    return bGarageShowroomPreviewActive && IsLocallyControlled() &&
        IsValid(PC) && PC->IsLocalController() && Mode &&
        Mode->CanBrowseGarage() && !Mode->IsFocusLossGuardActive() &&
        !Mode->IsManualRacePauseActive();
}

void ALatebrakeCarPawn::SelectGarageCameraOrbit(int32 Index)
{
    if (!CanSelectGarageCameraOrbit() || Index < 0 ||
        Index >= latebrake::GarageOrbitCount || Index == GarageCameraOrbitIndex)
        return;
    GarageCameraOrbitIndex = Index;
    GarageCameraFit.Reset();
    bGarageCameraViewPresented = false;
}

void ALatebrakeCarPawn::OrbitGarageCamera()
{
    // F4 and direct mouse selection share the same guarded cosmetic path.
    SelectGarageCameraOrbit((GarageCameraOrbitIndex + 1) % latebrake::GarageOrbitCount);
}

void ALatebrakeCarPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
    // Preserve the entire normal camera path, including engine-managed
    // effects and the player's actual selected driving camera preset.
    // Override only the view result for an authenticated local Garage.
    Super::CalcCamera(DeltaTime, OutResult);
    bGarageCameraViewPresented = false;
    if (!bGarageShowroomPreviewActive || !HasActorBegunPlay() ||
        !IsLocallyControlled() || !bOriginalVehicleVisualReady ||
        !IsValid(OriginalVehicleVisual) || !OriginalVehicleVisual->IsVisible() ||
        GetCommittedOriginalWheelParts() != 4 ||
        !GarageCameraGeometry.Ready() ||
        GarageCameraGeometry.VehicleId() != GetPresentedGarageVehicleId() ||
        OriginalRenderedRimStyle != static_cast<int32>(GetPresentedGarageLook().Wheels))
        return;

    APlayerController* PC = Cast<APlayerController>(GetController());
    UWorld* World = GetWorld();
    const ALatebrakeRaceGameMode* Mode =
        World ? World->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!IsValid(PC) || !Mode || !Mode->CanBrowseGarage())
        return;
    int32 Width = 0, Height = 0;
    PC->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0)
        return;
    const double Aspect = static_cast<double>(Width) / static_cast<double>(Height);
    latebrake::vehiclemesh::ShowroomOrbitRequest Request{};
    static constexpr double OrbitYawDegrees[] = {35.0, 90.0, 145.0, -90.0};
    static_assert(UE_ARRAY_COUNT(OrbitYawDegrees) == latebrake::GarageOrbitCount);
    if (GarageCameraOrbitIndex < 0 || GarageCameraOrbitIndex >= latebrake::GarageOrbitCount)
        return;
    Request.YawDegrees = OrbitYawDegrees[GarageCameraOrbitIndex];
    Request.AspectRatio = Aspect;
#if !PLATFORM_ANDROID
    const auto Lane = latebrake::CalculateGarageShowroomLane(
        Width, Height, latebrake::CalculateHudLayout(Width, Height),
        false, true, latebrake::MenuPage::Garage);
    if (Lane.Valid)
    {
        Request.CenterX = Lane.CenterX;
        Request.CenterY = Lane.CenterY;
        Request.HalfWidth = Lane.HalfWidth;
        Request.HalfHeight = Lane.HalfHeight;
    }
#endif
    if (!latebrake::vehiclemesh::ValidShowroomOrbitRequest(Request))
        return;
    const auto PriorPose = GarageCameraFit.Pose();
    if (!GarageCameraFit.Ready() || PriorPose.AspectRatio != Aspect ||
        PriorPose.CenterX != Request.CenterX || PriorPose.CenterY != Request.CenterY ||
        PriorPose.HalfWidth != Request.HalfWidth || PriorPose.HalfHeight != Request.HalfHeight)
    {
        // Fit the cached points only on entry, orbit, resize or a new
        // mesh commit. Never recompose or re-upload render triangles.
        if (!GarageCameraFit.FitPrepared(GarageCameraGeometry, Request))
            return;
    }
    const auto Pose = GarageCameraFit.Pose();
    const FTransform MeshTransform = OriginalVehicleVisual->GetComponentTransform();
    // The source render root cancels the physics cube's nonuniform scale.
    // A developer override of that contract must fall back, not crop or
    // distort the camera by silently treating scaled points as unit space.
    if (MeshTransform.ContainsNaN() ||
        !MeshTransform.GetScale3D().Equals(FVector::OneVector, 0.001))
        return;
    const FVector Eye = MeshTransform.TransformPosition(
        FVector(Pose.Eye.X, Pose.Eye.Y, Pose.Eye.Z));
    const FVector Target = MeshTransform.TransformPosition(
        FVector(Pose.Target.X, Pose.Target.Y, Pose.Target.Z));
    const FVector Forward = MeshTransform.TransformVectorNoScale(
        FVector(Pose.Forward.X, Pose.Forward.Y, Pose.Forward.Z));
    const FVector Up = MeshTransform.TransformVectorNoScale(
        FVector(Pose.Up.X, Pose.Up.Y, Pose.Up.Z));
    if (Eye.ContainsNaN() || Target.ContainsNaN() ||
        Forward.ContainsNaN() || Up.ContainsNaN())
        return;
    const FRotator Rotation = FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();
    if (Rotation.ContainsNaN())
        return;

    // The garage is currently a parked view in the existing track world.
    // Do not claim a dedicated unobstructed studio or force a camera
    // through a wall; retain Super's view when a visibility trace blocks it.
    FHitResult Obstruction;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(GarageShowroomCamera), false, this);
    if (World->LineTraceSingleByChannel(Obstruction, Target, Eye, ECC_Visibility, Query))
        return;

    OutResult.Location = Eye;
    OutResult.Rotation = Rotation;
    OutResult.FOV = static_cast<float>(Pose.HorizontalFovDegrees);
    OutResult.AspectRatio = static_cast<float>(Aspect);
    OutResult.bConstrainAspectRatio = true;
    OutResult.ProjectionMode = ECameraProjectionMode::Perspective;
    bGarageCameraViewPresented = true;
}
