#pragma once

#include "AI/RivalRecoveryOccupancy.h"

#include <array>
#include <cmath>

namespace latebrake
{
// The current Unreal game has one human and one autonomous rival. This
// selector does not pretend that the experimental six-car grid is live.
// Keep the historic recovery's longitudinal coordinate; only lateral
// alternatives are allowed when the primary landing is obstructed.
inline constexpr std::array<double,5> SingleRivalRecoveryOffsetsCm{
    0.0,-400.0,400.0,-600.0,600.0};

enum class SingleRivalRecoveryStatus
{
    InvalidGate,
    UnknownHuman,
    InvalidPrimary,
    AllCandidatesBlocked,
    PrimaryClear,
    LateralClear
};

struct SingleRivalRecoverySelection final
{
    SingleRivalRecoveryStatus Status=SingleRivalRecoveryStatus::InvalidGate;
    CircuitPose Pose{};
    double LateralOffsetCm=0.0;
    int EvaluatedCandidates=0;
    RecoveryClearanceReason PrimaryReason=RecoveryClearanceReason::UnknownOccupancy;

    bool Safe() const noexcept
    {
        return Status==SingleRivalRecoveryStatus::PrimaryClear ||
            Status==SingleRivalRecoveryStatus::LateralClear;
    }
};

// The four original recovery headings are cardinal, allowing exact
// perpendicular offsets without a floating-point change in along-track
// progress. The next gate and its approach segment remain unchanged.
inline bool TrySingleRivalRecoveryPose(int nextGate,double lateralCm,
    CircuitPose& out) noexcept
{
    out={};
    if (!IsCircuitGate(nextGate) || !std::isfinite(lateralCm) ||
        std::abs(lateralCm)>600.0)
        return false;
    out=RivalRecovery(nextGate);
    switch(nextGate)
    {
    case 0: out.Position.Y+=lateralCm; break;
    case 1: out.Position.X-=lateralCm; break;
    case 2: out.Position.Y-=lateralCm; break;
    case 3: out.Position.X+=lateralCm; break;
    default: return false;
    }
    return IsValidStartGridPose(out);
}

inline bool IsQualifiedSingleRivalHuman(const RecoveryObstacle& human) noexcept
{
    return human.Verified && human.Slot==HumanGridSlot &&
        human.ActorToken!=0 && IsValidStartGridPose(human.Pose) &&
        std::isfinite(human.VelocityX) && std::isfinite(human.VelocityY) &&
        std::hypot(human.VelocityX,human.VelocityY)<=15000.0;
}

// The observed human is mandatory. Do not treat a missing player or
// corrupt velocity as an empty road. An invalid primary pose is not a
// reason to move to some unrelated location. A successful preflight is
// only permission to attempt the existing engine-verified teleport.
inline SingleRivalRecoverySelection SelectSingleRivalRecovery(
    int nextGate,const RecoveryObstacle& human) noexcept
{
    SingleRivalRecoverySelection result;
    if (!IsCircuitGate(nextGate))
        return result;
    if (!IsQualifiedSingleRivalHuman(human))
    {
        result.Status=SingleRivalRecoveryStatus::UnknownHuman;
        return result;
    }

    std::array<RecoveryObstacle,MaxRecoveryObstacles> observed{};
    observed[0]=human;
    for (const double lateral:SingleRivalRecoveryOffsetsCm)
    {
        CircuitPose pose{};
        if (!TrySingleRivalRecoveryPose(nextGate,lateral,pose))
        {
            if (lateral==0.0)
            {
                result.Status=SingleRivalRecoveryStatus::InvalidPrimary;
                return result;
            }
            continue;
        }

        ++result.EvaluatedCandidates;
        const auto clearance=EvaluateRivalRecoveryClearance(pose,observed,1);
        if (lateral==0.0)
        {
            result.PrimaryReason=clearance.Reason;
            if (!clearance.Safe() &&
                clearance.Reason!=RecoveryClearanceReason::TrafficCollision)
            {
                result.Status=clearance.Reason==RecoveryClearanceReason::UnknownOccupancy
                    ? SingleRivalRecoveryStatus::UnknownHuman
                    : SingleRivalRecoveryStatus::InvalidPrimary;
                return result;
            }
        }
        if (clearance.Reason==RecoveryClearanceReason::UnknownOccupancy)
        {
            result.Status=SingleRivalRecoveryStatus::UnknownHuman;
            return result;
        }
        if (!clearance.Safe()) continue;

        result.Status=lateral==0.0
            ? SingleRivalRecoveryStatus::PrimaryClear
            : SingleRivalRecoveryStatus::LateralClear;
        result.Pose=pose;
        result.LateralOffsetCm=lateral;
        return result;
    }
    result.Status=SingleRivalRecoveryStatus::AllCandidatesBlocked;
    return result;
}
} // namespace latebrake
