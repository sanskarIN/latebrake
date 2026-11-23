#include "Vehicles/VehicleFrontLampRig.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
namespace
{
void Check(bool value,const char* reason)
{
    if (!value){std::cerr<<"Unreal headlamp rig: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);}
}
void TestOriginalAndFallbackAlignment()
{
    using namespace latebrake;
    for(const bool generated:{false,true})
    {
        for(const bool active:{false,true})
        {
            const auto p=PlanFrontLampRig(generated,active);
            Check(p.OriginalSurface==generated,
                "source mesh visibility selects matching actual lamp geometry");
            Check(p.Mounts[0].Ycm==-p.Mounts[1].Ycm &&
                p.Mounts[0].Xcm==p.Mounts[1].Xcm &&
                p.Mounts[0].Zcm==p.Mounts[1].Zcm,
                "front lights remain symmetric and aligned");
            for(const auto& m:p.Mounts)
                Check(FrontLampPoseIsSafe(m),
                    "each lamp sits at a plausible bumper height");
            Check(p.BeamInnerDegrees>0.0 &&
                p.BeamInnerDegrees<p.BeamOuterDegrees &&
                p.BeamOuterDegrees<=32.0 &&
                p.AttenuationRadiusCm<=9000.0,
                "bounded low-cost forward beam has a valid cone");
            Check(p.OutputIntensity==(active?p.ActiveIntensity:0.0),
                "only qualified race driving enables light emission");
        }
    }
    const auto a=PlanFrontLampRig(false,true);
    const auto b=PlanFrontLampRig(true,true);
    Check(a.Mounts[0].Xcm==194.0,
        "fallback lamp is near stock 190cm front bumper");
    Check(b.Mounts[0].Xcm>a.Mounts[0].Xcm,
        "longer original source mesh moves light ahead of body front");
    Check(std::abs(b.Mounts[0].Zcm-(85.0+
        latebrake::vehiclemesh::OriginalVehicleVisualOffsetZCm))<1.e-9,
        "authored 3D lamp height tracks generator's alignment offset");
}
void TestNoLatchedVisualOutput()
{
    using namespace latebrake;
    for(int i=0;i<200000;++i)
    {
        const bool valid=(i%3)==0;
        const bool mesh=(i%5)==0;
        const auto r=PlanFrontLampRig(mesh,valid);
        Check((r.OutputIntensity>0.0)==valid,
            "200k pause/focus/resume frames cannot retain stale light");
        Check(r.OutputIntensity>=0.0 &&
            std::isfinite(r.OutputIntensity) &&
            r.OutputIntensity<=r.ActiveIntensity,
            "lamp energy cannot become negative, NaN or unbounded");
    }
}
}
int main()
{
    TestOriginalAndFallbackAlignment();
    TestNoLatchedVisualOutput();
    std::cout<<"Real Unreal front spot-light geometry and state passed.\n";
    return EXIT_SUCCESS;
}
