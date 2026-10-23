#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
#include "UI/TouchDriveState.h"
#include "UI/TouchPressTracker.h"
#include "UI/TouchViewportGuard.h"
#include "LatebrakeCarPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

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
    void SetAutonomousControls(float InThrottle, float InSteering, bool bInBrake);
    void ClearDriverInputs();
    void HandleAppInputInterruption();

    void SetGarageTuning(float Acceleration, float Grip, float TopSpeed, int32 VehicleId);
    int32 GetSelectedVehicleId() const { return GarageVehicleId; }
    // Every RecoverTo (including race restart) changes this revision so
    // checkpoint actors never interpolate across an authorized teleport.
    uint64 GetTeleportRevision() const { return TeleportRevision; }
    ELatebrakeHandlingMode GetHandlingMode() const { return HandlingMode; }
    bool IsTouchSteeringActive() const { return TouchDrive.SteeringActive(); }
    bool IsTouchThrottleActive() const { return TouchDrive.ThrottleActive(); }
    bool IsTouchBrakeActive() const { return TouchDrive.BrakeActive(); }

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

private:
    void ThrottleInput(float Value);
    void SteerInput(float Value);
    void BrakePressed();
    void BrakeReleased();
    void RecoverPressed();
    void RestartPressed();
    void ChangeCamera();
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
    bool bNearCamera = false;
    latebrake::TouchDriveState TouchDrive;
    latebrake::TouchPressTracker TouchPresses;
    latebrake::TouchViewportGuard TouchViewport;

    UPROPERTY(EditAnywhere, Category = "Vehicle")
    ELatebrakeHandlingMode HandlingMode = ELatebrakeHandlingMode::Arcade;
};
