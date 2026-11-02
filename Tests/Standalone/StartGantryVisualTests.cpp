#include "Racing/StartGantryVisual.h"
#include <cstdlib>
#include <iostream>
#include <limits>

static void Require(bool condition, const char* label)
{
    if (!condition)
    {
        std::cerr << "StartGantryVisual FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    const auto Visual = [](RaceStartPhase phase)
    {
        return PlanStartGantryVisual(phase, true, true, false, false);
    };
    for (const auto caseValue : {RaceStartPhase::Three, RaceStartPhase::Two,
                                 RaceStartPhase::One, RaceStartPhase::Go,
                                 RaceStartPhase::Racing})
    {
        const auto state = Visual(caseValue);
        Require(state.Active, "ready session is displayable");
        const int reds = caseValue == RaceStartPhase::Three ? 3 :
            caseValue == RaceStartPhase::Two ? 2 :
            caseValue == RaceStartPhase::One ? 1 : 0;
        Require(state.RedBulbs == reds, "red bulbs track authoritative phase");
        Require(state.GreenBulb == (caseValue == RaceStartPhase::Go),
                "green is GO only, never racing/red");
        for (int mask = 1; mask < 16; ++mask)
        {
            const auto blocked = PlanStartGantryVisual(caseValue,
                (mask & 1) == 0, (mask & 2) == 0,
                (mask & 4) != 0, (mask & 8) != 0);
            Require(!blocked.Active && blocked.RedBulbs == 0 &&
                !blocked.GreenBulb, "failed prerequisite hides all lights");
        }
    }
    Require(!Visual(static_cast<RaceStartPhase>(999)).Active,
            "invalid phase is dark");

    RaceStartLights actual;
    auto visual = [&actual]()
    {
        return PlanStartGantryVisual(actual.Phase(), true, true, false, false);
    };
    Require(visual().RedBulbs == 3, "new start shows three red");
    Require(actual.Advance(0.5, true) == 0.0 && visual().RedBulbs == 3,
            "one qualified half-second not two");
    Require(actual.Advance(0.5, false) == 0.0 && visual().RedBulbs == 3,
            "paused frame cannot skip lights");
    Require(actual.Advance(0.5, true) == 0.0 && visual().RedBulbs == 2,
            "one elapsed second shows two red");
    Require(actual.Advance(0.5, true) == 0.0, "red countdown no clock");
    Require(actual.Advance(0.5, true) == 0.0 && visual().RedBulbs == 1,
            "two elapsed seconds show one red");
    Require(actual.Advance(0.5, true) == 0.0, "last red frame no clock");
    Require(actual.Advance(0.5, true) == 0.0 &&
            actual.IsGreen() && visual().GreenBulb,
            "exact GO boundary green without free clock");
    Require(actual.Advance(1.0, true) == 1.0 &&
            visual().Active && !visual().GreenBulb,
            "running phase dark after GO flash");
    actual.Reset();
    Require(visual().RedBulbs == 3 && !actual.IsGreen(),
            "restart returns to three red without stale GO");
    std::cout << "Start gantry presentation policy passed.\n";
    return EXIT_SUCCESS;
}
