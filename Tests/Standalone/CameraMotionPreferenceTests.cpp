#include "Vehicles/CameraMotionPreference.h"
#include "Vehicles/CornerLookAheadCamera.h"
#include "Vehicles/SpeedReactiveCameraFov.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Require(bool condition,const char* message)
{
    if(!condition)
    {
        std::cerr<<"CameraMotionPreference: "<<message<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
bool Near(double a,double b,double tolerance=1e-8)
{
    return std::abs(a-b)<tolerance;
}

void ModesAndBlendAuthority()
{
    Require(NextCameraMotionMode(CameraMotionMode::Dynamic)==CameraMotionMode::Reduced &&
        NextCameraMotionMode(CameraMotionMode::Reduced)==CameraMotionMode::Fixed &&
        NextCameraMotionMode(CameraMotionMode::Fixed)==CameraMotionMode::Dynamic,
        "complete deliberate cycle");
    Require(NextCameraMotionMode(static_cast<CameraMotionMode>(255))==CameraMotionMode::Fixed &&
        !ValidCameraMotionMode(CameraMotionMode::Count),"invalid mode falls back safely");
    constexpr std::array<CameraView,4> views{
        CameraView::Chase,CameraView::CloseChase,CameraView::Hood,CameraView::High};
    for(auto mode:{CameraMotionMode::Dynamic,CameraMotionMode::Reduced,CameraMotionMode::Fixed,
        static_cast<CameraMotionMode>(255)})
    for(auto from:views) for(auto to:views)
    for(bool live:{false,true}) for(bool back:{false,true})
    {
        const bool expected=live && !back && from!=CameraView::Hood && to!=CameraView::Hood &&
            (mode==CameraMotionMode::Dynamic || mode==CameraMotionMode::Reduced);
        Require(AllowCameraMotionBlend(mode,from,to,live,back)==expected,
            "all view transitions and authority combinations");
    }
    Require(!AllowCameraMotionBlend(CameraMotionMode::Dynamic,static_cast<CameraView>(255),
        CameraView::Chase,true,false),"unknown camera cannot animate");
}

void EffectsAtEveryPresetAndRate()
{
    for(auto view:{CameraView::Chase,CameraView::CloseChase,CameraView::Hood,CameraView::High})
    {
        const auto preset=GetCameraPreset(view);
        for(auto mode:{CameraMotionMode::Dynamic,CameraMotionMode::Reduced,CameraMotionMode::Fixed})
        {
            const auto effect=EffectsForCameraMotion(mode);
            double referenceFov=0,referenceYaw=0;
            for(int rate:{30,60,120,240})
            {
                SpeedReactiveCameraFov fov;
                CornerLookAheadCamera yaw;
                double a=preset.FieldOfViewDegrees,b=0;
                for(int frame=0;frame<rate*3;++frame)
                {
                    a=fov.Sample(preset.FieldOfViewDegrees,230,1.0/rate,true,false,effect.FovStrength);
                    b=yaw.Sample(3000,1.0,1.0/rate,true,false,view,effect.YawStrength);
                }
                const double maxYaw=(view==CameraView::High?4.0:6.0)*effect.YawStrength;
                Require(a>=preset.FieldOfViewDegrees &&
                    a<=std::min(105.0,preset.FieldOfViewDegrees+6.0*effect.FovStrength),
                    "bounded speed effect");
                Require(b>=0 && b<=maxYaw,"bounded corner effect");
                if(view==CameraView::Hood) Require(b==0,"hood never has corner lead");
                if(mode==CameraMotionMode::Fixed)
                    Require(a==preset.FieldOfViewDegrees && b==0 &&
                        !fov.HasActiveSample() && !yaw.HasActiveSample(),"fixed effects remain inactive");
                else
                    Require(a>preset.FieldOfViewDegrees &&
                        (view==CameraView::Hood || b>0),"enabled effects actually work");
                if(rate==30){referenceFov=a;referenceYaw=b;}
                else Require(Near(a,referenceFov) && Near(b,referenceYaw),"rate-independent response");
            }
        }
    }
}

void PreferenceSwitchAndInvalidStrength()
{
    SpeedReactiveCameraFov fov;
    CornerLookAheadCamera yaw;
    const auto sample=[&](double strength)
    {
        return std::array<double,2>{fov.Sample(90,230,0.05,true,false,strength),
            yaw.Sample(3000,-1,0.05,true,false,CameraView::Chase,strength)};
    };
    std::array<double,2> result{};
    for(int i=0;i<60;++i) result=sample(1.0);
    Require(result[0]>95.0 && result[1]<-5.0,"dynamic effect reaches full range");
    result=sample(0.4);
    Require(result[0]<=92.4 && result[1]>=-2.4-1e-9,"reduced caps old dynamic state immediately");
    result=sample(0.0);
    Require(result[0]==90 && result[1]==0,"fixed restores base on first sample");
    Require(!fov.HasActiveSample() && !yaw.HasActiveSample(),"no stale dynamic state");
    result=sample(0.4);
    Require(result[0]>90 && result[0]<92.4 && result[1]<0 && result[1]>-2.4,
        "reduced restarts from neutral");
    for(double invalid:{-1.0,1.01,std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN()})
    {
        result=sample(invalid);
        Require(result[0]==90 && result[1]==0 && !fov.HasActiveSample() &&
            !yaw.HasActiveSample(),"invalid strength fails closed");
    }
    result=sample(1.0);
    Require(fov.Sample(90,230,0.05,true,true,1)==90 &&
        yaw.Sample(3000,1,0.05,true,true,CameraView::Chase,1)==0,"rear view retains priority");
    Require(fov.Sample(90,230,0.05,false,false,0.4)==90 &&
        yaw.Sample(3000,1,0.05,false,false,CameraView::Chase,0.4)==0,"nonhuman rejected");
    Require(fov.Sample(90,-10,0.05,true,false,0.4)==90 &&
        yaw.Sample(-10,1,0.05,true,false,CameraView::Chase,0.4)==0,"reverse restores neutral");
}
}
int main()
{
    ModesAndBlendAuthority();
    EffectsAtEveryPresetAndRate();
    PreferenceSwitchAndInvalidStrength();
    std::cout<<"Camera motion preference and bounded effect composition passed\n";
}
