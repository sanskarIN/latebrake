#include "Racing/RivalFinishObservation.h"

#include <cstdlib>
#include <iostream>

namespace
{
void Require(bool good, const char* why)
{
    if (!good)
    {
        std::cerr << "RivalFinishObservation FAIL: " << why << '\n';
        std::exit(EXIT_FAILURE);
    }
}
}
int main()
{
    using namespace latebrake;
    using State = RivalFinishObservationState;
    RivalFinishObservationSample original{
        EventKind::Circuit, false, true, true, 4, 3, true, true,
        4, 3, 0, 0};
    for (int lap = 0; lap < 3; ++lap)
    for (int gate = 0; gate < 4; ++gate)
    {
        auto test = original;
        test.RivalCompletedLaps = lap;
        test.RivalNextGate = gate;
        Require(ObserveRivalFinish(test).State == State::RivalStillOnRoute,
                "valid unfinished rival sequence is not winner");
    }
    auto completed = original;
    completed.RivalCompletedLaps = 3;
    completed.RivalNextGate = 0;
    Require(ObserveRivalFinish(completed).State ==
            State::RivalRouteAlreadyComplete, "full route complete");
    completed.RivalNextGate = 1;
    Require(ObserveRivalFinish(completed).State == State::Unavailable,
            "final next gate must be zero");
    for (int bad : {-1,4,100})
    {
        auto t = original;
        t.RivalNextGate = bad;
        Require(ObserveRivalFinish(t).State == State::Unavailable,
                "out-of-range next gate");
        t = original;
        t.RivalCompletedLaps = bad;
        Require(ObserveRivalFinish(t).State == State::Unavailable,
                "invalid completed laps");
    }
    for (int wrong : {0,1,3,5,256})
    {
        auto t = original;
        t.RivalGates = wrong;
        Require(ObserveRivalFinish(t).State == State::Unavailable,
                "mismatched rival gates");
        t = original;
        t.PlayerGates = wrong;
        Require(ObserveRivalFinish(t).State == State::Unavailable,
                "mismatched player gates");
    }
    for (int wrong : {0,1,2,4,100})
    {
        auto t = original;
        t.RivalLaps = wrong;
        Require(ObserveRivalFinish(t).State == State::Unavailable,
                "mismatched rival laps");
        t = original;
        t.PlayerLaps = wrong;
        Require(ObserveRivalFinish(t).State == State::Unavailable,
                "mismatched player laps");
    }
    for (int flag = 0; flag < 4; ++flag)
    {
        auto t = original;
        if (flag == 0) t.PlayerFinished = false;
        if (flag == 1) t.EligibleTiming = false;
        if (flag == 2) t.RivalPresent = false;
        if (flag == 3) t.RivalPhysicsReady = false;
        Require(ObserveRivalFinish(t).State == State::Unavailable,
                "unqualified session hides observation");
    }
    auto career = original;
    career.Career = true;
    Require(ObserveRivalFinish(career).State == State::Unavailable,
            "career never receives experimental ranking");
    auto solo = original;
    solo.Event = EventKind::TimeAttack;
    Require(ObserveRivalFinish(solo).State == State::Unavailable,
            "time attack cannot have rival finish");
    auto unknown = original;
    unknown.Event = static_cast<EventKind>(99);
    Require(ObserveRivalFinish(unknown).State == State::Unavailable,
            "invalid race type");
    Require(RivalFinishObservation{}.State == State::Unavailable,
            "reset state has no stale result");
    std::cout << "Rival route finish observation tests passed.\n";
}
