#pragma once

#include <algorithm>
#include <cmath>

namespace latebrake
{
// Engine-independent coordinates shared by the touch dispatcher and HUD.
// Pixel rectangles deliberately avoid the top-left race telemetry panel.
struct TouchRect final
{
    double X = 0.0;
    double Y = 0.0;
    double Width = 0.0;
    double Height = 0.0;

    bool Contains(double x, double y) const noexcept
    {
        return std::isfinite(x) && std::isfinite(y) &&
            x >= X && x <= X + Width && y >= Y && y <= Y + Height;
    }
};

enum class TouchControl
{
    None,
    Steering,
    Throttle,
    Brake,
    Pause,
    Restart,
    Recover,
    CareerNext,
    CareerStart,
    MenuBrowse
};

struct TouchControlLayout final
{
    TouchRect Steering;
    TouchRect Throttle;
    TouchRect Brake;
    TouchRect Pause;
    TouchRect Restart;
    TouchRect Recover;
    TouchRect CareerNext;
    TouchRect CareerStart;
    TouchRect MenuBrowse;
    bool Valid = false;
};

inline TouchControlLayout CalculateTouchControlLayout(
    double viewportWidth, double viewportHeight) noexcept
{
    if (!std::isfinite(viewportWidth) || !std::isfinite(viewportHeight) ||
        viewportWidth < 240.0 || viewportHeight < 240.0)
    {
        return {};
    }

    // The steering pad occupies the lower-left; throttle and brake are
    // separately visible on the lower-right, with a neutral gap between.
    return {
        {viewportWidth * 0.04, viewportHeight * 0.62,
         viewportWidth * 0.42, viewportHeight * 0.32},
        {viewportWidth * 0.59, viewportHeight * 0.52,
         viewportWidth * 0.37, viewportHeight * 0.19},
        {viewportWidth * 0.59, viewportHeight * 0.75,
         viewportWidth * 0.37, viewportHeight * 0.19},
        {viewportWidth * 0.69, viewportHeight * 0.04,
         viewportWidth * 0.27, viewportHeight * 0.10},
        {viewportWidth * 0.69, viewportHeight * 0.16,
         viewportWidth * 0.27, viewportHeight * 0.10},
        {viewportWidth * 0.69, viewportHeight * 0.28,
         viewportWidth * 0.27, viewportHeight * 0.10},
        {viewportWidth * 0.43, viewportHeight * 0.04,
         viewportWidth * 0.23, viewportHeight * 0.10},
        {viewportWidth * 0.43, viewportHeight * 0.16,
         viewportWidth * 0.23, viewportHeight * 0.10},
        {viewportWidth * 0.43, viewportHeight * 0.28,
         viewportWidth * 0.23, viewportHeight * 0.10},
        true
    };
}

inline TouchControl HitTestTouchControl(const TouchControlLayout& layout,
                                       double x, double y) noexcept
{
    if (!layout.Valid)
    {
        return TouchControl::None;
    }
    if (layout.Steering.Contains(x, y))
    {
        return TouchControl::Steering;
    }
    if (layout.Throttle.Contains(x, y))
    {
        return TouchControl::Throttle;
    }
    if (layout.Brake.Contains(x, y))
    {
        return TouchControl::Brake;
    }
    if (layout.Pause.Contains(x, y))
    {
        return TouchControl::Pause;
    }
    if (layout.Restart.Contains(x, y))
    {
        return TouchControl::Restart;
    }
    if (layout.Recover.Contains(x, y))
    {
        return TouchControl::Recover;
    }
    if (layout.CareerNext.Contains(x, y))
    {
        return TouchControl::CareerNext;
    }
    if (layout.CareerStart.Contains(x, y))
    {
        return TouchControl::CareerStart;
    }
    if (layout.MenuBrowse.Contains(x, y))
    {
        return TouchControl::MenuBrowse;
    }
    return TouchControl::None;
}

inline double TouchSteeringValue(const TouchControlLayout& layout,
                                double x) noexcept
{
    if (!layout.Valid || !std::isfinite(x) || layout.Steering.Width <= 0.0)
    {
        return 0.0;
    }
    const double middle = layout.Steering.X + layout.Steering.Width * 0.5;
    return std::clamp((x - middle) / (layout.Steering.Width * 0.5), -1.0, 1.0);
}
} // namespace latebrake
