#pragma once

namespace latebrake
{
// Original, engine-neutral optional trim for the six existing garage IDs.
// Units: scale ratios or car-relative Unreal centimetres, never vehicle
// physics or newly unlocked items. All meshes are non-colliding cosmetics.
struct VehicleBodyKitRecipe final
{
    const char* Name;
    double SplitterWidthScale;
    double SideSkirtLengthScale;
    double DiffuserWidthScale;
    double ScoopHeightCm;
    double RearFinHeightCm;
    bool HasHoodScoop;
    bool HasRearFins;
};

inline constexpr VehicleBodyKitRecipe BodyKitForVehicle(int vehicleId) noexcept
{
    switch (vehicleId)
    {
    case 1: // Kestrel: broad splitter and restrained hood induction.
        return {"SPRINT", 1.12, 1.02, 0.94, 15.0, 0.0, true, false};
    case 2: // Phantom: streamlined nose, long side blades and tall rear fins.
        return {"GT", 0.91, 1.17, 1.04, 0.0, 31.0, false, true};
    case 3: // Tempest: widest front aero, large scoop and track-day fins.
        return {"TRACK", 1.20, 0.93, 1.12, 23.0, 26.0, true, true};
    case 4: // Glider: slim front, extended skirts and an aero rear.
        return {"TOURING", 0.88, 1.25, 1.09, 0.0, 0.0, false, false};
    case 5: // Nova: pronounced scoop, wide diffuser and tall rear fins.
        return {"APEX", 1.14, 1.10, 1.20, 20.0, 35.0, true, true};
    default: // Arrow and invalid IDs: conservative wingless base trim.
        return {"STREET", 1.00, 1.00, 1.00, 0.0, 0.0, false, false};
    }
}
} // namespace latebrake
