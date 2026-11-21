#include "UI/HudInstrumentDriveBadge.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "HUD drive badge FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void VerifyScale(double scale)
{
    using namespace latebrake;
    const InstrumentRect panel{340.0, 280.0, 278.0 * scale,
                               214.0 * scale};
    const auto expanded = CalculateHudInstrumentDriveBadge(
        panel, scale, 78.0 * scale, 24.0 * scale, 15.0 * scale,
        108.0 * scale, 15.0 * scale);
    Check(expanded.Visible && expanded.ShowExtended,
          "normal laptop panel shows estimated RPM when it fits");
    Check(std::abs(expanded.Width - 108.0 * scale) < 1.0e-9,
          "expanded label keeps actual measured width");
    Check(std::abs(expanded.Right -
                   (panel.X + panel.Width - 16.0 * scale)) < 1.0e-9,
          "right edge anchors to inner panel margin");
    Check(expanded.X >= panel.X + (16.0 + 78.0 + 14.0) * scale,
          "speed and gear labels never overlap");
    Check(expanded.Right + 5.0 * scale <
              panel.X + panel.Width, "background inside panel");

    const auto compact = CalculateHudInstrumentDriveBadge(
        panel, scale, 134.0 * scale, 24.0 * scale, 15.0 * scale,
        108.0 * scale, 15.0 * scale);
    Check(compact.Visible && !compact.ShowExtended,
          "busy speed row degrades to gear-only text");
    Check(std::abs(compact.Width - 24.0 * scale) < 1.0e-9,
          "compact fallback keeps full, measured gear label");
    Check(compact.X - 5.0 * scale >=
              panel.X + (16.0 + 134.0 + 9.0) * scale,
          "compact background does not collide with speed");

    const auto noRoom = CalculateHudInstrumentDriveBadge(
        panel, scale, 210.0 * scale, 24.0 * scale, 15.0 * scale,
        108.0 * scale, 15.0 * scale);
    Check(!noRoom.Visible,
          "too-long speed text suppresses rather than overlaps badge");

    const auto tallExtended = CalculateHudInstrumentDriveBadge(
        panel, scale, 78.0 * scale, 24.0 * scale, 15.0 * scale,
        108.0 * scale, 28.0 * scale);
    Check(tallExtended.Visible && !tallExtended.ShowExtended,
          "expanded text approaching meter row falls back to compact");
    Check(!CalculateHudInstrumentDriveBadge(
        panel, scale, 78.0 * scale, 24.0 * scale, 25.0 * scale,
        108.0 * scale, 15.0 * scale).Visible,
        "oversized compact glyphs are not drawn into first meter");
}
} // namespace

int main()
{
    using namespace latebrake;

    for (double scale : {0.85, 0.9, 1.0, 1.2})
    {
        VerifyScale(scale);
    }
    const InstrumentRect panel{100.0, 200.0, 278.0, 214.0};
    const auto normal = CalculateHudInstrumentDriveBadge(
        panel, 1.0, 88.0, 24.0, 14.0, 110.0, 14.0);
    Check(normal.Visible && normal.ShowExtended,
          "reference instrument badge is shown");
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    for (double invalid : {nan, inf, -1.0, 0.0})
    {
        Check(!CalculateHudInstrumentDriveBadge(
            panel, invalid, 88, 24, 14, 110, 14).Visible,
            "invalid drawing scale is rejected");
        Check(!CalculateHudInstrumentDriveBadge(
            panel, 1.0, invalid, 24, 14, 110, 14).Visible,
            "invalid measured speed width is rejected");
        Check(!CalculateHudInstrumentDriveBadge(
            panel, 1.0, 88, invalid, 14, 110, 14).Visible,
            "invalid measured compact label width is rejected");
        Check(!CalculateHudInstrumentDriveBadge(
            panel, 1.0, 88, 24, invalid, 110, 14).Visible,
            "invalid compact text height is rejected");
    }
    const auto badExpanded = CalculateHudInstrumentDriveBadge(
        panel, 1.0, 88, 24, 14, nan, inf);
    Check(badExpanded.Visible && !badExpanded.ShowExtended,
          "nonfinite optional RPM measurement preserves safe gear-only mode");

    auto broken = panel;
    broken.X = nan;
    Check(!CalculateHudInstrumentDriveBadge(
        broken, 1.0, 88, 24, 14, 110, 14).Visible,
        "nonfinite panel position fails closed");
    broken = panel;
    broken.Y = -4.0;
    Check(!CalculateHudInstrumentDriveBadge(
        broken, 1.0, 88, 24, 14, 110, 14).Visible,
        "negative panel position fails closed");
    broken = panel;
    broken.Width = 190.0;
    Check(!CalculateHudInstrumentDriveBadge(
        broken, 1.0, 88, 24, 14, 110, 14).Visible,
        "undersized panel fails closed");
    broken = panel;
    broken.Height = 45.0;
    Check(!CalculateHudInstrumentDriveBadge(
        broken, 1.0, 88, 24, 14, 110, 14).Visible,
        "short panel fails closed");

    // All supported desktop layouts must preserve a readable gear label
    // in the right-hand instrument panel when a real gear sample exists.
    int visible = 0;
    for (int width = 900; width <= 3440; width += 31)
    {
        for (int height = 600; height <= 1440; height += 23)
        {
            const auto layout = CalculateHudLayout(width, height);
            const auto cluster = CalculateHudInstrumentCluster(
                width, height, layout, false, false, false,
                124.0, 0.5, 0.4, 0.1);
            if (!cluster.Visible)
            {
                continue;
            }
            const double s = cluster.Scale;
            const auto badge = CalculateHudInstrumentDriveBadge(
                cluster.Panel, s, 92.0 * s, 22.0 * s, 15.0 * s,
                114.0 * s, 15.0 * s);
            Check(badge.Visible, "supported desktop badge remains visible");
            Check(badge.X - 5.0 * s >= cluster.Panel.X &&
                  badge.Right + 5.0 * s <
                      cluster.Panel.X + cluster.Panel.Width &&
                  badge.Right < width,
                  "badge and background remain on screen");
            ++visible;
        }
    }
    Check(visible > 1000, "dense viewport sweep ran");
    std::cout << "Adaptive gear / estimated-RPM badge tests passed.\n";
    return EXIT_SUCCESS;
}
