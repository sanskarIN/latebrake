#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "UI/TouchDriveState.h"
#include "Racing/DrivingSessionStats.h"
#include "Racing/DriftComboMeter.h"
#include "Racing/VehicleGForceMeter.h"
#include "Racing/ChassisSlipAngleMeter.h"
#include "Racing/ObservedBrakeDistance.h"
#include "Vehicles/CameraPresets.h"
#include "Vehicles/HeldLookBackCamera.h"
#include "Vehicles/SpeedReactiveCameraFov.h"
#include "Vehicles/ArcadeNitroBoost.h"
#include "Vehicles/WheelVisualMotion.h"
#include "Vehicles/VehicleSignalLights.h"
#include "UI/TouchPressTracker.h"
#include "UI/TouchViewportGuard.h"
#include "LatebrakeCarPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UPointLightComponent;

UENUM(BlueprintType)
enum class ELatebrakeHandlingMode : uint8
{
    Arcade UMETA(DisplayName = "Arcade"),
    SimulationInspired UMETA(DisplayName = "Simulation Inspired")
};

/**
 * Asset-light starter car: Chaos rigid-body forces acting on an engine cube.
 * Later milestones will replace this controller with Chaos wheeled-vehicle
 * suspension and tire simulation, using original vehicle skeletal meshes.
 */
UCLASS()
class LATEBRAKE_API ALatebrakeCarPawn : public APawn
{
    GENERATED_BODY()

public:
    ALatebrakeCarPawn();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void UnPossessed() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // Not every possessed pawn has a usable rigid body. The race clock,
    // checkpoint gates and paid recovery must all use this same check.
    // Actual installed collision and vehicle handling still require QA.
    bool IsRacePhysicsReady() const;
    float GetSpeedKmH() const;
    // Signed Chaos forward velocity for advisory start telemetry; NaN if unavailable.
    float GetForwardSpeedKmH() const;
    const latebrake::DrivingSessionStats& GetDrivingStats() const { return DrivingStats; }
    latebrake::CameraView GetCameraView() const noexcept { return CameraViews.Current(); }
    void ResetDrivingStats() { LookBackReleased(); ResetSpeedCameraFov(); DrivingStats.Reset(); DriftMeter.Reset(); GForceMeter.Reset(); SlipAngleMeter.Reset(); BrakeDistance.Reset(); NitroBoost.Reset(); WheelMotion.Reset(); ApplyWheelVisualPose(); }
    const latebrake::DriftComboMeter& GetDriftMeter() const { return DriftMeter; }
    const latebrake::VehicleGForceMeter& GetVehicleGForceMeter() const { return GForceMeter; }
    const latebrake::ChassisSlipAngleMeter& GetChassisSlipAngleMeter() const { return SlipAngleMeter; }
    const latebrake::ObservedBrakeDistance& GetObservedBrakeDistance() const { return BrakeDistance; }
    void SetAutonomousControls(float InThrottle, float InSteering, bool bInBrake);
    void ClearDriverInputs();
    void HandleAppInputInterruption();
    void HandleAppFocusLost();

    void SetGarageTuning(float Acceleration, float Grip, float TopSpeed, int32 VehicleId);
    int32 GetSelectedVehicleId() const { return GarageVehicleId; }
    // Every RecoverTo (including race restart) changes this revision so
    // checkpoint actors never interpolate across an authorized teleport.
    uint64 GetTeleportRevision() const { return TeleportRevision; }
    ELatebrakeHandlingMode GetHandlingMode() const { return HandlingMode; }
    bool IsTouchSteeringActive() const { return TouchDrive.SteeringActive(); }
    bool IsTouchThrottleActive() const { return TouchDrive.ThrottleActive(); }
    bool IsTouchBrakeActive() const { return TouchDrive.BrakeActive(); }
    bool IsHandbrakeHeld() const noexcept { return bHandbrakeHeld; }
    const latebrake::ArcadeNitroBoost& GetNitroBoost() const noexcept { return NitroBoost; }

    // Physical placement must succeed before GameMode can commit its five-
    // second recovery charge. The Blueprint-compatible void API remains
    // available to existing restart/rival callers.
    bool TryRecoverTo(const FVector& Position, const FRotator& Rotation);

    UFUNCTION(BlueprintCallable, Category = "Latebrake|Vehicle")
    void RecoverTo(const FVector& Position, const FRotator& Rotation);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> Chassis;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> Cabin;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vehicle")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> FrontLeftWheel;

    UPROPERTY(VisibleAnywhere, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> FrontRightWheel;

    UPROPERTY(VisibleAnywhere, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> RearLeftWheel;

    UPROPERTY(VisibleAnywhere, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> RearRightWheel;

    // Small, collision-free wheel spokes make cylinder rotation visible.
    UPROPERTY(VisibleAnywhere, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> FrontLeftSpoke;
    UPROPERTY(VisibleAnywhere, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> FrontRightSpoke;
    UPROPERTY(VisibleAnywhere, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> RearLeftSpoke;
    UPROPERTY(VisibleAnywhere, Category = "Vehicle")
    TObjectPtr<UStaticMeshComponent> RearRightSpoke;

    // Bounded, decorative rear lights only. They are not physics sensors.
    UPROPERTY(VisibleAnywhere, Category = "Vehicle|Visual Signals")
    TObjectPtr<UPointLightComponent> RearLeftSignal;
    UPROPERTY(VisibleAnywhere, Category = "Vehicle|Visual Signals")
    TObjectPtr<UPointLightComponent> RearRightSignal;
    UPROPERTY(VisibleAnywhere, Category = "Vehicle|Visual Signals")
    TObjectPtr<UPointLightComponent> ReverseSignal;

private:
    void ThrottleInput(float Value);
    void SteerInput(float Value);
    void BrakePressed();
    void BrakeReleased();
    void HandbrakePressed();
    void HandbrakeReleased();
    void NitroPressed();
    void NitroReleased();
    void NeutralizeMotionWhenDrivingBlocked();
    void UpdateWheelVisuals(double DeltaSeconds, double ForwardSpeedCmPerSecond,
                            double VisualSteering);
    void ApplyWheelVisualPose();
    void ApplySignalLights(bool bRaceDrivingAllowed, bool bEffectiveBraking,
                           double SignedForwardSpeedCmPerSecond);
    void RecoverPressed();
    void RestartPressed();
    void ChangeCamera();
    void LookBackPressed();
    void LookBackReleased();
    void ToggleBestLapGhost();
    void ApplyCameraPreset();
    void ResetSpeedCameraFov();
    void UpdateSpeedCameraFov(double FrameSeconds, double SignedForwardSpeedKmH);
    void TogglePause();
    void TouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
    void TouchMoved(ETouchIndex::Type FingerIndex, FVector Location);
    void TouchReleased(ETouchIndex::Type FingerIndex, FVector Location);
    void ApplyTouch(ETouchIndex::Type FingerIndex, const FVector& Location,
                    bool bAllowAcquire);
    void SelectCircuit();
    void SelectTimeAttack();
    void CareerNext();
    void CareerStart();
    void MenuToggle();
    void MenuNext();
    void MenuPrevious();
    void MenuDetailsNext();
    void MenuDetailsPrevious();
    void MenuActivate();
    void NextGarageVehicle();
    void UnlockGarageVehicle();
    void UpgradeGarageEngine();
    void UpgradeGarageGrip();
    void SelectArcade();
    void SelectSimulation();

    FDelegateHandle BackgroundInputHandle;
    FDelegateHandle DeactivatedInputHandle;
    FDelegateHandle ForegroundInputHandle;

    float Throttle = 0.f;
    float Steering = 0.f;
    float SmoothedSteering = 0.f;
    float SmoothedThrottle = 0.f;
    float GarageAcceleration = 1.f;
    float GarageGrip = 1.f;
    float GarageTopSpeed = 1.f;
    int32 GarageVehicleId = 0;
    uint64 TeleportRevision = 0;
    bool bBraking = false;
    bool bHandbrakeHeld = false;
    float CurrentNitroSpeedCapScale = 1.f;
    latebrake::CameraViewCycle CameraViews;
    latebrake::HeldLookBackCamera LookBackCamera;
    latebrake::SpeedReactiveCameraFov ReactiveFov;
    latebrake::DrivingSessionStats DrivingStats;
    latebrake::DriftComboMeter DriftMeter;
    latebrake::VehicleGForceMeter GForceMeter;
    latebrake::ChassisSlipAngleMeter SlipAngleMeter;
    latebrake::ObservedBrakeDistance BrakeDistance;
    latebrake::ArcadeNitroBoost NitroBoost;
    latebrake::WheelVisualMotion WheelMotion;
    latebrake::TouchDriveState TouchDrive;
    latebrake::TouchPressTracker TouchPresses;
    latebrake::TouchViewportGuard TouchViewport;

    UPROPERTY(EditAnywhere, Category = "Vehicle")
    ELatebrakeHandlingMode HandlingMode = ELatebrakeHandlingMode::Arcade;
};
