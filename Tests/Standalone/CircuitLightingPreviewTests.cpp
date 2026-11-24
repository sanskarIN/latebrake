#include "Track/CircuitLightingPreview.h"
#include <cstdlib>
#include <iostream>
#include <limits>
namespace {
void Check(bool yes,const char* why)
{
    if(!yes){std::cerr<<"Unreal circuit lighting preview: "<<why<<'\n';
        std::exit(EXIT_FAILURE);}
}
void FixedRenderPresets()
{
    using namespace latebrake;
    const auto day=LightingForMood(CircuitLightingMood::Day);
    const auto dusk=LightingForMood(CircuitLightingMood::Dusk);
    Check(ValidCircuitLightingPreset(day)&&
          ValidCircuitLightingPreset(dusk),
          "both shipped lighting profiles are finite and bounded");
    Check(day.SunPitchDeg==-55.0 && day.SunYawDeg==-30.0 &&
          day.SunIntensity==6.0 && day.SkyIntensity==1.0,
          "original DAY scene lighting preserved on first launch");
    Check(dusk.SunPitchDeg>day.SunPitchDeg &&
          dusk.SunIntensity<day.SunIntensity &&
          dusk.SkyIntensity<day.SkyIntensity &&
          dusk.SunIntensity>0.0,
          "DUSK is a lower-sun lit visual, not a dark unlit track");
    Check(day.PreviewOnly && dusk.PreviewOnly,
          "rendering preset cannot claim authoritative race progression");
    Check(LightingForMood(static_cast<CircuitLightingMood>(255u)).SunIntensity
          ==day.SunIntensity,
          "unknown future lighting enum uses safe DAY without corrupt state");
    auto corrupted=day;
    corrupted.SunIntensity=std::numeric_limits<double>::quiet_NaN();
    Check(!ValidCircuitLightingPreset(corrupted),
          "nonfinite actor intensity never admitted");
    corrupted=day;corrupted.SkyColor.B=2.0;
    Check(!ValidCircuitLightingPreset(corrupted),
          "unsafe channel value never used on the scene");
}
void SettingsOnlyAuthority()
{
    using namespace latebrake;
    int allowed=0;
    for(unsigned bits=0;bits<32u;++bits)
    {
        const bool menu=(bits&1u)!=0;
        const bool settings=(bits&2u)!=0;
        const bool focus=(bits&4u)!=0;
        const bool manual=(bits&8u)!=0;
        const bool actors=(bits&16u)!=0;
        const bool want=menu&&settings&&!focus&&!manual&&actors;
        const bool actual=CanChangeCircuitLighting(
            menu,settings,focus,manual,actors);
        Check(actual==want,"all 32 menu/focus/actor states exact");
        allowed+=actual?1:0;
    }
    Check(allowed==1,"only one safe five-input configuration allows toggle");
    unsigned seed=0x44332211u;
    for(int i=0;i<250000;++i)
    {
        seed=seed*1664525u+1013904223u;
        const bool menu=(seed&1u)!=0,settings=(seed&2u)!=0;
        const bool focus=(seed&4u)!=0,manual=(seed&8u)!=0;
        const bool actors=(seed&16u)!=0;
        const bool expected=menu&&settings&&!focus&&!manual&&actors;
        Check(CanChangeCircuitLighting(menu,settings,focus,manual,actors)==
              expected,"250k repeatable state fuzz matches strict authority");
    }
}
}
int main()
{
    FixedRenderPresets();
    SettingsOnlyAuthority();
    std::cout<<"Unreal source circuit lighting mode policy passed.\n";
    return EXIT_SUCCESS;
}
