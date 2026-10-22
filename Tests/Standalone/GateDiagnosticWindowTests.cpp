#include "Racing/GateDiagnosticWindow.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Gate QA window FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    GateDiagnosticWindow window;
    const CheckpointTraceResult pass{
        CheckpointTraceDecision::Crossed, CircuitGateDecision::Crossed};
    const CheckpointTraceResult wrongWay{
        CheckpointTraceDecision::GateRejected,
        CircuitGateDecision::NotMovingForward};
    const CheckpointTraceResult teleport{
        CheckpointTraceDecision::TeleportRebase,
        CircuitGateDecision::NoForwardPlaneCrossing};

    Require(!window.IsReady() && window.Counts().TotalSamples() == 0U &&
            window.ElapsedSeconds() == 0.0,
            "new QA window is empty and not ready");

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    Require(!window.Record(wrongWay, nan) &&
            !window.Record(wrongWay, inf) &&
            !window.Record(wrongWay, -5.0) &&
            !window.Record(teleport, 0.0),
            "malformed, backwards and zero frame clocks never advance window");
    Require(window.ElapsedSeconds() == 0.0 &&
            window.Counts().TotalSamples() == 4U &&
            window.Counts().CountTrace(
                CheckpointTraceDecision::TeleportRebase) == 1U &&
            window.Counts().CountGateRejection(
                CircuitGateDecision::NotMovingForward) == 3U,
            "invalid durations still count diagnoses without poisoning timer");

    Require(!window.Record(pass, 10.0) &&
            window.ElapsedSeconds() == 1.0,
            "one ten-second hitch counts as at most one second");
    Require(!window.Record(pass, 8.0) &&
            window.ElapsedSeconds() == 2.0,
            "repeated long hitches do not fabricate ten-second interval");

    for (int i = 0; i < 7; ++i)
    {
        Require(!window.Record(pass, 1.0),
                "strict ten-second boundary not triggered early");
    }
    Require(window.ElapsedSeconds() == 9.0 &&
            window.Counts().TotalSamples() == 13U,
            "nine seconds and thirteen decisions recorded");
    Require(window.Record(pass, 1.0) && window.IsReady() &&
            window.ElapsedSeconds() == GateDiagnosticWindow::IntervalSeconds,
            "exact interval triggers one summary");
    Require(window.Record(pass, 10000000.0) &&
            window.ElapsedSeconds() == GateDiagnosticWindow::IntervalSeconds,
            "ready interval remains bounded until explicitly reset");
    Require(window.Counts().TotalSamples() == 15U &&
            window.Counts().CountTrace(
                CheckpointTraceDecision::Crossed) == 11U,
            "late caller retains correct sample counts");

    window.Reset();
    Require(!window.IsReady() && window.Counts().TotalSamples() == 0U &&
            window.ElapsedSeconds() == 0.0,
            "reset completely isolates the next QA window");
    for (int i = 0; i < 59; ++i)
    {
        Require(!window.Record(teleport, 1.0 / 6.0),
                "sub-second accumulation respects ten-second threshold");
    }
    Require(!window.IsReady() && window.ElapsedSeconds() < 10.0,
            "sub-second near boundary stays below trigger");
    Require(window.Record(teleport, 1.0 / 6.0) &&
            window.Counts().CountTrace(
                CheckpointTraceDecision::TeleportRebase) == 60U,
            "sixty consecutive one-sixth-second observations reach interval");

    std::cout << "Fixed-memory QA window clock and reset passed.\n";
    return EXIT_SUCCESS;
}
