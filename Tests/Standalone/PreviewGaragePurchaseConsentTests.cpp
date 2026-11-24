
#include "PreviewGaragePurchaseConsent.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
void Check(bool good,const char* reason)
{
    if(!good){std::cerr<<"Garage purchase consent: "<<reason<<'\n';
              std::exit(EXIT_FAILURE);}
}
void TwoStepPurchase()
{
    using namespace latebrake_preview;
    PreviewGaragePurchaseConsent c;
    Check(c.Request(1,1u,150,140)==PreviewPurchaseChoice::Offer,
          "first valid action only displays a timed offer");
    Check(c.PendingFor(1)&&!c.PendingFor(2)&&c.Remaining==8.0f,
          "offer is tied to exact viewed locked model");
    Check(c.Request(1,1u,150,140)==PreviewPurchaseChoice::Confirm &&
          !c.PendingFor(1),"second deliberate action confirms once");
    Check(c.Request(1,1u,150,140)==PreviewPurchaseChoice::Offer,
          "replay action cannot double spend after confirmation");
    c.Reset();
    Check(c.Request(1,1u,139,140)==PreviewPurchaseChoice::Rejected,
          "insufficient credits cannot even arm a purchase");
    Check(c.Request(0,1u,500,140)==PreviewPurchaseChoice::Rejected,
          "owned or starter car cannot be bought again");
    Check(c.Request(-1,1u,500,140)==PreviewPurchaseChoice::Rejected &&
          c.Request(12,1u,500,140)==PreviewPurchaseChoice::Rejected,
          "invalid IDs fail closed");
    Check(c.Request(4,1u,500,0)==PreviewPurchaseChoice::Rejected,
          "no zero-priced paid purchase");
}
void TimeoutsAndScope()
{
    using namespace latebrake_preview;
    PreviewGaragePurchaseConsent c;
    Check(c.Request(5,1u,2000,1100)==PreviewPurchaseChoice::Offer,
          "purchase intent granted to affordable car");
    c.Advance(7.99f);
    Check(c.PendingFor(5)&&c.Remaining>0,
          "pending intent stays armed inside eight seconds");
    c.Advance(0.02f);
    Check(!c.PendingFor(5)&&c.Target==-1,
          "exact expiry cancels purchasing");
    Check(c.Request(5,1u,2000,1100)==PreviewPurchaseChoice::Offer,
          "post-timeout first action is a new offer");
    Check(c.Request(6,1u,2000,1450)==PreviewPurchaseChoice::Offer &&
          !c.PendingFor(5)&&c.PendingFor(6),
          "different car cannot inherit earlier consent");
    c.Reset();
    Check(!c.PendingFor(6),"menu/focus reset rejects previous offer");
    c.Request(2,1u,500,280);
    c.Advance(std::numeric_limits<float>::quiet_NaN());
    Check(!c.PendingFor(2),"non-finite dt cannot leave a stale offer");
    c.Request(2,1u,500,280);
    c.Advance(-1.0f);
    Check(!c.PendingFor(2),"invalid negative dt fails closed");
}
void DeterministicStress()
{
    using namespace latebrake_preview;
    PreviewGaragePurchaseConsent c;
    unsigned seed=0x191a591u,mask=1u;
    for(int n=0;n<250000;++n)
    {
        seed=1664525u*seed+1013904223u;
        int viewed=static_cast<int>((seed>>5)%15u)-2;
        if((seed&63u)==0u)c.Reset();
        if((seed&7u)==0u)
            c.Advance(static_cast<float>((seed>>17)%50u)*0.1f);
        if((seed&31u)==0u && ValidPreviewCar(viewed))
            mask|=1u<<static_cast<unsigned int>(viewed);
        const int previous=c.Target;
        auto decision=c.Request(viewed,mask,
            static_cast<int>((seed>>8)%6000u),500);
        Check((decision!=PreviewPurchaseChoice::Confirm) ||
              (previous==viewed && c.Target==-1 && c.Remaining==0.0f),
              "250k random actions cannot confirm unrelated cars");
        Check(c.Target==-1 || (ValidPreviewCar(c.Target) &&
              c.Remaining>0.0f && c.Remaining<=8.0f),
              "pending offer always for known model and bounded interval");
    }
}
}
int main()
{
    TwoStepPurchase();TimeoutsAndScope();DeterministicStress();
    std::cout<<"2D two-action purchase consent passed.\n";
    return EXIT_SUCCESS;
}
