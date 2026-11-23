#pragma once

#include "UI/HudLayout.h"
#include "UI/MenuDetailPager.h"
#include "UI/HudMenuVerticalPlan.h"
#include "UI/HudTextBounds.h"

#include <array>
#include <algorithm>
#include <cmath>

namespace latebrake
{
// Responsive desktop navigation dock for the existing Unreal Canvas menu.
// Pointer targets remain at least 44 pixels high even when HUD text scales
// down on smaller laptop windows. Each action still maps to the existing
// authoritative GameMode; this policy cannot purchase, race, save or steer.
enum class DesktopMenuMouseAction
{
    None, PreviousPage, NextPage, DetailsUp, DetailsDown, Activate, Close,
    GaragePaint, GarageStripe, GarageWheels, GarageReset
};

struct DesktopMenuMouseRect final
{
    double X = 0.0;
    double Y = 0.0;
    double Width = 0.0;
    double Height = 0.0;

    bool Contains(double x, double y) const noexcept
    {
        // Bounds can be supplied to the portable hit tester independently
        // of CalculateDesktopMenuMouseLayout. Malformed rectangles must not
        // become invisible click traps while the game menu is open.
        // Finite origin and extent are insufficient: their sum can
        // overflow. Reject such rectangles rather than admitting clicks
        // into a target with an effectively infinite far edge.
        const double right = X + Width;
        const double bottom = Y + Height;
        return std::isfinite(x) && std::isfinite(y) &&
            std::isfinite(X) && std::isfinite(Y) &&
            std::isfinite(Width) && std::isfinite(Height) &&
            std::isfinite(right) && std::isfinite(bottom) &&
            Width > 0.0 && Height > 0.0 &&
            x >= X && x < right && y >= Y && y < bottom;
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

// A menu's detail-paging arrows must advertise the SAME availability in
// both the rendered dock and click dispatch. Header navigation and CLOSE
// remain available even when a page has no scrollable detail rows.
struct DesktopMenuMouseAvailability final
{
    bool DetailsUp = false;
    bool DetailsDown = false;
};

inline DesktopMenuMouseAvailability AvailableDesktopMenuDetails(
    MenuPage visiblePage, const MenuDetailPager& pager) noexcept
{
    // After a page switch, do not expose stale scroll actions until DrawHUD
    // synchronizes the actual visible page. Also fail closed on a closed menu.
    if (visiblePage == MenuPage::Closed || pager.GetPage() != visiblePage)
    {
        return {};
    }
    const MenuDetailWindow window = pager.Window();
    return {window.HasPrevious, window.HasNext};
}

inline bool DesktopMenuMouseActionEnabled(
    DesktopMenuMouseAction action,
    DesktopMenuMouseAvailability availability) noexcept
{
    switch (action)
    {
    case DesktopMenuMouseAction::PreviousPage:
    case DesktopMenuMouseAction::NextPage:
    case DesktopMenuMouseAction::Activate:
    case DesktopMenuMouseAction::Close:
        return true;
    case DesktopMenuMouseAction::DetailsUp:
        return availability.DetailsUp;
    case DesktopMenuMouseAction::DetailsDown:
        return availability.DetailsDown;
    default:
        return false;
    }
}

inline DesktopMenuMouseLayout CalculateDesktopMenuMouseLayout(
    double width, double height, HudLayout layout,
    bool touchUI, bool menuOpen) noexcept
{
    if (touchUI || !menuOpen || layout.Compact ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width < 900.0 || height < 600.0 ||
        !std::isfinite(layout.Left) || layout.Left < 0.0 ||
        !std::isfinite(layout.Scale) || layout.Scale <= 0.0)
    {
        return {};
    }

    const double scale = std::clamp(layout.Scale, 0.6, 1.2);
    // Preserve legible pointer targets at 900x600 and on laptop windows
    // where text scaling is smaller. Never shrink a click below 44 px
    // vertically, or two columns below their readable caption widths.
    const double buttonW = std::max(112.0, 126.0 * scale);
    const double buttonH = std::max(44.0, 44.0 * scale);
    const double gap = std::max(8.0, 10.0 * scale);
    const double rightMargin = std::max(16.0, 22.0 * scale);
    const double left = width - (rightMargin + buttonW * 2.0 + gap);
    const double top = std::max(72.0, 100.0 * scale);
    const double bottom = top + 3.0 * buttonH + 2.0 * gap;
    if (!std::isfinite(left) || !std::isfinite(bottom) ||
        left < std::max(width * 0.58, layout.Left + 270.0 * scale) ||
        bottom > height - std::max(80.0, 140.0 * scale))
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

// Wheel navigation is confined to the visible left menu-detail lane.
// Its bounds exclude the desktop button dock and the SAVE FAILED notice.
inline DesktopMenuMouseRect CalculateDesktopMenuWheelZone(
    double width, double height, HudLayout layout,
    bool touchUI, bool menuOpen) noexcept
{
    if (touchUI || !menuOpen ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width < 240.0 || height < 240.0)
    {
        return {};
    }

    const HudTextBounds text =
        CalculateHudTextBounds(width, height, layout, false);
    const HudMenuVerticalPlan vertical =
        CalculateHudMenuVerticalPlan(width, height, layout, false);
    if (!text.Valid || !vertical.Valid)
    {
        return {};
    }
    const DesktopMenuMouseLayout dock =
        CalculateDesktopMenuMouseLayout(width, height, layout, false, true);
    const double right = dock.Valid
        ? std::min(text.Right, dock.Left - 12.0 * layout.Scale)
        : text.Right;
    const double bottom =
        vertical.OverflowNoticeY + 10.0 * layout.Scale;
    const double zoneWidth = right - text.Left;
    const double zoneHeight = bottom - layout.Top;
    if (!std::isfinite(right) || !std::isfinite(bottom) ||
        !std::isfinite(zoneWidth) || !std::isfinite(zoneHeight) ||
        zoneWidth <= 0.0 || zoneHeight <= 0.0 || bottom >= height)
    {
        return {};
    }
    return {text.Left, layout.Top, zoneWidth, zoneHeight};
}

// A valid pointer alone does not scroll: there must be a real adjacent
// detail page, and the wheel direction must be exactly one step.
inline bool AdmitDesktopMenuWheel(
    const DesktopMenuMouseRect& zone, double x, double y, int direction,
    DesktopMenuMouseAvailability available) noexcept
{
    if (!zone.Contains(x, y))
    {
        return false;
    }
    return direction == -1 ? available.DetailsUp :
           direction == 1 ? available.DetailsDown : false;
}

inline DesktopMenuMouseAction HitTestDesktopMenuMouse(
    const DesktopMenuMouseLayout& layout, double x, double y,
    DesktopMenuMouseAvailability availability = {true, true}) noexcept
{
    if (!layout.Valid || !std::isfinite(x) || !std::isfinite(y))
    {
        return DesktopMenuMouseAction::None;
    }
    for (const DesktopMenuMouseButton& button : layout.Buttons)
    {
        if (button.Bounds.Contains(x, y))
        {
            // A disabled arrow has no click authority and cannot leak its
            // action to the Pawn's menu input dispatcher.
            return DesktopMenuMouseActionEnabled(button.Action, availability)
                ? button.Action : DesktopMenuMouseAction::None;
        }
    }
    return DesktopMenuMouseAction::None;
}
// Optional fourth/fifth dock row, only when the visible page is Garage.
// The pre-existing six menu buttons and their paging semantics are
// unchanged. Every cosmetic pointer hit maps to one owner-gated action.
struct GarageStyleMouseLayout final
{
    bool Valid = false;
    std::array<DesktopMenuMouseButton, 4> Buttons{};
};

inline GarageStyleMouseLayout CalculateGarageStyleMouseLayout(
    double width, double height, HudLayout hud,
    bool touchUI, bool menuOpen, MenuPage page) noexcept
{
    if (page != MenuPage::Garage) return {};
    const auto dock = CalculateDesktopMenuMouseLayout(
        width, height, hud, touchUI, menuOpen);
    if (!dock.Valid) return {};
    const auto& first = dock.Buttons[4].Bounds;
    const auto& last = dock.Buttons[5].Bounds;
    const double scale = std::clamp(hud.Scale, 0.6, 1.2);
    const double gap = std::max(8.0, 10.0 * scale);
    // Keep the existing "WHEEL: DETAILS" hint ABOVE these new controls.
    const double top = last.Y + last.Height + std::max(26.0, 28.0 * scale);
    const double finalBottom = top + first.Height * 2.0 + gap;
    if (!std::isfinite(top) || !std::isfinite(finalBottom) ||
        finalBottom > height - 18.0)
        return {};

    GarageStyleMouseLayout result;
    result.Valid = true;
    result.Buttons = {{
        {DesktopMenuMouseAction::GaragePaint,
            {first.X, top, first.Width, first.Height}},
        {DesktopMenuMouseAction::GarageStripe,
            {last.X, top, last.Width, last.Height}},
        {DesktopMenuMouseAction::GarageWheels,
            {first.X, top + first.Height + gap, first.Width, first.Height}},
        {DesktopMenuMouseAction::GarageReset,
            {last.X, top + first.Height + gap, last.Width, last.Height}}
    }};
    return result;
}

inline DesktopMenuMouseAction HitTestGarageStyleMouse(
    const GarageStyleMouseLayout& layout,
    double x, double y, bool enabled = true) noexcept
{
    if (!enabled || !layout.Valid ||
        !std::isfinite(x) || !std::isfinite(y))
        return DesktopMenuMouseAction::None;
    for (const auto& button : layout.Buttons)
        if (button.Bounds.Contains(x, y)) return button.Action;
    return DesktopMenuMouseAction::None;
}
} // namespace latebrake
