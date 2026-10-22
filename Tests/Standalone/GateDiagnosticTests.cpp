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
} // namespace

int main()
{
    using namespace latebrake;
    constexpr double height = 150.0;

    // Each physical gate has a different axis/forward normal. Regression
    // assertions cover the exact same four-gate geometry used by gameplay.
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
        const CircuitGateSample slowBefore{
            {center.X - dx * 601.0, center.Y - dy * 601.0}, height};

        Require(EvaluateCircuitGatePlane(gate, before, after,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::Crossed,
                "all four genuine directional crossings accepted");
        Require(EvaluateCircuitGatePlane(gate, after, after,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::NoForwardPlaneCrossing,
                "stationary position does not cross a plane");
        Require(EvaluateCircuitGatePlane(gate, before, after,
                                        -dx * 250.0, -dy * 250.0) ==
                CircuitGateDecision::NotMovingForward,
                "wrong-way velocity rejected and classified");
        Require(EvaluateCircuitGatePlane(gate, offBefore, offAfter,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::OutsideLateralCorridor,
                "lateral miss identified");
        Require(EvaluateCircuitGatePlane(gate, highBefore, highAfter,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::OutsideVerticalCorridor,
                "airborne crossing rejected");
        Require(EvaluateCircuitGatePlane(gate, longBefore, longAfter,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::ExcessiveFrameTravel,
                "overlong sweep rejected");
        Require(EvaluateCircuitGatePlane(gate, slowBefore, after,
                                        dx * 250.0, dy * 250.0) ==
                CircuitGateDecision::Crossed,
                "legitimate bounded sweep outside visual sphere accepted");
        Require(CrossesCircuitGatePlane(gate, before, after,
                                       dx * 250.0, dy * 250.0),
                "legacy gameplay bool and typed evaluator both pass");
        Require(!CrossesCircuitGatePlane(gate, longBefore, longAfter,
                                        dx * 250.0, dy * 250.0),
                "legacy gameplay bool and typed evaluator both reject");

        CheckpointMotionTrace trace;
        const auto initial = trace.SampleDetailed(
            gate, 1, before, dx * 250.0, dy * 250.0);
        Require(initial.Trace == CheckpointTraceDecision::InitialBaseline &&
                !initial.Crossed(), "first sample is baseline only");
        const auto jump = trace.SampleDetailed(
            gate, 2, after, dx * 250.0, dy * 250.0);
        Require(jump.Trace == CheckpointTraceDecision::TeleportRebase &&
                !jump.Crossed(), "authorized short teleport cannot earn gate");
        Require(!trace.Sample(gate, 2, before,
                              -dx * 250.0, -dy * 250.0),
                "driving back to approach side cannot score");
        const auto drive = trace.SampleDetailed(
            gate, 2, after, dx * 250.0, dy * 250.0);
        Require(drive.Trace == CheckpointTraceDecision::Crossed &&
                drive.Gate == CircuitGateDecision::Crossed && drive.Crossed(),
                "true crossing earns gate once after teleport");
        const auto repeat = trace.SampleDetailed(
            gate, 2, after, dx * 250.0, dy * 250.0);
        Require(repeat.Trace == CheckpointTraceDecision::GateRejected &&
                repeat.Gate == CircuitGateDecision::NoForwardPlaneCrossing,
                "duplicate sample cannot count checkpoint twice");
    }

    const CircuitGateSample before{{EastX - 100.0, SouthY}, height};
    const CircuitGateSample after{{EastX + 100.0, SouthY}, height};
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Require(EvaluateCircuitGatePlane(-1, before, after, 250.0, 0.0) ==
            CircuitGateDecision::InvalidGate,
            "invalid gate index rejected");
    Require(EvaluateCircuitGatePlane(0, before,
            {{nan, SouthY}, height}, 250.0, 0.0) ==
            CircuitGateDecision::InvalidSample,
            "nonfinite location rejected");
    Require(EvaluateCircuitGatePlane(0, before, after, nan, 0.0) ==
            CircuitGateDecision::InvalidVelocity,
            "nonfinite velocity rejected");
    Require(EvaluateCircuitGatePlane(0, before, after, 149.0, 0.0) ==
            CircuitGateDecision::NotMovingForward,
            "below-threshold forward speed rejected");

    CheckpointMotionTrace trace;
    const auto invalid = trace.SampleDetailed(
        0, 1, {{nan, SouthY}, height}, 250.0, 0.0);
    Require(invalid.Trace == CheckpointTraceDecision::InvalidPosition &&
            invalid.Gate == CircuitGateDecision::InvalidSample &&
            !trace.HasBaseline(), "invalid sample clears stale trajectory");
    Require(CircuitGateDecisionName(CircuitGateDecision::OutsideLateralCorridor)[0]
            == 'o', "gate reason name remains available without allocation");
    Require(CheckpointTraceDecisionName(CheckpointTraceDecision::TeleportRebase)[0]
            == 't', "trace reason name remains available without allocation");

    std::cout << "Checkpoint diagnostic and teleport parity passed.\n";
    return EXIT_SUCCESS;
}
