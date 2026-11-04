#include "Track/RacingLineLayout.h"

#include <cmath>
#include <iostream>
#include <initializer_list>
#include <stdexcept>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

bool Near(double a, double b, double tolerance = 1.0e-8)
{
    return std::abs(a - b) <= tolerance;
}
}

int main()
{
    using namespace latebrake;
    try
    {
        Require(RacingLineMarkCount == 88, "expected bounded, fixed-size line");
        Require(!PlannedRacingLineMark(-1).Valid &&
                !PlannedRacingLineMark(RacingLineMarkCount).Valid &&
                !PlannedRacingLineMark(1000000).Valid,
                "out-of-range marks must be absent");
        int straight = 0;
        int corner = 0;
        for (int i = 0; i < RacingLineMarkCount; ++i)
        {
            const auto mark = PlannedRacingLineMark(i);
            Require(mark.Valid, "planned mark missing");
            Require(std::isfinite(mark.Center.X) &&
                    std::isfinite(mark.Center.Y) &&
                    std::isfinite(mark.HeadingDegrees) &&
                    std::isfinite(mark.LengthCm),
                    "planned marks must have finite transforms");
            Require(mark.LengthCm > 0.0 && mark.LengthCm < 250.0,
                    "segments must be visible but never span the whole road");
            Require(IsOnPavedSurface(mark.Center),
                    "every painted mark must stay on actual pavement");
            if (mark.Section == RacingLineSection::Straight)
            {
                ++straight;
                const int side = i / RacingLineMarksPerSection;
                Require(Near(mark.HeadingDegrees, side * 90.0),
                        "straight direction must match lap orientation");
                if (side % 2 == 0)
                    Require(Near(std::abs(mark.Center.Y),
                                 std::abs(SouthY + RacingLineInsetCm)),
                            "horizontal guide must remain on inset lane");
                else
                    Require(Near(std::abs(mark.Center.X),
                                 std::abs(EastX - RacingLineInsetCm)),
                            "vertical guide must remain on inset lane");
            }
            else
            {
                ++corner;
                const double beginning = (i / RacingLineMarksPerSection) * 90.0;
                Require(mark.HeadingDegrees > beginning &&
                        mark.HeadingDegrees < beginning + 90.0,
                        "tangential corner heading must turn forward");
                Require(mark.LengthCm < 100.0,
                        "corner dash must be short enough for curvature");
            }
        }
        Require(straight == 48 && corner == 40,
                "each of four sections must include its own straight and bend");

        // Validate the full loop, including the often-missed last-to-first
        // seam. A mark can have a valid centre yet still point backwards,
        // jump across the road, or place its actual cube tip off pavement.
        for (int i = 0; i < RacingLineMarkCount; ++i)
        {
            const auto mark = PlannedRacingLineMark(i);
            const auto next = PlannedRacingLineMark(
                (i + 1) % RacingLineMarkCount);
            const double radians =
                mark.HeadingDegrees * RacingLinePi / 180.0;
            const double aheadX = std::cos(radians);
            const double aheadY = std::sin(radians);
            const double dx = next.Center.X - mark.Center.X;
            const double dy = next.Center.Y - mark.Center.Y;
            Require(std::hypot(dx, dy) > 1.0 &&
                    std::hypot(dx, dy) < 350.0,
                    "guide marks must form a continuous bounded loop");
            Require(dx * aheadX + dy * aheadY > 1.0,
                    "successive guide marks must advance along the car heading");
            double turn = next.HeadingDegrees - mark.HeadingDegrees;
            while (turn < -180.0) turn += 360.0;
            while (turn > 180.0) turn -= 360.0;
            Require(turn >= -1.0 && turn < 10.0,
                    "guide heading must turn smoothly in circuit direction");
            // Verify both actual ends of the physical cube as well as centre
            // and modest lateral width, not just mathematical anchor points.
            for (int sign : {-1, 1})
            {
                CircuitPoint tip{
                    mark.Center.X +
                        sign * 0.5 * mark.LengthCm * aheadX,
                    mark.Center.Y +
                        sign * 0.5 * mark.LengthCm * aheadY
                };
                Require(IsOnPavedSurface(tip),
                        "entire longitudinal guide segment must be on pavement");
                CircuitPoint edge{
                    mark.Center.X - sign * 10.0 * aheadY,
                    mark.Center.Y + sign * 10.0 * aheadX
                };
                Require(IsOnPavedSurface(edge),
                        "guide stripe width must fit the paved circuit");
            }
        }
        // Golden placement, including the first and last turn. These positions
        // and orientations must not silently invert when visual code changes.
        const auto first = PlannedRacingLineMark(0);
        Require(first.Center.X < 0.0 && Near(first.Center.Y, SouthY + 250.0) &&
                Near(first.HeadingDegrees, 0.0),
                "first south-facing reference wrong");
        const auto last = PlannedRacingLineMark(RacingLineMarkCount - 1);
        Require(last.Center.X < 0.0 && last.Center.Y < 0.0 &&
                last.HeadingDegrees > 350.0 &&
                last.HeadingDegrees < 360.0,
                "last southwest turn must reconnect eastbound");
        std::cout << "88 original-circuit racing-line marks passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
