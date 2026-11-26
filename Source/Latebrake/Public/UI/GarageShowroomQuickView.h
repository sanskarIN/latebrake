#pragma once
#include "UI/GarageShowroomSchematic.h"

namespace latebrake
{
// Six small view-only buttons share the existing schematic title row.
// This is source UI geometry, never an authorization to equip, buy or save.
struct GarageQuickViewLayout final
{
    bool Valid = false;
    std::array<DesktopMenuMouseButton, VehicleCount> Buttons{};
};

inline GarageQuickViewLayout CalculateGarageQuickViewLayout(
    const GarageSchematicPlan& schematic) noexcept
{
    GarageQuickViewLayout result;
    if (!schematic.Valid || schematic.Count > schematic.Pieces.size())
        return result;
    const auto& panel = schematic.Panel;
    constexpr double gap=3.0, pad=8.0, height=20.0;
    const double width=(panel.Width-2.0*pad-
        (VehicleCount-1)*gap)/VehicleCount;
    const double right=panel.X+panel.Width;
    const double bottom=panel.Y+panel.Height;
    const double y=panel.Y+3.0;
    if (!std::isfinite(panel.X) || !std::isfinite(panel.Y) ||
        !std::isfinite(panel.Width) || !std::isfinite(panel.Height) ||
        !std::isfinite(right) || !std::isfinite(bottom) ||
        panel.X<0.0 || panel.Y<0.0 ||
        !std::isfinite(width) || width<25.0 || y+height>bottom ||
        right<=panel.X || bottom<=panel.Y)
        return {};
    // The schematic is an externally supplied bounded drawing plan.
    // Reject malformed or detached pieces before using its hit regions.
    for (std::size_t j=0;j<schematic.Count;j++)
    {
        const auto& r=schematic.Pieces[j].Bounds;
        const double rRight=r.X+r.Width;
        const double rBottom=r.Y+r.Height;
        if (!std::isfinite(r.X) || !std::isfinite(r.Y) ||
            !std::isfinite(r.Width) || !std::isfinite(r.Height) ||
            !std::isfinite(rRight) || !std::isfinite(rBottom) ||
            r.Width<=0.0 || r.Height<=0.0 ||
            rRight<=r.X || rBottom<=r.Y ||
            r.X<panel.X || r.Y<panel.Y ||
            rRight>right || rBottom>bottom)
            return {};
    }
    for (int i=0;i<VehicleCount;i++)
    {
        const double x=panel.X+pad+i*(width+gap);
        const DesktopMenuMouseRect bounds{x,y,width,height};
        if (bounds.X<panel.X || bounds.Y<panel.Y ||
            bounds.X+bounds.Width>right || bounds.Y+bounds.Height>bottom)
            return {};
        // Fail closed if a future painted schematic uses the header.
        for (std::size_t j=0;j<schematic.Count;j++)
        {
            const auto& r=schematic.Pieces[j].Bounds;
            if (bounds.X<r.X+r.Width && bounds.X+bounds.Width>r.X &&
                bounds.Y<r.Y+r.Height && bounds.Y+bounds.Height>r.Y)
                return {};
        }
        result.Buttons[static_cast<std::size_t>(i)] =
            {GarageQuickViewAction(i),bounds};
    }
    result.Valid=true;
    return result;
}

inline DesktopMenuMouseAction HitTestGarageQuickView(
    const GarageQuickViewLayout& layout, double x, double y,
    bool browseEnabled) noexcept
{
    if (!browseEnabled || !layout.Valid ||
        !std::isfinite(x) || !std::isfinite(y))
        return DesktopMenuMouseAction::None;
    DesktopMenuMouseAction candidate=DesktopMenuMouseAction::None;
    for (std::size_t i=0;i<layout.Buttons.size();++i)
    {
        const auto& button=layout.Buttons[i];
        const auto& r=button.Bounds;
        // A read-only quick-view target can never dispatch another menu
        // action, even if the layout was corrupted after construction.
        if (button.Action!=GarageQuickViewAction(static_cast<int>(i)) ||
            !std::isfinite(r.X) || !std::isfinite(r.Y) ||
            !std::isfinite(r.Width) || !std::isfinite(r.Height) ||
            !std::isfinite(r.X+r.Width) || !std::isfinite(r.Y+r.Height) ||
            r.X<0.0 || r.Y<0.0 || r.Width<25.0 || r.Height<20.0 ||
            r.X+r.Width<=r.X || r.Y+r.Height<=r.Y)
            return DesktopMenuMouseAction::None;
        if (!r.Contains(x,y)) continue;
        if (candidate!=DesktopMenuMouseAction::None)
            return DesktopMenuMouseAction::None; // ambiguous overlap
        candidate=button.Action;
    }
    return candidate;
}
} // namespace latebrake
