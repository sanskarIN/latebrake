#include "Vehicles/OriginalVehicleTintMaterialPolicy.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

namespace
{
void Require(bool okay, const char* reason)
{
    if (!okay) {
        std::cerr << "Original Unreal vehicle tint policy: " << reason << '\n';
        std::exit(EXIT_FAILURE);
    }
}
void TestCandidateCoverage()
{
    using namespace latebrake::vehiclemesh;
    Require(SupportedVehicleTintNames.size() == 5u,
        "five engine/material naming conventions are recognized");
    for (std::size_t i=0;i<SupportedVehicleTintNames.size();++i)
    {
        Require(SupportedVehicleTintNames[i] != nullptr &&
                std::strlen(SupportedVehicleTintNames[i])>0u,
            "every candidate is a real, nonempty vector name");
        Require(SelectVehicleTintCandidate(1u<<i)==static_cast<int>(i),
            "each material naming convention is selectable");
        for (std::size_t j=i+1;j<SupportedVehicleTintNames.size();++j)
            Require(std::strcmp(SupportedVehicleTintNames[i],
                SupportedVehicleTintNames[j])!=0,
                "no aliases can mask or collide with a preferred name");
    }
    for (std::uint32_t mask=0;mask<65536u;++mask)
    {
        int reference=-1;
        for (std::size_t j=0;j<SupportedVehicleTintNames.size();++j)
            if ((mask&(1u<<j))!=0u) { reference=static_cast<int>(j);break; }
        Require(SelectVehicleTintCandidate(mask)==reference,
            "all 65,536 parameter masks choose their first supported tint");
    }
    Require(SelectVehicleTintCandidate(
        std::numeric_limits<std::uint32_t>::max())==0,
        "unsupported future bits cannot change preferred known parameter");
    Require(SelectVehicleTintCandidate(1u<<17)==-1,
        "an unknown parameter must not pretend to be tintable");
}
void TestVisibilityHandoff()
{
    using namespace latebrake::vehiclemesh;
    for (int bits=0;bits<8;++bits)
    {
        const bool sections=(bits&1)!=0;
        const bool materials=(bits&2)!=0;
        const bool tint=(bits&4)!=0;
        Require(CanPresentOriginalVehicleVisual(sections,materials,tint)==
                (bits==7),
            "all eight incomplete original-mesh combinations retain fallback");
    }
}
} // namespace
int main()
{
    TestCandidateCoverage();
    TestVisibilityHandoff();
    std::cout << "Original Unreal vehicle tint compatibility policy passed.\n";
    return EXIT_SUCCESS;
}
