#pragma once

#include "UI/HudLayout.h"
#include "UI/TouchControlLayout.h"

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Reserve menu overflow and SAVE FAILED banners instead of allowing
// variable-length results/garage text to cross the lower steering pad.
// Presentation-only geometry; does not modify menu navigation or input.
struct HudMenuVerticalPlan final
{
    bool Valid = false;
    int MaxDetailsRows = 0;
    double OverflowNoticeY = 0.0;
    double SaveWarningY = 0.0;
};

inline HudMenuVerticalPlan CalculateHudMenuVerticalPlan(
    double width, double height, HudLayout layout, bool touchUI) noexcept
{
    if (!std::isfinite(width) || !std::isfinite(height) ||
        width < 240.0 || height < 240.0 ||
        !std::isfinite(layout.Scale) || layout.Scale <= 0.0 ||
        !std::isfinite(layout.Top) || layout.Top < 0.0)
    {
        return {};
    }

    const double step = 28.0 * layout.Scale;
    // External callers can supply malformed scale independently of the
    // normal [0.60, 1.20] HUD layout. A subpixel or overflowing row height
    // cannot define reliably distinct notice/detail baselines.
    if (!std::isfinite(step) || step < 1.0)
    {
        return {};
    }
    const double bottom = touchUI
        ? CalculateTouchControlLayout(width, height).Steering.Y
        : height;
    if (!std::isfinite(bottom))
    {
        return {};
    }
    // Normal text, one optional overflow notice, and one independent
    // save-failed notice each get a separate baseline.
    const double saveY = bottom - 1.8 * step;
    const double overflowY = saveY - 1.25 * step;
    const double latestDetailY = overflowY - 1.25 * step;
    if (!std::isfinite(latestDetailY) || latestDetailY < layout.Top ||
        saveY + step > bottom)
    {
        return {};
    }

    const double rowIndex = std::floor(
        (latestDetailY - layout.Top) / step);
    if (!std::isfinite(rowIndex) || rowIndex < 0.0)
    {
        return {};
    }
    // Saturate in floating point BEFORE the integer cast: an otherwise
    // finite but extremely tall viewport must not cause undefined
    // out-of-range floating-point-to-int conversion.
    const int maxRows =
        static_cast<int>(std::min(rowIndex, 89.0)) + 1;
    return {true, maxRows, overflowY, saveY};
}
} // namespace latebrake
