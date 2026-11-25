#pragma once

#include "UI/HudMenuMouseControls.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace latebrake
{
// Measured button text may never occupy an adjacent hit target or a
// reserved accessory such as the paint swatch. The renderer owns strings
// and font measurement; this policy only chooses a bounded placement.
struct HudCaptionExtent final
{
    double Width = 0.0;
    double Height = 0.0;
};

struct HudButtonCaptionFit final
{
    bool Valid = false;
    bool ShortCaption = false;
    double Scale = 0.0;
    double X = 0.0;
    double Y = 0.0;
};

template<typename Measure>
HudButtonCaptionFit FitHudButtonCaption(
    DesktopMenuMouseRect bounds, double preferredScale,
    double horizontalInset, double verticalInset,
    double accessoryWidth, bool hasShortCaption, Measure&& measure)
{
    const double right = bounds.X + bounds.Width;
    const double bottom = bounds.Y + bounds.Height;
    if (!std::isfinite(bounds.X) || !std::isfinite(bounds.Y) ||
        !std::isfinite(bounds.Width) || !std::isfinite(bounds.Height) ||
        !std::isfinite(right) || !std::isfinite(bottom) ||
        bounds.X < 0.0 || bounds.Y < 0.0 ||
        bounds.Width <= 0.0 || bounds.Height <= 0.0 ||
        right <= bounds.X || bottom <= bounds.Y ||
        !std::isfinite(preferredScale) || preferredScale <= 0.0 ||
        preferredScale > 4.0 ||
        !std::isfinite(horizontalInset) || horizontalInset < 0.0 ||
        !std::isfinite(verticalInset) || verticalInset < 0.0 ||
        !std::isfinite(accessoryWidth) || accessoryWidth < 0.0)
        return {};

    const double left = bounds.X + horizontalInset;
    const double top = bounds.Y + verticalInset;
    const double textRight = right - horizontalInset - accessoryWidth;
    const double textBottom = bottom - verticalInset;
    const double availableWidth = textRight - left;
    const double availableHeight = textBottom - top;
    if (!std::isfinite(availableWidth) || !std::isfinite(availableHeight) ||
        left < bounds.X || top < bounds.Y ||
        textRight > right || textBottom > bottom ||
        availableWidth <= 0.0 || availableHeight <= 0.0)
        return {};

    // Prefer the full semantic label. Try a bounded sequence of scales
    // rather than relying on linear glyph scaling or guessed lengths.
    // Use a readable short label before reducing the text further.
    constexpr std::array<double, 4> Factors{1.0, 0.92, 0.84, 0.76};
    for (const double factor : Factors)
    {
        const double scale = preferredScale * factor;
        for (const bool shortLabel : {false, true})
        {
            if (shortLabel && !hasShortCaption) continue;
            const auto extent = measure(shortLabel, scale);
            if (!std::isfinite(extent.Width) || !std::isfinite(extent.Height) ||
                extent.Width <= 0.0 || extent.Height <= 0.0 ||
                extent.Width > availableWidth || extent.Height > availableHeight)
                continue;
            const double x = left + (availableWidth - extent.Width) * 0.5;
            const double y = top + (availableHeight - extent.Height) * 0.5;
            if (std::isfinite(x) && std::isfinite(y) &&
                x >= left && y >= top &&
                x + extent.Width <= textRight &&
                y + extent.Height <= textBottom)
                return {true, shortLabel, scale, x, y};
        }
    }
    // No clipped, microscopic, or misleading text if even a short label
    // cannot be measured inside the rectangle.
    return {};
}
} // namespace latebrake
