#include "UI/HudInstrumentCluster.h"

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
        std::cerr << "Instruments: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
void VerifyViewport(double width, double height)
{
    using namespace latebrake;
    const auto layout = CalculateHudLayout(width, height);
    const auto gauge = CalculateHudInstrumentCluster(
        width, height, layout, false, false, false, 127.5, 0.7, 0.4, 0.2);
    Require(gauge.Visible, "expected wide desktop gauge");
    Require(gauge.Scale >= 0.85 && gauge.Scale <= 1.2,
            "speed and meter labels retain usable laptop font scale");
    Require(gauge.Panel.X > width * 0.53, "no left text collision");
    Require(gauge.Panel.X + gauge.Panel.Width < width, "panel right bound");
    Require(gauge.Panel.Y >= 0 && gauge.Panel.Y + gauge.Panel.Height < height,
            "panel vertical bounds and footer gap");
    // The real font supplies this width. Both the long label and the
    // independent percent must remain within the narrow laptop panel.
    const double percentWidth = 42.0 * gauge.Scale;
    const auto text = CalculateHudInstrumentMeterText(
        gauge.Panel, gauge.Scale, percentWidth);
    Require(text.Valid, "instrument meter text has both visible lanes");
    Require(text.LabelX >= gauge.Panel.X &&
            text.PercentX - text.LabelX - text.LabelMaxWidth >=
                8.0 * gauge.Scale - 0.00001 &&
            text.PercentX + percentWidth <=
                gauge.Panel.X + gauge.Panel.Width + 0.00001 &&
            text.PercentRight <= gauge.Panel.X + gauge.Panel.Width,
            "measured percent and clipped label never overlap");
    Require(gauge.NitroFraction == 0.7 &&
            gauge.EngineHeatFraction == 0.4 &&
            gauge.ChassisConditionFraction == 0.8,
            "actual normalized meters");
}
} // namespace

int main()
{
    using namespace latebrake;
    VerifyViewport(900, 600);
    VerifyViewport(960, 640);
    VerifyViewport(1024, 768);
    VerifyViewport(1100, 680);
    VerifyViewport(1280, 720);
    VerifyViewport(1920, 1080);
    VerifyViewport(3440, 1440);
    const auto layout = CalculateHudLayout(1280, 720);
    const auto gauge = [&](bool touch, bool menu, bool finished,
                           double speed, double nitro, double heat,
                           double damage)
    {
        return CalculateHudInstrumentCluster(1280, 720, layout, touch,
            menu, finished, speed, nitro, heat, damage);
    };
    Require(!gauge(true, false, false, 80, 0.5, 0.4, 0.1).Visible,
            "never overlap Android controls");
    Require(!gauge(false, true, false, 80, 0.5, 0.4, 0.1).Visible,
            "menus own the viewport");
    Require(!gauge(false, false, true, 80, 0.5, 0.4, 0.1).Visible,
            "results own the viewport");
    for (const double w : {0.0, 240.0, 640.0, 800.0, 899.0})
    {
        Require(!CalculateHudInstrumentCluster(w, 720,
            CalculateHudLayout(w, 720), false, false, false,
            30, 0.5, 0.3, 0).Visible, "compact viewport");
    }
    Require(!CalculateHudInstrumentCluster(1280, 599,
        CalculateHudLayout(1280, 599), false, false, false,
        30, 0.5, 0.3, 0).Visible, "viewport below minimum height");
    Require(CalculateHudInstrumentCluster(1280, 600,
        CalculateHudLayout(1280, 600), false, false, false,
        30, 0.5, 0.3, 0).Visible,
        "short desktop retains readable right-side instrument panel");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (int i = 0; i < 4; ++i)
    {
        const double values[4] = {100.0, 0.4, 0.3, 0.2};
        Require(!gauge(false, false, false,
            i == 0 ? nan : values[0], i == 1 ? nan : values[1],
            i == 2 ? nan : values[2], i == 3 ? nan : values[3]).Visible,
            "never draw inaccurate invalid telemetry");
    }
    Require(!CalculateHudInstrumentCluster(
        std::numeric_limits<double>::infinity(), 720, layout,
        false, false, false, 80, 0.2, 0.3, 0).Visible,
        "invalid dimensions");
    const auto low = gauge(false, false, false, -25.0, -1.0, -0.5, -2.0);
    const auto high = gauge(false, false, false, 2000.0, 9.0, 9.0, 9.0);
    Require(low.Visible && low.SpeedKmH == 0 &&
            low.NitroFraction == 0 && low.EngineHeatFraction == 0 &&
            low.ChassisConditionFraction == 1, "lower clamp");
    Require(high.Visible && high.SpeedKmH == 999 &&
            high.NitroFraction == 1 && high.EngineHeatFraction == 1 &&
            high.ChassisConditionFraction == 0, "upper clamp");
    // Reject invalid font measurements and undersized panels. Never
    // invent percentage placement from an invalid glyph width.
    const auto panel = gauge(false, false, false,
        105, 0.5, 0.6, 0.3).Panel;
    Require(!CalculateHudInstrumentMeterText(panel, 1.0, nan).Valid,
            "nonfinite glyph measurement cannot draw");
    Require(!CalculateHudInstrumentMeterText(panel, 1.0,
        std::numeric_limits<double>::infinity()).Valid,
        "infinite measured width cannot draw");
    Require(!CalculateHudInstrumentMeterText(panel, 1.0, -2).Valid &&
            !CalculateHudInstrumentMeterText(panel, 1.0, 0).Valid,
            "nonpositive measured width cannot draw");
    Require(!CalculateHudInstrumentMeterText(panel, 1.0, 9999).Valid,
            "giant percentage cannot spill off the panel");
    Require(!CalculateHudInstrumentMeterText(panel, nan, 20).Valid &&
            !CalculateHudInstrumentMeterText(panel, 0, 20).Valid,
            "invalid text scale cannot draw");
    auto brokenPanel = panel;
    brokenPanel.X = nan;
    Require(!CalculateHudInstrumentMeterText(brokenPanel, 1, 20).Valid,
            "nonfinite panel position rejected");
    brokenPanel = panel;
    brokenPanel.Width = 1;
    Require(!CalculateHudInstrumentMeterText(brokenPanel, 1, 20).Valid,
            "too-narrow panel must not clip over other HUD text");
    brokenPanel = panel;
    brokenPanel.Height = -1;
    Require(!CalculateHudInstrumentMeterText(brokenPanel, 1, 20).Valid,
            "invalid panel height rejected");
    // The C++ policy handles measurements, while the renderer uses the
    // common UTF-16-safe ellipsis selection from HudTextBounds.
    // Dense deterministic viewport sweep: never render offscreen.
    for (int width = 900; width <= 3600; width += 17)
    {
        for (int height = 600; height <= 1600; height += 19)
        {
            const auto plan = CalculateHudInstrumentCluster(
                width, height, CalculateHudLayout(width, height),
                false, false, false, 105, 0.5, 0.5, 0.5);
            if (plan.Visible)
            {
                Require(plan.Panel.X >= 0 && plan.Panel.Y >= 0 &&
                    plan.Panel.X + plan.Panel.Width < width &&
                    plan.Panel.Y + plan.Panel.Height < height,
                    "dense sweep bounded");
            }
        }
    }
    std::cout << "HUD instruments tests passed.\n";
    return EXIT_SUCCESS;
}
