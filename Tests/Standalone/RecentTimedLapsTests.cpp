#include "Racing/RecentTimedLaps.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool value, const char* reason)
{
    if (!value)
    {
        std::cerr << "Recent lap history FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}

int main()
{
    using namespace latebrake;
    RecentTimedLaps history;
    Require(history.Size() == 0 && !history.GetRecent(0).Valid(),
            "empty ring has no fabricated lap data");
    Require(!history.Record(0, 5.0) &&
            !history.Record(1, 0.0) &&
            !history.Record(1, -1.0) &&
            !history.Record(1, std::numeric_limits<double>::quiet_NaN()) &&
            !history.Record(1, std::numeric_limits<double>::infinity()),
            "invalid laps are excluded");
    Require(history.Size() == 0, "invalid input cannot advance ring");

    Require(history.Record(1, 2.5), "first positive lap accepted");
    Require(!history.Record(1, 2.0) && !history.Record(0, 2.0) &&
            !history.Record(-1, 2.0),
            "duplicate and out-of-order lap numbers rejected");
    Require(history.Size() == 1 &&
            history.GetRecent(0).CompletedLap == 1 &&
            history.GetRecent(0).Seconds == 2.5,
            "rejected lap must preserve the previous duration and count");
    Require(history.Record(3, 4.0), "skipped but increasing lap index allowed");
    Require(!history.Record(2, 4.0) && !history.Record(3, 4.0),
            "backdated or repeated lap cannot corrupt latest sample");
    Require(history.GetRecent(0).CompletedLap == 3 &&
            history.GetRecent(0).Seconds == 4.0,
            "strict ordering keeps latest timed lap unambiguous");
    history.Reset();
    for (int lap = 1; lap <= RecentTimedLaps::Capacity + 3; ++lap)
    {
        Require(history.Record(lap, static_cast<double>(lap) + 0.5),
                "valid timed lap recorded");
    }
    Require(history.Size() == RecentTimedLaps::Capacity,
            "history remains fixed capacity when full");
    Require(history.GetRecent(0).CompletedLap == 11 &&
            history.GetRecent(0).Seconds == 11.5,
            "newest lap is returned with matching duration");
    Require(history.GetRecent(RecentTimedLaps::Capacity - 1).CompletedLap == 4,
            "oldest surviving lap is correct after wrap");
    Require(!history.Record(11, 0.2) && !history.Record(10, 0.2),
            "wrapped ring cannot accept repeated or backdated lap IDs");
    Require(history.Size() == RecentTimedLaps::Capacity &&
            history.GetRecent(0).CompletedLap == 11 &&
            history.GetRecent(0).Seconds == 11.5 &&
            history.GetRecent(RecentTimedLaps::Capacity - 1).CompletedLap == 4,
            "rejected wrapped write leaves oldest and newest entries intact");
    Require(history.Record(12, 12.5) &&
            history.GetRecent(0).CompletedLap == 12 &&
            history.GetRecent(RecentTimedLaps::Capacity - 1).CompletedLap == 5,
            "valid next lap after wrap evicts only oldest physical entry");
    Require(!history.GetRecent(-1).Valid() &&
            !history.GetRecent(RecentTimedLaps::Capacity).Valid(),
            "invalid index fails closed");

    history.Reset();
    Require(history.Size() == 0 && !history.GetRecent(0).Valid(),
            "restart discards all prior lap data");
    Require(history.Record(1, 9.0) &&
            history.GetRecent(0).CompletedLap == 1 &&
            !history.GetRecent(1).Valid(),
            "post-restart history does not expose previous session");
    std::cout << "Bounded timed-lap ring regression passed.\n";
    return EXIT_SUCCESS;
}
