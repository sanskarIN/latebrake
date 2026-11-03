#include "Racing/RivalSameSegmentOrder.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool valid, const char* reason)
{
    if (!valid)
    {
        std::cerr << "Rival same-segment order FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    const auto equal = CompareRivalCreditedGates(
        {4, 3, 0, 0, 0, 0, true, true, true});
    RivalSameSegmentSample sample{equal, 0, {-1250, -1800},
                                  {-1800, -1450}, true};
    auto result = CompareRivalSameSegment(sample);
    Require(result.Valid && result.Leader == RivalSegmentLeader::Player &&
            std::abs(result.AlongRoadSeparationMeters - 5.5) < 1e-9,
            "player physically farther along same verified south straight");
    sample.PlayerPosition = {-2400, -1800};
    result = CompareRivalSameSegment(sample);
    Require(result.Valid && result.Leader == RivalSegmentLeader::Rival &&
            std::abs(result.AlongRoadSeparationMeters - 6.0) < 1e-9,
            "rival lead is measured from actual straight projection");
    sample.PlayerPosition = {-1780, -1800};
    result = CompareRivalSameSegment(sample);
    Require(result.Valid && result.Leader == RivalSegmentLeader::TooClose &&
            result.AlongRoadSeparationMeters == 0.0,
            "sub-three-meter margin cannot claim clear lead");

    sample.PlayerPosition = {-1250, -1800};
    sample.NextGate = 1;
    Require(!CompareRivalSameSegment(sample).Valid,
            "next gate must match previously credited ordinal");
    sample.NextGate = 0;
    sample.CreditedGates = CompareRivalCreditedGates(
        {4, 3, 0, 1, 0, 0, true, true, true});
    Require(!CompareRivalSameSegment(sample).Valid,
            "different verified gate ordinals never use spatial comparison");
    sample.CreditedGates = equal;
    sample.PlayerPosition = {0, 0};
    Require(!CompareRivalSameSegment(sample).Valid,
            "off-road chassis cannot fabricate lead from projected coordinates");
    sample.PlayerPosition = {-1250, -1050};
    Require(!CompareRivalSameSegment(sample).Valid,
            "wide same-road shoulder does not qualify as shared straight lane");
    sample.PlayerPosition = {-2790, -1800};
    Require(!CompareRivalSameSegment(sample).Valid,
            "turn entry/exit is ambiguous and must be suppressed");
    sample.PlayerPosition = {std::numeric_limits<double>::quiet_NaN(), -1800};
    Require(!CompareRivalSameSegment(sample).Valid,
            "nonfinite real chassis position is unavailable");
    sample.PlayerPosition = {-1250, -1800};
    sample.RaceActive = false;
    Require(!CompareRivalSameSegment(sample).Valid,
            "paused/ineligible race is not a physical position observation");

    // Cover every permitted straight direction and a rollover gate ordinal.
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const CircuitPoint a = CircuitGate((gate + 3) % 4);
        const CircuitPoint b = CircuitGate(gate);
        const auto at = [=](double t) {
            return CircuitPoint{a.X + (b.X - a.X) * t,
                                a.Y + (b.Y - a.Y) * t};
        };
        sample = {CompareRivalCreditedGates({4, 3, 1, gate,
                  1, gate, true, true, true}), gate, at(0.72), at(0.42), true};
        result = CompareRivalSameSegment(sample);
        Require(result.Valid && result.Leader == RivalSegmentLeader::Player &&
                result.AlongRoadSeparationMeters > 10.0,
                "physical segment direction is correct for all four legs");
        sample.PlayerPosition = at(0.42);
        sample.RivalPosition = at(0.72);
        result = CompareRivalSameSegment(sample);
        Require(result.Valid && result.Leader == RivalSegmentLeader::Rival,
                "order reverses when the real vehicle positions reverse");
    }

    // No accumulation, allocations, or sample-to-sample hidden authority.
    sample = {equal, 0, {-1250, -1800}, {-1800, -1800}, true};
    for (int i = 0; i < 20000; ++i)
    {
        result = CompareRivalSameSegment(sample);
        Require(result.Valid && result.Leader == RivalSegmentLeader::Player &&
                result.AlongRoadSeparationMeters == 5.5,
                "20k frame replay leaves accepted geometric order unchanged");
    }
    std::cout << "Same verified gate, same physical circuit straight QA passed.\n";
    return EXIT_SUCCESS;
}
