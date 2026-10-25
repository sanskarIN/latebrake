#pragma once

namespace latebrake
{
// Unreal supplies the engine result plus a post-teleport snapshot. A reported
// successful move is not sufficient for a paid recovery if the physical
// chassis is unusable, rotated differently or still moving unexpectedly.
// All units are Unreal centimetres, degrees and seconds. Portable C++20.
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

    // Append new fields so older explicitly constructed source snapshots
    // remain well-formed, but default to NOT ready until Unreal verifies the
    // post-move body. Every production caller must provide all fields.
    double TargetPitchDegrees = 0.0;
    double TargetRollDegrees = 0.0;
    double ActualPitchDegrees = 0.0;
    double ActualRollDegrees = 0.0;
    bool ChassisReadyAfterMove = false;
    double PostLinearVelocityX = 0.0;
    double PostLinearVelocityY = 0.0;
    double PostLinearVelocityZ = 0.0;
    double PostAngularVelocityXDegrees = 0.0;
    double PostAngularVelocityYDegrees = 0.0;
    double PostAngularVelocityZDegrees = 0.0;
};

enum class RecoveryTeleportDecision
{
    Verified,
    EngineRejected,
    NonfinitePose,
    PositionMismatch,
    HeadingMismatch,
    OrientationMismatch,
    PostMovePhysicsUnavailable,
    NonfinitePostMoveMotion,
    ResidualMotion
};

RecoveryTeleportDecision EvaluateRecoveryTeleport(
    const RecoveryTeleportObservation& sample) noexcept;

bool IsRecoveryTeleportVerified(
    const RecoveryTeleportObservation& sample) noexcept;

const char* RecoveryTeleportDecisionName(
    RecoveryTeleportDecision decision) noexcept;
} // namespace latebrake
