#pragma once
#include "Garage/GarageAppearance.h"

namespace latebrake
{
// Owned garage car changes must never render a previous car's saved rims
// before the selected vehicle's new look arrives. This is a pure cosmetic
// scheduling decision; not a permission to equip, spend, race or save.
struct GarageVisualTransition final
{
    int VehicleId = 0;
    GarageLook Look{};
    bool VehicleChanged = false;
    bool AppearanceChanged = false;
    bool RimsChanged = false;
    bool UpdateFallbackShape = false;
    bool RebuildSourceMesh = false;
    bool RefreshMaterials = false;
};

constexpr GarageVisualTransition PlanGarageVisualTransition(
    int currentId, GarageLook currentLook,
    int requestedId, GarageLook requestedLook,
    bool actorHasBegunPlay) noexcept
{
    GarageVisualTransition plan{};
    plan.VehicleId = GarageState::IsVehicle(requestedId) ? requestedId : 0;
    plan.Look = GarageState::IsVehicle(requestedId) &&
        ValidGarageLook(requestedLook)
        ? requestedLook : FactoryGarageLook(plan.VehicleId);
    plan.VehicleChanged = plan.VehicleId != currentId;
    plan.AppearanceChanged = !SameGarageLook(plan.Look, currentLook);
    plan.RimsChanged = plan.Look.Wheels != currentLook.Wheels;
    plan.UpdateFallbackShape = plan.VehicleChanged;
    plan.RebuildSourceMesh = actorHasBegunPlay &&
        (plan.VehicleChanged || plan.RimsChanged);
    plan.RefreshMaterials = actorHasBegunPlay &&
        (plan.VehicleChanged || plan.AppearanceChanged);
    return plan;
}

static_assert(!PlanGarageVisualTransition(0,{},0,{},true).RebuildSourceMesh);
static_assert(PlanGarageVisualTransition(
    0,FactoryGarageLook(0),1,FactoryGarageLook(1),true).RebuildSourceMesh);
static_assert(!PlanGarageVisualTransition(
    0,FactoryGarageLook(0),0,GarageLook{1,0,0},true).RebuildSourceMesh);
} // namespace latebrake
