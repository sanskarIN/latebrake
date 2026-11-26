#pragma once

#include "Garage/GarageAppearance.h"

namespace latebrake::vehiclemesh
{
// Shared original rim geometry. The mesh composer and the desktop
// schematic consume these exact values; neither changes tire contact.
inline constexpr double OriginalVehiclePi=3.14159265358979323846;
inline constexpr int OriginalVehicleRimStyleCount=3;
static_assert(OriginalVehicleRimStyleCount==latebrake::GarageWheelOptions,
    "Original car rim meshes must match saved GarageLook wheel options");

struct OriginalVehicleRimRecipe final
{
    int Spokes = 0;
    double HalfAngle = 0.0;
    double InnerFraction = 0.0;
    double OuterFraction = 0.0;
};

constexpr OriginalVehicleRimRecipe OriginalRimRecipeForStyle(int style) noexcept
{
    switch (style)
    {
    case 0: return {7,0.09,0.16,0.91};
    case 1: return {5,0.17,0.12,0.94};
    case 2: return {12,0.065,0.22,0.97};
    default: return {};
    }
}
constexpr int OriginalVehicleSpokesForStyle(int style) noexcept
{
    return OriginalRimRecipeForStyle(style).Spokes;
}
static_assert(OriginalVehicleSpokesForStyle(0)==7);
static_assert(OriginalVehicleSpokesForStyle(1)==5);
static_assert(OriginalVehicleSpokesForStyle(2)==12);
} // namespace latebrake::vehiclemesh
