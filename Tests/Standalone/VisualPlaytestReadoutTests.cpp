#include "Vehicles/VehicleVisualPlaytestReadout.h"
#include <cstdlib>
#include <iostream>
#include <string_view>
namespace
{
void Require(bool yes, const char* message)
{
    if(!yes){std::cerr<<"Visual playtest diagnostics: "<<message<<'\n';
        std::exit(EXIT_FAILURE);}
}
}
int main()
{
    using namespace latebrake;
    int observed=0;
    for(unsigned bits=0;bits<32u;++bits)
    {
        const bool pawn=(bits&1u)!=0;
        const bool mesh=(bits&2u)!=0;
        const bool color=(bits&4u)!=0;
        const bool lamps=(bits&8u)!=0;
        const bool dusk=(bits&16u)!=0;
        const auto r=ReportVisualPlaytest(
            pawn,mesh,color,lamps,dusk);
        const auto expected=!pawn
            ? VisualPlaytestSurface::NoDriver
            : !mesh ? VisualPlaytestSurface::LegacyCubeFallback
            : color ? VisualPlaytestSurface::SourceGeneratedSurface
            : VisualPlaytestSurface::SourceMaterialIncomplete;
        Require(r.Surface==expected,
            "32 combinations correctly expose real pawn source state");
        Require(r.PaintParameterDiscovered==(pawn&&color) &&
            r.FrontLampsRequested==(pawn&&lamps) &&
            r.DuskPreviewSelected==dusk,
            "never attribute component state to missing player pawn");
        Require(!r.VerifiedByEngineRendering,
            "source observations cannot certify rendered visual quality");
        const std::string_view name=VisualPlaytestSurfaceLabel(r.Surface);
        Require(!name.empty() && name!="UNKNOWN",
            "every observed state has a readable explicit label");
        for(int wheels=-2;wheels<=7;++wheels)
        {
            const auto counted=ReportVisualPlaytest(
                pawn,mesh,color,lamps,dusk,wheels);
            const int expectedCount=
                pawn&&mesh&&color&&wheels>=0&&wheels<=4
                    ? wheels : 0;
            Require(counted.AnimatedWheelPartsCommitted==expectedCount,
                "wheels are counted only on valid committed 3D meshes");
            Require(!counted.VerifiedByEngineRendering,
                "0/4 through 4/4 component status cannot claim GPU proof");
        }
        ++observed;
    }
    const auto unknown=VisualPlaytestSurfaceLabel(
        static_cast<VisualPlaytestSurface>(255));
    Require(std::string_view(unknown)=="UNKNOWN",
        "corrupted enum cannot be misreported as a good mesh");
    unsigned seed=0x2f457982u;
    for(int i=0;i<150000;++i)
    {
        seed=seed*1664525u+1013904223u;
        const auto r=ReportVisualPlaytest((seed&1u)!=0,
            (seed&2u)!=0,(seed&4u)!=0,(seed&8u)!=0,
            (seed&16u)!=0);
        Require(!r.VerifiedByEngineRendering,
            "150k source observations never claim native renderer QA");
        Require(!r.FrontLampsRequested || (seed&1u)!=0,
            "headlamp request cannot claim a missing driver");
    }
    Require(observed==32,"all cases executed");
    std::cout<<"Read-only live Unreal visual QA policy passed.\n";
    return EXIT_SUCCESS;
}
