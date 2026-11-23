#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace latebrake::vehiclemesh
{
// The original 3D mesh uses a real Unreal material, not a custom shader
// asset in this repository. Only a parameter that actually exists on
// the loaded material can provide its 10 per-section automotive colors.
// These exact names are checked against UE GetAllVectorParameterInfo;
// guessing "Color" and hiding the working mesh is not acceptable.
inline constexpr std::array<const char*,5> SupportedVehicleTintNames{{
    "Color", "Colour", "BaseColor", "BaseColour", "Tint"
}};

inline constexpr std::uint32_t VehicleTintKnownMask =
    (1u << SupportedVehicleTintNames.size()) - 1u;

constexpr int SelectVehicleTintCandidate(
    std::uint32_t materialParameterMask) noexcept
{
    const std::uint32_t supported =
        materialParameterMask & VehicleTintKnownMask;
    for (std::size_t i=0;i<SupportedVehicleTintNames.size();++i)
        if ((supported & (1u << i)) != 0u)
            return static_cast<int>(i);
    return -1;
}

// A fully created procedural mesh is NOT enough to replace the visible
// legacy car: it must also support real per-section color updates.
// No collision or vehicle tuning changes are permitted by this policy.
constexpr bool CanPresentOriginalVehicleVisual(
    bool validSections, bool allocatedMaterials,
    bool tintParameterFound) noexcept
{
    return validSections && allocatedMaterials && tintParameterFound;
}

static_assert(SelectVehicleTintCandidate(0u) == -1);
static_assert(SelectVehicleTintCandidate(1u) == 0);
static_assert(SelectVehicleTintCandidate(2u) == 1);
static_assert(SelectVehicleTintCandidate(1u << 10) == -1);
static_assert(!CanPresentOriginalVehicleVisual(true,true,false));
} // namespace latebrake::vehiclemesh
