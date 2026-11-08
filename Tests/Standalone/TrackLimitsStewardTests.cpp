#include "Track/TrackLimitsSteward.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using latebrake::CircuitPoint;
using latebrake::TrackLimitsEvent;
using latebrake::TrackLimitsSteward;

constexpr CircuitPoint kRoad{0.0, latebrake::SouthY};
constexpr CircuitPoint kGrass{0.0, 0.0};

void Require(bool condition, const char* reason)
{
    if (!condition)
    {
        std::cerr << "TrackLimitsSteward FAILED: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Clock final
{
    bool Accept = true;
    int Calls = 0;
    double PenaltiesSeconds = 0.0;
    bool Charge(double seconds)
    {
        ++Calls;
        if (!Accept || seconds != 5.0) return false;
        PenaltiesSeconds += seconds;
        return true;
    }
};

TrackLimitsEvent Frame(TrackLimitsSteward& steward, Clock& clock,
                       CircuitPoint where, double speed = 90.0,
                       double dt = 0.1, bool ready = true)
{
    return steward.Update(where, speed, dt, ready,
        [&](double seconds) { return clock.Charge(seconds); });
}

void Rejoin(TrackLimitsSteward& steward, Clock& clock)
{
    bool completed = false;
    for (int i = 0; i < 6; ++i)
    {
        completed |= Frame(steward, clock, kRoad) ==
            TrackLimitsEvent::CleanRejoin;
    }
    Require(completed, "half-second clean rejoin must rearm");
}

void Excursion(TrackLimitsSteward& steward, Clock& clock)
{
    bool warned = false;
    bool recorded = false;
    for (int frame = 0; frame < 17; ++frame)
    {
        auto event = Frame(steward, clock, kGrass);
        warned |= event == TrackLimitsEvent::WarningStarted;
        recorded |= event == TrackLimitsEvent::ViolationRecorded ||
                    event == TrackLimitsEvent::PenaltyApplied ||
                    event == TrackLimitsEvent::PenaltyRefused;
    }
    Require(warned && recorded, "valid moving excursion warns and resolves");
}

void TestThresholdsAndThreeStrikePenalty()
{
    TrackLimitsSteward steward;
    Clock clock;
    for (int i = 0; i < 5; ++i)
        Require(Frame(steward, clock, kGrass) == TrackLimitsEvent::None,
                "no premature warning");
    Require(Frame(steward, clock, kGrass) ==
            TrackLimitsEvent::WarningStarted, "warning starts at 0.6s");
    Require(steward.Snapshot().Warning, "HUD sees warning");
    bool counted = false;
    for (int i = 0; i < 10; ++i)
        counted |= Frame(steward, clock, kGrass) ==
                   TrackLimitsEvent::ViolationRecorded;
    Require(counted && steward.Snapshot().Violations == 1,
            "first excursion registers one violation");
    for (int i = 0; i < 200; ++i) Frame(steward, clock, kGrass);
    Require(steward.Snapshot().Violations == 1 && clock.Calls == 0,
            "remaining on grass cannot farm repeated penalties");
    Rejoin(steward, clock);
    Excursion(steward, clock);
    Require(steward.Snapshot().Violations == 2 && clock.Calls == 0,
            "second strike has no charge");
    Rejoin(steward, clock);
    Excursion(steward, clock);
    Require(steward.Snapshot().Violations == 3 &&
            steward.Snapshot().Penalties == 1 &&
            clock.Calls == 1 && clock.PenaltiesSeconds == 5.0,
            "third strike charges exactly once");
    for (int i = 0; i < 200; ++i) Frame(steward, clock, kGrass);
    Require(clock.Calls == 1, "latch prevents double charge");
    steward.Reset();
    Require(steward.Snapshot().Violations == 0 &&
            steward.Snapshot().Penalties == 0 &&
            !steward.Snapshot().Warning,
            "restart clears the attempt's strike history");
}

void TestFailedPenaltyDoesNotInventCredit()
{
    TrackLimitsSteward steward;
    Clock clock;
    Excursion(steward, clock);
    Rejoin(steward, clock);
    Excursion(steward, clock);
    Rejoin(steward, clock);
    clock.Accept = false;
    Excursion(steward, clock);
    Require(steward.Snapshot().Violations == 2 &&
            steward.Snapshot().Penalties == 0 && clock.Calls == 1,
            "failed race-clock charge cannot record third strike");
    for (int i = 0; i < 30; ++i) Frame(steward, clock, kGrass);
    Require(clock.Calls == 1, "rejected charge cannot run every tick");
    clock.Accept = true;
    Rejoin(steward, clock);
    Excursion(steward, clock);
    Require(steward.Snapshot().Violations == 3 &&
            steward.Snapshot().Penalties == 1 && clock.Calls == 2,
            "subsequent independently qualified excursion can charge");
}

void TestSuspensionInvalidInputAndRejoin()
{
    TrackLimitsSteward steward;
    Clock clock;
    for (int i = 0; i < 10; ++i) Frame(steward, clock, kGrass);
    Require(steward.Snapshot().Warning, "active warning setup");
    Require(Frame(steward, clock, kGrass, 100.0, 0.1, false) ==
            TrackLimitsEvent::Suspended, "pause suspends exposure");
    Require(!steward.Snapshot().Warning &&
            steward.Snapshot().Violations == 0,
            "pause removes warning without inventing strike");
    for (int i = 0; i < 10; ++i) Frame(steward, clock, kGrass, 0.0);
    Require(steward.Snapshot().OffPavementMovingSeconds == 0.0,
            "stationary offroad cannot accrue moving violation");
    for (double dt : {0.0, -0.1, 0.251,
                      std::numeric_limits<double>::infinity(),
                      std::numeric_limits<double>::quiet_NaN()})
        Require(Frame(steward, clock, kGrass, 90.0, dt) ==
                TrackLimitsEvent::Suspended, "bad dt fails closed");
    for (double bad : {std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::quiet_NaN()})
    {
        Require(Frame(steward, clock, {bad, 0.0}) ==
                TrackLimitsEvent::Suspended, "bad location fails closed");
        Require(Frame(steward, clock, kGrass, bad) ==
                TrackLimitsEvent::Suspended, "bad velocity fails closed");
    }
    Require(steward.Snapshot().Violations == 0 && clock.Calls == 0,
            "invalid physics cannot issue a time penalty");
    Excursion(steward, clock);
    Frame(steward, clock, kRoad, 90.0, 0.2);
    Frame(steward, clock, kGrass);
    Require(steward.Snapshot().Violations == 1,
            "brief road touch cannot rearm offroad episode");
    Rejoin(steward, clock);
    Excursion(steward, clock);
    Require(steward.Snapshot().Violations == 2,
            "clean rejoin allows separate second offense");
    Require(Frame(steward, clock, kRoad, 90.0, 0.1, false) ==
            TrackLimitsEvent::Suspended,
            "driver switch or focus loss clears accrual");
    Require(steward.Snapshot().Violations == 2,
            "pause and focus preserve recorded strikes");
}

void TestFrameRatesAndReversedTravel()
{
    for (double dt : {1.0/30.0, 1.0/60.0, 1.0/120.0})
    {
        TrackLimitsSteward steward;
        Clock clock;
        int warnings = 0;
        int violations = 0;
        for (int i = 0; i < 300; ++i)
        {
            const TrackLimitsEvent event =
                Frame(steward, clock, kGrass, -85.0, dt);
            warnings += event == TrackLimitsEvent::WarningStarted;
            violations += event == TrackLimitsEvent::ViolationRecorded;
        }
        Require(warnings == 1 && violations == 1 &&
                steward.Snapshot().Violations == 1 && clock.Calls == 0,
                "30/60/120 FPS reverse motion has the same one-strike result");
    }
}
}

int main()
{
    TestThresholdsAndThreeStrikePenalty();
    TestFailedPenaltyDoesNotInventCredit();
    TestSuspensionInvalidInputAndRejoin();
    TestFrameRatesAndReversedTravel();
    std::cout << "TrackLimitsSteward: warnings, atomic penalty, restarts, "
                 "30/60/120 FPS and input safety PASS (portable only).\n";
    return EXIT_SUCCESS;
}
