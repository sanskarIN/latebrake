#include "UI/GaragePurchaseMouseLayout.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
using namespace latebrake;
void Check(bool pass, const char* message)
{
    if (!pass)
    {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void OfferStates()
{
    for (int mask=0;mask<=KnownVehicleMask;++mask)
        for (int credits : {0, 149, 150, 199, 200, 599, 600, 10000})
            for (int engine=0;engine<=3;++engine)
                for (int grip=0;grip<=3;++grip)
                {
                    const GarageSnapshot snapshot{credits,mask,0,engine,grip};
                    GarageState garage;
                    garage.Load(snapshot);
                    const auto offers=BuildGaragePurchasePanel(
                        snapshot,GarageEquipUiState::Ready,{});
                    const int next=garage.NextLockedVehicle();
                    Check(offers[0].Subject==next,"sequential unlock subject");
                    Check(offers[0].Price==(next<0?0:GarageState::Definition(next).UnlockCost),
                        "next unlock price");
                    Check(offers[1].Price==garage.NextUpgradeCost(UpgradeKind::Engine),
                        "shared engine price");
                    Check(offers[2].Price==garage.NextUpgradeCost(UpgradeKind::Grip),
                        "shared grip price");
                    for (const auto& offer:offers)
                    {
                        const auto expected=offer.Price==0?GarageOfferStatus::Complete:
                            credits<offer.Price?GarageOfferStatus::NeedCredits:GarageOfferStatus::Preview;
                        Check(offer.Status==expected,"offer state follows actual garage");
                        Check(offer.Enabled()==(expected==GarageOfferStatus::Preview),
                            "only affordable unfinished offers enabled");
                        Check(offer.Shortfall==std::max(0,offer.Price-credits),"exact shortfall");
                    }
                    for (const auto access:{GarageEquipUiState::NotInGarage,
                        GarageEquipUiState::Suspended,GarageEquipUiState::ReadOnly,
                        GarageEquipUiState::CareerLocked})
                    {
                        const auto denied=BuildGaragePurchasePanel(snapshot,access,{});
                        for (const auto& offer:denied)
                            Check(!offer.Enabled() && offer.Status==GarageOfferStatus::Blocked,
                                "mutation context always overrides affordability");
                    }
                }
}

void ConsentAndExpiry()
{
    GarageSnapshot snapshot{10000,1,0,0,0};
    GaragePurchaseConfirmation consent;
    const auto baseline=BuildGaragePurchasePanel(snapshot,GarageEquipUiState::Ready,{});
    for (std::size_t i=0;i<baseline.size();++i)
    {
        const auto& offer=baseline[i];
        consent.Reset();
        Check(!consent.ConfirmOrStage(offer.Kind,snapshot,offer.Subject,offer.Price,10),
            "first intent stages only");
        const auto pending=consent.Preview(snapshot,12);
        const auto active=BuildGaragePurchasePanel(snapshot,GarageEquipUiState::Ready,pending);
        for (std::size_t j=0;j<active.size();++j)
            Check(active[j].Status==(i==j?GarageOfferStatus::Confirm:GarageOfferStatus::Preview),
                "only exact matching offer prompts confirmation");
        Check(active[i].SecondsRemaining==6,"real countdown");
        const auto expired=BuildGaragePurchasePanel(snapshot,GarageEquipUiState::Ready,
            consent.Preview(snapshot,18.01));
        Check(expired[i].Status==GarageOfferStatus::Preview,"expired click must stage again");
        const auto paused=BuildGaragePurchasePanel(snapshot,GarageEquipUiState::Suspended,pending);
        Check(!paused[i].Enabled(),"suspension disables pending offer");
        auto changed=snapshot;
        --changed.Credits;
        const auto stale=BuildGaragePurchasePanel(changed,GarageEquipUiState::Ready,
            consent.Preview(changed,12));
        Check(stale[i].Status==GarageOfferStatus::Preview,"stale snapshot cannot confirm");
        auto malformed=pending;
        malformed.SecondsRemaining=std::numeric_limits<double>::quiet_NaN();
        Check(BuildGaragePurchasePanel(snapshot,GarageEquipUiState::Ready,malformed)[i].Status
            ==GarageOfferStatus::Preview,"nonfinite countdown never confirms");
        malformed=pending;
        ++malformed.Price;
        Check(BuildGaragePurchasePanel(snapshot,GarageEquipUiState::Ready,malformed)[i].Status
            ==GarageOfferStatus::Preview,"mismatched price never confirms");
        Check(consent.ConfirmOrStage(offer.Kind,snapshot,offer.Subject,offer.Price,12),
            "explicit second matching intent confirms");
        Check(!consent.HasPending(),"confirmation consumed exactly once");
    }
}

void PresentationAdmission()
{
    const GarageSnapshot snapshot{10000,1,0,0,0};
    GaragePurchaseConfirmation consent;
    const auto offer=BuildGaragePurchasePanel(snapshot,GarageEquipUiState::Ready,{})[0];
    const DesktopMenuMouseButton button{GarageOfferAction(0),{100,100,80,44}};
    GaragePresentedOffer shown{true,button.Bounds,offer};
    Check(CanClickPresentedGarageOffer(shown,button,offer),"visible preview admits first click");
    Check(!consent.ConfirmOrStage(offer.Kind,snapshot,offer.Subject,offer.Price,10),
        "first click stages approval");
    const auto pending=BuildGaragePurchasePanel(snapshot,GarageEquipUiState::Ready,
        consent.Preview(snapshot,10.01))[0];
    Check(!CanClickPresentedGarageOffer(shown,button,pending),
        "second click before rendered confirmation rejected");
    shown.Offer=pending;
    const auto later=BuildGaragePurchasePanel(snapshot,GarageEquipUiState::Ready,
        consent.Preview(snapshot,12))[0];
    Check(CanClickPresentedGarageOffer(shown,button,later),"visible pending offer admits deliberate second click");
    Check(!CanClickPresentedGarageOffer(shown,button,offer),"expiry cannot restage on stale confirm label");
    auto moved=button;
    moved.Bounds.X+=1;
    Check(!CanClickPresentedGarageOffer(shown,moved,later),"resized or moved target rejects stale clicks");
    auto changed=later;
    ++changed.Subject;
    Check(!CanClickPresentedGarageOffer(shown,button,changed),"changed vehicle rejects stale clicks");
    changed=later;
    ++changed.Price;
    Check(!CanClickPresentedGarageOffer(shown,button,changed),"changed cost rejects stale clicks");
    changed=later;
    changed.SecondsRemaining=8;
    Check(!CanClickPresentedGarageOffer(shown,button,changed),"new countdown cannot inherit old presentation");
    shown.Visible=false;
    Check(!CanClickPresentedGarageOffer(shown,button,later),"unmeasured or absent caption rejects clicks");
}

void PointerGeometry()
{
    for (const auto& screen: {std::array<int,2>{900,600},{1280,720},
        {1920,1080},{2560,1080},{3840,2160}})
    {
        const double w=screen[0], h=screen[1];
        const auto hud=CalculateHudLayout(w,h);
        const auto lane=CalculateGarageShowroomLane(w,h,hud,false,true,MenuPage::Garage);
        const auto strip=CalculateGaragePurchaseMouseLayout(lane,w,h,hud);
        Check(strip.Valid,"desktop offer strip is available");
        for (std::size_t i=0;i<strip.Buttons.size();++i)
        {
            const auto& b=strip.Buttons[i];
            const auto& r=b.Bounds;
            Check(r.Height>=44 && r.Width>=60 && r.Y+r.Height<=lane.Aperture.Y,
                "button minimum size and aperture clearance");
            Check(HitTestGaragePurchaseMouse(strip,r.X+r.Width/2,r.Y+r.Height/2)
                ==GarageOfferAction(i),"button center routes exactly");
            auto corrupted=strip;
            corrupted.Buttons[i].Action=DesktopMenuMouseAction::Activate;
            Check(HitTestGaragePurchaseMouse(corrupted,r.X+r.Width/2,r.Y+r.Height/2)
                ==DesktopMenuMouseAction::None,"wrong action fails closed");
        }
        auto overlapping=strip;
        overlapping.Buttons[1].Bounds=strip.Buttons[0].Bounds;
        const auto& r=strip.Buttons[0].Bounds;
        Check(HitTestGaragePurchaseMouse(overlapping,r.X+1,r.Y+1)
            ==DesktopMenuMouseAction::None,"overlap fails closed");
        Check(!CalculateGaragePurchaseMouseLayout({},w,h,hud).Valid,"no lane, no hidden hit");
    }
}
}
int main()
{
    OfferStates();
    ConsentAndExpiry();
    PresentationAdmission();
    PointerGeometry();
    std::cout << "Garage purchase panel tests passed.\n";
    return EXIT_SUCCESS;
}
