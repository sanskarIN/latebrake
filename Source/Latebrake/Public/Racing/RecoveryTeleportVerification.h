#pragma once

namespace latebrake
{
// The Unreal actor reports whether its teleport was accepted and where the
// chassis actually ended up. Pure C++20 for independent negative tests.
struct RecoveryTeleportObservation final
{
    bool EngineAccepted = false;
    double TargetX = 0.0;
    double TargetY = 0.0;
    double TargetZ = 0.0;
    double TargetYawDegrees = 0.0;
    double ActualX = 0.0;
    double ActualY = 0.0;
    double ActualZ = 0.0;
    double ActualYawDegrees = 0.0;
};

enum class RecoveryTeleportDecision
{
    Verified,
    EngineRejected,
    NonfinitePose,
    PositionMismatch,
    HeadingMismatch
};

RecoveryTeleportDecision EvaluateRecoveryTeleport(
    const RecoveryTeleportObservation& sample) noexcept;

bool IsRecoveryTeleportVerified(
    const RecoveryTeleportObservation& sample) noexcept;

const char* RecoveryTeleportDecisionName(
    RecoveryTeleportDecision decision) noexcept;
} // namespace latebrake
