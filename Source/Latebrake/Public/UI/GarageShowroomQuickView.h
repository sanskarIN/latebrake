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
    if (!schematic.Valid) return result;
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
        width<25.0 || y+height>bottom)
        return {};
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
    for (const auto& button:layout.Buttons)
        if (button.Bounds.Contains(x,y)) return button.Action;
    return DesktopMenuMouseAction::None;
}
} // namespace latebrake
