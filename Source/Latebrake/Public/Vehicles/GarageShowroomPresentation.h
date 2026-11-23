#pragma once

#include "Vehicles/GarageVisualTransition.h"

namespace latebrake
{
// Only the *visible cosmetic shell* can be previewed. GarageState remains
// authoritative for equipped ID, ownership, credits, driving and SaveGame.
// The menu cursor is not a physical vehicle-selection or purchase request.
struct GarageShowroomRequest final
{
    bool GarageMenuVisible=false;
    int ViewedVehicleId=0;
    GarageLook ViewedLook{};
    int EquippedVehicleId=0;
    GarageLook EquippedLook{};
    bool WasPreviewActive=false;
    int PreviousPreviewVehicleId=0;
    GarageLook PreviousPreviewLook{};
    bool ActorHasBegunPlay=false;
};

struct GarageShowroomPresentation final
{
    bool PreviewActive=false;
    int DisplayVehicleId=0;
    GarageLook DisplayLook{};
    bool ShapeChanged=false;
    bool AppearanceChanged=false;
    bool RimShapeChanged=false;
    bool RebuildSourceMesh=false;
    bool RefreshMaterials=false;
};

constexpr GarageShowroomPresentation PlanGarageShowroomPresentation(
    GarageShowroomRequest request) noexcept
{
    // Corrupted or future SaveGame data is resolved to a safe starter
    // cosmetic identity. Actual unlocked cars are never changed here.
    const int equippedId=GarageState::IsVehicle(request.EquippedVehicleId)
        ? request.EquippedVehicleId : 0;
    const GarageLook equippedLook=ValidGarageLook(request.EquippedLook)
        ? request.EquippedLook : FactoryGarageLook(equippedId);

    const bool priorPreview=request.WasPreviewActive &&
        GarageState::IsVehicle(request.PreviousPreviewVehicleId) &&
        ValidGarageLook(request.PreviousPreviewLook);
    const bool nextPreview=request.GarageMenuVisible &&
        GarageState::IsVehicle(request.ViewedVehicleId) &&
        ValidGarageLook(request.ViewedLook);

    const int currentDisplayedId=priorPreview
        ? request.PreviousPreviewVehicleId : equippedId;
    const GarageLook currentDisplayedLook=priorPreview
        ? request.PreviousPreviewLook : equippedLook;
    const int nextDisplayedId=nextPreview
        ? request.ViewedVehicleId : equippedId;
    const GarageLook nextDisplayedLook=nextPreview
        ? request.ViewedLook : equippedLook;

    const auto transition=PlanGarageVisualTransition(
        currentDisplayedId,currentDisplayedLook,
        nextDisplayedId,nextDisplayedLook,request.ActorHasBegunPlay);

    return {
        nextPreview,nextDisplayedId,nextDisplayedLook,
        transition.UpdateFallbackShape,
        transition.AppearanceChanged,
        transition.RimsChanged,
        transition.RebuildSourceMesh,
        transition.RefreshMaterials
    };
}

static_assert(!PlanGarageShowroomPresentation({}).PreviewActive);
static_assert(PlanGarageShowroomPresentation({
    true,3,FactoryGarageLook(3),
    0,FactoryGarageLook(0),
    false,0,FactoryGarageLook(0),true
}).RebuildSourceMesh);
static_assert(PlanGarageShowroomPresentation({
    false,3,FactoryGarageLook(3),
    0,FactoryGarageLook(0),
    true,3,FactoryGarageLook(3),true
}).DisplayVehicleId==0);
} // namespace latebrake
