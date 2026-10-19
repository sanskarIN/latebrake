#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
struct HudLayout final
{
    double Scale = 1.0;
    double Left = 24.0;
    double Top = 18.0;
    double LineHeight = 24.0;
    bool Compact = false;
};

inline HudLayout CalculateHudLayout(double width, double height) noexcept
{
    if (!std::isfinite(width) || !std::isfinite(height) ||
        width < 240.0 || height < 240.0)
    {
        // Defensive fallback, including zero-sized windows while resizing.
        return {0.65, 16.0, 10.0, 17.0, true};
    }
    const double scale = std::clamp(
        std::min(width / 1280.0, height / 720.0), 0.60, 1.20);
    const bool compact = width < 900.0 || height < 600.0;
    return {scale, 24.0 * scale, 18.0 * scale,
            (compact ? 23.0 : 26.0) * scale, compact};
}
} // namespace latebrake
