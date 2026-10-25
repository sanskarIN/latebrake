#pragma once

#include "UI/HudLayout.h"
#include "UI/TouchControlLayout.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Vertical pixel layout for the EXISTING text HUD. This never changes
// physics, input ownership, race progression, saved records or rewards.
// Regular telemetry is budgeted; two dedicated status baselines stay
// available even when every optional driving statistic is present.
struct HudVerticalPlan final
{
    bool Valid = false;
    int MaxTelemetryLines = 0;
    double SaveWarningY = 0.0;
    double RaceStatusY = 0.0;
    double FooterY = 0.0;
};

// touchUI reflects the actual platform, not the compact layout flag:
// small Windows windows still need a footer; Android requires clear
// space before the GAS/BRAKE and steering hit-test rectangles.
inline HudVerticalPlan CalculateHudVerticalPlan(
    double width, double height, HudLayout layout, bool touchUI) noexcept
{
    if (!std::isfinite(width) || !std::isfinite(height) ||
        width < 240.0 || height < 240.0 ||
        !std::isfinite(layout.Scale) || layout.Scale <= 0.0 ||
        !std::isfinite(layout.Top) || layout.Top < 0.0 ||
        !std::isfinite(layout.LineHeight) || layout.LineHeight <= 0.0)
    {
        return {};
    }

    const double step = layout.LineHeight;
    const double outcomeY = touchUI
        ? height * 0.45
        : height - 54.0 * layout.Scale;
    const double saveY = outcomeY - step;
    const double footerY = height - 26.0 * layout.Scale;

    // DrawText text height varies with font, font size and DPI. Reserve
    // an extra quarter line below every ordinary telemetry baseline.
    const double lastTelemetryY = saveY - 1.25 * step;
    const double availableLines = (lastTelemetryY - layout.Top) / step;
    if (!std::isfinite(availableLines) || availableLines < 0.0 ||
        !std::isfinite(saveY) || !std::isfinite(footerY) ||
        saveY <= layout.Top || footerY <= outcomeY)
    {
        return {};
    }
    int maxLines = static_cast<int>(std::floor(
        std::min(availableLines, 79.0))) + 1;
    maxLines = std::clamp(maxLines, 0, 80);

    if (touchUI)
    {
        const TouchControlLayout pads =
            CalculateTouchControlLayout(width, height);
        if (!pads.Valid ||
            outcomeY + step > pads.Throttle.Y ||
            outcomeY + step > pads.Steering.Y)
        {
            return {};
        }
    }
    return {true, maxLines, saveY, outcomeY, footerY};
}
} // namespace latebrake
