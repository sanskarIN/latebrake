#include "Vehicles/GarageVisualTransition.h"
#include <cstdlib>
#include <iostream>
namespace
{
void Require(bool value,const char* why)
{
    if(!value){std::cerr<<"Garage atomic visual transition: "<<why<<'\n';
        std::exit(EXIT_FAILURE);}
}
}
int main()
{
    using namespace latebrake;
    unsigned long long cases=0;
    for(int oldId=0;oldId<VehicleCount;++oldId)
    for(int newId=0;newId<VehicleCount;++newId)
    for(int oldRim=0;oldRim<GarageWheelOptions;++oldRim)
    for(int newRim=0;newRim<GarageWheelOptions;++newRim)
    for(int paint=0;paint<GaragePaintOptions;++paint)
    for(int stripe=0;stripe<GarageStripeOptions;++stripe)
    {
        const GarageLook before{0u,0u,static_cast<std::uint8_t>(oldRim)};
        const GarageLook next{
            static_cast<std::uint8_t>(paint),
            static_cast<std::uint8_t>(stripe),
            static_cast<std::uint8_t>(newRim)};
        const auto p=PlanGarageVisualTransition(
            oldId,before,newId,next,true);
        const bool carChanged=oldId!=newId;
        const bool appearanceChanged=!SameGarageLook(before,next);
        Require(p.VehicleId==newId && SameGarageLook(p.Look,next),
            "valid selected car/look arrive together");
        Require(p.VehicleChanged==carChanged &&
                p.UpdateFallbackShape==carChanged,
            "only car selection changes fallback source silhouette");
        Require(p.RimsChanged==(oldRim!=newRim) &&
                p.RebuildSourceMesh==(carChanged||oldRim!=newRim),
            "one combined native mesh rebuild even for simultaneous car/rim change");
        Require(p.RefreshMaterials==(carChanged||appearanceChanged),
            "paint/stripe-only updates material without mesh regeneration");
        const auto again=PlanGarageVisualTransition(
            p.VehicleId,p.Look,p.VehicleId,p.Look,true);
        Require(!again.VehicleChanged && !again.AppearanceChanged &&
                !again.RimsChanged && !again.UpdateFallbackShape &&
                !again.RebuildSourceMesh && !again.RefreshMaterials,
            "reapplying same chosen car and saved look is fully inert");
        const auto deferred=PlanGarageVisualTransition(
            oldId,before,newId,next,false);
        Require(deferred.VehicleId==newId &&
                SameGarageLook(deferred.Look,next) &&
                !deferred.RebuildSourceMesh && !deferred.RefreshMaterials,
            "construction-time style update never touches unavailable materials");
        ++cases;
    }
    const auto invalidId=PlanGarageVisualTransition(
        3,GarageLook{2,1,2},88,GarageLook{7,2,2},true);
    Require(invalidId.VehicleId==0 &&
            SameGarageLook(invalidId.Look,FactoryGarageLook(0)) &&
            invalidId.VehicleChanged && invalidId.RebuildSourceMesh,
        "unknown/future car ID fails closed to starter factory appearance");
    const auto invalidLook=PlanGarageVisualTransition(
        2,GarageLook{1,0,0},2,GarageLook{255,0,7},true);
    Require(invalidLook.VehicleId==2 &&
            SameGarageLook(invalidLook.Look,FactoryGarageLook(2)),
        "corrupt saved look is never sent to native 3D mesh renderer");
    const auto onlyStripe=PlanGarageVisualTransition(
        1,GarageLook{1,0,2},1,GarageLook{1,2,2},true);
    Require(!onlyStripe.RebuildSourceMesh && onlyStripe.RefreshMaterials,
        "stripe change reuses mesh and wheel shape");
    const auto onlyPaint=PlanGarageVisualTransition(
        1,GarageLook{1,0,2},1,GarageLook{3,0,2},true);
    Require(!onlyPaint.RebuildSourceMesh && onlyPaint.RefreshMaterials,
        "paint-only change reuses existing 3D geometry");
    Require(cases==static_cast<unsigned long long>(VehicleCount)*
        VehicleCount*GarageWheelOptions*GarageWheelOptions*
        GaragePaintOptions*GarageStripeOptions,
        "all distinct current/next car/rim/paint/stripe options traversed");
    std::cout<<"Atomic visual transitions checked: "<<cases<<" valid cases\n";
    return EXIT_SUCCESS;
}
