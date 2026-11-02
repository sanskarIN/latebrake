#pragma once

// Source-only, engine-independent policy for the optional 2D Windows preview.
// The user-facing events must agree with the number of cars actually spawned,
// simulated, rendered and included in race results.
namespace latebrake_preview
{
struct RaceModePolicy final
{
    int RequiredLaps = 0;
    int RivalCount = 0;
    bool Valid = false;
};

constexpr RaceModePolicy GetRaceModePolicy(int eventIndex) noexcept
{
    switch (eventIndex)
    {
    case 0: return {3, 3, true}; // Circuit Race: three local AI rivals.
    case 1: return {1, 0, true}; // Time Attack: truly solo timed lap.
    case 2: return {2, 3, true}; // Rival Duel: racing AI determines outcome.
    case 3: return {2, 0, true}; // Checkpoint Rush: solo timed challenge.
    default: return {};         // Unknown mode must not spawn cars.
    }
}

constexpr bool IsSoloMode(RaceModePolicy policy) noexcept
{
    return policy.Valid && policy.RivalCount == 0;
}
} // namespace latebrake_preview
