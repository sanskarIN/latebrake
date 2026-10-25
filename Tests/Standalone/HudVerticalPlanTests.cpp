#include "UI/HudVerticalPlan.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* label)
{
    if (!condition)
    {
        std::cerr << "HudVerticalPlan FAIL: " << label << '\n';
        std::exit(EXIT_FAILURE);
    }
}
void CheckSafe(
    double width, double height, bool touch, const char* label)
{
    using namespace latebrake;
    const HudLayout layout = CalculateHudLayout(width, height);
    const HudVerticalPlan plan =
        CalculateHudVerticalPlan(width, height, layout, touch);
    Check(plan.Valid, label);
    Check(plan.MaxTelemetryLines > 0 &&
          plan.MaxTelemetryLines <= 80,
          "finite bounded telemetry capacity");
    const double lastLineTop = layout.Top +
        (plan.MaxTelemetryLines - 1) * layout.LineHeight;
    Check(lastLineTop + 1.25 * layout.LineHeight <=
          plan.SaveWarningY + 1e-8,
          "all admitted telemetry rows clear the reserved SAVE warning");
    Check(plan.SaveWarningY + layout.LineHeight <=
          plan.RaceStatusY + 1e-8,
          "SAVE and PAUSE/FINISH banners never overlap vertically");
    Check(plan.RaceStatusY < plan.FooterY,
          "race outcome is above the footer");
    if (touch)
    {
        const TouchControlLayout pads =
            CalculateTouchControlLayout(width, height);
        Check(pads.Valid &&
              plan.RaceStatusY + layout.LineHeight <=
                  pads.Throttle.Y + 1e-8 &&
              plan.RaceStatusY + layout.LineHeight <=
                  pads.Steering.Y + 1e-8,
              "outcome status does not overlap lower physical touch pads");
    }
}
}

int main()
{
    using namespace latebrake;
    for (const auto& dims : {
        std::array<double, 2>{240.0, 240.0},
        {640.0, 360.0},
        {1280.0, 720.0},
        {1920.0, 1080.0},
        {2560.0, 1440.0},
        {360.0, 640.0},
        {800.0, 1080.0}
    })
    {
        CheckSafe(dims[0], dims[1], false,
                  "desktop/compact reference dimensions are safe");
        CheckSafe(dims[0], dims[1], true,
                  "touch reference dimensions keep driving pads visible");
    }

    const HudVerticalPlan desktop =
        CalculateHudVerticalPlan(1280.0, 720.0,
                                 CalculateHudLayout(1280.0, 720.0), false);
    const HudVerticalPlan mobile =
        CalculateHudVerticalPlan(640.0, 360.0,
                                 CalculateHudLayout(640.0, 360.0), true);
    Check(desktop.MaxTelemetryLines >= 18 &&
          mobile.MaxTelemetryLines >= 7,
          "essential title/event/speed/lap/time fit before optional stats");
    Check(desktop.MaxTelemetryLines > mobile.MaxTelemetryLines,
          "desktop can display more optional rows than landscape phone");

    // This projection never authorizes scoring or touch input on malformed
    // sizes, even if the older HudLayout returns a defensive fallback.
    for (double bad : {
        0.0, 239.0, -1.0,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity()
    })
    {
        Check(!CalculateHudVerticalPlan(
            bad, 720.0, CalculateHudLayout(bad, 720.0), false).Valid,
            "bad viewport width rejected");
        Check(!CalculateHudVerticalPlan(
            1280.0, bad, CalculateHudLayout(1280.0, bad), true).Valid,
            "bad viewport height rejected");
    }
    HudLayout corrupt = CalculateHudLayout(1280.0, 720.0);
    corrupt.LineHeight = std::numeric_limits<double>::quiet_NaN();
    Check(!CalculateHudVerticalPlan(1280.0, 720.0, corrupt, false).Valid,
          "nonfinite line spacing rejected");
    corrupt = CalculateHudLayout(1280.0, 720.0);
    corrupt.Scale = 0.0;
    Check(!CalculateHudVerticalPlan(1280.0, 720.0, corrupt, false).Valid,
          "invalid scaling rejected");

    std::cout << "HUD vertical budget tests passed (portable C++20).\n";
    return EXIT_SUCCESS;
}
