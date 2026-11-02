#include "Racing/GateObjectiveVisual.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool check, const char* message)
{
    if (!check)
    {
        std::cerr << "GateObjectiveVisual FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    static_assert(CircuitGateCount == 4);
    static_assert(!PlanGateObjectiveVisual(0, 0, 4, false, false).Visible);
    static_assert(!PlanGateObjectiveVisual(0, 0, 4, true, true).Visible);
    static_assert(PlanGateObjectiveVisual(2, 2, 4, true, false).Highlighted);
    for (int next = 0; next < CircuitGateCount; ++next)
    {
        int highlighted = 0;
        for (int gate = 0; gate < CircuitGateCount; ++gate)
        {
            const GateObjectiveVisual plan =
                PlanGateObjectiveVisual(gate, next, CircuitGateCount, true, false);
            Require(plan.Visible, "all valid route gates visible");
            Require(plan.Highlighted == (gate == next), "only next gate highlighted");
            Require(plan.RadiusScale == (gate == next ? 14.0 : 8.0),
                    "scales reflect active/inactive state");
            Require(plan.HeightScale == (gate == next ? 0.16 : 0.08),
                    "active height remains cosmetic");
            if (plan.Highlighted) ++highlighted;
        }
        Require(highlighted == 1, "exactly one highlighted checkpoint");
    }
    for (int gate : {-10, -1, 4, 100})
        Require(!PlanGateObjectiveVisual(gate, 0, 4, true, false).Visible,
                "invalid gate is invisible");
    for (int next : {-10, -1, 4, 100})
        Require(!PlanGateObjectiveVisual(0, next, 4, true, false).Visible,
                "invalid progress hides all objectives");
    for (int count : {-1, 0, 1, 3, 5})
        Require(!PlanGateObjectiveVisual(0, 0, count, true, false).Visible,
                "wrong gate count hides objective");
    Require(!PlanGateObjectiveVisual(0, 0, 4, false, true).Visible,
            "unready finished session is invisible");
    std::cout << "3D gate objective visual policy passed.\n";
}
