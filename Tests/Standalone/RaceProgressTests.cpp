#include "Racing/RaceProgress.h"
#include "Racing/RaceEvents.h"
#include "Racing/GateValidation.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
} // namespace

int main()
{
    using latebrake::RaceProgress;
    using latebrake::RaceState;

    RaceProgress race(4, 3);
    Check(race.GetState() == RaceState::Running, "new race starts running");
    Check(race.GetCompletedLaps() == 0, "new race has zero completed laps");
    Check(!race.PassCheckpoint(1), "cannot bypass first checkpoint");
    Check(!race.PassCheckpoint(-1), "reject negative checkpoint");
    Check(!race.PassCheckpoint(4), "reject out-of-range checkpoint");
    Check(race.PassCheckpoint(0), "accept first checkpoint");
    Check(!race.PassCheckpoint(0), "duplicate checkpoint ignored");
    Check(race.GetNextCheckpoint() == 1, "next checkpoint increments");

    race.Advance(1.25);
    race.Advance(-1.0);
    race.Advance(std::numeric_limits<double>::infinity());
    race.Advance(std::numeric_limits<double>::quiet_NaN());
    Check(std::abs(race.GetElapsedSeconds() - 1.25) < 1e-9, "timer rejects invalid deltas");

    for (int lap = 0; lap < 3; ++lap)
    {
        for (int checkpoint = lap == 0 ? 1 : 0; checkpoint < 4; ++checkpoint)
        {
            Check(race.PassCheckpoint(checkpoint), "accept ordered checkpoint");
        }
        Check(race.GetCompletedLaps() == lap + 1, "completed lap count advances");
        Check(race.IsFinished() == (lap == 2), "finish only on required lap");
    }

    const double finishTime = race.GetElapsedSeconds();
    race.Advance(20.0);
    Check(race.GetElapsedSeconds() == finishTime, "finished race clock is frozen");
    Check(!race.PassCheckpoint(0), "finished race rejects extra checkpoints");
    race.Reset();
    Check(race.GetElapsedSeconds() == 0.0, "reset clock");
    Check(race.GetNextCheckpoint() == 0, "reset checkpoint");
    Check(!race.IsFinished(), "reset finishes");
    Check(race.GetCompletedLaps() == 0, "reset lap count");

    RaceProgress single(0, 0);
    Check(single.GetCheckpointCount() == 1, "clamp invalid checkpoint count");
    Check(single.GetRequiredLaps() == 1, "clamp invalid lap count");
    Check(single.PassCheckpoint(0), "single checkpoint race passes");
    Check(single.IsFinished(), "single checkpoint race finishes");

    // v0.1.1 regression coverage: ordered sector and fastest lap timing.
    RaceProgress timed(2, 3);
    timed.Advance(4.0);
    Check(timed.PassCheckpoint(0), "lap one first gate");
    Check(std::abs(timed.GetLastSectorSeconds() - 4.0) < 1e-9, "first sector time");
    timed.Advance(6.0);
    Check(timed.PassCheckpoint(1), "lap one finish");
    Check(std::abs(timed.GetLastLapSeconds() - 10.0) < 1e-9, "first lap time");
    Check(std::abs(timed.GetBestLapSeconds() - 10.0) < 1e-9, "initial fastest lap");
    Check(std::abs(timed.GetCurrentLapSeconds()) < 1e-9, "lap clock restarts");
    timed.Advance(3.0);
    Check(!timed.PassCheckpoint(1), "wrong gate cannot generate fake split");
    Check(std::abs(timed.GetLastSectorSeconds() - 6.0) < 1e-9, "invalid gate preserves split");
    Check(timed.PassCheckpoint(0), "lap two first gate");
    timed.Advance(5.0);
    Check(timed.PassCheckpoint(1), "lap two finish");
    Check(std::abs(timed.GetLastLapSeconds() - 8.0) < 1e-9, "second lap time");
    Check(std::abs(timed.GetBestLapSeconds() - 8.0) < 1e-9, "new fastest lap");
    timed.Advance(10.0);
    Check(timed.PassCheckpoint(0), "lap three first gate");
    timed.Advance(2.0);
    Check(timed.PassCheckpoint(1), "lap three finish");
    Check(timed.IsFinished(), "timed race finished");
    Check(std::abs(timed.GetLastLapSeconds() - 12.0) < 1e-9, "last completed lap");
    Check(std::abs(timed.GetBestLapSeconds() - 8.0) < 1e-9, "slower lap preserves fastest");
    timed.Reset();
    Check(timed.GetBestLapSeconds() == 0.0, "reset best lap");
    Check(timed.GetLastSectorSeconds() == 0.0, "reset splits");

    RaceProgress overflow(1, 1);
    overflow.Advance(std::numeric_limits<double>::max());
    overflow.Advance(std::numeric_limits<double>::max());
    Check(std::isfinite(overflow.GetElapsedSeconds()), "overflow does not poison clock");
    Check(overflow.GetElapsedSeconds() == std::numeric_limits<double>::max(),
        "overflowing addition is ignored");

    // The four course gates only register forward, meaningful crossing.
    using latebrake::CanPassCircuitGate;
    Check(CanPassCircuitGate(0, 151.0, 0.0), "eastbound gate accepts eastward speed");
    Check(CanPassCircuitGate(1, 0.0, 151.0), "northbound gate accepts northward speed");
    Check(CanPassCircuitGate(2, -151.0, 0.0), "westbound gate accepts westward speed");
    Check(CanPassCircuitGate(3, 0.0, -151.0), "southbound gate accepts southward speed");
    Check(!CanPassCircuitGate(0, -500.0, 0.0), "wrong way rejected");
    Check(!CanPassCircuitGate(1, 500.0, 0.0), "wrong axis rejected");
    Check(!CanPassCircuitGate(0, 149.0, 0.0), "below minimum velocity rejected");
    Check(!CanPassCircuitGate(4, 500.0, 0.0), "invalid gate index rejected");
    Check(!CanPassCircuitGate(0, std::numeric_limits<double>::quiet_NaN(), 0.0),
          "nonfinite velocity rejected");
    Check(!CanPassCircuitGate(0, 500.0, 0.0, -1.0), "invalid speed requirement rejected");

    // Recovery must not provide a free time advantage.
    RaceProgress penalized(1, 1);
    penalized.Advance(10.0);
    Check(penalized.AddTimePenalty(5.0), "valid recovery penalty accepted");
    Check(std::abs(penalized.GetElapsedSeconds() - 15.0) < 1e-9,
        "penalty increases race clock");
    Check(!penalized.AddTimePenalty(-1.0), "negative penalty rejected");
    Check(!penalized.AddTimePenalty(std::numeric_limits<double>::infinity()),
        "infinite penalty rejected");
    Check(!penalized.AddTimePenalty(std::numeric_limits<double>::quiet_NaN()),
        "NaN penalty rejected");
    Check(penalized.PassCheckpoint(0), "penalized race finishes");
    Check(!penalized.AddTimePenalty(5.0), "cannot penalize finished race");
    Check(std::abs(penalized.GetElapsedSeconds() - 15.0) < 1e-9,
        "finish clock remains frozen after rejected penalty");

    // v0.2.0: event catalog is deterministic and suitable for offline use.
    using latebrake::EventKind;
    Check(latebrake::IsValidEvent(EventKind::Circuit), "circuit is recognized");
    Check(latebrake::IsValidEvent(EventKind::TimeAttack), "time attack is recognized");
    Check(!latebrake::IsValidEvent(static_cast<EventKind>(123)), "unknown event rejected");
    const auto circuitEvent = latebrake::GetRaceEvent(EventKind::Circuit);
    const auto timeTrial = latebrake::GetRaceEvent(EventKind::TimeAttack);
    Check(circuitEvent.RequiredLaps == 3, "circuit has three laps");
    Check(circuitEvent.CheckpointCount == 4, "circuit has four gates");
    Check(circuitEvent.SpawnRival, "circuit enables a rival");
    Check(timeTrial.RequiredLaps == 1, "time attack runs one lap");
    Check(timeTrial.CheckpointCount == 4, "time attack keeps ordered gates");
    Check(!timeTrial.SpawnRival, "time attack disables rivals");
    Check(latebrake::GetRaceEvent(static_cast<EventKind>(123)).Kind == EventKind::Circuit,
        "invalid event falls back to circuit configuration");

    std::cout << "All Latebrake portable race tests passed.\n";
    return EXIT_SUCCESS;
}
