#include "Racing/GateDiagnosticCounters.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* why)
{
    if (!condition)
    {
        std::cerr << "Gate diagnostic counters FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    GateDiagnosticCounters counters;
    Require(counters.TotalSamples() == 0U &&
            counters.CountTrace(CheckpointTraceDecision::Crossed) == 0U,
            "new window has no samples");

    counters.Observe({CheckpointTraceDecision::InitialBaseline,
                      CircuitGateDecision::NoForwardPlaneCrossing});
    counters.Observe({CheckpointTraceDecision::TeleportRebase,
                      CircuitGateDecision::NoForwardPlaneCrossing});
    counters.Observe({CheckpointTraceDecision::InvalidPosition,
                      CircuitGateDecision::InvalidSample});
    counters.Observe({CheckpointTraceDecision::GateRejected,
                      CircuitGateDecision::OutsideLateralCorridor});
    counters.Observe({CheckpointTraceDecision::GateRejected,
                      CircuitGateDecision::NotMovingForward});
    counters.Observe({CheckpointTraceDecision::Crossed,
                      CircuitGateDecision::Crossed});

    Require(counters.TotalSamples() == 6U, "every sample counted once");
    Require(counters.CountTrace(CheckpointTraceDecision::GateRejected) == 2U,
            "typed gate rejection trace total accurate");
    Require(counters.CountTrace(CheckpointTraceDecision::TeleportRebase) == 1U,
            "authorized reposition separately counted");
    Require(counters.CountTrace(CheckpointTraceDecision::Crossed) == 1U,
            "accepted physical crossing counted");
    Require(counters.CountGateRejection(CircuitGateDecision::OutsideLateralCorridor)
            == 1U, "lane rejection counted");
    Require(counters.CountGateRejection(CircuitGateDecision::NotMovingForward)
            == 1U, "wrong-way rejection counted");
    Require(counters.CountGateRejection(CircuitGateDecision::InvalidSample) == 0U,
            "malformed baseline must not be miscounted as evaluated gate");
    Require(counters.CountGateRejection(CircuitGateDecision::Crossed) == 0U,
            "crossed never counted as rejection");

    // Test the detailed output against a real trace rather than invented
    // enum values alone. No game frames or Unreal symbols are required.
    CheckpointMotionTrace trace;
    constexpr CircuitGateSample before{{EastX - 100.0, SouthY}, 150.0};
    constexpr CircuitGateSample after{{EastX + 100.0, SouthY}, 150.0};
    GateDiagnosticCounters actual;
    actual.Observe(trace.SampleDetailed(0, 10, before, 250.0, 0.0));
    actual.Observe(trace.SampleDetailed(0, 11, after, 250.0, 0.0));
    actual.Observe(trace.SampleDetailed(0, 11, before, -250.0, 0.0));
    actual.Observe(trace.SampleDetailed(0, 11, after, 250.0, 0.0));
    Require(actual.TotalSamples() == 4U &&
            actual.CountTrace(CheckpointTraceDecision::InitialBaseline) == 1U &&
            actual.CountTrace(CheckpointTraceDecision::TeleportRebase) == 1U &&
            actual.CountTrace(CheckpointTraceDecision::Crossed) == 1U &&
            actual.CountGateRejection(CircuitGateDecision::NotMovingForward) == 1U,
            "real motion decisions summarized without awarding teleports");

    counters.Merge(actual);
    Require(counters.TotalSamples() == 10U &&
            counters.CountTrace(CheckpointTraceDecision::Crossed) == 2U &&
            counters.CountGateRejection(CircuitGateDecision::NotMovingForward) == 2U,
            "two window snapshots merge exactly");

    // Self-merge is allowed; counts saturate deterministically even at
    // UINT32_MAX and never wrap to zero on long QA sessions.
    for (int i = 0; i < 35; ++i)
    {
        counters.Merge(counters);
    }
    const auto max = std::numeric_limits<std::uint32_t>::max();
    Require(counters.TotalSamples() == max &&
            counters.CountTrace(CheckpointTraceDecision::Crossed) == max &&
            counters.CountGateRejection(CircuitGateDecision::NotMovingForward) == max,
            "saturated counters never overflow");

    Require(counters.CountTrace(static_cast<CheckpointTraceDecision>(999)) == 0U &&
            counters.CountGateRejection(static_cast<CircuitGateDecision>(999)) == 0U,
            "invalid category query fails closed");
    counters.Reset();
    Require(counters.TotalSamples() == 0U &&
            counters.CountTrace(CheckpointTraceDecision::Crossed) == 0U &&
            counters.CountGateRejection(CircuitGateDecision::NotMovingForward) == 0U,
            "reset clears entire window");

    std::cout << "Bounded diagnostic summary tests passed.\n";
    return EXIT_SUCCESS;
}
