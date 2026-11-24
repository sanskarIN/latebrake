#include "Vehicles/VehicleRearSignalRenderRig.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
namespace
{
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"Unreal rear signal render positions: "<<why<<'\n';
        std::exit(EXIT_FAILURE);}
}
}
int main()
{
    using namespace latebrake;
    for(bool original:{false,true})
    {
        const auto lamps=PlanRearSignalVisualOffsets(original);
        Require(lamps.size()==3u,
            "same three native Unreal point-light components retained");
        for(const auto& p:lamps)
            Require(RearSignalNearVisibleBumper(original,p),
                "every rendered signal sits behind its visible bumper");
        Require(lamps[0].Y<0.0 && lamps[1].Y>0.0 &&
                lamps[0].Y==-lamps[1].Y &&
                lamps[0].X==lamps[1].X &&
                lamps[0].Z==lamps[1].Z &&
                lamps[2].Y==0.0 && lamps[2].X<lamps[0].X,
                "red left/right and central white remain symmetric");
        if(!original)
            for(std::size_t i=0;i<lamps.size();++i)
                Require(lamps[i].X==VehicleRearLampOffsets[i].X &&
                    lamps[i].Y==VehicleRearLampOffsets[i].Y &&
                    lamps[i].Z==VehicleRearLampOffsets[i].Z,
                    "fallback rear lamp layout stays exactly unchanged");
        else
            Require(lamps[0].X < VehicleRearLampOffsets[0].X-20.0,
                "long source mesh moves actual light behind extended body");
    }
    auto invalid=PlanRearSignalVisualOffsets(true)[0];
    invalid.X=-190.0;
    Require(!RearSignalNearVisibleBumper(true,invalid),
        "legacy position is inside long source body and never accepted");
    invalid=PlanRearSignalVisualOffsets(true)[0];
    invalid.Y=-180.0;
    Require(!RearSignalNearVisibleBumper(true,invalid),
        "detached point light cannot be treated as bumper illumination");
    for(int i=0;i<200000;++i)
    {
        const auto signals=PlanRearSignalVisualOffsets((i&1)!=0);
        for(auto p:signals)
            Require(std::isfinite(p.X)&&std::isfinite(p.Y)&&
                std::isfinite(p.Z),"rear visual lights remain finite");
    }
    std::cout<<"Original 3D and fallback rear visual signals validated.\n";
    return EXIT_SUCCESS;
}
