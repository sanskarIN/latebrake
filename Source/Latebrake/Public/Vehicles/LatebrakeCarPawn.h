#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
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
    ELatebrakeHandlingMode GetHandlingMode() const { return HandlingMode; }

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
    void SelectArcade();
    void SelectSimulation();

    float Throttle = 0.f;
    float Steering = 0.f;
    bool bBraking = false;
    bool bNearCamera = false;

    UPROPERTY(EditAnywhere, Category = "Vehicle")
    ELatebrakeHandlingMode HandlingMode = ELatebrakeHandlingMode::Arcade;
};
