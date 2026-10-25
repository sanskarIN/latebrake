#pragma once

namespace latebrake
{
// Portable mirror of the independent race-clock/checkpoint readiness gates.
// Authorizing physics driving must not depend only on SetGamePaused:
// Unreal may continue ticking after a rejected pause request.
struct PlayerDriveConditions final
{
    bool HasAuthoritativeMode = false;
    bool CircuitReady = false;
    bool MenuOpen = false;
    bool EnginePaused = false;
    bool FocusSuspended = false;
    bool RaceFinished = false;
    // After a restart, a finite physics body alone does not prove the
    // player actually reached the intended grid location.
    // A caller that forgets this field is NOT allowed to opt into drive.
    // The authoritative Unreal pawn supplies the actual GameMode state.
    bool RestartPlacementReady = false;
};

enum class PlayerDriveDecision
{
    Allowed,
    NoAuthoritativeMode,
    CircuitNotReady,
    MenuOpen,
    EnginePaused,
    FocusSuspended,
    RaceFinished,
    RestartPlacementUnverified
};

inline constexpr PlayerDriveDecision EvaluatePlayerDrive(
    PlayerDriveConditions state) noexcept
{
    // Deliberately fail closed. This is a gameplay input policy, not proof
    // that Chaos simulation itself can be paused on any installed platform.
    if (!state.HasAuthoritativeMode)
        return PlayerDriveDecision::NoAuthoritativeMode;
    if (!state.RestartPlacementReady)
        return PlayerDriveDecision::RestartPlacementUnverified;
    if (!state.CircuitReady)
        return PlayerDriveDecision::CircuitNotReady;
    if (state.FocusSuspended)
        return PlayerDriveDecision::FocusSuspended;
    if (state.MenuOpen)
        return PlayerDriveDecision::MenuOpen;
    if (state.EnginePaused)
        return PlayerDriveDecision::EnginePaused;
    if (state.RaceFinished)
        return PlayerDriveDecision::RaceFinished;
    return PlayerDriveDecision::Allowed;
}

inline constexpr bool CanApplyPlayerDrive(PlayerDriveConditions state) noexcept
{
    return EvaluatePlayerDrive(state) == PlayerDriveDecision::Allowed;
}
} // namespace latebrake
