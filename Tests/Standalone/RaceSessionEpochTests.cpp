#include "Racing/RaceSessionEpoch.h"
#include "Racing/CheckpointSessionTrace.h"
#include "Racing/RaceProgress.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "Race session epoch FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    RaceSessionEpoch session;
    CheckpointSessionTrace trace;
    RaceProgress race(CircuitGateCount, 1);
    const CircuitPoint gate = CircuitGate(0);
    const CircuitGateSample approach{{gate.X - 100.0, gate.Y}, 150.0};
    const CircuitGateSample passed{{gate.X + 100.0, gate.Y}, 150.0};

    Require(session.Current() == 0, "epoch begins at zero");
    Require(CrossesCircuitGatePlane(0, approach, passed, 400.0, 0.0),
            "motion would count as a forward crossing without a barrier");
    Require(!trace.SampleDetailed(0, 0, session.Current(),
                                  approach, 400.0, 0.0).Crossed(),
            "initial sample is baseline");

    // A standalone Escape/gamepad pause and return must not bridge motion.
    session.OnPauseTransition();
    session.OnPauseTransition();
    Require(session.Current() == 2, "two separate pause edges advance epoch");
    Require(!trace.SampleDetailed(0, 0, session.Current(),
                                  passed, 400.0, 0.0).Crossed(),
            "resume first sample does not reuse pre-pause position");
    Require(race.GetNextCheckpoint() == 0, "unobserved pause is not a lap");

    // Career/challenge/quick-race menu dismissal can happen on a separate
    // code path from ToggleMenu and may have zero checkpoint ticks inside it.
    Require(!trace.SampleDetailed(0, 0, session.Current(),
                                  approach, 400.0, 0.0).Crossed(),
            "new approach baseline");
    session.OnMenuReturnToRace();
    Require(session.Current() == 3, "closing an active menu advances epoch");
    Require(!trace.SampleDetailed(0, 0, session.Current(),
                                  passed, 400.0, 0.0).Crossed(),
            "menu return cannot award a crossing without observed motion");

    // Race selection and results replay may restart the race without ever
    // observing the teleport in the checkpoint actor.
    Require(!trace.SampleDetailed(0, 0, session.Current(),
                                  approach, 400.0, 0.0).Crossed(),
            "approach is not a crossing");
    session.OnRaceRestart();
    race.Reset();
    Require(session.Current() == 4 && race.GetNextCheckpoint() == 0,
            "restart resets race and advances checkpoint revision");
    Require(!trace.SampleDetailed(0, 0, session.Current(),
                                  passed, 400.0, 0.0).Crossed(),
            "restart across checkpoint cannot earn a phantom gate");
    Require(!trace.SampleDetailed(0, 0, session.Current(),
                                  approach, -400.0, 0.0).Crossed(),
            "returning to start side does not score");
    Require(trace.SampleDetailed(0, 0, session.Current(),
                                  passed, 400.0, 0.0).Crossed(),
            "genuine uninterrupted crossing still works");
    Require(race.PassCheckpoint(0) && race.GetNextCheckpoint() == 1,
            "valid crossing advances real RaceProgress");

    // Explicit reset after a driver swap never reuses the last location,
    // even when the race session revision itself did not change.
    trace.Reset();
    Require(!trace.SampleDetailed(0, 0, session.Current(),
                                  passed, 400.0, 0.0).Crossed(),
            "driver swap trace reset establishes a new baseline");

    // Direct R/restart from an open pause/results menu both resets the
    // authoritative race and dismisses that menu. The first observed car
    // location after this compound transition must remain only a baseline.
    RaceSessionEpoch directRestart;
    CheckpointSessionTrace directTrace;
    Require(!directTrace.SampleDetailed(0, 2, directRestart.Current(),
                                        approach, 400.0, 0.0).Crossed(),
            "direct restart has an approach baseline");
    directRestart.OnPauseTransition();
    directRestart.OnRaceRestart();
    directRestart.OnMenuReturnToRace();
    Require(directRestart.Current() == 3,
            "pause plus accepted restart and menu dismissal are separate edges");
    Require(!directTrace.SampleDetailed(0, 2, directRestart.Current(),
                                        passed, 400.0, 0.0).Crossed(),
            "direct menu restart cannot bridge an unobserved gate");

    RaceSessionEpoch nearWrap;
    // Drive through the epoch's public events with no mutation backdoor.
    // The counter's API promises monotonically advancing observed revisions
    // in normal play; uint64 rollover remains defined by C++ arithmetic.
    const auto a = nearWrap.Current();
    nearWrap.OnRaceRestart();
    Require(nearWrap.Current() == a + 1, "restart advances revision");
    nearWrap.OnMenuReturnToRace();
    Require(nearWrap.Current() == a + 2, "menu return advances revision");
    nearWrap.OnPauseTransition();
    Require(nearWrap.Current() == a + 3, "pause advances revision");
    static_assert(std::numeric_limits<RaceSessionEpoch::Revision>::max() > 0,
                  "epoch must have unsigned revision representation");

    std::cout << "Race session epoch transitions passed.\n";
    return EXIT_SUCCESS;
}
