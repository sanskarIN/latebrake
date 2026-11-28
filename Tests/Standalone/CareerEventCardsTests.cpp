#include "UI/CareerEventCards.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

namespace
{
using namespace latebrake;

void Require(bool ok,const char* why)
{
    if(!ok)
    {
        std::cerr<<"CareerEventCards: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
CareerEventCardsSnapshot Snapshot(MenuPage page=MenuPage::Career,
    CareerSnapshot progress={},GarageSnapshot garage={},int selected=-1)
{
    return BuildCareerEventCardsSnapshot(true,page,true,false,false,selected,17,
        progress,garage);
}

void InitialAndUnlockedReadiness()
{
    auto snapshot=Snapshot();
    Require(snapshot.CanBrowse() && snapshot.Selected==0,"first available career target");
    for(int id=0;id<CareerEventCount;++id)
    {
        const auto card=DescribeCareerEventCard(snapshot,id);
        Require(card.Valid && card.EventId==id &&
            card.Name==GetCareerEvent(id)->Name && card.Laps==GetCareerEvent(id)->Laps,
            "all authored events described by identity");
        const bool available=id==0 || id==6;
        Require(card.Selectable==available &&
            card.Readiness==(available?CareerCardReadiness::Ready:
                CareerCardReadiness::LockedPrerequisite),"prerequisites respected");
        Require(card.Selected==(id==0) && card.Medal==CareerMedal::None &&
            card.FirstCredits==GetCareerEvent(id)->FirstCredits,"initial medals and rewards");
    }
    CareerSnapshot progress;
    progress.Medals[0]=1;progress.Medals[1]=2;progress.Medals[2]=3;
    snapshot=Snapshot(MenuPage::Career,progress,{},10);
    Require(snapshot.Selected==10,"eligible event remains selected for inspection");
    Require(DescribeCareerEventCard(snapshot,0).Readiness==CareerCardReadiness::Replay &&
        DescribeCareerEventCard(snapshot,1).Medal==CareerMedal::Silver &&
        DescribeCareerEventCard(snapshot,2).Readiness==CareerCardReadiness::Mastered &&
        DescribeCareerEventCard(snapshot,2).FirstCredits==0,"real saved medals");
    const auto kestrel=DescribeCareerEventCard(snapshot,10);
    Require(kestrel.Valid && kestrel.Selectable && kestrel.Selected &&
        kestrel.Readiness==CareerCardReadiness::NeedsCar && kestrel.RequiredVehicle==1,
        "missing car can be inspected, not represented as ready");
    auto owned=Snapshot(MenuPage::Career,progress,{0,3,0,0,0},10);
    Require(DescribeCareerEventCard(owned,10).Readiness==CareerCardReadiness::Ready,
        "real ownership clears the required-car advisory");
    auto readonly=snapshot;readonly.ReadOnly=true;
    Require(DescribeCareerEventCard(readonly,10).Selectable,
        "read-only profile may inspect a pending choice");
    readonly.FocusSuspended=true;
    Require(!DescribeCareerEventCard(readonly,10).Selectable,"focus suspension blocks clicks");
}

void PageFiltersAndSanitation()
{
    for(auto page:{MenuPage::Career,MenuPage::Championships,MenuPage::Challenges})
    {
        const auto snapshot=Snapshot(page);
        int count=0;
        for(int id=0;id<CareerEventCount;++id)
        {
            const auto card=DescribeCareerEventCard(snapshot,id);
            Require(card.Valid==IsEventInCareerMenu(page,id),"page membership exact");
            if(card.Valid)++count;
        }
        Require(count==(page==MenuPage::Career?12:6),"complete relevant page");
    }
    auto snapshot=Snapshot(MenuPage::Challenges,{}, {},0);
    Require(snapshot.Selected==6,"stale cursor resolves to first visible eligible event");
    snapshot=Snapshot(MenuPage::Championships,{}, {},6);
    Require(snapshot.Selected==0,"championship fallback stays within page");
    CareerSnapshot bad;bad.Medals[2]=3;bad.Medals[0]=255;
    snapshot=Snapshot(MenuPage::Career,bad,{0,-1,5,99,-3});
    Require(snapshot.Medals[0]==0 && snapshot.Medals[2]==0 &&
        snapshot.OwnedVehicleMask==1,"save normalization before card display");
    auto corrupt=snapshot;corrupt.Medals[2]=3;
    Require(!DescribeCareerEventCard(corrupt,0).Valid,"mixed medal sequence rejected");
    corrupt=snapshot;corrupt.Medals[0]=4;
    Require(!DescribeCareerEventCard(corrupt,0).Valid,"unknown medal rejected");
    corrupt=snapshot;corrupt.OwnedVehicleMask=0;
    Require(!DescribeCareerEventCard(corrupt,0).Valid,"missing starter ownership rejected");
    corrupt=snapshot;corrupt.OwnedVehicleMask=128|1;
    Require(!DescribeCareerEventCard(corrupt,0).Valid,"unknown car bit rejected");
    Require(!DescribeCareerEventCard(snapshot,-1).Valid &&
        !DescribeCareerEventCard(snapshot,CareerEventCount).Valid,"invalid ids");
}

struct View final
{
    double Width=0,Height=0;
    HudLayout Hud{};
    HudMenuVerticalPlan Vertical{};
    HudMenuReadablePanel Readable{};
    DesktopMenuMouseLayout Dock{};
    double SafeWidth=0;
    View(double w=1280,double h=720,bool touch=false):Width(w),Height(h)
    {
        Hud=CalculateHudLayout(w,h);
        Vertical=CalculateHudMenuVerticalPlan(w,h,Hud,touch);
        const auto text=CalculateHudTextBounds(w,h,Hud,touch);
        Dock=CalculateDesktopMenuMouseLayout(w,h,Hud,touch,true);
        SafeWidth=text.MaxWidthPixels;
        if(Dock.Valid)SafeWidth=std::min(SafeWidth,Dock.Left-text.Left-12*Hud.Scale);
        Readable=CalculateHudMenuReadablePanel(w,h,Hud,Vertical,SafeWidth,true);
    }
    CareerEventCardsLayout Plan(MenuPage page=MenuPage::Career,int pinned=2,
        bool touch=false) const
    {
        return CalculateCareerEventCardsLayout(Width,Height,Hud,Vertical,Readable,
            SafeWidth,Hud.Top+pinned*28*Hud.Scale,pinned,touch,page);
    }
};

void CheckLayout(const View& v,const CareerEventCardsLayout& p,int pinned)
{
    Require(ValidCareerEventCardsLayout(p),"valid complete card grid");
    Require(p.Count==(p.Page==MenuPage::Career?12:6) &&
        p.Panel.Width<=v.SafeWidth+1e-7 && p.Panel.X==v.Hud.Left,
        "all page cards fit left lane");
    const int remaining=v.Vertical.MaxDetailsRows-pinned-p.ReservedRows;
    Require(remaining>=CareerCardsMinimumDetailRows &&
        p.NextDetailY+(remaining-1)*28*v.Hud.Scale<=
            v.Vertical.OverflowNoticeY-1.25*28*v.Hud.Scale+1e-7,
        "details and status remain accessible");
    if(v.Dock.Valid)
        for(const auto& b:v.Dock.Buttons)
            Require(!SettingsRectsOverlap(p.Panel,b.Bounds),"existing navigation dock separated");
    int last=-1;
    for(int i=0;i<p.Count;++i)
    {
        const auto& card=p.Cards[static_cast<std::size_t>(i)];
        Require(card.EventId>last && IsEventInCareerMenu(p.Page,card.EventId) &&
            card.Bounds.Height>=44 && card.Bounds.Width>=100,"stable event ids and target sizes");
        last=card.EventId;
    }
}

CareerEventCardsPresentation Presented(const CareerEventCardsLayout& p,
    CareerEventCardsSnapshot s)
{
    CareerEventCardsPresentation out;
    out.Valid=true;out.Layout=p;out.Snapshot=s;out.Rendered.fill(true);
    return out;
}

CareerEventCardsHit Hit(const CareerEventCardsPresentation& shown,
    const CareerEventCardsSnapshot& current,int index)
{
    const auto& b=shown.Layout.Cards[static_cast<std::size_t>(index)].Bounds;
    return HitPresentedCareerEventCards(shown,current,shown.Layout.Width,
        shown.Layout.Height,b.X+b.Width/2,b.Y+b.Height/2);
}

void ResponsiveAndStaleInput()
{
    for(const auto& size:std::array<std::array<double,2>,6>{{
        {900,600},{1120,600},{1280,720},{1920,1080},{2560,1440},{3440,1440}}})
    {
        const View v(size[0],size[1]);
        for(auto page:{MenuPage::Career,MenuPage::Championships,MenuPage::Challenges})
            CheckLayout(v,v.Plan(page),2);
    }
    const View v;
    Require(!v.Plan(MenuPage::Garage).Valid && !v.Plan(MenuPage::Career,2,true).Valid &&
        !View(640,480).Plan().Valid,"compact and unrelated page fallback");
    auto bad=v;bad.SafeWidth=100;
    Require(!bad.Plan().Valid,"insufficient card width");
    bad=v;bad.Vertical.MaxDetailsRows=5;
    Require(!bad.Plan().Valid,"insufficient remaining details");
    const auto p=v.Plan();
    auto s=Snapshot();
    auto shown=Presented(p,s);
    for(int i=0;i<p.Count;++i)
    {
        const auto desc=DescribeCareerEventCard(s,p.Cards[static_cast<std::size_t>(i)].EventId);
        const auto hit=Hit(shown,s,i);
        Require(hit.Consumed && hit.EventId==(desc.Selectable?desc.EventId:-1),
            "locked visible card never selects or falls through");
    }
    for(int fault=0;fault<7;++fault)
    {
        auto stale=s;
        switch(fault)
        {
        case 0:stale.MenuOpen=false;break;
        case 1:stale.Page=MenuPage::Championships;break;
        case 2:stale.LocalDriver=false;break;
        case 3:stale.FocusSuspended=true;break;
        case 4:++stale.Revision;break;
        case 5:stale.Medals[0]=1;break;
        case 6:stale.OwnedVehicleMask=3;break;
        }
        const auto hit=Hit(shown,stale,0);
        Require(hit.Consumed && hit.EventId<0,"stale state revokes card");
    }
    shown.Rendered[0]=false;
    Require(Hit(shown,s,0).EventId<0,"unrendered card not actionable");
    shown.Rendered[0]=true;
    const auto& b=p.Cards[0].Bounds;
    auto hit=HitPresentedCareerEventCards(shown,s,p.Width+1,p.Height,b.X+1,b.Y+1);
    Require(hit.Consumed && hit.EventId<0,"resized target revoked");
    hit=HitPresentedCareerEventCards(shown,s,p.Width,p.Height,0,0);
    Require(!hit.Consumed,"outside grid keeps native menu navigation");
    auto malformed=shown;
    malformed.Layout.Cards[0].EventId=1;
    Require(!ValidCareerEventCardsLayout(malformed.Layout) &&
        Hit(malformed,s,0).EventId<0,"wrong card identity refused");
    malformed=shown;malformed.Layout.Cards[1].Bounds=malformed.Layout.Cards[0].Bounds;
    Require(!ValidCareerEventCardsLayout(malformed.Layout),"overlapping targets refused");
}

void RandomSanitizedCareerAndViewports()
{
    std::mt19937 rng(517726);
    int visible=0;
    for(int n=0;n<6000;++n)
    {
        CareerSnapshot saved;
        for(auto& medal:saved.Medals)medal=static_cast<std::uint8_t>(rng()%5);
        const int selected=static_cast<int>(rng()%CareerEventCount);
        const int mask=static_cast<int>(rng()%128);
        const MenuPage page=n%3==0?MenuPage::Career:n%3==1?
            MenuPage::Championships:MenuPage::Challenges;
        const auto snapshot=Snapshot(page,saved,{0,mask,0,0,0},selected);
        CareerProgress career;career.Load(saved);
        GarageState garage;garage.Load({0,mask,0,0,0});
        const View v(240+rng()%3800,240+rng()%2000);
        const auto p=v.Plan(page);
        if(!p.Valid)continue;
        ++visible;CheckLayout(v,p,2);
        const auto shown=Presented(p,snapshot);
        for(int i=0;i<p.Count;++i)
        {
            const int id=p.Cards[static_cast<std::size_t>(i)].EventId;
            const auto desc=DescribeCareerEventCard(snapshot,id);
            const auto* definition=GetCareerEvent(id);
            Require(desc.Valid && desc.Medal==career.Medal(id) &&
                desc.Selectable==(career.Status(id)!=CareerStatus::Locked),
                "canonical progression drives selection");
            if(desc.Readiness==CareerCardReadiness::NeedsCar)
                Require(!garage.Owns(definition->RequiredVehicle),"missing car is real");
            const auto hit=Hit(shown,snapshot,i);
            Require(hit.EventId==(desc.Selectable?id:-1),"exact visible event only");
        }
    }
    Require(visible>2000,"broad desktop catalogue coverage");
}
} // namespace

int main()
{
    InitialAndUnlockedReadiness();PageFiltersAndSanitation();
    ResponsiveAndStaleInput();RandomSanitizedCareerAndViewports();
    std::cout<<"Career event card readiness, layout and selection tests passed\n";
}
