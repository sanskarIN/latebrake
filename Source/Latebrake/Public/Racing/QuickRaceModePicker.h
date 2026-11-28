#pragma once

#include "AI/RivalDifficultyPreset.h"
#include "Racing/RaceEvents.h"

namespace latebrake
{
// Practice reuses actual configured checkpoints and physics. Its lap
// counts deliberately differ from the ranked/persisted event definitions.
enum class QuickPracticePreset
{
    None,
    SoloThree,
    SoloFive,
    RivalFive,
    RivalTen
};

struct QuickPracticeDefinition final
{
    bool Valid = false;
    EventKind Event = EventKind::Circuit;
    int CheckpointCount = 0;
    int RequiredLaps = 0;
    bool SpawnRival = false;
    const char* Name = "";
};

constexpr QuickPracticeDefinition GetQuickPracticeDefinition(
    QuickPracticePreset preset) noexcept
{
    switch (preset)
    {
    case QuickPracticePreset::SoloThree:
        return {true, EventKind::TimeAttack, 4, 3, false, "SOLO PRACTICE / 3 LAPS"};
    case QuickPracticePreset::SoloFive:
        return {true, EventKind::TimeAttack, 4, 5, false, "SOLO PRACTICE / 5 LAPS"};
    case QuickPracticePreset::RivalFive:
        return {true, EventKind::Circuit, 4, 5, true, "RIVAL PRACTICE / 5 LAPS"};
    case QuickPracticePreset::RivalTen:
        return {true, EventKind::Circuit, 4, 10, true, "RIVAL PRACTICE / 10 LAPS"};
    default: return {};
    }
}

constexpr bool IsQuickPractice(QuickPracticePreset preset) noexcept
{
    return GetQuickPracticeDefinition(preset).Valid;
}

// Session-only pending UI choice; never a SaveGame field. The original
// four choices remain available, followed by four non-persistent practices.
struct QuickRaceMenuChoice final
{
    RivalDifficultyPreset Difficulty = RivalDifficultyPreset::Standard;
    bool SoloTimeAttack = false;
    QuickPracticePreset Practice = QuickPracticePreset::None;
};

constexpr bool IsValidQuickRaceMenuChoice(
    QuickRaceMenuChoice choice) noexcept
{
    return IsRivalDifficultyPreset(choice.Difficulty) &&
        (choice.Practice == QuickPracticePreset::None ||
         (IsQuickPractice(choice.Practice) && !choice.SoloTimeAttack &&
          choice.Difficulty == RivalDifficultyPreset::Standard));
}

constexpr QuickRaceMenuChoice PracticeMenuChoice(QuickPracticePreset preset) noexcept
{
    return IsQuickPractice(preset)
        ? QuickRaceMenuChoice{RivalDifficultyPreset::Standard, false, preset}
        : QuickRaceMenuChoice{};
}

// Browse: STANDARD -> EXPERT -> SOLO TIME ATTACK -> ROOKIE ->
// SOLO 3 -> SOLO 5 -> RIVAL 5 -> RIVAL 10 -> STANDARD.
constexpr QuickRaceMenuChoice NextQuickRaceMenuChoice(
    QuickRaceMenuChoice current) noexcept
{
    if (!IsValidQuickRaceMenuChoice(current))
    {
        return {};
    }
    switch (current.Practice)
    {
    case QuickPracticePreset::SoloThree:
        return PracticeMenuChoice(QuickPracticePreset::SoloFive);
    case QuickPracticePreset::SoloFive:
        return PracticeMenuChoice(QuickPracticePreset::RivalFive);
    case QuickPracticePreset::RivalFive:
        return PracticeMenuChoice(QuickPracticePreset::RivalTen);
    case QuickPracticePreset::RivalTen:
        return {};
    default: break;
    }
    if (current.SoloTimeAttack)
    {
        return {RivalDifficultyPreset::Rookie, false};
    }
    if (current.Difficulty == RivalDifficultyPreset::Rookie)
    {
        return PracticeMenuChoice(QuickPracticePreset::SoloThree);
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
    if (!IsValidQuickRaceMenuChoice(current)) return EventKind::Circuit;
    const auto practice = GetQuickPracticeDefinition(current.Practice);
    if (practice.Valid) return practice.Event;
    return current.SoloTimeAttack ? EventKind::TimeAttack : EventKind::Circuit;
}

constexpr RivalDifficultyPreset QuickRaceRivalDifficulty(
    QuickRaceMenuChoice current) noexcept
{
    return current.SoloTimeAttack || IsQuickPractice(current.Practice) ||
           !IsValidQuickRaceMenuChoice(current)
        ? RivalDifficultyPreset::Standard
        : current.Difficulty;
}
} // namespace latebrake
