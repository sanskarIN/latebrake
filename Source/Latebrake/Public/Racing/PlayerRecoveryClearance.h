#pragma once

#include "AI/RivalRecoveryOccupancy.h"
#include "Racing/RecoveryCharge.h"

#include <array>
#include <cmath>

namespace latebrake
{
// Recovery remains the original game-only paid teleport. Do not move
// the player to a different lane, skip a gate or change the five-second
// charge. In the single-rival scene we can reject a landing that the
// observed opponent occupies or is moving through. Time Attack is an
// explicitly solo scene, not a missing/unknown traffic observation.
enum class PlayerRecoveryClearanceStatus
{
    InvalidGate,
    InvalidLanding,
    UnknownTraffic,
    Occupied,
    Clear
};

struct PlayerRecoveryClearance final
{
    PlayerRecoveryClearanceStatus Status=PlayerRecoveryClearanceStatus::InvalidGate;
    CircuitPose Pose{};
    int Gate=-1;
    bool Safe() const noexcept { return Status==PlayerRecoveryClearanceStatus::Clear; }
};

inline PlayerRecoveryClearance EvaluatePlayerRecoveryClearance(
    int nextGate,bool expectsRival,bool observedRival,
    const RecoveryObstacle& rival) noexcept
{
    PlayerRecoveryClearance result;
    if (!IsCircuitGate(nextGate))
        return result;

    const auto pose=PlayerRecovery(nextGate);
    result.Gate=nextGate;
    result.Status=PlayerRecoveryClearanceStatus::InvalidLanding;
    if (!RecoveryChassisFitsPhysicalAsphalt(pose) ||
        StartGridFootprintTouchesBarrier(pose) ||
        EvaluateCircuitSpawnPose(pose)!=CircuitSpawnDecision::Safe)
        return result;

    if (expectsRival!=observedRival)
    {
        result.Status=PlayerRecoveryClearanceStatus::UnknownTraffic;
        return result;
    }
    if (expectsRival)
    {
        if (!rival.Verified || rival.Slot!=PrimaryRivalGridSlot ||
            rival.ActorToken==0)
        {
            result.Status=PlayerRecoveryClearanceStatus::UnknownTraffic;
            return result;
        }
        std::array<RecoveryObstacle,MaxRecoveryObstacles> observed{};
        observed[0]=rival;
        const auto clearance=EvaluateRivalRecoveryClearance(pose,observed,1);
        if (!clearance.Safe())
        {
            result.Status=clearance.Reason==RecoveryClearanceReason::TrafficCollision
                ? PlayerRecoveryClearanceStatus::Occupied
                : PlayerRecoveryClearanceStatus::UnknownTraffic;
            return result;
        }
    }
    result.Pose=pose;
    result.Status=PlayerRecoveryClearanceStatus::Clear;
    return result;
}

// Short race-clock-based notice of the last *actual* request. No real
// time delays, timers, extra fees, or automatic retry; a paused race
// does not consume the notice interval. Restart clears prior attempts.
enum class PlayerRecoveryNotice
{
    None,
    Occupied,
    TrafficUnavailable,
    LandingUnavailable,
    FeeUnavailable,
    PlacementUnverified,
    VerifiedAndCharged
};

inline PlayerRecoveryNotice DescribeRecoveryClearanceFailure(
    PlayerRecoveryClearanceStatus status) noexcept
{
    switch(status)
    {
    case PlayerRecoveryClearanceStatus::Occupied:
        return PlayerRecoveryNotice::Occupied;
    case PlayerRecoveryClearanceStatus::UnknownTraffic:
        return PlayerRecoveryNotice::TrafficUnavailable;
    case PlayerRecoveryClearanceStatus::InvalidGate:
    case PlayerRecoveryClearanceStatus::InvalidLanding:
        return PlayerRecoveryNotice::LandingUnavailable;
    default: return PlayerRecoveryNotice::None;
    }
}

class PlayerRecoveryNoticeState final
{
public:
    static constexpr double LifetimeSeconds=3.0;
    static constexpr double MaximumRaceSeconds=604800.0;

    void Reset() noexcept { Notice=PlayerRecoveryNotice::None; RecordedSeconds=0.0; }

    bool Record(PlayerRecoveryNotice notice,double raceSeconds) noexcept
    {
        if (!ValidNotice(notice) || !ValidTime(raceSeconds))
        {
            Reset();
            return false;
        }
        Notice=notice;
        RecordedSeconds=raceSeconds;
        return true;
    }

    PlayerRecoveryNotice At(double currentRaceSeconds) const noexcept
    {
        if (!ValidTime(currentRaceSeconds) || currentRaceSeconds<RecordedSeconds ||
            currentRaceSeconds-RecordedSeconds>LifetimeSeconds)
            return PlayerRecoveryNotice::None;
        return Notice;
    }

private:
    static bool ValidTime(double seconds) noexcept
    {
        return std::isfinite(seconds) && seconds>=0.0 && seconds<=MaximumRaceSeconds;
    }

    static bool ValidNotice(PlayerRecoveryNotice notice) noexcept
    {
        switch(notice)
        {
        case PlayerRecoveryNotice::Occupied:
        case PlayerRecoveryNotice::TrafficUnavailable:
        case PlayerRecoveryNotice::LandingUnavailable:
        case PlayerRecoveryNotice::FeeUnavailable:
        case PlayerRecoveryNotice::PlacementUnverified:
        case PlayerRecoveryNotice::VerifiedAndCharged:
            return true;
        default: return false;
        }
    }

    PlayerRecoveryNotice Notice=PlayerRecoveryNotice::None;
    double RecordedSeconds=0.0;
};

// A real attempt first passes the existing clock/driver eligibility and
// staged fee transaction. A blocked landing must not invoke the engine
// callback at all. Distinguish failure before placement from an attempted
// but unverified physical move. This helper cannot itself move a pawn.
template<typename TryVerifiedPlacement>
PlayerRecoveryNotice TryPaidRecoveryWithClearance(
    RaceProgress& race,int& recoveries,bool circuitReady,bool inactive,
    bool driverReady,const PlayerRecoveryClearance& clearance,
    TryVerifiedPlacement&& tryPlacement)
{
    if (!CanScorePlayerCheckpoint(race,circuitReady,inactive,driverReady))
        return PlayerRecoveryNotice::None;
    if (!IsCircuitGate(clearance.Gate) ||
        clearance.Gate!=race.GetNextCheckpoint())
        return PlayerRecoveryNotice::LandingUnavailable;
    if (!clearance.Safe())
        return DescribeRecoveryClearanceFailure(clearance.Status);

    // Clearance belongs to the exact current checkpoint. A stale or
    // altered candidate must never inherit a previous landing approval.
    const auto expected=PlayerRecovery(clearance.Gate);
    if (clearance.Pose.Position.X!=expected.Position.X ||
        clearance.Pose.Position.Y!=expected.Position.Y ||
        clearance.Pose.HeadingDegrees!=expected.HeadingDegrees)
        return PlayerRecoveryNotice::LandingUnavailable;

    bool attempted=false;
    const bool committed=TryChargePlayerRecoveryAfterVerifiedTeleport(
        race,recoveries,circuitReady,inactive,driverReady,[&]() {
            attempted=true;
            return static_cast<bool>(
                std::forward<TryVerifiedPlacement>(tryPlacement)(clearance.Pose));
        });
    if (committed) return PlayerRecoveryNotice::VerifiedAndCharged;
    return attempted ? PlayerRecoveryNotice::PlacementUnverified
                     : PlayerRecoveryNotice::FeeUnavailable;
}
} // namespace latebrake
