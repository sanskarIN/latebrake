#pragma once

#include "Garage/GarageAppearance.h"

namespace latebrake
{
// Original deterministic source-only paint schemes for the six immutable
// garage vehicle IDs. These colors are visual identity only: no car unlock,
// body dimensions, Chaos contact physics, power, saves, or race awards.
struct VehiclePaintRgb final
{
    double R;
    double G;
    double B;
};

struct VehiclePaintRecipe final
{
    const char* Name;
    VehiclePaintRgb Body;
    VehiclePaintRgb Accent;
    VehiclePaintRgb Canopy;
};

inline constexpr VehiclePaintRecipe PaintForVehicle(int vehicleId) noexcept
{
    switch (vehicleId)
    {
    case 1: // Kestrel — fiery sprint with graphite aerodynamic accents.
        return {"EMBER ORANGE", {0.94, 0.30, 0.09},
                {0.13, 0.16, 0.21}, {0.09, 0.14, 0.20}};
    case 2: // Phantom — electric violet with ice white aerodynamic trim.
        return {"ELECTRIC VIOLET", {0.48, 0.29, 0.88},
                {0.88, 0.92, 0.99}, {0.11, 0.10, 0.19}};
    case 3: // Tempest — scarlet track car and dark titanium trim.
        return {"RACING SCARLET", {0.89, 0.10, 0.19},
                {0.14, 0.17, 0.22}, {0.10, 0.13, 0.17}};
    case 4: // Glider — sea glass green with a light accent.
        return {"SEA GLASS", {0.13, 0.67, 0.54},
                {0.90, 0.94, 0.85}, {0.08, 0.18, 0.20}};
    case 5: // Nova — bright solar yellow with dark contrast.
        return {"SOLAR YELLOW", {0.95, 0.74, 0.12},
                {0.12, 0.14, 0.19}, {0.09, 0.13, 0.17}};
    default: // Arrow and invalid IDs; never imply paid/unowned identity.
        return {"SKY BLUE", {0.11, 0.58, 0.87},
                {0.89, 0.94, 0.98}, {0.08, 0.16, 0.24}};
    }
}
// A user-selected paint is visual-only. The six legacy factory choices
// map EXACTLY to the old fixed recipes, including accents and canopy.
inline constexpr VehiclePaintRecipe PaintForGarageLook(
    int vehicleId, GarageLook look) noexcept
{
    if (!GarageState::IsVehicle(vehicleId) ||
        !ValidGarageLook(look))
        return PaintForVehicle(vehicleId);
    switch (look.Paint)
    {
    case 0: return PaintForVehicle(0);
    case 1: return PaintForVehicle(1);
    case 2: return PaintForVehicle(2);
    case 3: return PaintForVehicle(3);
    case 4: return PaintForVehicle(4);
    case 5: return PaintForVehicle(5);
    case 6: return {"GRAPHITE", {0.16, 0.19, 0.24},
                    {0.79, 0.82, 0.87}, {0.09, 0.13, 0.18}};
    case 7: return {"PEARL WHITE", {0.92, 0.93, 0.91},
                    {0.12, 0.19, 0.27}, {0.10, 0.14, 0.20}};
    default: return PaintForVehicle(vehicleId);
    }
}
} // namespace latebrake
