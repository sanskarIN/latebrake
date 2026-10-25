#pragma once

namespace latebrake
{
// A race restart resets the authoritative clock before its Unreal pawn is
// relocated. Preserve a separate start-placement gate: merely having a
// finite, simulated chassis does NOT prove the restart actually moved it to
// the requested grid location. No engine headers or allocations (C++20).
class RestartPlacementGuard final
{
public:
    enum class State
    {
        InitialSpawn,
        AwaitingPlacement,
        RejectedPlacement,
        VerifiedPlacement
    };

    // The initial player spawn uses the actual APlayerStart path, not an
    // explicit restart. Its physical readiness is checked separately.
    bool CanRace() const noexcept
    {
        return Current == State::InitialSpawn ||
               Current == State::VerifiedPlacement;
    }

    void OnRestartRequested() noexcept
    {
        // Invalidate the old start authorization BEFORE calling Unreal.
        Current = State::AwaitingPlacement;
    }

    // Only a pending restart can accept a placement result. A rejection
    // remains latched until another explicit restart requests a fresh move.
    bool OnRestartPlacementResult(bool verified) noexcept
    {
        if (Current != State::AwaitingPlacement)
        {
            return false;
        }
        Current = verified ? State::VerifiedPlacement : State::RejectedPlacement;
        return verified;
    }

    State CurrentState() const noexcept { return Current; }

private:
    State Current = State::InitialSpawn;
};
} // namespace latebrake
