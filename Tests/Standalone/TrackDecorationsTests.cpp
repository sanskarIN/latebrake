#include "Track/TrackDecorations.h"
#include <cstdlib>
#include <iostream>

static void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Decorations FAIL: " << message << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using namespace latebrake;
    Require(MarkersPerStraight == 7 && StraightCount == 4, "expected track segment counts");
    Require(!LaneStripe(-1, 0).Valid && !LaneStripe(4, 0).Valid,
            "invalid stripe section rejected");
    Require(!LaneStripe(0, -1).Valid && !LaneStripe(0, 7).Valid,
            "invalid stripe sequence rejected");
    Require(!TrackCurb(0, -1, false).Valid && !TrackCurb(0, 7, true).Valid,
            "invalid curb marker rejected");
    Require(!TrackCurb(-1, 0, true).Valid && !TrackCurb(4, 0, true).Valid,
            "invalid curb section rejected");

    int stripes = 0;
    int curbs = 0;
    for (int side = 0; side < StraightCount; ++side)
    {
        for (int segment = 0; segment < MarkersPerStraight; ++segment)
        {
            const auto dash = LaneStripe(side, segment);
            Require(dash.Valid, "dash generated");
            Require(IsOnPavedSurface(dash.Center), "dash remains on paved road");
            Require(dash.ScaleX > 0.0 && dash.ScaleY > 0.0, "positive dash dimensions");
            ++stripes;
            for (int edge = 0; edge < 2; ++edge)
            {
                const auto curb = TrackCurb(side, segment, edge == 1);
                Require(curb.Valid, "curb generated");
                Require(IsOnPavedSurface(curb.Center), "curb remains on paved road");
                Require(curb.ScaleX > 0.0 && curb.ScaleY > 0.0,
                        "positive curb dimensions");
                ++curbs;
            }
        }
    }
    Require(stripes == 28 && curbs == 56, "all decorative pieces generated");
    Require(ValidGrandstand(0) && ValidGrandstand(1), "two grandstand scenes exist");
    Require(!ValidGrandstand(-1) && !ValidGrandstand(2), "invalid grandstand rejected");
    Require(!IsOnPavedSurface(GrandstandCenter(0)) &&
            !IsOnPavedSurface(GrandstandCenter(1)),
            "grandstands outside racing surface");
    std::cout << "Track decoration plans passed.\n";
}
