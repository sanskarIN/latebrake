#pragma once

namespace latebrake
{
// Source-only original geometric silhouettes for six fictional garage cars.
// They scale decorative Unreal primitives; do not change chassis collision,
// tire physics, game rewards, progression or material licensing.
struct VehicleVisualRecipe final
{
    double HoodLengthScale;
    double RearDeckLengthScale;
    double RoofWidthScale;
    double WingSpanScale;
    double WingHeightCm;
    bool HasRearWing;
};

inline constexpr VehicleVisualRecipe VisualRecipeForVehicle(int vehicleId) noexcept
{
    switch (vehicleId)
    {
    case 1: // Kestrel: longer front and a low, wide rear wing.
        return {1.25, 0.86, 0.92, 1.18, 102.0, true};
    case 2: // Phantom: shorter front, longer rear deck, elevated narrow wing.
        return {0.88, 1.20, 1.10, 0.91, 118.0, true};
    case 3: // Tempest: long low nose and compact low-profile wing.
        return {1.32, 0.93, 0.88, 1.05, 91.0, true};
    case 4: // Glider: short nose, long tail and taller cockpit canopy.
        return {0.83, 1.31, 1.15, 0.98, 112.0, true};
    case 5: // Nova: stretched rear and widest wing of the original range.
        return {1.10, 1.13, 1.02, 1.28, 108.0, true};
    default: // Arrow: balanced body with a clean wingless rear.
        return {1.00, 1.00, 1.00, 0.0, 0.0, false};
    }
}
} // namespace latebrake
