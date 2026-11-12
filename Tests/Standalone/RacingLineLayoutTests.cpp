#include "Track/RacingLineLayout.h"

#include <cmath>
#include <iostream>
#include <initializer_list>
#include <limits>
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

// Unlike the abstract paved-corridor test, this examines the actual top of
// the four scale-100cm physical asphalt cubes instantiated by GameMode.
bool LiesOnPhysicalAsphalt(latebrake::CircuitPoint p)
{
    for (int i = 1; i <= 4; ++i)
    {
        const auto slab = latebrake::PlannedCollisionBlock(i);
        if (slab.Style != latebrake::CollisionBlockStyle::Asphalt) continue;
        if (std::abs(p.X - slab.Center.X) <=
                slab.ScaleX * latebrake::RacingLineCubeSizeCm * 0.5 &&
            std::abs(p.Y - slab.Center.Y) <=
                slab.ScaleY * latebrake::RacingLineCubeSizeCm * 0.5)
        {
            return true;
        }
    }
    return false;
}
}

int main()
{
    using namespace latebrake;
    try
    {
        Require(RacingLineMarkCount == 88, "expected bounded, fixed-size line");
        Require(RacingLineStraightPieceCount == 48 &&
                RacingLineCornerPieceCount == 40 &&
                RacingLineStraightPieceCount +
                    RacingLineCornerPieceCount == RacingLineMarkCount,
                "guide must have exact planned batches for efficient rendering");
        Require(Near(RacingLineAsphaltTopCm, -1.0),
                "painted guide must be positioned above actual asphalt top");
        const double markBottom =
            RacingLinePaintCenterZCm - RacingLinePaintThicknessCm * 0.5;
        Require(Near(markBottom - RacingLineAsphaltTopCm,
                     RacingLinePaintSurfaceGapCm),
                "mesh underside must clear real asphalt by specified gap");
        Require(RacingLinePaintSurfaceGapCm >= 0.1 &&
                RacingLinePaintSurfaceGapCm < 0.5,
                "avoid z-fighting and visibly floating stripes");
        Require(RacingLinePaintThicknessCm <= 1.0 &&
                RacingLinePaintWidthCm == 20.0,
                "paint strips must remain thin and readable");

        // The production runtime geometry guard must reject corrupted
        // placements before Unreal allocates a visible mesh.
        const RacingLineMark validFirst = PlannedRacingLineMark(0);
        Require(IsValidRacingLinePaintFootprint(validFirst),
                "known-good first painted rectangle must be valid");
        Require(!IsValidRacingLinePaintFootprint({}),
                "uninitialized optional guide geometry must be invalid");
        RacingLineMark offRoad = validFirst;
        offRoad.Center.Y = SouthY + RoadHalfWidth -
            RacingLinePaintWidthCm * 0.25;
        Require(IsOnPavedSurface(offRoad.Center) &&
                !IsValidRacingLinePaintFootprint(offRoad),
                "a centre on road must not conceal a stripe edge on grass");
        // A carefully chosen rotated stripe at the southeast INSIDE
        // asphalt seam passes the old 25 samples, yet about 32 cm² of
        // its actual paint rectangle crosses an unsampled grass sliver.
        // This must fail with the analytic 15-subset asphalt-union gate.
        RacingLineMark hiddenOverhang{
            {1992.2333413551903, -1020.779553524833},
            57.50528463582525, 218.50926082863054,
            RacingLineSection::Corner, true
        };
        int sampledOnAsphalt = 0;
        const double hiddenRadians =
            hiddenOverhang.HeadingDegrees * RacingLinePi / 180.0;
        const double forwardX = std::cos(hiddenRadians);
        const double forwardY = std::sin(hiddenRadians);
        for (int longitudinal = -2; longitudinal <= 2; ++longitudinal)
        {
            for (int lateral = -2; lateral <= 2; ++lateral)
            {
                const double along = 0.25 * longitudinal *
                    hiddenOverhang.LengthCm;
                const double across = 0.25 * lateral *
                    RacingLinePaintWidthCm;
                const CircuitPoint oldSample{
                    hiddenOverhang.Center.X + along * forwardX -
                        across * forwardY,
                    hiddenOverhang.Center.Y + along * forwardY +
                        across * forwardX
                };
                if (IsOnPhysicalRacingLineAsphalt(oldSample))
                    ++sampledOnAsphalt;
            }
        }
        Require(sampledOnAsphalt == 25,
                "adversarial stripe must fool the old 5x5 point sampler");
        const double hiddenFullArea =
            hiddenOverhang.LengthCm * RacingLinePaintWidthCm;
        const double hiddenCoveredArea =
            RacingLineAsphaltCoveredAreaCm2(hiddenOverhang);
        Require(hiddenCoveredArea > 0.0 &&
                hiddenCoveredArea < hiddenFullArea - 25.0,
                "analytic union catches unsampled off-asphalt paint sliver");
        Require(!IsValidRacingLinePaintFootprint(hiddenOverhang),
                "production painter must fail closed when sample grid misses grass");
        RacingLineMark invalidSection = validFirst;
        invalidSection.Section = static_cast<RacingLineSection>(15);
        Require(!IsValidRacingLinePaintFootprint(invalidSection),
                "unsupported material section must reject painted geometry");
        RacingLineMark invalidLength = validFirst;
        invalidLength.LengthCm = 251.0;
        Require(!IsValidRacingLinePaintFootprint(invalidLength),
                "an oversized guide cube must never render");
        RacingLineMark nonfinite = validFirst;
        nonfinite.HeadingDegrees =
            std::numeric_limits<double>::infinity();
        Require(!IsValidRacingLinePaintFootprint(nonfinite),
                "nonfinite guide headings must be rejected");
        Require(!IsOnPhysicalRacingLineAsphalt(
                    {std::numeric_limits<double>::quiet_NaN(), SouthY}),
                "nonfinite asphalt sample must fail closed");
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
            Require(IsValidRacingLinePaintFootprint(mark),
                    "every planned 3D stripe must pass production road-footprint gate");
            Require(Near(RacingLineAsphaltCoveredAreaCm2(mark),
                         mark.LengthCm * RacingLinePaintWidthCm, 1.0e-5),
                    "complete stripe polygon, not merely 25 samples, lies on asphalt");
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

            // An oriented 100-cm cube is a RECTANGLE, not just a point or
            // line. Verify every physical painted corner fits an actual
            // collision-bearing asphalt slab, even at the four junctions.
            for (int longitudinal : {-1, 1})
            {
                for (int lateral : {-1, 1})
                {
                    const double halfLength =
                        longitudinal * 0.5 * mark.LengthCm;
                    const double halfWidth =
                        lateral * 0.5 * RacingLinePaintWidthCm;
                    const CircuitPoint footprint{
                        mark.Center.X + halfLength * aheadX -
                            halfWidth * aheadY,
                        mark.Center.Y + halfLength * aheadY +
                            halfWidth * aheadX
                    };
                    Require(LiesOnPhysicalAsphalt(footprint),
                            "painted footprint corner is outside actual asphalt");
                }
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
