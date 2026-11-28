#include "UI/GarageVehicleCards.h"
#include "UI/GarageShowroomLane.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok)
    {
        std::cerr<<"GarageVehicleCards: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
GarageVehicleCardsSnapshot Snapshot(GarageSnapshot g={},int viewed=0)
{
    GarageAppearanceState looks;
    return BuildGarageVehicleCardsSnapshot(true,true,false,false,viewed,17,g,looks);
}
void RealOwnershipAndCosts()
{
    auto cards=DescribeGarageVehicleCards(Snapshot({600,1,0,0,0}));
    Require(cards[0].Valid && cards[0].Status==GarageCardStatus::Equipped &&
        cards[0].Viewed && cards[0].CanBrowse,"starter is equipped and viewed");
    Require(cards[1].Status==GarageCardStatus::UnlockReady &&
        cards[1].RouteCost==600 && cards[1].RouteShortfall==0,"next affordable unlock");
    Require(cards[2].Status==GarageCardStatus::CreditsRequired &&
        cards[2].RouteCost==1800 && cards[2].RouteShortfall==1200,
        "cumulative cost not merely sticker price");
    cards=DescribeGarageVehicleCards(Snapshot({10000,1,0,0,0},5));
    Require(cards[5].Status==GarageCardStatus::RouteRequired && cards[5].Viewed &&
        cards[5].RouteCars==5 && cards[5].RouteCost==9500 &&
        cards[5].RouteShortfall==0,"affordability never skips earlier unlocks");
    cards=DescribeGarageVehicleCards(Snapshot({0,7,2,3,2},1));
    Require(cards[0].Status==GarageCardStatus::Owned &&
        cards[1].Status==GarageCardStatus::Owned && cards[1].Viewed &&
        cards[2].Status==GarageCardStatus::Equipped && !cards[2].Viewed,
        "viewing does not imply equipped");
}
void PaintAndCorruption()
{
    GarageAppearanceState looks;
    Require(looks.Choose(1,GarageLookLayer::Paint,7),"actual pearl paint");
    Require(looks.Choose(1,GarageLookLayer::Stripe,2),"actual stripe");
    Require(looks.Choose(1,GarageLookLayer::Wheels,1),"actual rims");
    auto snap=BuildGarageVehicleCardsSnapshot(true,true,false,false,1,1,
        {0,3,0,0,0},looks);
    const auto card=DescribeGarageVehicleCards(snap)[1];
    const auto expected=PaintForGarageLook(1,looks.Get(1)).Body;
    Require(card.Valid && card.Customized && SameGarageLook(card.Look,looks.Get(1)) &&
        card.Swatch.R==expected.R && card.Swatch.G==expected.G &&
        card.Swatch.B==expected.B,"swatch sourced from real paint recipe");
    for(int fault=0;fault<8;++fault)
    {
        auto bad=snap;
        switch(fault)
        {
        case 0:bad.Viewed=-1;break;
        case 1:bad.Equipped=5;break;
        case 2:bad.OwnedMask=-1;break;
        case 3:bad.OwnedMask=0;break;
        case 4:bad.Credits=-1;break;
        case 5:bad.Engine=4;break;
        case 6:bad.Grip=-1;break;
        case 7:bad.Looks[0]=255;break;
        }
        for(const auto& row:DescribeGarageVehicleCards(bad))
            Require(!row.Valid,"corrupt receipt cannot advertise cars");
    }
    snap.ReadOnly=true;
    Require(DescribeGarageVehicleCards(snap)[1].CanBrowse,"read-only permits observation");
    snap.FocusSuspended=true;
    Require(!DescribeGarageVehicleCards(snap)[1].CanBrowse,"focus suspension denies browse");
    const auto sanitized=Snapshot({-5,-1,5,99,-1});
    Require(sanitized.Equipped==0 && sanitized.OwnedMask==1 &&
        sanitized.Credits==0 && sanitized.Engine==3 && sanitized.Grip==0,
        "canonical load sanitation");
}
struct View final
{
    double W,H,Safe;
    HudLayout Hud{};
    HudMenuVerticalPlan Vertical{};
    HudMenuReadablePanel Readable{};
    DesktopMenuMouseLayout Dock{};
    GarageShowroomLane Showroom{};
    View(double w=1280,double h=720):W(w),H(h),Safe(0)
    {
        Hud=CalculateHudLayout(w,h);
        Vertical=CalculateHudMenuVerticalPlan(w,h,Hud,false);
        const auto text=CalculateHudTextBounds(w,h,Hud,false);
        Dock=CalculateDesktopMenuMouseLayout(w,h,Hud,false,true);
        Showroom=CalculateGarageShowroomLane(w,h,Hud,false,true,MenuPage::Garage);
        Safe=text.MaxWidthPixels;
        if(Dock.Valid)Safe=std::min(Safe,Dock.Left-text.Left-12*Hud.Scale);
        if(Showroom.Valid)Safe=std::min(Safe,Showroom.TextWidth);
        Readable=CalculateHudMenuReadablePanel(w,h,Hud,Vertical,Safe,true);
    }
    GarageVehicleCardsLayout Plan(int pinned=2,bool garage=true,bool touch=false) const
    {
        return CalculateGarageVehicleCardsLayout(W,H,Hud,Vertical,Readable,
            Safe,Hud.Top+pinned*28*Hud.Scale,pinned,garage,touch);
    }
};
void CheckLayout(const View& v,const GarageVehicleCardsLayout& p)
{
    Require(ValidGarageVehicleCardsLayout(p) && p.Panel.Width<=v.Safe+1e-7,
        "bounded left panel");
    const int remaining=v.Vertical.MaxDetailsRows-2-p.ReservedRows;
    Require(remaining>=GarageVehicleCardsMinimumDetails &&
        p.NextDetailY+(remaining-1)*28*v.Hud.Scale<=
            v.Vertical.OverflowNoticeY-1.25*28*v.Hud.Scale+1e-7,
        "details and warnings protected");
    if(v.Showroom.Valid)
        Require(!SettingsRectsOverlap(p.Panel,v.Showroom.Aperture),"live car remains unobscured");
    if(v.Dock.Valid)
        for(const auto& b:v.Dock.Buttons)
            Require(!SettingsRectsOverlap(p.Panel,b.Bounds),"navigation remains separate");
}
GarageVehicleCardsPresentation Shown(const GarageVehicleCardsLayout& p,
    GarageVehicleCardsSnapshot s)
{
    GarageVehicleCardsPresentation out;
    out.Valid=true;out.Layout=p;out.Snapshot=s;out.Rendered.fill(true);
    return out;
}
GarageVehicleCardsHit Hit(const GarageVehicleCardsPresentation& shown,
    const GarageVehicleCardsSnapshot& s,int i,bool palette=false)
{
    const auto& r=shown.Layout.Cards[static_cast<std::size_t>(i)].Bounds;
    return HitPresentedGarageVehicleCards(shown,s,shown.Layout.Width,shown.Layout.Height,
        r.X+r.Width*0.5,r.Y+r.Height*0.5,palette);
}
void LayoutAndPointerAuthority()
{
    for(const auto& dims:std::array<std::array<double,2>,5>{{
        {900,600},{1280,720},{1920,1080},{2560,1440},{3440,1440}}})
    {
        const View v(dims[0],dims[1]);CheckLayout(v,v.Plan());
    }
    const View v;
    Require(!v.Plan(2,false).Valid && !v.Plan(2,true,true).Valid &&
        !View(640,480).Plan().Valid,"compact and other page fallback");
    auto narrow=v;narrow.Safe=100;
    Require(!narrow.Plan().Valid,"tiny lane fallback");
    auto shortPage=v;shortPage.Vertical.MaxDetailsRows=8;
    Require(!shortPage.Plan().Valid,"preserve detail budget");
    auto s=Snapshot();auto shown=Shown(v.Plan(),s);
    for(int i=0;i<VehicleCount;++i)
    {
        Require(Hit(shown,s,i).Vehicle==i,"locked cars can be previewed");
        Require(Hit(shown,s,i,true).Consumed && Hit(shown,s,i,true).Vehicle<0,
            "palette owns input while open");
        shown.Rendered[static_cast<std::size_t>(i)]=false;
        Require(Hit(shown,s,i).Vehicle<0,"unrendered target blocked");
        shown.Rendered[static_cast<std::size_t>(i)]=true;
    }
    for(int fault=0;fault<10;++fault)
    {
        auto stale=s;
        switch(fault)
        {
        case 0:stale.Open=false;break;
        case 1:stale.Local=false;break;
        case 2:stale.FocusSuspended=true;break;
        case 3:++stale.Revision;break;
        case 4:stale.Viewed=1;break;
        case 5:stale.OwnedMask=3;break;
        case 6:++stale.Credits;break;
        case 7:++stale.Engine;break;
        case 8:stale.Looks[0]=7;break;
        case 9:stale.ReadOnly=true;break;
        }
        const auto hit=Hit(shown,stale,0);
        Require(hit.Consumed && hit.Vehicle<0,"stale receipt cannot browse");
    }
    const auto& r=shown.Layout.Cards[0].Bounds;
    auto h=HitPresentedGarageVehicleCards(shown,s,v.W+1,v.H,r.X+1,r.Y+1,false);
    Require(h.Consumed && h.Vehicle<0,"resize invalidates");
    h=HitPresentedGarageVehicleCards(shown,s,v.W,v.H,0,0,false);
    Require(!h.Consumed,"outside lane unaffected");
    shown.Layout.Cards[1].Bounds=shown.Layout.Cards[0].Bounds;
    Require(!ValidGarageVehicleCardsLayout(shown.Layout) && Hit(shown,s,0).Vehicle<0,
        "overlapping targets fail closed");
}
void RandomCanonicalCatalogues()
{
    std::mt19937 rng(210987);
    int visible=0;
    for(int n=0;n<6000;++n)
    {
        const GarageSnapshot saved{static_cast<int>(rng()%20000),
            static_cast<int>(rng()%140)-5,static_cast<int>(rng()%8),
            static_cast<int>(rng()%6),static_cast<int>(rng()%6)};
        const int viewed=static_cast<int>(rng()%VehicleCount);
        const auto snapshot=Snapshot(saved,viewed);
        const auto data=DescribeGarageVehicleCards(snapshot);
        const auto catalogue=BuildGarageCatalogView(saved);
        const View view(240+rng()%3800,240+rng()%2000);
        const auto p=view.Plan();
        for(int id=0;id<VehicleCount;++id)
        {
            const auto& d=data[static_cast<std::size_t>(id)];
            const auto& c=catalogue.Cars[static_cast<std::size_t>(id)];
            Require(d.Valid && d.Id==id && d.Viewed==(viewed==id) &&
                d.RouteCost==c.RouteCost && d.RouteShortfall==c.RouteCreditsShort,
                "actual catalogue authority across random saves");
            if(d.Status==GarageCardStatus::UnlockReady)
                Require(c.IsNextUnlock && c.RouteBudgetReady,"no skip purchase indication");
        }
        if(!p.Valid)continue;
        ++visible;CheckLayout(view,p);
        const auto shown=Shown(p,snapshot);
        Require(Hit(shown,snapshot,viewed).Vehicle==viewed,"exact visible selection");
    }
    Require(visible>2000,"broad desktop coverage");
}
} // namespace
int main()
{
    RealOwnershipAndCosts();PaintAndCorruption();LayoutAndPointerAuthority();
    RandomCanonicalCatalogues();
    std::cout<<"Garage vehicle cards, appearance and guarded browsing passed\n";
}
