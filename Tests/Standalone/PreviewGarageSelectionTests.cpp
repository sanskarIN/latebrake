#include "PreviewGarageSelection.h"
#include <cstdlib>
#include <iostream>

namespace
{
void Check(bool ok,const char* message)
{
    if(!ok){std::cerr<<"Garage preview/equip failure: "<<message<<'\n';
            std::exit(EXIT_FAILURE);}
}
void BasicOwnershipAndPersistence()
{
    using namespace latebrake_preview;
    PreviewGarageSelection s;
    s.Restore(1u,0);
    Check(s.Equipped==0 && s.Viewed==0 && s.Invariant(1u),
          "legacy save chooses the owned starter");
    Check(s.Browse(11) && s.Equipped==0 && s.Viewed==11,
          "locked browsing never changes race car");
    Check(!s.EquipViewed(1u) && s.Equipped==0,
          "locked preview never equips");
    s.Restore((1u<<5)|1u,5);
    Check(s.Equipped==5 && s.Viewed==5,
          "valid V3/V4 owned selection survives");
    Check(s.Browse(1) && !s.EquipViewed((1u<<5)|1u) &&
          s.Equipped==5, "locked preview cannot steal equipped car");
    Check(s.Browse(0) && s.EquipViewed((1u<<5)|1u) &&
          s.Equipped==0,"explicit owned equip changes vehicle");
    s.Restore(1u,5);
    Check(s.Equipped==0 && s.Viewed==0,
          "legacy stored locked preview is repaired from owned mask");
    s.Restore(1u<<8,11);
    Check(s.Equipped==8 && s.Viewed==8,
          "append-only owned car fallback");
    s.Restore(0u,11);
    Check(s.Equipped==0,"corrupt no-owned mask safely defaults to starter");
}
void InputNavigationAndFailClosed()
{
    using namespace latebrake_preview;
    PreviewGarageSelection s; s.Restore(1u,0);
    Check(!s.Browse(-1) && !s.Browse(12) && !s.Browse(100000) &&
          s.Equipped==0 && s.Viewed==0,"invalid pointer IDs ignored");
    Check(!s.BrowseDelta(2) && !s.BrowseDelta(-2),
          "invalid navigation deltas rejected");
    for(int i=0;i<PreviewCarCount;++i)
        Check(s.BrowseDelta(1) && s.Viewed==(i+1)%PreviewCarCount,
              "next wraps through twelve showroom cars");
    Check(s.BrowseDelta(-1) && s.Viewed==11 && s.Equipped==0,
          "previous wraps without equipping");
    Check(!s.EquipViewed(1u),"locked last car cannot equip");
    s.Restore((1u<<11)|1u,0);
    Check(s.Browse(11) && s.EquipViewed((1u<<11)|1u) &&
          s.Equipped==11,"last appended model can be owned and equipped");
}
void DeterministicStress()
{
    using namespace latebrake_preview;
    PreviewGarageSelection s; s.Restore(1u,0);
    unsigned int seed=0xe71c91b1u,owned=1u;
    for(int i=0;i<300000;++i)
    {
        seed=1664525u*seed+1013904223u;
        const int id=static_cast<int>((seed>>9)%17u)-2;
        if((seed&7u)==0u && ValidPreviewCar(id))
            owned|=1u<<static_cast<unsigned int>(id);
        const int old=s.Equipped;
        if((seed&1u)!=0u) s.Browse(id);
        else s.BrowseDelta((seed&2u)!=0u?-1:1);
        Check(s.Equipped==old,"300k browse actions do not equip");
        if((seed&16u)!=0u) {
            const int before=s.Equipped;
            const bool eligible=s.EquipViewed(owned);
            Check(eligible==OwnsPreviewCar(owned,s.Viewed),
                  "equip succeeds iff viewed car owned");
            Check(eligible?s.Equipped==s.Viewed:s.Equipped==before,
                  "failed equip does not change current car");
        }
        Check(s.Invariant(owned),"live garage selection always valid");
    }
}
}
int main()
{
    BasicOwnershipAndPersistence();
    InputNavigationAndFailClosed();
    DeterministicStress();
    std::cout<<"2D preview garage browse/equip separation passed.\n";
    return EXIT_SUCCESS;
}
