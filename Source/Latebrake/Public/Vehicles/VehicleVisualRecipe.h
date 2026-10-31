#pragma once

namespace latebrake
{
// Pure presentation recipe for the three fictional prototype garage cars.
// This changes decorative primitive meshes only, NEVER chassis collision,
// mass, tuning, rewards or race authority.
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
    default: // Arrow: balanced body with a clean wingless rear.
        return {1.00, 1.00, 1.00, 0.0, 0.0, false};
    }
}
} // namespace latebrake
