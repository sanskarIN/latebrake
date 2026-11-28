#include "Game/LatebrakeRaceGameMode.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicles/LatebrakeCarPawn.h"

void ALatebrakeRaceGameMode::ResetPlayerSectorMotion() noexcept
{
    PlayerSectorMotion.Reset(Race.GetCheckpointCount(),Race.GetRequiredLaps());
    PlayerLapMotion.Reset(Race.GetCheckpointCount(),Race.GetRequiredLaps());
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
    const latebrake::SectorMotionReceipt Receipt{AcceptedOrdinal,Gate,Race.GetElapsedSeconds(),
        Race.GetTotalPenaltySeconds(),Race.GetLastSectorSeconds()};
    PlayerSectorMotion.AcceptGate(Receipt);
    const auto Sector=GetPlayerSectorMotionReview();
    PlayerLapMotion.Accept(Receipt,Sector.Valid?Sector.Last:latebrake::SectorMotionRow{});
}

latebrake::PlayerLapMotionReview ALatebrakeRaceGameMode::GetPlayerLapMotionReview() const noexcept
{
    return PlayerLapMotion.Read(
        Race.GetCompletedLaps()*Race.GetCheckpointCount()+Race.GetNextCheckpoint(),
        Race.GetElapsedSeconds(),Race.GetTotalPenaltySeconds(),Race.IsFinished());
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

#if !UE_BUILD_SHIPPING
void ALatebrakeRaceGameMode::LogPlayerSectorMotionAudit(int32 Gate,int32 AcceptedOrdinal) const
{
    // One record per independently accepted player gate, immediately
    // following the authoritative race audit. Do not emit frame logs.
    if (RaceAuditAttempt<=0) return;
    const auto Review=GetPlayerSectorMotionReview();
    const auto& Row=Review.Last;
    const double Elapsed=Race.GetElapsedSeconds();
    const double Penalty=Race.GetTotalPenaltySeconds();
    if (!latebrake::ValidSectorMotionClock(Elapsed,Penalty)) return;
    const bool Available=Review.Valid && Row.ValidTiming &&
        Row.Ordinal==AcceptedOrdinal && Row.Gate==Gate;
    if (!Available)
    {
        UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_SECTOR_AUDIT v=1 attempt=%d ordinal=%d gate=%d "
            "elapsed=%.9f penalty=%.9f available=0"),
            RaceAuditAttempt,AcceptedOrdinal,Gate,Elapsed,Penalty);
        return;
    }
    const auto& M=Row.Motion;
    UE_LOG(LogTemp,Display,TEXT("LATEBRAKE_SECTOR_AUDIT v=1 attempt=%d ordinal=%d gate=%d "
        "elapsed=%.9f penalty=%.9f available=1 quality=%d "
        "active=%.9f fee=%.9f official=%.9f observed=%.9f coverage=%.9f "
        "samples=%llu intervals=%llu breaks=%llu rejected=%llu recovery=%llu jump=%llu "
        "distance_cm=%.9f speed_integral=%.9f peak=%.9f lateral_integral=%.9f "
        "low=%.9f slip=%.9f reverse=%.9f partial=%.9f off=%.9f"),
        RaceAuditAttempt,AcceptedOrdinal,Gate,Elapsed,Penalty,static_cast<int32>(Row.Quality),
        Row.ActiveSeconds,Row.FeeSeconds,Row.OfficialSeconds,M.ObservedSeconds,Row.Coverage,
        static_cast<unsigned long long>(M.Samples),static_cast<unsigned long long>(M.Intervals),
        static_cast<unsigned long long>(M.Breaks),static_cast<unsigned long long>(M.Rejected),
        static_cast<unsigned long long>(M.RecoveryBreaks),static_cast<unsigned long long>(M.PositionBreaks),
        M.PlanarDistanceCm,M.SpeedIntegral,M.PeakSpeed,M.LateralIntegral,
        M.LowSpeedSeconds,M.HighSlipSeconds,M.ReverseSeconds,M.PartialRoadSeconds,M.FullOffRoadSeconds);
}
#endif
