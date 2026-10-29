#include "Vehicles/FrameStableDamping.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "Frame stable damping FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool Near(double lhs, double rhs)
{
    return std::abs(lhs - rhs) < 1e-11;
}
}

int main()
{
    using latebrake::FrameStableDampingRate;
    // Normal 60 FPS handling is precisely unchanged for the grip, braking,
    // coast and opposing-throttle resistance rates used by the real pawn.
    for (double rate : {8.5, 4.25, 7.0, 3.0, 0.55})
    {
        Require(Near(FrameStableDampingRate(rate, 1.0 / 60.0), rate),
                "normal frames preserve the established damping rates");
    }
    Require(Near(FrameStableDampingRate(8.5, 0.25), 4.0),
            "long frame cannot over-brake the sideways component");
    Require(Near(FrameStableDampingRate(7.0, 0.5), 2.0),
            "long frame cannot reverse braking through this force alone");
    Require(Near(FrameStableDampingRate(3.0, 1.0), 1.0),
            "engine braking shares the same impulse bound");
    Require(Near(FrameStableDampingRate(0.0, 0.1), 0.0),
            "zero damping is not turned into a new force");

    for (double frame : {0.001, 0.016, 0.1, 0.125, 0.5, 2.0, 60.0})
    {
        for (double rate : {0.0, 0.55, 3.0, 4.25, 7.0, 8.5, 500.0})
        {
            const double bounded = FrameStableDampingRate(rate, frame);
            Require(std::isfinite(bounded) && bounded >= 0.0 &&
                    bounded <= rate && bounded * frame <= 1.0 + 1e-12,
                    "finite rate never exceeds the one-frame impulse budget");
            for (double speed : {-6500.0, -500.0, -1.0, 0.0,
                                 1.0, 500.0, 6500.0})
            {
                const double next = speed - speed * bounded * frame;
                Require(std::abs(next) <= std::abs(speed) + 1e-8 &&
                        (speed == 0.0 || next * speed >= -1e-8),
                        "damping alone neither increases nor reverses axis momentum");
            }
        }
    }

    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double invalid : {nan, inf, -inf})
    {
        Require(FrameStableDampingRate(invalid, 0.016) == 0.0,
                "nonfinite damping rate rejects force");
        Require(FrameStableDampingRate(7.0, invalid) == 0.0,
                "nonfinite frame rejects force");
    }
    Require(FrameStableDampingRate(-1.0, 0.016) == 0.0 &&
            FrameStableDampingRate(7.0, 0.0) == 0.0 &&
            FrameStableDampingRate(7.0, -1.0) == 0.0,
            "negative or zero inputs fail closed");
    Require(Near(FrameStableDampingRate(7.0, 1e-200), 7.0),
            "tiny finite frame retains finite requested rate");
    Require(std::isfinite(FrameStableDampingRate(7.0, 1e200)),
            "huge finite frame still returns finite bounded damping");
    std::cout << "Frame stable damping tests passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
