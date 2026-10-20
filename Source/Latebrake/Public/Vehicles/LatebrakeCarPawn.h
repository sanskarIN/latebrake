#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"
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

    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    float GetSpeedKmH() const;
    void SetAutonomousControls(float InThrottle, float InSteering, bool bInBrake);
    void ClearDriverInputs();
    void SetGarageTuning(float Acceleration, float Grip, float TopSpeed, int32 VehicleId);
    int32 GetSelectedVehicleId() const { return GarageVehicleId; }
    ELatebrakeHandlingMode GetHandlingMode() const { return HandlingMode; }
    bool IsTouchSteeringActive() const { return SteeringFinger != INDEX_NONE; }
    bool IsTouchThrottleActive() const { return bTouchThrottle; }
    bool IsTouchBrakeActive() const { return bTouchBrake; }

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
    void NextGarageVehicle();
    void UnlockGarageVehicle();
    void UpgradeGarageEngine();
    void UpgradeGarageGrip();
    void SelectArcade();
    void SelectSimulation();

    float Throttle = 0.f;
    float Steering = 0.f;
    float SmoothedSteering = 0.f;
    float SmoothedThrottle = 0.f;
    float GarageAcceleration = 1.f;
    float GarageGrip = 1.f;
    float GarageTopSpeed = 1.f;
    int32 GarageVehicleId = 0;
    bool bBraking = false;
    bool bNearCamera = false;
    int32 SteeringFinger = INDEX_NONE;
    int32 PedalFinger = INDEX_NONE;
    float TouchSteering = 0.f;
    bool bTouchThrottle = false;
    bool bTouchBrake = false;

    UPROPERTY(EditAnywhere, Category = "Vehicle")
    ELatebrakeHandlingMode HandlingMode = ELatebrakeHandlingMode::Arcade;
};
