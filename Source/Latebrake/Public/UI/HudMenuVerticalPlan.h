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

    int maxRows = static_cast<int>(
        std::floor((latestDetailY - layout.Top) / step)) + 1;
    maxRows = std::clamp(maxRows, 0, 90);
    return {true, maxRows, overflowY, saveY};
}
} // namespace latebrake
