#include "Racing/QuickRaceModePicker.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool good, const char* reason)
{
    if (!good)
    {
        std::cerr << "QuickRaceModePicker FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}
int main()
{
    using namespace latebrake;
    QuickRaceMenuChoice chosen{};
    Require(chosen.Difficulty == RivalDifficultyPreset::Standard &&
            !chosen.SoloTimeAttack &&
            QuickRaceMenuEvent(chosen) == EventKind::Circuit,
            "default Quick Race is existing 3-lap local-rival circuit");

    const auto selected = chosen;
    chosen = NextQuickRaceMenuChoice(chosen);
    Require(chosen.Difficulty == RivalDifficultyPreset::Expert &&
            !chosen.SoloTimeAttack, "browse Standard -> Expert");
    chosen = NextQuickRaceMenuChoice(chosen);
    Require(chosen.SoloTimeAttack &&
            QuickRaceMenuEvent(chosen) == EventKind::TimeAttack &&
            QuickRaceRivalDifficulty(chosen) == RivalDifficultyPreset::Standard,
            "browse Expert -> real solo time attack with no rival tuning");
    Require(GetRaceEvent(QuickRaceMenuEvent(chosen)).RequiredLaps == 1 &&
            !GetRaceEvent(QuickRaceMenuEvent(chosen)).SpawnRival,
            "solo choice selects existing one-lap event without rival");
    chosen = NextQuickRaceMenuChoice(chosen);
    Require(!chosen.SoloTimeAttack &&
            chosen.Difficulty == RivalDifficultyPreset::Rookie,
            "browse solo -> Rookie");
    chosen = NextQuickRaceMenuChoice(chosen);
    Require(chosen.Difficulty == selected.Difficulty &&
            !chosen.SoloTimeAttack &&
            GetRaceEvent(QuickRaceMenuEvent(chosen)).RequiredLaps == 3 &&
            GetRaceEvent(QuickRaceMenuEvent(chosen)).SpawnRival,
            "browse all four choices returns to original Standard circuit");
    for (int i = 0; i < 10000; ++i)
    {
        chosen = NextQuickRaceMenuChoice(chosen);
        Require(IsValidQuickRaceMenuChoice(chosen),
                "cycling never leaves finite valid menu state");
        const auto event = GetRaceEvent(QuickRaceMenuEvent(chosen));
        Require(event.CheckpointCount == 4 &&
                event.RequiredLaps == (chosen.SoloTimeAttack ? 1 : 3) &&
                event.SpawnRival != chosen.SoloTimeAttack,
                "each selection agrees with real race catalog");
    }

    const QuickRaceMenuChoice invalid{
        static_cast<RivalDifficultyPreset>(999), true
    };
    Require(!IsValidQuickRaceMenuChoice(invalid) &&
            QuickRaceMenuEvent(invalid) == EventKind::Circuit &&
            QuickRaceRivalDifficulty(invalid) == RivalDifficultyPreset::Standard,
            "malformed tier cannot force an unapproved race or AI tune");
    chosen = NextQuickRaceMenuChoice(invalid);
    Require(chosen.Difficulty == RivalDifficultyPreset::Standard &&
            !chosen.SoloTimeAttack, "malformed menu state resets to default");
    const QuickRaceMenuChoice soloFromRookie{
        RivalDifficultyPreset::Rookie, true
    };
    chosen = NextQuickRaceMenuChoice(soloFromRookie);
    Require(chosen.Difficulty == RivalDifficultyPreset::Rookie &&
            !chosen.SoloTimeAttack,
            "leaving solo never retains misleading Expert/Rookie race effects");

    const QuickRaceMenuChoice preserved{RivalDifficultyPreset::Expert, false};
    Require(QuickRaceRivalDifficulty(preserved) == RivalDifficultyPreset::Expert,
            "regular Circuit Quick Race retains selected Expert tier");
    std::cout << "Quick Race four-way event picker passed.\n";
}
