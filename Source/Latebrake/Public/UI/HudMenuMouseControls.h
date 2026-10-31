#pragma once

#include "UI/HudLayout.h"

#include <array>
#include <algorithm>
#include <cmath>

namespace latebrake
{
// Source-only fixed navigation dock for the prototype desktop Canvas menu.
// The returned actions map to existing authoritative GameMode menu methods:
// this policy cannot purchase cars, award credits, start events or steer.
enum class DesktopMenuMouseAction
{
    None, PreviousPage, NextPage, DetailsUp, DetailsDown, Activate, Close
};

struct DesktopMenuMouseRect final
{
    double X = 0.0;
    double Y = 0.0;
    double Width = 0.0;
    double Height = 0.0;

    bool Contains(double x, double y) const noexcept
    {
        return std::isfinite(x) && std::isfinite(y) &&
            x >= X && x < X + Width && y >= Y && y < Y + Height;
    }
};

struct DesktopMenuMouseButton final
{
    DesktopMenuMouseAction Action = DesktopMenuMouseAction::None;
    DesktopMenuMouseRect Bounds;
};

struct DesktopMenuMouseLayout final
{
    bool Valid = false;
    double Left = 0.0;
    std::array<DesktopMenuMouseButton, 6> Buttons{};
};

inline DesktopMenuMouseLayout CalculateDesktopMenuMouseLayout(
    double width, double height, HudLayout layout,
    bool touchUI, bool menuOpen) noexcept
{
    if (touchUI || !menuOpen || layout.Compact ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width < 1100.0 || height < 680.0 ||
        !std::isfinite(layout.Left) || layout.Left < 0.0 ||
        !std::isfinite(layout.Scale) || layout.Scale <= 0.0)
    {
        return {};
    }

    const double scale = std::clamp(layout.Scale, 0.6, 1.2);
    const double buttonW = 126.0 * scale;
    const double buttonH = 44.0 * scale;
    const double gap = 10.0 * scale;
    const double left = width - (22.0 * scale + buttonW * 2.0 + gap);
    const double top = 100.0 * scale;
    const double bottom = top + 3.0 * buttonH + 2.0 * gap;
    if (!std::isfinite(left) || !std::isfinite(bottom) ||
        left < std::max(width * 0.58, layout.Left + 270.0 * scale) ||
        bottom > height - 140.0 * scale)
    {
        return {};
    }

    DesktopMenuMouseLayout result;
    result.Valid = true;
    result.Left = left;
    result.Buttons = {{
        {DesktopMenuMouseAction::PreviousPage,
            {left, top, buttonW, buttonH}},
        {DesktopMenuMouseAction::NextPage,
            {left + buttonW + gap, top, buttonW, buttonH}},
        {DesktopMenuMouseAction::DetailsUp,
            {left, top + buttonH + gap, buttonW, buttonH}},
        {DesktopMenuMouseAction::DetailsDown,
            {left + buttonW + gap, top + buttonH + gap, buttonW, buttonH}},
        {DesktopMenuMouseAction::Activate,
            {left, top + 2.0 * (buttonH + gap), buttonW, buttonH}},
        {DesktopMenuMouseAction::Close,
            {left + buttonW + gap, top + 2.0 * (buttonH + gap), buttonW, buttonH}}
    }};
    return result;
}

inline DesktopMenuMouseAction HitTestDesktopMenuMouse(
    const DesktopMenuMouseLayout& layout, double x, double y) noexcept
{
    if (!layout.Valid || !std::isfinite(x) || !std::isfinite(y))
    {
        return DesktopMenuMouseAction::None;
    }
    for (const DesktopMenuMouseButton& button : layout.Buttons)
    {
        if (button.Bounds.Contains(x, y))
        {
            return button.Action;
        }
    }
    return DesktopMenuMouseAction::None;
}
} // namespace latebrake
