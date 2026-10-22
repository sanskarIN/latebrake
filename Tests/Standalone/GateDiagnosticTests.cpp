#include "Racing/CheckpointMotionTrace.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Gate diagnostic FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    constexpr double height = 150.0;
    for (int gate = 0; gate < CircuitGateCount; ++gate)
    {
        const CircuitPoint center = CircuitGate(gate);
        const double dx = gate == 0 ? 1.0 : gate == 2 ? -1.0 : 0.0;
        const double dy = gate == 1 ? 1.0 : gate == 3 ? -1.0 : 0.0;
        const CircuitGateSample before{
            {center.X - dx * 100.0, center.Y - dy * 100.0}, height};
        const CircuitGateSample after{
            {center.X + dx * 100.0, center.Y + dy * 100.0}, height};
        const CircuitGateSample offBefore{
            {before.XY.X + dy * 550.0, before.XY.Y + dx * 550.0}, height};
        const CircuitGateSample offAfter{
            {after.XY.X + dy * 550.0, after.XY.Y + dx * 550.0}, height};
        const CircuitGateSample highBefore{
            before.XY, GateCenterHeightCm + GateVerticalHalfHeightCm + 20.0};
        const CircuitGateSample highAfter{
            after.XY, GateCenterHeightCm + GateVerticalHalfHeightCm + 20.0};
        const CircuitGateSample longBefore{
            {center.X - dx * 1150.0, center.Y - dy * 1150.0}, height};
        const CircuitGateSample longAfter{
            {center.X + dx * 150.0, center.Y + dy * 150.0}, height};

        Require(EvaluateCircuitGatePlane(gate, before, after,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::Crossed,
                "all four genuine directional crossings accepted");
        Require(EvaluateCircuitGatePlane(gate, after, after,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::NoForwardPlaneCrossing,
                "stationary sample does not cross a plane");
        Require(EvaluateCircuitGatePlane(gate, before, after,
                                        -dx * 250.0, -dy * 250.0) ==
                CircuitGateDecision::NotMovingForward,
                "wrong-way velocity classified");
        Require(EvaluateCircuitGatePlane(gate, offBefore, offAfter,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::OutsideLateralCorridor,
                "off-track crossing classified");
        Require(EvaluateCircuitGatePlane(gate, highBefore, highAfter,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::OutsideVerticalCorridor,
                "airborne crossing classified");
        Require(EvaluateCircuitGatePlane(gate, longBefore, longAfter,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::ExcessiveFrameTravel,
                "oversized frame sweep classified");
        Require(CrossesCircuitGatePlane(gate, before, after,
                                       dx * 250.0, dy * 250.0),
                "existing bool gate behavior unchanged");

        CheckpointMotionTrace trace;
        const auto initial = trace.SampleDetailed(
            gate, 1, before, dx * 250.0, dy * 250.0);
        Require(initial.Trace == CheckpointTraceDecision::InitialBaseline &&
                !initial.Crossed(), "initial sample establishes baseline");
        const auto jump = trace.SampleDetailed(
            gate, 2, after, dx * 250.0, dy * 250.0);
        Require(jump.Trace == CheckpointTraceDecision::TeleportRebase &&
                !jump.Crossed(), "authorized short teleport cannot earn gate");
        Require(!trace.Sample(gate, 2, before,
                              -dx * 250.0, -dy * 250.0),
                "backtrack for another attempt stays rejected");
        const auto real = trace.SampleDetailed(
            gate, 2, after, dx * 250.0, dy * 250.0);
        Require(real.Trace == CheckpointTraceDecision::Crossed &&
                real.Gate == CircuitGateDecision::Crossed && real.Crossed(),
                "subsequent true crossing earns one gate");
    }

    const CircuitGateSample before{{EastX - 100.0, SouthY}, height};
    const CircuitGateSample after{{EastX + 100.0, SouthY}, height};
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Require(EvaluateCircuitGatePlane(-1, before, after, 250.0, 0.0) ==
            CircuitGateDecision::InvalidGate, "out-of-range gate classified");
    Require(EvaluateCircuitGatePlane(0, before,
            {{nan, SouthY}, height}, 250.0, 0.0) ==
            CircuitGateDecision::InvalidSample, "nonfinite location classified");
    Require(EvaluateCircuitGatePlane(0, before, after, nan, 0.0) ==
            CircuitGateDecision::InvalidVelocity, "nonfinite speed classified");
    Require(EvaluateCircuitGatePlane(0, before, after, 149.0, 0.0) ==
            CircuitGateDecision::NotMovingForward, "slow speed classified");

    CheckpointMotionTrace trace;
    const auto invalid = trace.SampleDetailed(
        0, 1, {{nan, SouthY}, height}, 250.0, 0.0);
    Require(invalid.Trace == CheckpointTraceDecision::InvalidPosition &&
            invalid.Gate == CircuitGateDecision::InvalidSample &&
            !trace.HasBaseline(), "corrupt location clears trace baseline");
    Require(CircuitGateDecisionName(CircuitGateDecision::OutsideLateralCorridor)[0]
            == 'o', "gate reason exposes stable text");
    Require(CheckpointTraceDecisionName(CheckpointTraceDecision::TeleportRebase)[0]
            == 't', "trace reason exposes stable text");

    std::cout << "Directional gate diagnostic and teleport parity passed.\n";
    return EXIT_SUCCESS;
}
