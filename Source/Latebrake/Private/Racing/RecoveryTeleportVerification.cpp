#include "Racing/RecoveryTeleportVerification.h"

#include <cmath>

namespace latebrake
{
namespace
{
constexpr double MaxRecoveryPositionErrorCm = 5.0;
constexpr double MaxRecoveryOrientationErrorDegrees = 2.0;
constexpr double MaxResidualSpeedCmPerSecond = 1.0;
constexpr double MaxResidualAngularSpeedDegreesPerSecond = 1.0;

bool IsRotationWithinTolerance(double actual, double target) noexcept
{
    // Compare equivalent wrapped Euler-axis readings (e.g. 179 and -181).
    // The caller has already rejected nonfinite inputs.
    const double difference = std::remainder(actual - target, 360.0);
    return std::isfinite(difference) &&
           std::abs(difference) <= MaxRecoveryOrientationErrorDegrees;
}
}

RecoveryTeleportDecision EvaluateRecoveryTeleport(
    const RecoveryTeleportObservation& sample) noexcept
{
    if (!sample.EngineAccepted)
    {
        return RecoveryTeleportDecision::EngineRejected;
    }
    if (!std::isfinite(sample.TargetX) || !std::isfinite(sample.TargetY) ||
        !std::isfinite(sample.TargetZ) ||
        !std::isfinite(sample.TargetYawDegrees) ||
        !std::isfinite(sample.TargetPitchDegrees) ||
        !std::isfinite(sample.TargetRollDegrees) ||
        !std::isfinite(sample.ActualX) || !std::isfinite(sample.ActualY) ||
        !std::isfinite(sample.ActualZ) ||
        !std::isfinite(sample.ActualYawDegrees) ||
        !std::isfinite(sample.ActualPitchDegrees) ||
        !std::isfinite(sample.ActualRollDegrees))
    {
        return RecoveryTeleportDecision::NonfinitePose;
    }

    const double dx = sample.ActualX - sample.TargetX;
    const double dy = sample.ActualY - sample.TargetY;
    const double dz = sample.ActualZ - sample.TargetZ;
    const double offset = std::hypot(std::hypot(dx, dy), dz);
    if (!std::isfinite(offset) || offset > MaxRecoveryPositionErrorCm)
    {
        return RecoveryTeleportDecision::PositionMismatch;
    }
    if (!IsRotationWithinTolerance(
            sample.ActualYawDegrees, sample.TargetYawDegrees))
    {
        return RecoveryTeleportDecision::HeadingMismatch;
    }
    if (!IsRotationWithinTolerance(
            sample.ActualPitchDegrees, sample.TargetPitchDegrees) ||
        !IsRotationWithinTolerance(
            sample.ActualRollDegrees, sample.TargetRollDegrees))
    {
        return RecoveryTeleportDecision::OrientationMismatch;
    }

    // A successful transform can still leave a chassis without simulation,
    // blocking collision, a usable mesh, finite motion or positive mass.
    // The engine-owned pawn gathers its live physics readiness after moving.
    if (!sample.ChassisReadyAfterMove)
    {
        return RecoveryTeleportDecision::PostMovePhysicsUnavailable;
    }
    if (!std::isfinite(sample.PostLinearVelocityX) ||
        !std::isfinite(sample.PostLinearVelocityY) ||
        !std::isfinite(sample.PostLinearVelocityZ) ||
        !std::isfinite(sample.PostAngularVelocityXDegrees) ||
        !std::isfinite(sample.PostAngularVelocityYDegrees) ||
        !std::isfinite(sample.PostAngularVelocityZDegrees))
    {
        return RecoveryTeleportDecision::NonfinitePostMoveMotion;
    }

    // Chaos teleports should not preserve enough unintended motion to
    // immediately fling the recovered car off its selected safe position.
    const double linearSpeed = std::hypot(
        std::hypot(sample.PostLinearVelocityX, sample.PostLinearVelocityY),
        sample.PostLinearVelocityZ);
    const double angularSpeed = std::hypot(
        std::hypot(sample.PostAngularVelocityXDegrees,
                   sample.PostAngularVelocityYDegrees),
        sample.PostAngularVelocityZDegrees);
    if (!std::isfinite(linearSpeed) ||
        !std::isfinite(angularSpeed) ||
        linearSpeed > MaxResidualSpeedCmPerSecond ||
        angularSpeed > MaxResidualAngularSpeedDegreesPerSecond)
    {
        return RecoveryTeleportDecision::ResidualMotion;
    }
    return RecoveryTeleportDecision::Verified;
}

bool IsRecoveryTeleportVerified(
    const RecoveryTeleportObservation& sample) noexcept
{
    return EvaluateRecoveryTeleport(sample) == RecoveryTeleportDecision::Verified;
}

const char* RecoveryTeleportDecisionName(
    RecoveryTeleportDecision decision) noexcept
{
    switch (decision)
    {
    case RecoveryTeleportDecision::Verified: return "verified";
    case RecoveryTeleportDecision::EngineRejected: return "engine-rejected";
    case RecoveryTeleportDecision::NonfinitePose: return "nonfinite-pose";
    case RecoveryTeleportDecision::PositionMismatch: return "position-mismatch";
    case RecoveryTeleportDecision::HeadingMismatch: return "heading-mismatch";
    case RecoveryTeleportDecision::OrientationMismatch:
        return "orientation-mismatch";
    case RecoveryTeleportDecision::PostMovePhysicsUnavailable:
        return "post-move-physics-unavailable";
    case RecoveryTeleportDecision::NonfinitePostMoveMotion:
        return "nonfinite-post-move-motion";
    case RecoveryTeleportDecision::ResidualMotion: return "residual-motion";
    }
    return "unknown";
}
} // namespace latebrake
