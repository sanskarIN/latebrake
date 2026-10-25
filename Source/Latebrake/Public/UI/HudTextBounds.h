#pragma once

#include "UI/HudLayout.h"
#include "UI/TouchControlLayout.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace latebrake
{
// The ordinary HUD panel must not render across actual phone action
// hit regions. This rectangle is advisory presentation geometry only:
// it does not modify touch hit testing, driving, scoring or saving.
struct HudTextBounds final
{
    bool Valid = false;
    double Left = 0.0;
    double Right = 0.0;
    double MaxWidthPixels = 0.0;
};

inline HudTextBounds CalculateHudTextBounds(
    double width, double height, HudLayout layout, bool touchUI) noexcept
{
    if (!std::isfinite(width) || !std::isfinite(height) ||
        width < 240.0 || height < 240.0 ||
        !std::isfinite(layout.Left) || layout.Left < 0.0 ||
        !std::isfinite(layout.Scale) || layout.Scale <= 0.0)
    {
        return {};
    }

    const double margin = std::max(4.0, width * 0.02);
    double right = width - margin;
    if (touchUI)
    {
        const TouchControlLayout pads =
            CalculateTouchControlLayout(width, height);
        if (!pads.Valid)
        {
            return {};
        }
        // Career, browse and pause buttons span much of the top half.
        // A fixed left column is safer than guessing from text baseline.
        right = std::min({pads.CareerNext.X,
                          pads.MenuBrowse.X, pads.Pause.X}) - margin;
    }
    const double available = right - layout.Left;
    if (!std::isfinite(right) || !std::isfinite(available) ||
        available < 1.0 || right > width)
    {
        return {};
    }
    return {true, layout.Left, right, available};
}

// Returns the number of source code units that fit before a suffix.
// Full length means keep the original. -1 means even the suffix cannot
// fit, and the caller must not draw a glyph. A callback evaluates the
// actual font measurement of "source prefix + suffix" (not a guessed
// monospaced character count). Binary search needs <= log2(n) probes.
template<typename MeasurePrefixWithSuffix>
int ChooseHudTextPrefix(
    int length, double maxWidth, double fullWidth,
    double suffixWidth, MeasurePrefixWithSuffix measure) noexcept
{
    if (length < 0 || !std::isfinite(maxWidth) ||
        maxWidth <= 0.0 || !std::isfinite(fullWidth) ||
        fullWidth < 0.0 || !std::isfinite(suffixWidth) ||
        suffixWidth < 0.0)
    {
        return -1;
    }
    if (fullWidth <= maxWidth)
    {
        return length;
    }
    if (suffixWidth > maxWidth)
    {
        return -1;
    }
    int low = 0;
    int high = length;
    while (low < high)
    {
        const int middle = low + (high - low + 1) / 2;
        const double measuredWidth = measure(middle);
        if (std::isfinite(measuredWidth) &&
            measuredWidth >= 0.0 && measuredWidth <= maxWidth)
        {
            low = middle;
        }
        else
        {
            high = middle - 1;
        }
    }
    return low;
}
} // namespace latebrake
