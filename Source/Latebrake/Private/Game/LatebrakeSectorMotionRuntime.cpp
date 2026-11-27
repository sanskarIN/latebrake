#include "Game/LatebrakeRaceGameMode.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/LatebrakeCarPawn.h"

void ALatebrakeRaceGameMode::ResetPlayerSectorMotion() noexcept
{
    PlayerSectorMotion.Reset(Race.GetCheckpointCount(),Race.GetRequiredLaps());
    SectorMotionPawn.Reset();
    bSectorMotionPawnBound=false;
}

void ALatebrakeRaceGameMode::ObservePlayerSectorMotion(double LiveDelta,bool bClockAdvanced)
{
    if (!bClockAdvanced || Race.IsFinished() || !IsRaceStartGreen() ||
        !HasLocalPlayerDriver() || !IsRestartPlacementReady())
    {
        PlayerSectorMotion.Suspend();
        return;
    }
    const auto* Car=Cast<ALatebrakeCarPawn>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!IsValid(Car) || !Car->IsLocallyControlled() || !Car->IsRacePhysicsReady() ||
        Car->GetWorld()!=GetWorld() ||
        (bSectorMotionPawnBound && SectorMotionPawn.Get()!=Car))
    {
        PlayerSectorMotion.Suspend();
        return;
    }
    // A replacement actor cannot borrow the earlier generation's samples,
    // even if Unreal reuses an actor identifier during this run.
    if (!bSectorMotionPawnBound)
    {
        SectorMotionPawn=const_cast<ALatebrakeCarPawn*>(Car);
        bSectorMotionPawnBound=true;
    }
    const FVector P=Car->GetActorLocation();
    const FVector V=Car->GetVelocity();
    const auto Heading=Car->GetActorRotation();
    if (P.ContainsNaN() || V.ContainsNaN() || Heading.ContainsNaN())
    {
        PlayerSectorMotion.Suspend();
        return;
    }
    latebrake::SectorMotionFrame Sample;
    Sample.Actor=static_cast<std::uint64_t>(Car->GetUniqueID())+1u;
    Sample.EngineFrame=static_cast<std::uint64_t>(GFrameCounter);
    Sample.SessionRevision=CheckpointSession.Current();
    Sample.TeleportRevision=Car->GetTeleportRevision();
    Sample.Ordinal=Race.GetCompletedLaps()*Race.GetCheckpointCount()+Race.GetNextCheckpoint();
    Sample.ElapsedSeconds=Race.GetElapsedSeconds();
    Sample.PenaltySeconds=Race.GetTotalPenaltySeconds();
    Sample.AdvancedSeconds=LiveDelta;
    Sample.Position={P.X,P.Y};
    Sample.WorldZ=P.Z;
    Sample.VelocityX=V.X;
    Sample.VelocityY=V.Y;
    Sample.VelocityZ=V.Z;
    Sample.HeadingDegrees=Heading.Yaw;
    Sample.PhysicalBodyObserved=true;
    Sample.ClockAdvanced=true;
    PlayerSectorMotion.Observe(Sample);
}

void ALatebrakeRaceGameMode::ObservePlayerSectorGate(int32 Gate,int32 AcceptedOrdinal) noexcept
{
    // This caller is reached only after the actual ordered gate was
    // accepted. Keep the official split, active time and fees distinct.
    PlayerSectorMotion.AcceptGate({AcceptedOrdinal,Gate,Race.GetElapsedSeconds(),
        Race.GetTotalPenaltySeconds(),Race.GetLastSectorSeconds()});
}

latebrake::SectorMotionReview ALatebrakeRaceGameMode::GetPlayerSectorMotionReview() const noexcept
{
    return PlayerSectorMotion.Read(
        Race.GetCompletedLaps()*Race.GetCheckpointCount()+Race.GetNextCheckpoint(),
        Race.GetElapsedSeconds(),Race.GetTotalPenaltySeconds(),Race.IsFinished());
}

latebrake::SectorMotionHistory ALatebrakeRaceGameMode::GetPlayerSectorMotionHistory() const noexcept
{
    if (!Race.IsFinished()) return {};
    return PlayerSectorMotion.ReadFinishedHistory(
        Race.GetCompletedLaps()*Race.GetCheckpointCount()+Race.GetNextCheckpoint(),
        Race.GetElapsedSeconds(),Race.GetTotalPenaltySeconds());
}
