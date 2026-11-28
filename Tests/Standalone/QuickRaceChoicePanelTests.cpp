#include "UI/QuickRaceChoicePanel.h"

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
        std::cerr<<"QuickRaceChoicePanel: "<<why<<'\n';
        std::exit(EXIT_FAILURE);
    }
}
QuickRaceChoiceSnapshot Snapshot()
{
    return {true,MenuPage::QuickRace,true,false,false,false,0,1,1};
}
QuickRaceChoicePanelLayout Layout(double w,double h,bool touch=false,
    MenuPage page=MenuPage::QuickRace)
{
    const auto hud=CalculateHudLayout(w,h);
    const auto dock=CalculateDesktopMenuMouseLayout(w,h,hud,touch,true);
    if(!dock.Valid)return {};
    const HudMenuReadablePanel text{true,hud.Left,hud.Top,
        dock.Left-hud.Left-12*hud.Scale,h-hud.Top-20,40};
    return CalculateQuickRaceChoicePanel(w,h,hud,dock,text,touch,page);
}

void ChoicesAndDescriptions()
{
    auto snapshot=Snapshot();
    auto choice=QuickRaceChoiceAt(0);
    for(int i=0;i<QuickRaceChoiceCount;++i)
    {
        Require(QuickRaceChoiceIndex(choice)==i &&
            QuickRaceChoiceIndex(QuickRaceChoiceAt(i))==i,"direct choices match browse cycle");
        const auto desc=DescribeQuickRaceChoice(snapshot,i);
        Require(desc.Valid && desc.Enabled && desc.Selected==(i==0) &&
            desc.Practice==(i>=4) && desc.ProfileRecordsPossible==(i<4),"mode disclosure");
        const int laps[]={3,3,1,3,3,5,5,10};
        const int rivals[]={1,1,0,1,0,0,1,1};
        Require(desc.Laps==laps[i] && desc.RequestedRivals==rivals[i] &&
            std::string_view(desc.Title).size()>0 && std::string_view(desc.Status).size()>0,
            "actual event dimensions and labels");
        choice=NextQuickRaceMenuChoice(choice);
    }
    Require(QuickRaceChoiceIndex(choice)==0,"complete cycle");
    snapshot.ExperimentalFleetFlag=true;
    for(int i=0;i<8;++i)
    {
        const auto desc=DescribeQuickRaceChoice(snapshot,i);
        const bool fleet=i==0 || i==1 || i==3;
        Require(desc.RequestedRivals==(fleet?5:(i==6||i==7?1:0)) &&
            desc.ProfileRecordsPossible==(i==2),"fleet flag does not affect practice or solo");
        if(fleet)Require(std::string_view(desc.Status)=="FLEET TEST","explicit test-only disclosure");
    }
    snapshot.ExperimentalFleetFlag=false;
    snapshot.ReadOnlyProfile=true;
    for(int i=0;i<8;++i)
    {
        const auto desc=DescribeQuickRaceChoice(snapshot,i);
        Require(!desc.ProfileRecordsPossible && desc.Enabled,"read-only still allows inspection");
        Require(std::string_view(desc.Status)==(i<4?"READ ONLY":"SESSION ONLY"),"actual persistence boundary");
    }
    snapshot.FocusSuspended=true;
    Require(!snapshot.CanSelect() && !DescribeQuickRaceChoice(snapshot,0).Enabled &&
        std::string_view(DescribeQuickRaceChoice(snapshot,0).Status)=="INPUT LOCKED","focus guard");
    Require(!IsValidQuickRaceMenuChoice(QuickRaceChoiceAt(-1)) &&
        !IsValidQuickRaceMenuChoice(QuickRaceChoiceAt(8)) &&
        !DescribeQuickRaceChoice(snapshot,99).Valid,"invalid indexes");
}

void LayoutAndHits()
{
    auto layout=Layout(1280,720);
    Require(layout.Valid && ValidQuickRaceChoiceLayout(layout),"desktop layout");
    auto snapshot=Snapshot();
    QuickRaceChoicePresentation shown;
    shown.Valid=true;shown.Layout=layout;shown.Snapshot=snapshot;shown.Rendered.fill(true);
    for(int i=0;i<8;++i)
    {
        const auto& b=layout.Cards[static_cast<std::size_t>(i)].Bounds;
        const double x=b.X+b.Width/2,y=b.Y+b.Height/2;
        auto hit=HitPresentedQuickRaceChoice(shown,snapshot,1280,720,x,y);
        Require(hit.Consumed && hit.Index==i,"exact visible card");
        auto stale=snapshot;++stale.ChoiceRevision;
        hit=HitPresentedQuickRaceChoice(shown,stale,1280,720,x,y);
        Require(hit.Consumed && hit.Index==-1,"stale pending choice blocked");
        stale=snapshot;++stale.PageRevision;
        Require(HitPresentedQuickRaceChoice(shown,stale,1280,720,x,y).Index==-1,"stale page");
        stale=snapshot;stale.Selected=(i+1)%8;
        Require(HitPresentedQuickRaceChoice(shown,stale,1280,720,x,y).Index==-1,"changed selection");
        stale=snapshot;stale.FocusSuspended=true;
        Require(HitPresentedQuickRaceChoice(shown,stale,1280,720,x,y).Index==-1,"focus lost");
        stale=snapshot;stale.LocalPlayer=false;
        Require(HitPresentedQuickRaceChoice(shown,stale,1280,720,x,y).Index==-1,"local player lost");
        stale=snapshot;stale.Page=MenuPage::Garage;
        Require(HitPresentedQuickRaceChoice(shown,stale,1280,720,x,y).Index==-1,"page changed");
        stale=snapshot;stale.ReadOnlyProfile=true;
        Require(HitPresentedQuickRaceChoice(shown,stale,1280,720,x,y).Index==-1,"caption state changed");
        stale=snapshot;stale.ExperimentalFleetFlag=true;
        Require(HitPresentedQuickRaceChoice(shown,stale,1280,720,x,y).Index==-1,"fleet status changed");
        Require(HitPresentedQuickRaceChoice(shown,snapshot,1281,720,x,y).Index==-1,"resize invalidates");
        shown.Rendered[static_cast<std::size_t>(i)]=false;
        Require(HitPresentedQuickRaceChoice(shown,snapshot,1280,720,x,y).Index==-1,"unrendered caption cannot select");
        shown.Rendered[static_cast<std::size_t>(i)]=true;
    }
    const auto header=layout.Header;
    auto hit=HitPresentedQuickRaceChoice(shown,snapshot,1280,720,header.X+1,header.Y+1);
    Require(hit.Consumed && hit.Index==-1,"header not selectable");
    Require(!HitPresentedQuickRaceChoice(shown,snapshot,1280,720,0,0).Consumed,"outside not consumed");
    Require(!Layout(1280,720,true).Valid && !Layout(800,600).Valid &&
        !Layout(1280,720,false,MenuPage::Settings).Valid,"fallback to original controls");
    auto bad=shown;
    bad.Layout.Cards[7].Bounds=bad.Layout.Cards[0].Bounds;
    Require(!ValidQuickRaceChoiceLayout(bad.Layout),"overlap malformed");
    const auto b=layout.Cards[0].Bounds;
    Require(HitPresentedQuickRaceChoice(bad,snapshot,1280,720,b.X+1,b.Y+1).Index==-1,
        "entire malformed plan denied");
    bad=shown;bad.Layout.Cards[0].Index=99;
    Require(!ValidQuickRaceChoiceLayout(bad.Layout),"malformed card index");
    bad=shown;bad.Layout.Cards[0].Title.Width=std::numeric_limits<double>::infinity();
    Require(!ValidQuickRaceChoiceLayout(bad.Layout),"nonfinite caption geometry");
    Require(!HitPresentedQuickRaceChoice(shown,snapshot,1280,720,
        std::numeric_limits<double>::quiet_NaN(),0).Consumed,"nonfinite hit");
}

void ResponsiveSweep()
{
    std::mt19937 rng(46280);
    int visible=0;
    for(int i=0;i<10000;++i)
    {
        const double width=240+static_cast<double>(rng()%3600);
        const double height=240+static_cast<double>(rng()%2000);
        const auto plan=Layout(width,height);
        if(!plan.Valid)continue;
        ++visible;
        Require(ValidQuickRaceChoiceLayout(plan),"all generated plans independently validate");
        const auto hud=CalculateHudLayout(width,height);
        const auto dock=CalculateDesktopMenuMouseLayout(width,height,hud,false,true);
        Require(dock.Valid,"real navigation dock exists");
        for(const auto& card:plan.Cards)
        {
            Require(card.Bounds.Width>=96 && card.Bounds.Height>=44,"minimum click target");
            for(const auto& button:dock.Buttons)
                Require(!SettingsRectsOverlap(card.Bounds,button.Bounds),"no navigation overlap");
            Require(card.Bounds.X>=dock.Left &&
                ContainsSettingsRect({0,0,width,height},card.Bounds),"right lane viewport safe");
        }
    }
    Require(visible>100,"nontrivial responsive coverage");
}
}
int main()
{
    ChoicesAndDescriptions();
    LayoutAndHits();
    ResponsiveSweep();
    std::cout<<"Quick Race direct choice panel tests passed\n";
}
