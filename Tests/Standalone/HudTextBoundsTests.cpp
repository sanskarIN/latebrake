#include "UI/HudTextBounds.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <utility>

namespace
{
void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::cerr << "HUD text bounds FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void CheckViewport(double width, double height, bool touch)
{
    using namespace latebrake;
    const auto layout = CalculateHudLayout(width, height);
    const auto bounds =
        CalculateHudTextBounds(width, height, layout, touch);
    Check(bounds.Valid && bounds.MaxWidthPixels >= 1.0,
          "supported viewport yields finite usable text lane");
    Check(bounds.Left == layout.Left && bounds.Right <= width,
          "bounds stay inside viewport and start at actual HUD origin");
    Check(std::abs(bounds.Right - bounds.Left -
                   bounds.MaxWidthPixels) < 1e-8,
          "maximum width matches the physical lane");
    if (touch)
    {
        const auto pads = CalculateTouchControlLayout(width, height);
        Check(bounds.Right < pads.CareerNext.X,
              "no horizontal collision with leftmost career navigation");
        Check(bounds.Right < pads.MenuBrowse.X &&
              bounds.Right < pads.Pause.X,
              "no horizontal collision with top Android actions");
        Check(bounds.Right < pads.Throttle.X,
              "no collision with race GAS pad");
    }
}
} // namespace

int main()
{
    using namespace latebrake;
    for (auto [width, height] : {
        std::pair{240.0, 240.0},
        std::pair{360.0, 640.0},
        std::pair{640.0, 360.0},
        std::pair{800.0, 480.0},
        std::pair{1280.0, 720.0},
        std::pair{1920.0, 1080.0},
        std::pair{2560.0, 1440.0}
    })
    {
        CheckViewport(width, height, false);
        CheckViewport(width, height, true);
    }

    const auto mobile = CalculateHudTextBounds(
        640.0, 360.0, CalculateHudLayout(640.0, 360.0), true);
    const auto desktop = CalculateHudTextBounds(
        640.0, 360.0, CalculateHudLayout(640.0, 360.0), false);
    Check(mobile.MaxWidthPixels < desktop.MaxWidthPixels,
          "actual mobile action rectangles narrow the left text panel");

    const auto monospace = [](int prefix) {
        return static_cast<double>(prefix) + 3.0;
    };
    Check(ChooseHudTextPrefix(20, 9.0, 20.0, 3.0, monospace) == 6,
          "overlong text is accurately truncated with three-dot suffix");
    Check(ChooseHudTextPrefix(20, 21.0, 20.0, 3.0, monospace) == 20,
          "text that fits has no unnecessary ellipsis");
    Check(ChooseHudTextPrefix(20, 3.0, 20.0, 3.0, monospace) == 0,
          "bare ellipsis is allowed at an extremely narrow width");
    Check(ChooseHudTextPrefix(20, 2.0, 20.0, 3.0, monospace) == -1,
          "no draw when even ellipsis cannot fit");

    Check(ChooseHudTextPrefix(10, 9.0, 100.0, 3.0,
          [](int) { return 0.0; }) == 9,
          "contradictory font callback cannot admit oversized original");
    Check(ChooseHudTextPrefix(0, 1.0, 10.0, 2.0,
          [](int) { return 2.0; }) == -1,
          "zero-length malformed width cannot bypass suffix policy");

    int calls = 0;
    const int clipped = ChooseHudTextPrefix(
        1'000'000, 24.0, 1'000'000.0, 3.0,
        [&calls](int prefix) {
            ++calls;
            return 3.0 + static_cast<double>(prefix);
        });
    Check(clipped == 21 && calls <= 21,
          "very long content remains logarithmic, not per-character");
    const auto n = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    for (double invalid : {n, inf, -1.0, 0.0})
    {
        Check(ChooseHudTextPrefix(10, invalid, 12.0, 3.0, monospace) == -1,
              "invalid width fails closed");
        Check(!CalculateHudTextBounds(
            invalid, 720.0, CalculateHudLayout(invalid, 720.0), true).Valid,
            "invalid viewport fails closed");
    }
    Check(ChooseHudTextPrefix(-1, 10.0, 100.0, 3.0, monospace) == -1,
          "negative text lengths rejected");
    Check(ChooseHudTextPrefix(10, 10.0, n, 3.0, monospace) == -1,
          "nonfinite full-font measurement rejected");
    Check(ChooseHudTextPrefix(10, 10.0, 20.0, inf, monospace) == -1,
          "nonfinite suffix measurement rejected");
    Check(ChooseHudTextPrefix(
        10, 10.0, 20.0, 3.0,
        [](int prefix) {
            return prefix <= 4 ? prefix + 3.0 :
                std::numeric_limits<double>::quiet_NaN();
        }) == 4,
        "nonfinite prefix font measurement never slips into the viewport");
    auto malformed = CalculateHudLayout(640.0, 360.0);
    malformed.Left = n;
    Check(!CalculateHudTextBounds(640.0, 360.0, malformed, true).Valid,
          "malformed layout left offset rejected");
    std::cout << "HUD measured text bounds passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
