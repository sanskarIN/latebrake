#include "Racing/RivalFinishClockReview.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool pass, const char* reason)
{
    if (!pass)
    {
        std::cerr << "RivalFinishClockReview FAIL: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a, double b) { return std::abs(a - b) < 1.0e-9; }
}

int main()
{
    using namespace latebrake;
    RivalFinishClockReview review;
    review.Configure(4, 3);
    Require(!review.HasRivalFinalGate(), "no fictitious rival finish");

    // Final gate must have been independently accepted; not just an
    // approximate rival lap count, close position, or arbitrary caller.
    Require(!review.RecordRivalAcceptedGate(11, 73.0, true),
            "unfinished rival cannot establish a finish clock");
    Require(!review.RecordRivalAcceptedGate(12, 73.0, false),
            "unverified rival route cannot establish a finish clock");
    Require(review.CompareAtPlayerFinish(12, 84.0, true, true).State ==
            RivalFinishClockState::Unavailable,
            "no rival final event means no finish margin");
    Require(review.RecordRivalAcceptedGate(12, 73.75, true),
            "first verified rival final gate saved");
    Require(!review.RecordRivalAcceptedGate(12, 3.0, true),
            "duplicate final gate cannot move timestamp backwards");
    Require(review.HasRivalFinalGate(), "live observation stored");

    auto result = review.CompareAtPlayerFinish(12, 78.25, true, true);
    Require(result.State == RivalFinishClockState::RivalEarlierObserved &&
            Near(result.RivalEarlierSeconds, 4.50),
            "qualified physical earlier rival gate gets observed gap");
    result = review.CompareAtPlayerFinish(12, 73.99, true, true);
    Require(result.State == RivalFinishClockState::NearSimultaneousUncertain &&
            result.RivalEarlierSeconds == 0.0,
            "tight actor tick gap never claims a definite race order");
    result = review.CompareAtPlayerFinish(12, 74.0, true, true);
    Require(result.State == RivalFinishClockState::NearSimultaneousUncertain,
            "exact threshold stays uncertain");
    result = review.CompareAtPlayerFinish(12, 74.01, true, true);
    Require(result.State == RivalFinishClockState::RivalEarlierObserved,
            "above tolerance permits an observed margin");
    Require(review.CompareAtPlayerFinish(12, 73.7, true, true).State ==
            RivalFinishClockState::Unavailable,
            "reversed or corrupt clock cannot be a rival lead");
    Require(review.CompareAtPlayerFinish(11, 80.0, true, true).State ==
            RivalFinishClockState::Unavailable,
            "player must finish exact same event");
    Require(review.CompareAtPlayerFinish(12, 80.0, false, true).State ==
            RivalFinishClockState::Unavailable,
            "ineligible player timing is never classified");
    Require(review.CompareAtPlayerFinish(12, 80.0, true, false).State ==
            RivalFinishClockState::Unavailable,
            "rival missing or unqualified at finish is not classified");
    Require(review.CompareAtPlayerFinish(12,
        std::numeric_limits<double>::quiet_NaN(), true, true).State ==
            RivalFinishClockState::Unavailable, "NaN time rejected");
    Require(review.CompareAtPlayerFinish(12,
        std::numeric_limits<double>::infinity(), true, true).State ==
            RivalFinishClockState::Unavailable, "infinite time rejected");
    Require(review.CompareAtPlayerFinish(12, 8.0*24.0*3600.0,
        true, true).State == RivalFinishClockState::Unavailable,
        "unrecordable elapsed race refused");

    review.Reset();
    Require(!review.HasRivalFinalGate() &&
            review.CompareAtPlayerFinish(12, 98.0, true, true).State ==
                RivalFinishClockState::Unavailable,
            "restart never reuses rival final clock");

    review.Configure(4, 1);
    Require(!review.RecordRivalAcceptedGate(12, 80.0, true),
            "different lap total cannot reuse previous finish ordinal");
    Require(review.RecordRivalAcceptedGate(4, 26.0, true),
            "four gates in one-lap event are handled");

    for (int invalid : {-1, 0, 101, 10000})
    {
        review.Configure(4, invalid);
        Require(!review.RecordRivalAcceptedGate(12, 50.0, true),
                "invalid lap count fails closed");
    }
    review.Configure(4, 3);
    Require(!review.RecordRivalAcceptedGate(12, 0.0, true) &&
            !review.RecordRivalAcceptedGate(12, -1.0, true) &&
            !review.RecordRivalAcceptedGate(12,
                std::numeric_limits<double>::infinity(), true),
            "invalid rival clocks never create a finish");
    Require(!review.RecordRivalAcceptedGate(12, 8.0*24.0*3600.0, true),
            "unrecordable rival clock rejected");
    std::cout << "Qualified rival final-gate timing review verified.\n";
    return EXIT_SUCCESS;
}
