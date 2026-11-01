#pragma once

#include "AI/RivalDifficultyPreset.h"
#include "Racing/RaceEvents.h"

namespace latebrake
{
// Four real choices in the existing Quick Race menu; the fourth uses
// the already implemented single-lap TimeAttack event, not a fake circuit.
// Session-only UI selection, never a SaveGame or classification input.
struct QuickRaceMenuChoice final
{
    RivalDifficultyPreset Difficulty = RivalDifficultyPreset::Standard;
    bool SoloTimeAttack = false;
};

constexpr bool IsValidQuickRaceMenuChoice(
    QuickRaceMenuChoice choice) noexcept
{
    return IsRivalDifficultyPreset(choice.Difficulty);
}

// Browse order from the default: STANDARD -> EXPERT -> SOLO TIME ATTACK
// -> ROOKIE -> STANDARD. While SOLO is selected, its retained Expert tier
// is ignored. Cycling after SOLO consistently starts from ROOKIE.
constexpr QuickRaceMenuChoice NextQuickRaceMenuChoice(
    QuickRaceMenuChoice current) noexcept
{
    if (!IsValidQuickRaceMenuChoice(current))
    {
        return {};
    }
    if (current.SoloTimeAttack)
    {
        return {RivalDifficultyPreset::Rookie, false};
    }
    if (current.Difficulty == RivalDifficultyPreset::Expert)
    {
        return {RivalDifficultyPreset::Expert, true};
    }
    return {NextRivalDifficulty(current.Difficulty), false};
}

constexpr EventKind QuickRaceMenuEvent(
    QuickRaceMenuChoice current) noexcept
{
    return current.SoloTimeAttack && IsValidQuickRaceMenuChoice(current)
        ? EventKind::TimeAttack
        : EventKind::Circuit;
}

constexpr RivalDifficultyPreset QuickRaceRivalDifficulty(
    QuickRaceMenuChoice current) noexcept
{
    return current.SoloTimeAttack || !IsValidQuickRaceMenuChoice(current)
        ? RivalDifficultyPreset::Standard
        : current.Difficulty;
}
} // namespace latebrake
