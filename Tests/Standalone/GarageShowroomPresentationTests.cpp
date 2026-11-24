#include "Vehicles/GarageShowroomPresentation.h"
#include "Vehicles/OriginalVehicleShowroomCamera.h"
#include <cstdlib>
#include <iostream>
namespace
{
void Require(bool good,const char* reason)
{
    if(!good){std::cerr<<"Visual-only 3D showroom transition: "
        <<reason<<'\n';std::exit(EXIT_FAILURE);}
}
void AllSixCarEntryExitPairs()
{
    using namespace latebrake;
    unsigned visited=0u;
    for(int equipped=0;equipped<VehicleCount;++equipped)
    for(int preview=0;preview<VehicleCount;++preview)
    for(int equippedRim=0;equippedRim<GarageWheelOptions;++equippedRim)
    for(int previewRim=0;previewRim<GarageWheelOptions;++previewRim)
    for(int previewPaint=0;previewPaint<GaragePaintOptions;++previewPaint)
    for(int previewStripe=0;previewStripe<GarageStripeOptions;++previewStripe)
    {
        const GarageLook raceLook{1u,0u,
            static_cast<std::uint8_t>(equippedRim)};
        const GarageLook viewedLook{
            static_cast<std::uint8_t>(previewPaint),
            static_cast<std::uint8_t>(previewStripe),
            static_cast<std::uint8_t>(previewRim)};
        const auto enter=PlanGarageShowroomPresentation({
            true,preview,viewedLook,equipped,raceLook,
            false,0,FactoryGarageLook(0),true});
        Require(enter.PreviewActive && enter.DisplayVehicleId==preview &&
                SameGarageLook(enter.DisplayLook,viewedLook),
                "visible showroom follows VIEWED, never silently equips it");
        Require(enter.ShapeChanged==(preview!=equipped) &&
                enter.RebuildSourceMesh==
                    (preview!=equipped || previewRim!=equippedRim),
                "only real visual shape/rim transitions rebuild geometry");
        const auto identical=PlanGarageShowroomPresentation({
            true,preview,viewedLook,equipped,raceLook,
            true,preview,viewedLook,true});
        Require(identical.PreviewActive &&
                !identical.ShapeChanged && !identical.AppearanceChanged &&
                !identical.RebuildSourceMesh && !identical.RefreshMaterials,
                "repeated source showroom sync is idempotent on every frame");
        const auto exit=PlanGarageShowroomPresentation({
            false,preview,viewedLook,equipped,raceLook,
            true,preview,viewedLook,true});
        Require(!exit.PreviewActive &&
                exit.DisplayVehicleId==equipped &&
                SameGarageLook(exit.DisplayLook,raceLook),
                "leaving showroom restores actual equipped body and saved look");
        Require(exit.RebuildSourceMesh==
                    (preview!=equipped || previewRim!=equippedRim),
                "restore rebuilds only if visible ID or rim differed");
        const auto pausedEntry=PlanGarageShowroomPresentation({
            true,preview,viewedLook,equipped,raceLook,
            false,0,{},false});
        Require(pausedEntry.PreviewActive &&
                !pausedEntry.RebuildSourceMesh &&
                !pausedEntry.RefreshMaterials,
                "uninitialized Pawn never allocates GPU materials");
        ++visited;
    }
    Require(visited==static_cast<unsigned>(
        VehicleCount*VehicleCount*GarageWheelOptions*
        GarageWheelOptions*GaragePaintOptions*GarageStripeOptions),
        "entire owned/unowned view x paint x stripe x rim state tested");
}
void AllRimsUseCommittedCameraGeometry()
{
    using namespace latebrake;
    using namespace latebrake::vehiclemesh;
    for(int car=0;car<VehicleCount;++car)
    for(int rim=0;rim<GarageWheelOptions;++rim)
    {
        const auto built=OriginalVehicleSurfaceComposer{}.Build(car,rim);
        OriginalVehicleShowroomGeometry points;
        Require(built.Valid && points.Build(built.Mesh),
                "actual source body and rim geometry prepares the camera");
        OriginalVehicleShowroomCamera fit;
        for(double yaw:{35.0,90.0,145.0,-90.0})
        for(double aspect:{0.5625,1.0,16.0/9.0,2.4})
        {
            ShowroomOrbitRequest q{};
            q.YawDegrees=yaw;
            q.AspectRatio=aspect;
            Require(fit.FitPrepared(points,q) && fit.VehicleId()==car,
                    "all 18 car/rim pairs frame in all four native orbit angles");
            for(const auto& vertex:built.Mesh.Vertices)
                Require(fit.Project(vertex.Position).InsidePaddedViewport,
                        "actual viewed rim and body vertices stay in frame");
        }
    }
}
void DeferredStartupAndInvalidIdentity()
{
    using namespace latebrake;
    for(int equipped=0;equipped<VehicleCount;++equipped)
    for(int viewed=0;viewed<VehicleCount;++viewed)
    for(int rim=0;rim<GarageWheelOptions;++rim)
    {
        const auto ownedLook=FactoryGarageLook(equipped);
        const GarageLook viewedLook{7u,2u,static_cast<std::uint8_t>(rim)};
        const auto deferred=PlanGarageShowroomPresentation({
            true,viewed,viewedLook,equipped,ownedLook,false,0,{},false});
        Require(deferred.PreviewActive &&
                deferred.DisplayVehicleId==viewed &&
                SameGarageLook(deferred.DisplayLook,viewedLook) &&
                !deferred.RebuildSourceMesh && !deferred.RefreshMaterials,
                "pre-BeginPlay stores complete displayed identity without allocation");
        const auto active=PlanGarageShowroomPresentation({
            true,viewed,viewedLook,equipped,ownedLook,
            deferred.PreviewActive,deferred.DisplayVehicleId,deferred.DisplayLook,true});
        Require(active.DisplayVehicleId==viewed &&
                SameGarageLook(active.DisplayLook,viewedLook) &&
                !active.RebuildSourceMesh && !active.RefreshMaterials,
                "BeginPlay must initialize stored presentation exactly once");
        const auto closed=PlanGarageShowroomPresentation({
            false,viewed,viewedLook,equipped,ownedLook,
            true,active.DisplayVehicleId,active.DisplayLook,true});
        Require(closed.DisplayVehicleId==equipped &&
                SameGarageLook(closed.DisplayLook,ownedLook),
                "leaving initialized preview restores equipped identity");
    }
    for(int invalid:{-1,VehicleCount,999})
    {
        const auto safe=PlanGarageShowroomPresentation({
            false,0,{},invalid,GarageLook{7u,2u,2u},false,0,{},true});
        Require(!safe.PreviewActive && safe.DisplayVehicleId==0 &&
                SameGarageLook(safe.DisplayLook,FactoryGarageLook(0)),
                "valid style from invalid car must not leak into starter fallback");
    }
}
void EditsAndUnownedBrowsingRemainReadOnly()
{
    using namespace latebrake;
    const GarageLook raceLook{2u,0u,0u};
    const GarageLook previewLook{6u,1u,1u};
    auto paintOnly=PlanGarageShowroomPresentation({
        true,4,GarageLook{7u,1u,1u},2,raceLook,
        true,4,previewLook,true});
    Require(!paintOnly.ShapeChanged && !paintOnly.RimShapeChanged &&
            !paintOnly.RebuildSourceMesh && paintOnly.RefreshMaterials,
            "viewed-car paint edit colors preview without rebuilding triangles");
    auto rim=PlanGarageShowroomPresentation({
        true,4,GarageLook{6u,1u,2u},2,raceLook,
        true,4,previewLook,true});
    Require(!rim.ShapeChanged && rim.RimShapeChanged &&
            rim.RebuildSourceMesh && rim.RefreshMaterials,
            "viewed-car rim update remeshes the displayed car once");
    const auto browse=PlanGarageShowroomPresentation({
        true,5,FactoryGarageLook(5),2,raceLook,
        true,4,previewLook,true});
    Require(browse.PreviewActive && browse.ShapeChanged &&
            browse.DisplayVehicleId==5 && browse.RebuildSourceMesh,
            "browsing locked sixth car changes only cosmetic presentation");
    const auto equipWhilePreview=PlanGarageShowroomPresentation({
        true,4,previewLook,5,FactoryGarageLook(5),
        true,4,previewLook,true});
    Require(equipWhilePreview.PreviewActive &&
            equipWhilePreview.DisplayVehicleId==4 &&
            !equipWhilePreview.RebuildSourceMesh &&
            !equipWhilePreview.RefreshMaterials,
            "actual equipment may change while view stays a different car");
    const auto restoreAfterEquip=PlanGarageShowroomPresentation({
        false,4,previewLook,5,FactoryGarageLook(5),
        true,4,previewLook,true});
    Require(restoreAfterEquip.DisplayVehicleId==5 &&
            restoreAfterEquip.RebuildSourceMesh &&
            !restoreAfterEquip.PreviewActive,
            "exit restores newly equipped car even if it was not previously displayed");
    const auto invalidBrowse=PlanGarageShowroomPresentation({
        true,77,previewLook,2,raceLook,
        true,4,previewLook,true});
    Require(!invalidBrowse.PreviewActive &&
            invalidBrowse.DisplayVehicleId==2 &&
            SameGarageLook(invalidBrowse.DisplayLook,raceLook),
            "invalid viewed ID cannot masquerade as an owned race vehicle");
    const auto corruptStyle=PlanGarageShowroomPresentation({
        true,1,GarageLook{255u,0u,0u},2,raceLook,
        false,0,{},true});
    Require(!corruptStyle.PreviewActive &&
            corruptStyle.DisplayVehicleId==2,
            "corrupt viewed appearance fails safe to real equipped car");
    const auto invalidEquipped=PlanGarageShowroomPresentation({
        false,1,{},254,GarageLook{255u,99u,99u},
        true,1,FactoryGarageLook(1),true});
    Require(invalidEquipped.DisplayVehicleId==0 &&
            SameGarageLook(invalidEquipped.DisplayLook,FactoryGarageLook(0)),
            "corrupt future equipped identity cannot escape starter safety");
}
}
int main()
{
    AllSixCarEntryExitPairs();
    DeferredStartupAndInvalidIdentity();
    AllRimsUseCommittedCameraGeometry();
    EditsAndUnownedBrowsingRemainReadOnly();
    std::cout<<"Read-only garage showroom entry/browse/style/exit passed.\n";
    return EXIT_SUCCESS;
}
