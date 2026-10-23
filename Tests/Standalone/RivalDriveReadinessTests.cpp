#include "AI/RivalDriveReadiness.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool ready, const char* label)
{
    if (!ready)
    {
        std::cerr << "Rival drive readiness FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    RaceProgress race(4, 1);
    const auto allowed = [&] (EventKind event, bool track, bool paused,
                              bool player, bool routeDone)
    {
        return CanDriveRival(race, event, track, paused, player, routeDone);
    };

    Require(allowed(EventKind::Circuit, true, false, true, false),
            "a ready circuit and active driver may run its rival");
    Require(!allowed(EventKind::TimeAttack, true, false, true, false),
            "time attack has no competitive autonomous rival");
    Require(!allowed(static_cast<EventKind>(-5), true, false, true, false),
            "invalid future event cannot move an autonomous car");
    Require(!allowed(EventKind::Circuit, false, false, true, false),
            "track startup failure freezes rival");
    Require(!allowed(EventKind::Circuit, true, true, true, false),
            "pause or menu freezes rival even under manual ticking");
    Require(!allowed(EventKind::Circuit, true, false, false, false),
            "no active local driver means no rival head start");
    Require(!allowed(EventKind::Circuit, true, false, true, true),
            "finished AI route never restarts from a tick");

    race.Advance(4.0);
    Require(allowed(EventKind::Circuit, true, false, true, false),
            "valid elapsed time does not disable AI");
    for (int gate = 0; gate < race.GetCheckpointCount(); ++gate)
    {
        Require(race.PassCheckpoint(gate),
                "ordered player checkpoint progress");
    }
    Require(race.IsFinished() &&
            !allowed(EventKind::Circuit, true, false, true, false),
            "actual player finish stops AI regardless of route telemetry");

    race.Reset();
    Require(allowed(EventKind::Circuit, true, false, true, false),
            "new authoritative race can move the rival again");
    Require(!allowed(EventKind::Circuit, false, true, false, true),
            "all denial conditions together remain denied");

    std::cout << "Rival drive readiness passed.\n";
    return EXIT_SUCCESS;
}
