#include "Racing/RecoveryTeleportVerification.h"

#include <cmath>

namespace latebrake
{
namespace
{
constexpr double MaxRecoveryPositionErrorCm = 5.0;
constexpr double MaxRecoveryHeadingErrorDegrees = 2.0;
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
        !std::isfinite(sample.ActualX) || !std::isfinite(sample.ActualY) ||
        !std::isfinite(sample.ActualZ) ||
        !std::isfinite(sample.ActualYawDegrees))
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
    // Equivalent headings such as 179 and -181 degrees must compare equal.
    const double headingDelta =
        std::remainder(sample.ActualYawDegrees - sample.TargetYawDegrees, 360.0);
    if (!std::isfinite(headingDelta) ||
        std::abs(headingDelta) > MaxRecoveryHeadingErrorDegrees)
    {
        return RecoveryTeleportDecision::HeadingMismatch;
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
    }
    return "unknown";
}
} // namespace latebrake
