#include "AI/LatebrakeRivalCar.h"

#include "AI/RivalPilot.h"
#include "Engine/World.h"
#include "Game/LatebrakeRaceGameMode.h"

ALatebrakeRivalCar::ALatebrakeRivalCar()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void ALatebrakeRivalCar::Tick(float DeltaSeconds)
{
    // Force integration is handled by the base car and runs even without a
    // player/AI controller. Only the next input is planned here.
    Super::Tick(DeltaSeconds);

    const ALatebrakeRaceGameMode* Mode =
        GetWorld() ? GetWorld()->GetAuthGameMode<ALatebrakeRaceGameMode>() : nullptr;
    if (!Mode || Mode->GetRace().IsFinished() ||
        Mode->GetSelectedEvent() != latebrake::EventKind::Circuit)
    {
        SetAutonomousControls(0.f, 0.f, true);
        return;
    }

    static const FVector Waypoints[] =
    {
        FVector(2800.f, -1800.f, 0.f),
        FVector(2800.f, 1800.f, 0.f),
        FVector(-2800.f, 1800.f, 0.f),
        FVector(-2800.f, -1800.f, 0.f)
    };
    constexpr int32 Count = UE_ARRAY_COUNT(Waypoints);
    const FVector Position = GetActorLocation();
    const FVector Velocity = GetVelocity();
    const double FlatSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();
    const double Heading = FMath::DegreesToRadians(static_cast<double>(GetActorRotation().Yaw));

    auto Command = latebrake::PlanRivalTurn(
        Position.X, Position.Y, Heading, FlatSpeed,
        Waypoints[NextWaypoint].X, Waypoints[NextWaypoint].Y);
    if (Command.Arrived)
    {
        NextWaypoint = (NextWaypoint + 1) % Count;
        Command = latebrake::PlanRivalTurn(
            Position.X, Position.Y, Heading, FlatSpeed,
            Waypoints[NextWaypoint].X, Waypoints[NextWaypoint].Y);
    }

    SetAutonomousControls(
        static_cast<float>(Command.Throttle),
        static_cast<float>(Command.Steering),
        Command.Brake);
}
