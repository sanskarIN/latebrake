#pragma once

#include "Vehicles/OriginalVehicleSurfaceMesh.h"
#include <array>
#include <cstdint>

namespace latebrake::vehiclemesh
{
// Artist-intent scalar targets, not an authored Unreal material graph.
// A parameter being exposed is not proof that a shader uses it visually.
enum class VehicleFinishParameter : std::uint8_t
{
    Roughness, Metallic, Specular, ClearCoat, ClearCoatRoughness, Count
};
inline constexpr std::array<const char*,5> VehicleFinishParameterNames{
    "Roughness","Metallic","Specular","ClearCoat","ClearCoatRoughness"};
static_assert(VehicleFinishParameterNames.size()==
    static_cast<std::size_t>(VehicleFinishParameter::Count));
inline constexpr std::uint32_t VehicleFinishKnownMask=(1u<<VehicleFinishParameterNames.size())-1u;

struct VehicleFinishScalars final
{
    double Roughness=0.5;
    double Metallic=0.0;
    double Specular=0.5;
    double ClearCoat=0.0;
    double ClearCoatRoughness=0.2;

    constexpr std::array<double,5> Values() const noexcept
    {return {Roughness,Metallic,Specular,ClearCoat,ClearCoatRoughness};}
};

constexpr bool ValidVehicleFinish(VehicleFinishScalars finish) noexcept
{
    for(double value:finish.Values())
        if(!(value>=0.0&&value<=1.0))return false;
    return true;
}

constexpr VehicleFinishScalars OriginalBodyPaintFinish(int paint) noexcept
{
    // Graphite is a satin finish; pearl white and the six factory colors
    // are clear-coated. The choice is derived from the existing paint ID,
    // so there is no schema migration or new purchase/physics authority.
    switch(paint)
    {
    case 0: return {0.27,0.20,0.5,0.8,0.16};
    case 1: return {0.24,0.28,0.5,0.85,0.14};
    case 2: return {0.22,0.38,0.5,0.9,0.13};
    case 3: return {0.25,0.12,0.5,0.85,0.14};
    case 4: return {0.28,0.25,0.5,0.75,0.18};
    case 5: return {0.26,0.15,0.5,0.85,0.15};
    case 6: return {0.48,0.32,0.5,0.25,0.32};
    case 7: return {0.23,0.18,0.5,0.9,0.12};
    default: return {};
    }
}
constexpr VehicleFinishScalars OriginalAlloyFinish(int rims) noexcept
{
    switch(rims)
    {
    case 0: return {0.42,0.80,0.5,0.0,0.3};
    case 1: return {0.23,0.93,0.5,0.0,0.2};
    case 2: return {0.31,0.85,0.5,0.15,0.2};
    default: return {};
    }
}

constexpr VehicleFinishScalars OriginalSurfaceFinish(
    OriginalVehicleMaterial material,GarageLook look) noexcept
{
    if(!ValidGarageLook(look))return {};
    switch(material)
    {
    case OriginalVehicleMaterial::BodyPaint: return OriginalBodyPaintFinish(look.Paint);
    case OriginalVehicleMaterial::Accent: return {0.42,0.15,0.5,0.12,0.32};
    case OriginalVehicleMaterial::ClearGlass: return {0.12,0.0,0.6,0.0,0.2};
    case OriginalVehicleMaterial::Rubber: return {0.88,0.0,0.27,0.0,0.5};
    case OriginalVehicleMaterial::Alloy: return OriginalAlloyFinish(look.Wheels);
    case OriginalVehicleMaterial::BrakeRotor: return {0.57,0.91,0.5,0.0,0.4};
    case OriginalVehicleMaterial::Headlamp: return {0.15,0.0,0.5,0.0,0.2};
    case OriginalVehicleMaterial::Taillamp: return {0.20,0.0,0.5,0.0,0.2};
    case OriginalVehicleMaterial::Intake: return {0.75,0.0,0.3,0.0,0.5};
    case OriginalVehicleMaterial::CarbonTrim: return {0.46,0.1,0.5,0.18,0.25};
    default: return {};
    }
}
constexpr VehicleFinishScalars OriginalStripeFinish(GarageLook look) noexcept
{
    if(!ValidGarageLook(look))return {};
    // Vinyl bands have a different target finish from the painted body.
    return {0.36,0.0,0.45,0.1,0.28};
}

struct VehicleFinishWrite final
{
    VehicleFinishParameter Parameter=VehicleFinishParameter::Count;
    double Value=0.0;
};
struct VehicleFinishPlan final
{
    std::array<VehicleFinishWrite,VehicleFinishParameterNames.size()> Writes{};
    std::size_t Count=0;
    std::uint32_t Mask=0u;
};

// Fail closed for invalid numeric recipes. Ignore unknown capability bits.
// In particular, an absent scalar never receives a guessed parameter write.
constexpr VehicleFinishPlan PlanVehicleFinishWrites(
    std::uint32_t exposed,VehicleFinishScalars finish) noexcept
{
    VehicleFinishPlan result;
    if(!ValidVehicleFinish(finish))return result;
    result.Mask=exposed&VehicleFinishKnownMask;
    const auto values=finish.Values();
    for(std::size_t i=0;i<values.size();++i)
        if((result.Mask&(1u<<i))!=0u)
            result.Writes[result.Count++]={static_cast<VehicleFinishParameter>(i),values[i]};
    return result;
}
constexpr int VehicleFinishCapabilityCount(std::uint32_t exposed) noexcept
{
    int count=0;
    for(std::size_t i=0;i<VehicleFinishParameterNames.size();++i)
        if((exposed&(1u<<i))!=0u)++count;
    return count;
}
static_assert(ValidVehicleFinish(OriginalBodyPaintFinish(7)));
static_assert(PlanVehicleFinishWrites(0u,OriginalAlloyFinish(1)).Count==0);
static_assert(VehicleFinishCapabilityCount(VehicleFinishKnownMask)==5);
} // namespace latebrake::vehiclemesh
