#include "UI/MainMenuDashboard.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>

namespace
{
using namespace latebrake;

void Require(bool ok,const char* reason)
{
    if(!ok)
    {
        std::cerr<<"MainMenuDashboard: "<<reason<<'\n';
        std::exit(EXIT_FAILURE);
    }
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
    MainMenuDashboardLayout Plan(int pinned=2,bool touch=false,
        MenuPage page=MenuPage::Main) const
    {
        return CalculateMainMenuDashboardLayout(Width,Height,Hud,Vertical,Readable,
            SafeWidth,Hud.Top+pinned*28*Hud.Scale,pinned,touch,page);
    }
};

MainMenuDashboardSnapshot Snapshot()
{
    return {true,MenuPage::Main,true,false,17};
}

MainMenuDashboardPresentation Presented(const MainMenuDashboardLayout& layout)
{
    MainMenuDashboardPresentation shown;
    shown.Valid=true;
    shown.Layout=layout;
    shown.Snapshot=Snapshot();
    shown.Rendered.fill(true);
    return shown;
}

MainMenuDashboardHit Hit(const MainMenuDashboardPresentation& shown,
    const MainMenuDashboardSnapshot& snapshot,int index)
{
    const auto& r=shown.Layout.Cards[static_cast<std::size_t>(index)].Bounds;
    return HitPresentedMainMenuDashboard(shown,snapshot,shown.Layout.Width,
        shown.Layout.Height,r.X+r.Width/2,r.Y+r.Height/2);
}

void ExplicitDestinations()
{
    std::array<bool,MenuPageNames.size()> seen{};
    for(const auto& destination:MainMenuDestinations)
    {
        const auto i=static_cast<std::size_t>(destination.Page);
        Require(i>1 && i<seen.size() && !seen[i],"distinct actual menu destination");
        seen[i]=true;
        Require(ValidMainMenuDestination(destination.Page) &&
            !std::string_view(destination.Title).empty() &&
            !std::string_view(destination.ShortTitle).empty() &&
            !std::string_view(destination.Description).empty(),"meaningful visible captions");
    }
    for(std::size_t i=2;i<seen.size();++i)
        Require(seen[i],"every existing page directly reachable");
    Require(!ValidMainMenuDestination(MenuPage::Closed) &&
        !ValidMainMenuDestination(MenuPage::Main) &&
        !ValidMainMenuDestination(static_cast<MenuPage>(255)),"not an activation or close action");
}

void CheckLayout(const View& view,const MainMenuDashboardLayout& layout,int pinned)
{
    Require(ValidMainMenuDashboardLayout(layout),"valid responsive dashboard");
    const auto& p=layout.Panel;
    Require(p.X==view.Hud.Left && p.Width<=view.SafeWidth+1e-7 &&
        std::abs(p.Y-view.Hud.Top-pinned*28*view.Hud.Scale)<1e-7,"protected text lane");
    const int remaining=view.Vertical.MaxDetailsRows-pinned-layout.ReservedRows;
    Require(remaining>=MainMenuMinimumRemainingRows &&
        layout.NextDetailY+(remaining-1)*28*view.Hud.Scale<=
            view.Vertical.OverflowNoticeY-1.25*28*view.Hud.Scale+1e-7,
        "save and overflow messages retain space");
    Require(ContainsSettingsRect({view.Readable.X,view.Readable.Y,
        view.Readable.Width,view.Readable.Height},p),"inside existing menu background");
    if(view.Dock.Valid)
        for(const auto& b:view.Dock.Buttons)
            Require(!SettingsRectsOverlap(p,b.Bounds),"never covers existing controls");
    const auto shown=Presented(layout);
    for(int i=0;i<MainMenuDestinationCount;++i)
    {
        const auto hit=Hit(shown,shown.Snapshot,i);
        Require(hit.Consumed && hit.Destination==
            MainMenuDestinations[static_cast<std::size_t>(i)].Page,"visible center dispatch");
        const auto& card=layout.Cards[static_cast<std::size_t>(i)];
        Require(card.Bounds.Height>=44 && card.Bounds.Width>=100,"minimum pointer size");
    }
}

void ResponsiveAndFallback()
{
    for(const auto& size:std::array<std::array<double,2>,7>{{
        {900,600},{1120,600},{1280,720},{1366,768},{1920,1080},
        {2560,1440},{3440,1440}}})
    {
        const View v(size[0],size[1]);
        CheckLayout(v,v.Plan(),2);
    }
    const View v;
    Require(!v.Plan(2,true).Valid && !v.Plan(2,false,MenuPage::Garage).Valid &&
        !View(640,480).Plan().Valid && !View(1280,480).Plan().Valid,
        "existing navigation for mobile, other pages and small windows");
    auto narrow=v;narrow.SafeWidth=190;
    Require(!narrow.Plan().Valid,"tiny left lane cannot expose invisible targets");
    narrow=v;narrow.SafeWidth=260;
    const auto two=narrow.Plan();
    Require(two.Valid && two.Columns==2,"two-column fallback");
    auto bad=v;bad.Vertical.MaxDetailsRows=5;
    Require(!bad.Plan().Valid,"preserve detail rows");
    bad=v;bad.Readable.Valid=false;
    Require(!bad.Plan().Valid,"no detached dashboard");
    Require(!CalculateMainMenuDashboardLayout(v.Width,v.Height,v.Hud,v.Vertical,
        v.Readable,v.SafeWidth,v.Hud.Top,2,false,MenuPage::Main).Valid,
        "stale pinned baseline refused");
}

void InputAdmissionAndStaleReceipts()
{
    const auto layout=View().Plan();
    auto shown=Presented(layout);
    const auto current=Snapshot();
    Require(current.AllowsNavigation(),"local visible main page");
    for(int i=0;i<MainMenuDestinationCount;++i)
    {
        shown.Rendered[static_cast<std::size_t>(i)]=false;
        auto hit=Hit(shown,current,i);
        Require(hit.Consumed && hit.Destination==MenuPage::Closed,"unrendered card consumes only");
        shown.Rendered[static_cast<std::size_t>(i)]=true;
    }
    for(int fault=0;fault<5;++fault)
    {
        auto stale=current;
        switch(fault)
        {
        case 0:stale.MenuOpen=false;break;
        case 1:stale.Page=MenuPage::Career;break;
        case 2:stale.LocalDriver=false;break;
        case 3:stale.FocusSuspended=true;break;
        case 4:++stale.Revision;break;
        }
        const auto hit=Hit(shown,stale,0);
        Require(hit.Consumed && hit.Destination==MenuPage::Closed,"stale receipt cannot navigate");
    }
    const auto& card=layout.Cards[0].Bounds;
    auto hit=HitPresentedMainMenuDashboard(shown,current,layout.Width+1,layout.Height,
        card.X+1,card.Y+1);
    Require(hit.Consumed && hit.Destination==MenuPage::Closed,"resize revokes target");
    hit=HitPresentedMainMenuDashboard(shown,current,layout.Width,layout.Height,
        layout.Header.X+1,layout.Header.Y+1);
    Require(hit.Consumed && hit.Destination==MenuPage::Closed,"header not clickable");
    hit=HitPresentedMainMenuDashboard(shown,current,layout.Width,layout.Height,0,0);
    Require(!hit.Consumed,"outside panel uses established dispatch");
    hit=HitPresentedMainMenuDashboard(shown,current,layout.Width,layout.Height,
        std::numeric_limits<double>::quiet_NaN(),0);
    Require(!hit.Consumed,"invalid pointer is not an action");
    shown.Valid=false;
    Require(!Hit(shown,current,0).Consumed,"cleared presentation owns no input");
}

void CorruptLayoutRejection()
{
    const auto valid=View().Plan();
    for(int fault=0;fault<10;++fault)
    {
        auto bad=valid;
        switch(fault)
        {
        case 0:bad.Cards[0].Destination=MenuPage::Garage;break;
        case 1:bad.Cards[0].Title=bad.Cards[0].Description;break;
        case 2:bad.Cards[1].Bounds=bad.Cards[0].Bounds;break;
        case 3:bad.Cards[0].Bounds.Height=1;break;
        case 4:bad.Scale=std::numeric_limits<double>::infinity();break;
        case 5:bad.Columns=0;break;
        case 6:bad.NextDetailY=bad.Panel.Y;break;
        case 7:bad.ReservedRows=0;break;
        case 8:bad.Header=bad.Cards[0].Bounds;break;
        case 9:bad.Width=1;break;
        }
        Require(!ValidMainMenuDashboardLayout(bad),"malformed presented geometry rejected");
        const auto shown=Presented(bad);
        const auto hit=Hit(shown,Snapshot(),0);
        Require(hit.Destination==MenuPage::Closed,"malformed card cannot navigate");
    }
}

void RandomViewportAndNavigation()
{
    std::mt19937 rng(727193);
    int visible=0;
    for(int n=0;n<8000;++n)
    {
        const View v(240+rng()%4000,240+rng()%2200);
        const int pinned=static_cast<int>(rng()%3);
        const auto layout=v.Plan(pinned);
        if(!layout.Valid)continue;
        ++visible;
        CheckLayout(v,layout,pinned);
        auto shown=Presented(layout);
        auto snapshot=shown.Snapshot;
        const int index=static_cast<int>(rng()%MainMenuDestinationCount);
        const auto admitted=Hit(shown,snapshot,index);
        Require(admitted.Destination!=MenuPage::Closed,"current card admitted");
        snapshot.Page=admitted.Destination;
        ++snapshot.Revision;
        Require(Hit(shown,snapshot,index).Destination==MenuPage::Closed,
            "same physical press cannot navigate from destination");
    }
    Require(visible>3000,"broad desktop coverage");
}
} // namespace

int main()
{
    ExplicitDestinations();
    ResponsiveAndFallback();
    InputAdmissionAndStaleReceipts();
    CorruptLayoutRejection();
    RandomViewportAndNavigation();
    std::cout<<"Main menu responsive layout and direct navigation tests passed\n";
}
