#pragma once

#include "AI/RivalPilot.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Session-only quick-race tuning for the existing, single physical rival.
// Career always uses Standard; this never alters vehicle specs, checkpoints,
// rewards, garage credits, saved progress, or collision/steering authority.
enum class RivalDifficultyPreset
{
    Rookie,
    Standard,
    Expert
};

constexpr bool IsRivalDifficultyPreset(RivalDifficultyPreset value) noexcept
{
    return value == RivalDifficultyPreset::Rookie ||
           value == RivalDifficultyPreset::Standard ||
           value == RivalDifficultyPreset::Expert;
}

constexpr RivalDifficultyPreset NextRivalDifficulty(
    RivalDifficultyPreset current) noexcept
{
    switch (current)
    {
    case RivalDifficultyPreset::Rookie: return RivalDifficultyPreset::Standard;
    case RivalDifficultyPreset::Standard: return RivalDifficultyPreset::Expert;
    case RivalDifficultyPreset::Expert: return RivalDifficultyPreset::Rookie;
    default: return RivalDifficultyPreset::Standard;
    }
}

constexpr const char* RivalDifficultyName(
    RivalDifficultyPreset current) noexcept
{
    switch (current)
    {
    case RivalDifficultyPreset::Rookie: return "ROOKIE";
    case RivalDifficultyPreset::Standard: return "STANDARD";
    case RivalDifficultyPreset::Expert: return "EXPERT";
    default: return "UNAVAILABLE";
    }
}

constexpr RivalDifficultyPreset EffectiveRivalDifficulty(
    RivalDifficultyPreset selected, bool isCareerRace) noexcept
{
    return isCareerRace || !IsRivalDifficultyPreset(selected)
        ? RivalDifficultyPreset::Standard : selected;
}

// Apply this AFTER route/corner planning, BEFORE existing physical grip,
// traffic, heat, yaw, slip and damage safety limits. Do not change steering,
// brake or route destination, never clear an already requested brake, and
// never increase throttle when any prior planner demanded zero.
inline PilotCommand ApplyRivalDifficulty(
    PilotCommand command, RivalDifficultyPreset difficulty,
    double actualPlanarSpeedCmPerSecond, bool physicalRaceReady) noexcept
{
    if (!physicalRaceReady || !IsRivalDifficultyPreset(difficulty) ||
        !std::isfinite(actualPlanarSpeedCmPerSecond) ||
        actualPlanarSpeedCmPerSecond < 0.0 ||
        actualPlanarSpeedCmPerSecond > 10000.0 ||
        !std::isfinite(command.Throttle) ||
        command.Throttle < 0.0 || command.Throttle > 1.0 ||
        !std::isfinite(command.Steering) ||
        command.Steering < -1.0 || command.Steering > 1.0)
    {
        return {}; // Fail-closed: no motor, neutral steering, brake held.
    }

    if (command.Brake || command.Arrived || command.Throttle <= 0.0)
    {
        return command;
    }

    if (difficulty == RivalDifficultyPreset::Rookie)
    {
        // Less maximum motor demand, never added acceleration/brake inputs.
        command.Throttle = std::min(command.Throttle, 0.55);
    }
    else if (difficulty == RivalDifficultyPreset::Expert &&
             // Modest increase only on an observed straight, never when
             // slowing for a corner or at high road speeds.
             std::abs(command.Steering) <= 0.12 &&
             actualPlanarSpeedCmPerSecond >= 600.0 &&
             actualPlanarSpeedCmPerSecond <= 2800.0)
    {
        command.Throttle = std::min(1.0, command.Throttle + 0.08);
    }
    // Standard is a bit-for-bit pass-through for the existing rival.
    return command;
}
} // namespace latebrake
