#include "UI/HudLayout.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

void Check(bool yes, const char* msg)
{
    if (!yes)
    {
        std::cerr << "HUD FAIL: " << msg << '\n';
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    using latebrake::CalculateHudLayout;
    const auto standard = CalculateHudLayout(1280, 720);
    Check(standard.Scale == 1.0 && !standard.Compact, "standard reference layout");
    const auto hd = CalculateHudLayout(1920, 1080);
    Check(hd.Scale == 1.2 && !hd.Compact, "wide desktop scale is bounded");
    const auto small = CalculateHudLayout(640, 360);
    Check(small.Scale == 0.6 && small.Compact, "small screen uses compact view");
    const auto narrow = CalculateHudLayout(800, 1080);
    Check(narrow.Compact && narrow.LineHeight > 0, "portrait uses compact view");
    const auto zero = CalculateHudLayout(0, 0);
    Check(zero.Compact && std::isfinite(zero.Scale), "resize zero safe");
    const auto corrupt = CalculateHudLayout(std::numeric_limits<double>::quiet_NaN(), 1080);
    Check(corrupt.Compact && corrupt.Left > 0, "nonfinite viewport falls back");
    const auto inf = CalculateHudLayout(std::numeric_limits<double>::infinity(), 1080);
    Check(inf.Compact, "infinite viewport fails safely");
    const auto tiny = CalculateHudLayout(240, 240);
    Check(tiny.Scale >= 0.6 && tiny.Compact, "minimum supported dimension");
    Check(hd.LineHeight > small.LineHeight, "line height scales with viewport");
    std::cout << "HUD layout tests passed.\n";
    return EXIT_SUCCESS;
}
