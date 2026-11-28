#pragma once

#include "Garage/GarageAppearance.h"
#include "UI/HudMenuMouseControls.h"
#include "Vehicles/VehicleBodyKitRecipe.h"
#include "Vehicles/VehicleVisualRecipe.h"

#include <array>
#include <algorithm>
#include <cmath>

namespace latebrake
{
// This is an honest TWO-DIMENSIONAL visual key for the primitive-mesh
// Unreal garage. It is not an offscreen UE 3D render or a physics proxy.
// Every coordinate is derived from the exact desktop garage button dock.
enum class GarageSchematicInk
{
    Shadow, Tire, Rim, Body, Accent, Canopy, Stripe
};

struct GarageSchematicPiece final
{
    DesktopMenuMouseRect Bounds;
    GarageSchematicInk Ink = GarageSchematicInk::Body;
};

struct GarageSchematicPlan final
{
    bool Valid = false;
    DesktopMenuMouseRect Panel;
    std::array<GarageSchematicPiece, 40> Pieces{};
    std::size_t Count = 0;
};

// Bounded drawing list: never return a partially valid plan or a rectangle
// extending into a click target, menu text column, or window edge.
inline GarageSchematicPlan CalculateGarageSchematic(
    double viewportWidth, double viewportHeight, HudLayout hud,
    const GarageStyleMouseLayout& styleDock, int vehicleId,
    GarageLook look) noexcept
{
    GarageSchematicPlan result;
    if (!styleDock.Valid || hud.Compact ||
        !GarageState::IsVehicle(vehicleId) || !ValidGarageLook(look) ||
        !std::isfinite(viewportWidth) || !std::isfinite(viewportHeight) ||
        !std::isfinite(hud.Scale) || hud.Scale <= 0.0 ||
        viewportWidth < 900.0 || viewportHeight < 600.0)
        return result;

    const auto& leftButton = styleDock.Buttons[4].Bounds;
    const auto& rightButton = styleDock.Buttons[5].Bounds;
    const double left = leftButton.X;
    const double right = rightButton.X + rightButton.Width;
    const double scale = std::clamp(hud.Scale, 0.6, 1.2);
    const double top = rightButton.Y + rightButton.Height +
        std::max(9.0, 12.0 * scale);
    const double bottom = viewportHeight - std::max(14.0, 20.0 * scale);
    const double height = std::min(bottom - top,
        std::max(126.0, 170.0 * scale));
    const double width = right - left;
    if (!std::isfinite(left) || !std::isfinite(right) ||
        !std::isfinite(top) || !std::isfinite(bottom) ||
        !std::isfinite(width) || !std::isfinite(height) ||
        width < 224.0 || height < 112.0 || left < 0.0 ||
        right > viewportWidth || top < 0.0 ||
        top + height > bottom || bottom > viewportHeight)
        return result;

    result.Panel = {left, top, width, height};
    // Never draw through the clickable button row. Use the actual dock
    // bounds, not hard-coded screen-dependent pixels.
    for (const auto& button : styleDock.Buttons)
    {
        const auto& r = button.Bounds;
        if (!std::isfinite(r.Y) || !std::isfinite(r.Height) ||
            r.Y + r.Height > top)
            return GarageSchematicPlan{};
    }

    const auto push = [&](GarageSchematicInk ink,
                          double x, double y,
                          double w, double h) -> bool
    {
        const double edgeX = x + w;
        const double edgeY = y + h;
        if (result.Count >= result.Pieces.size() ||
            !std::isfinite(x) || !std::isfinite(y) ||
            !std::isfinite(w) || !std::isfinite(h) ||
            !std::isfinite(edgeX) || !std::isfinite(edgeY) ||
            w <= 0.0 || h <= 0.0 ||
            x < result.Panel.X || y < result.Panel.Y ||
            edgeX > result.Panel.X + result.Panel.Width ||
            edgeY > result.Panel.Y + result.Panel.Height)
            return false;
        result.Pieces[result.Count++] = {{x,y,w,h},ink};
        return true;
    };

    const VehicleSpec spec = GarageState::Definition(vehicleId);
    const VehicleVisualRecipe silhouette =
        VisualRecipeForVehicle(vehicleId);
    const VehicleBodyKitRecipe kit = BodyKitForVehicle(vehicleId);

    const double x = left + width * 0.10;
    const double y = top + std::max(27.0, 27.0 * scale);
    const double w = width * 0.80;
    const double h = (height - (y - top) - 8.0) * 0.86;
    if (!std::isfinite(h) || h < 35.0) return GarageSchematicPlan{};

    bool ok = true;
    const auto add = [&](GarageSchematicInk ink,
                         double bx, double by,
                         double bw, double bh)
    {
        if (!push(ink,bx,by,bw,bh)) ok = false;
    };
    // Nose points right. Four tires and their rim inserts sit OUTSIDE the
    // painted chassis, unlike the real vehicle's physics-contact wheels.
    add(GarageSchematicInk::Shadow,
        x + w * 0.10, y + h * 0.27, w * 0.80, h * 0.50);
    for (double axle : {0.21, 0.71})
    {
        for (double tireY : {0.04, 0.78})
        {
            add(GarageSchematicInk::Tire,
                x + w * axle, y + h * tireY, w * 0.12, h * 0.18);
            const double rimWidth =
                look.Wheels == 2u ? 0.074 :
                look.Wheels == 1u ? 0.060 : 0.043;
            add(GarageSchematicInk::Rim,
                x + w * (axle + (0.12 - rimWidth) * 0.50),
                y + h * (tireY + 0.035),
                w * rimWidth, h * 0.110);
        }
    }
    // Three body slices use the same original six-car silhouette and body
    // kit recipes as the source-only Unreal pawn, never new car statistics.
    const double bodyTop = y + h * 0.31;
    const double bodyHeight = h * 0.43;
    add(GarageSchematicInk::Body,
        x + w * 0.13, bodyTop, w * 0.73, bodyHeight);
    add(GarageSchematicInk::Body,
        x + w * 0.66, y + h * 0.355,
        w * (0.20 + 0.025 * silhouette.HoodLengthScale), h * 0.34);
    add(GarageSchematicInk::Accent,
        x + w * 0.105, y + h * 0.285,
        w * (0.015 + 0.025 * silhouette.RearDeckLengthScale),
        h * 0.47);
    // Rooftop variations are intentionally visible in the image.
    const double canopyHeight =
        h * (0.20 + 0.075 * silhouette.RoofWidthScale);
    const double canopyY = y + h * 0.525 - canopyHeight * 0.5;
    add(GarageSchematicInk::Canopy,
        x + w * 0.34, canopyY,
        w * (0.235 + 0.045 * spec.CabinLength), canopyHeight);
    add(GarageSchematicInk::Accent,
        x + w * 0.87, y + h * (0.5 - 0.28 * kit.SplitterWidthScale),
        w * 0.030, h * 0.56 * kit.SplitterWidthScale);
    if (silhouette.HasRearWing)
    {
        add(GarageSchematicInk::Accent,
            x + w * 0.065,
            y + h * (0.5 - 0.22 * silhouette.WingSpanScale),
            w * 0.029, h * 0.44 * silhouette.WingSpanScale);
    }
    if (kit.HasHoodScoop)
    {
        add(GarageSchematicInk::Accent,
            x + w * 0.725, y + h * 0.477,
            w * 0.050, h * 0.095);
    }
    if (kit.HasRearFins)
    {
        add(GarageSchematicInk::Accent,
            x + w * 0.155, y + h * 0.302,
            w * 0.058, h * 0.030);
        add(GarageSchematicInk::Accent,
            x + w * 0.155, y + h * 0.715,
            w * 0.058, h * 0.030);
    }
    // These lines are cosmetic-only; they cannot create collision sensors.
    if (look.Stripe == 1u)
    {
        add(GarageSchematicInk::Stripe,
            x + w * 0.19, y + h * 0.515,
            w * 0.63, h * 0.026);
    }
    if (look.Stripe == 2u)
    {
        for (double stripeY : {0.454, 0.575})
            add(GarageSchematicInk::Stripe,
                x + w * 0.19, y + h * stripeY,
                w * 0.63, h * 0.023);
    }
    if (!ok) return GarageSchematicPlan{};
    result.Valid = true;
    return result;
}
} // namespace latebrake
