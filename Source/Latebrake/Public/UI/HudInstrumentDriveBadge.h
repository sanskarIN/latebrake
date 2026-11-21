#pragma once

#include "UI/HudInstrumentCluster.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Pure presentation geometry for a compact, independently measured
// automatic-gear / estimated-RPM badge beside the live speed text.
// Actual font measurements come from Unreal. Neither a gear label nor
// an estimated RPM grants authority over vehicle physics or race state.
struct HudInstrumentDriveBadge final
{
    bool Visible = false;
    bool ShowExtended = false;
    double X = 0.0;
    double Width = 0.0;
    double Right = 0.0;
};

// The speed uses a larger font than the gear. Measure both at their
// actual DrawText scales before calling this policy. The expanded label
// ("G3  4200 RPM EST" or "G3  SHIFT") is optional: a narrower window
// falls back to the bare gear, never painting over speed or the panel edge.
// A height limit keeps either label clear of the first instrument meter.
inline HudInstrumentDriveBadge CalculateHudInstrumentDriveBadge(
    InstrumentRect panel, double scale, double speedTextWidth,
    double compactTextWidth, double compactTextHeight,
    double extendedTextWidth, double extendedTextHeight) noexcept
{
    if (!std::isfinite(panel.X) || !std::isfinite(panel.Y) ||
        !std::isfinite(panel.Width) || !std::isfinite(panel.Height) ||
        panel.X < 0.0 || panel.Y < 0.0 ||
        panel.Width < 200.0 * scale || panel.Height < 90.0 * scale ||
        !std::isfinite(scale) || scale <= 0.0 || scale > 2.0 ||
        !std::isfinite(speedTextWidth) || speedTextWidth <= 0.0 ||
        !std::isfinite(compactTextWidth) || compactTextWidth <= 0.0 ||
        !std::isfinite(compactTextHeight) || compactTextHeight <= 0.0 ||
        compactTextHeight > 24.0 * scale)
    {
        return {};
    }

    const double inset = 16.0 * scale;
    const double minGap = 14.0 * scale;
    const double left = panel.X + inset;
    const double right = panel.X + panel.Width - inset;
    const double available = right - (left + speedTextWidth + minGap);

    if (!std::isfinite(left) || !std::isfinite(right) ||
        !std::isfinite(available) || available < compactTextWidth)
    {
        return {};
    }

    const bool expanded =
        std::isfinite(extendedTextWidth) && extendedTextWidth > 0.0 &&
        std::isfinite(extendedTextHeight) && extendedTextHeight > 0.0 &&
        extendedTextHeight <= 24.0 * scale &&
        extendedTextWidth <= available;

    const double chosenWidth = expanded
        ? extendedTextWidth : compactTextWidth;
    const double x = right - chosenWidth;
    // The background adds a five-scaled-pixel inset on each side.
    // Preserve at least nine scaled pixels from the speed glyphs and
    // eleven from the panel's right edge, even in the compact fallback.
    if (!std::isfinite(x) ||
        x - 5.0 * scale < left + speedTextWidth + 9.0 * scale ||
        x + chosenWidth + 5.0 * scale >
            panel.X + panel.Width - 11.0 * scale)
    {
        return {};
    }
    return {true, expanded, x, chosenWidth, right};
}
} // namespace latebrake
