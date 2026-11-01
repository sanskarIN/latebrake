#include "AI/RivalDifficultyPreset.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool good, const char* explanation)
{
    if (!good)
    {
        std::cerr << "RivalDifficultyPreset FAIL: " << explanation << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b)
{
    return std::abs(a - b) < 1e-12;
}
}

int main()
{
    using namespace latebrake;
    Check(NextRivalDifficulty(RivalDifficultyPreset::Rookie) ==
            RivalDifficultyPreset::Standard &&
          NextRivalDifficulty(RivalDifficultyPreset::Standard) ==
            RivalDifficultyPreset::Expert &&
          NextRivalDifficulty(RivalDifficultyPreset::Expert) ==
            RivalDifficultyPreset::Rookie,
          "quick-race selection cycles all three distinct presets");
    Check(RivalDifficultyName(RivalDifficultyPreset::Rookie)[0] == 'R' &&
          RivalDifficultyName(RivalDifficultyPreset::Standard)[0] == 'S' &&
          RivalDifficultyName(RivalDifficultyPreset::Expert)[0] == 'E',
          "each offered difficulty is distinct and displayable");
    const auto unknown = static_cast<RivalDifficultyPreset>(999);
    Check(!IsRivalDifficultyPreset(unknown) &&
          NextRivalDifficulty(unknown) == RivalDifficultyPreset::Standard &&
          EffectiveRivalDifficulty(unknown, false) ==
              RivalDifficultyPreset::Standard,
          "unknown selection falls back to safe standard behavior");
    for (auto selected : {RivalDifficultyPreset::Rookie,
                          RivalDifficultyPreset::Standard,
                          RivalDifficultyPreset::Expert})
    {
        Check(EffectiveRivalDifficulty(selected, true) ==
                  RivalDifficultyPreset::Standard,
              "career events cannot be made easier or harder by quick-race selection");
        Check(EffectiveRivalDifficulty(selected, false) == selected,
              "quick race uses selected setting on race start");
    }

    const PilotCommand straight{0.85, 0.0, false, false};
    auto rookie = ApplyRivalDifficulty(
        straight, RivalDifficultyPreset::Rookie, 1500.0, true);
    const auto standard = ApplyRivalDifficulty(
        straight, RivalDifficultyPreset::Standard, 1500.0, true);
    const auto expert = ApplyRivalDifficulty(
        straight, RivalDifficultyPreset::Expert, 1500.0, true);
    Check(Near(rookie.Throttle, 0.55) &&
          Near(standard.Throttle, 0.85) &&
          Near(expert.Throttle, 0.93) &&
          rookie.Throttle < standard.Throttle &&
          standard.Throttle < expert.Throttle &&
          rookie.Steering == straight.Steering &&
          expert.Steering == straight.Steering &&
          !rookie.Brake && !expert.Brake,
          "three levels produce distinct, bounded straight-line motor demand");

    for (const PilotCommand guarded : {
            PilotCommand{0.0, 0.7, true, false},
            PilotCommand{0.0, -0.4, false, true},
            PilotCommand{0.0, 0.0, false, false},
            PilotCommand{0.60, 0.2, true, false}
         })
    {
        for (auto tier : {RivalDifficultyPreset::Rookie,
                          RivalDifficultyPreset::Standard,
                          RivalDifficultyPreset::Expert})
        {
            const auto out = ApplyRivalDifficulty(guarded, tier, 1400.0, true);
            Check(out.Throttle == guarded.Throttle &&
                  out.Steering == guarded.Steering &&
                  out.Brake == guarded.Brake &&
                  out.Arrived == guarded.Arrived,
                  "difficulty may not override already requested braking or arrival");
        }
    }

    for (double speed : {0.0, 599.0, 2800.01, 6000.0})
    {
        const auto out = ApplyRivalDifficulty(
            straight, RivalDifficultyPreset::Expert, speed, true);
        Check(Near(out.Throttle, straight.Throttle),
              "expert boost unavailable at launch and high speed");
    }
    const PilotCommand corner{0.85, -0.5, false, false};
    Check(Near(ApplyRivalDifficulty(corner, RivalDifficultyPreset::Expert,
                                   1400.0, true).Throttle, corner.Throttle),
          "expert cannot amplify acceleration while steering into a corner");
    const PilotCommand low{0.16, 0.0, false, false};
    Check(ApplyRivalDifficulty(low, RivalDifficultyPreset::Rookie,
                              1400.0, true).Throttle == low.Throttle,
          "rookie cannot increase a slower planned throttle");
    const PilotCommand peak{1.0, 0.0, false, false};
    Check(Near(ApplyRivalDifficulty(peak, RivalDifficultyPreset::Expert,
                                   1400.0, true).Throttle, 1.0),
          "expert cannot exceed normalized motor command maximum");

    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity(),
                       -1.0, 10001.0})
    {
        const auto out = ApplyRivalDifficulty(
            straight, RivalDifficultyPreset::Expert, bad, true);
        Check(out.Throttle == 0.0 && out.Steering == 0.0 && out.Brake,
              "nonfinite/implausible physical speed fails closed");
    }
    for (const PilotCommand invalid : {
        PilotCommand{std::numeric_limits<double>::quiet_NaN(), 0.0, false, false},
        PilotCommand{1.1, 0.0, false, false},
        PilotCommand{-0.1, 0.0, false, false},
        PilotCommand{0.85, 1.1, false, false},
        PilotCommand{0.85, -1.1, false, false},
        PilotCommand{0.85, std::numeric_limits<double>::infinity(), false, false}
    })
    {
        const auto out = ApplyRivalDifficulty(
            invalid, RivalDifficultyPreset::Expert, 1500.0, true);
        Check(out.Brake && out.Throttle == 0.0 && out.Steering == 0.0,
              "malformed motor/steering demands fail closed");
    }
    Check(ApplyRivalDifficulty(straight, unknown, 1500.0, true).Brake &&
          ApplyRivalDifficulty(straight, RivalDifficultyPreset::Rookie,
                               1500.0, false).Brake,
          "unrecognized tier or unqualified race cannot propel rival");

    for (int i = 0; i < 100000; ++i)
    {
        const double speed = 700.0 + (i % 3000);
        const auto tier = static_cast<RivalDifficultyPreset>(i % 3);
        const auto out = ApplyRivalDifficulty(
            straight, tier, speed, true);
        Check(out.Throttle >= 0.0 && out.Throttle <= 1.0 &&
              std::isfinite(out.Throttle) && out.Steering == 0.0 &&
              !out.Brake, "100k valid comparisons remain finite and bounded");
    }
    std::cout << "Rival difficulty presets passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
