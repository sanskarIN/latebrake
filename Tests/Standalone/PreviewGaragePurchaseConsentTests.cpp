
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
void PriceAndInvalidState()
{
    using namespace latebrake_preview;
    PreviewGaragePurchaseConsent c;
    for(int car=1;car<PreviewCarCount;++car)
    {
        for(int first : {100,200,300})
        {
            for(int second : {100,200,300})
            {
                c.Reset();
                Check(c.Request(car,1u,1000,first)==PreviewPurchaseChoice::Offer,
                      "initial offer must quote the requested price");
                Check(c.OfferedPrice==first,"stored offer price");
                const auto result=c.Request(car,1u,1000,second);
                if(first==second)
                {
                    Check(result==PreviewPurchaseChoice::Confirm &&
                          c.OfferedPrice==0 && !c.PendingFor(car),
                          "unchanged quote confirms once and clears price");
                }
                else
                {
                    Check(result==PreviewPurchaseChoice::Offer &&
                          c.OfferedPrice==second && c.Remaining==8.0f,
                          "either higher or lower price needs fresh consent");
                    Check(c.Request(car,1u,1000,second)==PreviewPurchaseChoice::Confirm,
                          "fresh quote can be deliberately confirmed");
                }
            }
        }
    }
    c.Request(2,1u,500,280);
    Check(c.Request(2,1u,279,280)==PreviewPurchaseChoice::Rejected &&
          c.OfferedPrice==0 && !c.PendingFor(2),
          "lost affordability invalidates previous offer");
    c.Request(2,1u,500,280);
    Check(c.Request(2,5u,500,280)==PreviewPurchaseChoice::Rejected &&
          c.OfferedPrice==0,"new ownership invalidates offer");
    for(float bad : {0.0f,-1.0f,9.0f,
                    std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::quiet_NaN()})
    {
        c.Request(2,1u,500,280);
        c.Remaining=bad;
        c.Advance(0.0f);
        Check(c.Target==-1 && c.OfferedPrice==0 && c.Remaining==0.0f,
              "invalid internal timer is normalized before use");
    }
    c.Request(2,1u,500,280);
    c.OfferedPrice=0;
    c.Advance(0.0f);
    Check(c.Target==-1 && !c.PendingFor(2),"missing quote fails closed");
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
        const int previousPrice=c.OfferedPrice;
        const int price=100+static_cast<int>((seed>>20)%5u)*100;
        auto decision=c.Request(viewed,mask,
            static_cast<int>((seed>>8)%6000u),price);
        Check((decision!=PreviewPurchaseChoice::Confirm) ||
              (previous==viewed && previousPrice==price && c.Target==-1 &&
               c.Remaining==0.0f && c.OfferedPrice==0),
              "250k random actions cannot confirm unrelated cars");
        Check(c.Target==-1 || (ValidPreviewCar(c.Target) &&
              c.Remaining>0.0f && c.Remaining<=8.0f && c.OfferedPrice==price),
              "pending offer always for known model and bounded interval");
    }
}
}
int main()
{
    TwoStepPurchase();TimeoutsAndScope();PriceAndInvalidState();DeterministicStress();
    std::cout<<"2D two-action purchase consent passed.\n";
    return EXIT_SUCCESS;
}
