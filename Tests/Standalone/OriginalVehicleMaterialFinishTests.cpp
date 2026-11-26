#include "Vehicles/OriginalVehicleMaterialFinish.h"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
using namespace latebrake;
using namespace latebrake::vehiclemesh;
void Check(bool ok,const char* message)
{
    if(!ok){std::cerr<<message<<'\n';std::exit(EXIT_FAILURE);}
}
void ExhaustFinishPlans()
{
    for(int p=0;p<GaragePaintOptions;++p)
        for(int s=0;s<GarageStripeOptions;++s)
            for(int w=0;w<GarageWheelOptions;++w)
            {
                const GarageLook look{static_cast<std::uint8_t>(p),
                    static_cast<std::uint8_t>(s),static_cast<std::uint8_t>(w)};
                for(int section=0;section<=static_cast<int>(OriginalVehicleMaterial::Count);++section)
                {
                    const auto finish=section==static_cast<int>(OriginalVehicleMaterial::Count)
                        ? OriginalStripeFinish(look)
                        : OriginalSurfaceFinish(static_cast<OriginalVehicleMaterial>(section),look);
                    Check(ValidVehicleFinish(finish),"every surface finish is bounded");
                    for(std::uint32_t mask=0;mask<=VehicleFinishKnownMask;++mask)
                    {
                        const auto plan=PlanVehicleFinishWrites(mask,finish);
                        Check(plan.Mask==mask&&plan.Count==static_cast<std::size_t>(
                            VehicleFinishCapabilityCount(mask)),"exact exposed parameter count");
                        std::uint32_t observed=0;
                        for(std::size_t i=0;i<plan.Count;++i)
                        {
                            const auto k=static_cast<std::size_t>(plan.Writes[i].Parameter);
                            Check(k<VehicleFinishParameterNames.size(),"valid parameter identity");
                            Check((observed&(1u<<k))==0u,"each scalar written once");
                            observed|=1u<<k;
                            Check(plan.Writes[i].Value==finish.Values()[k],"writes preserve artist target");
                        }
                        Check(observed==mask,"no absent parameter writes");
                        const auto unknown=PlanVehicleFinishWrites(mask|0xffff0000u,finish);
                        Check(unknown.Mask==mask&&unknown.Count==plan.Count,"unknown flags ignored");
                    }
                }
            }
}
void SpecificMaterialRelationships()
{
    const GarageLook look{0,0,0};
    const auto body=OriginalSurfaceFinish(OriginalVehicleMaterial::BodyPaint,look);
    const auto tire=OriginalSurfaceFinish(OriginalVehicleMaterial::Rubber,look);
    const auto glass=OriginalSurfaceFinish(OriginalVehicleMaterial::ClearGlass,look);
    const auto alloy=OriginalSurfaceFinish(OriginalVehicleMaterial::Alloy,look);
    const auto stripe=OriginalStripeFinish(look);
    Check(tire.Roughness>body.Roughness&&tire.Metallic==0&&tire.ClearCoat==0,
        "rubber is rough and nonmetallic");
    Check(glass.Roughness<body.Roughness&&glass.Metallic==0,"glazing finish distinct");
    Check(alloy.Metallic>body.Metallic&&alloy.Metallic>0.7,"alloy target distinct");
    Check(stripe.Roughness>body.Roughness&&stripe.Metallic==0,"vinyl target distinct");
    Check(OriginalBodyPaintFinish(6).Roughness>OriginalBodyPaintFinish(7).Roughness,
        "graphite satin differs from pearl white");
    Check(OriginalAlloyFinish(1).Roughness<OriginalAlloyFinish(0).Roughness,
        "sport alloy target differs from factory");
    for(int p=0;p<GaragePaintOptions;++p)
        for(int w=0;w<GarageWheelOptions;++w)
        {
            const GarageLook a{static_cast<std::uint8_t>(p),0,static_cast<std::uint8_t>(w)};
            auto b=a;b.Stripe=2;
            for(int i=0;i<static_cast<int>(OriginalVehicleMaterial::Count);++i)
            {
                const auto material=static_cast<OriginalVehicleMaterial>(i);
                Check(OriginalSurfaceFinish(material,a).Values()==OriginalSurfaceFinish(material,b).Values(),
                    "stripe choice does not change other surface finishes");
            }
        }
}
void RejectBadScalars()
{
    const double nan=std::numeric_limits<double>::quiet_NaN();
    const double inf=std::numeric_limits<double>::infinity();
    for(double bad:{nan,inf,-inf,-0.01,1.01})
    {
        for(int field=0;field<5;++field)
        {
            VehicleFinishScalars finish;
            double* fields[]={&finish.Roughness,&finish.Metallic,&finish.Specular,
                &finish.ClearCoat,&finish.ClearCoatRoughness};
            *fields[field]=bad;
            Check(!ValidVehicleFinish(finish),"invalid scalar detected");
            const auto plan=PlanVehicleFinishWrites(VehicleFinishKnownMask,finish);
            Check(plan.Count==0&&plan.Mask==0,"invalid recipe cannot partially write");
        }
    }
    const auto neutral=VehicleFinishScalars{}.Values();
    Check(OriginalBodyPaintFinish(-1).Values()==neutral,"unknown paint neutral");
    Check(OriginalAlloyFinish(9).Values()==neutral,"unknown alloy neutral");
    Check(OriginalSurfaceFinish(static_cast<OriginalVehicleMaterial>(255),{}).Values()==neutral,
        "unknown material neutral");
    Check(OriginalSurfaceFinish(OriginalVehicleMaterial::Alloy,{0,0,255}).Values()==neutral,
        "corrupt look neutral");
    for(std::size_t i=0;i<VehicleFinishParameterNames.size();++i)
        for(std::size_t j=i+1;j<VehicleFinishParameterNames.size();++j)
            Check(std::string_view(VehicleFinishParameterNames[i])!=VehicleFinishParameterNames[j],
                "parameter names unique");
}
}
int main()
{
    ExhaustFinishPlans();
    SpecificMaterialRelationships();
    RejectBadScalars();
    std::cout<<"Original vehicle material finish plans passed.\n";
    return EXIT_SUCCESS;
}
