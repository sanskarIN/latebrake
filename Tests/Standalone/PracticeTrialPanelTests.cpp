#include "UI/PracticeTrialPanel.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

namespace
{
using namespace latebrake;
void Require(bool condition,const char* why)
{
    if(!condition){std::cerr<<"PracticeTrialPanel: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}
PracticeTrialPanelLayout Layout(double w,double h)
{
    const auto hud=CalculateHudLayout(w,h);
    const auto dock=CalculateDesktopMenuMouseLayout(w,h,hud,false,true);
    if(!dock.Valid)return {};
    const HudMenuReadablePanel text{true,hud.Left,hud.Top,
        dock.Left-hud.Left-12*hud.Scale,h-hud.Top-20,40};
    return CalculatePracticeTrialPanel(CalculateQuickRaceChoicePanel(w,h,hud,dock,text,
        false,MenuPage::QuickRace),text);
}
PracticeTrialPanelSnapshot Snapshot()
{
    return {{true,MenuPage::QuickRace,true,false,false,false,4,1,2},
        false,true,{2,1,3},false,{true,{3,1,3},PracticeExperimentReason::Untested,true,{1,1,3},60}};
}
PracticeTrialPanelHit Hit(const PracticeTrialPanelPresentation& shown,
    const PracticeTrialPanelSnapshot& now,std::size_t i)
{
    const auto r=shown.Layout.Controls[i].Bounds;
    return HitPresentedPracticeTrial(shown,now,shown.Layout.Width,shown.Layout.Height,
        r.X+r.Width/2,r.Y+r.Height/2);
}
void ButtonsAndPermissions()
{
    const auto plan=Layout(1920,1200);
    Require(plan.Valid,"large desktop layout");
    auto now=Snapshot();
    PracticeTrialPanelPresentation shown{true,plan,now,{}};
    shown.Rendered.fill(true);
    for(std::size_t i=0;i<plan.Controls.size();++i)
    {
        auto hit=Hit(shown,now,i);
        Require(hit.Consumed && hit.Accepted==(i!=7),"all enabled except already selected car");
        Require(hit.Action==plan.Controls[i].Action && hit.Vehicle==plan.Controls[i].Vehicle,
            "exact visible action");
        auto stale=now;++stale.Choice.ChoiceRevision;
        Require(Hit(shown,stale,i).Consumed && !Hit(shown,stale,i).Accepted,"stale revision consumed");
        stale=now;++stale.Choice.PageRevision;
        Require(!Hit(shown,stale,i).Accepted,"stale page");
        stale=now;stale.Key.Engine=2;
        Require(!Hit(shown,stale,i).Accepted,"changed actual tuning");
        stale=now;stale.Owned=true;
        Require(!Hit(shown,stale,i).Accepted,"changed caption");
        stale=now;stale.Experiments.Next.Engine=2;
        Require(!Hit(shown,stale,i).Accepted,"changed recommended target");
        stale=now;stale.ManualPause=true;
        Require(!Hit(shown,stale,i).Accepted,"manual pause");
        stale=now;stale.Choice.FocusSuspended=true;
        Require(!Hit(shown,stale,i).Accepted,"focus lost");
        stale=now;stale.Choice.Selected=0;
        Require(!Hit(shown,stale,i).Accepted,"not practice");
        stale=now;stale.Choice.LocalPlayer=false;
        Require(!Hit(shown,stale,i).Accepted,"not local");
        shown.Rendered[i]=false;
        Require(!Hit(shown,now,i).Accepted,"caption must be visible");
        shown.Rendered[i]=true;
    }
    now.TrialEnabled=false;shown.Snapshot=now;
    for(std::size_t i=0;i<plan.Controls.size();++i)
        Require(Hit(shown,now,i).Accepted==(i==0),"disabled trial exposes only enable");
    now.TrialEnabled=true;shown.Snapshot=now;
    now.Choice.ReadOnlyProfile=true;shown.Snapshot=now;
    Require(Hit(shown,now,3).Accepted,"read-only profile permits temporary tuning");
    const auto rect=plan.Controls[0].Bounds;
    const auto resized=HitPresentedPracticeTrial(shown,now,plan.Width+1,plan.Height,
        rect.X+1,rect.Y+1);
    Require(resized.Consumed && !resized.Accepted,"resize rejects stale pointer");
    const auto header=plan.Header;
    const auto blank=HitPresentedPracticeTrial(shown,now,plan.Width,plan.Height,
        header.X+1,header.Y+1);
    Require(blank.Consumed && !blank.Accepted,"header is informational");
    Require(!HitPresentedPracticeTrial(shown,now,plan.Width,plan.Height,0,0).Consumed,"outside");
}
void MalformedPlans()
{
    const auto plan=Layout(1920,1200);
    auto now=Snapshot();
    PracticeTrialPanelPresentation shown{true,plan,now,{}};
    shown.Rendered.fill(true);
    for(int fault=0;fault<9;++fault)
    {
        auto bad=shown;
        switch(fault)
        {
        case 0:bad.Layout.Controls[0].Bounds.Width=0;break;
        case 1:bad.Layout.Controls[1].Bounds=bad.Layout.Controls[0].Bounds;break;
        case 2:bad.Layout.Controls[5].Vehicle=99;break;
        case 3:bad.Layout.Controls[3].Action=PracticeTrialAction::Toggle;break;
        case 4:bad.Layout.Header=bad.Layout.Controls[0].Bounds;break;
        case 5:bad.Layout.Summary=bad.Layout.Header;break;
        case 6:bad.Layout.Controls[10].Bounds.X=-1;break;
        case 7:bad.Layout.Scale=std::numeric_limits<double>::infinity();break;
        case 8:bad.Layout.Controls[0].Vehicle=0;break;
        }
        Require(!ValidPracticeTrialPanelLayout(bad.Layout),"malformed plan rejected");
        const auto r=plan.Controls[2].Bounds;
        const auto hit=HitPresentedPracticeTrial(bad,now,plan.Width,plan.Height,
            r.X+1,r.Y+1);
        Require(!hit.Accepted,"malformed plan cannot authorize action");
    }
    Require(!Layout(800,600).Valid && !Layout(1280,600).Valid,"small fallback");
    Require(!Layout(std::numeric_limits<double>::quiet_NaN(),1200).Valid,"invalid viewport");
}
void ResponsiveCoverage()
{
    std::mt19937 rng(12833);
    int shown=0;
    for(int i=0;i<10000;++i)
    {
        const double w=240+static_cast<double>(rng()%3500);
        const double h=240+static_cast<double>(rng()%2200);
        const auto layout=Layout(w,h);
        if(!layout.Valid)continue;
        ++shown;
        Require(ValidPracticeTrialPanelLayout(layout),"all generated plans validate");
        const auto hud=CalculateHudLayout(w,h);
        const auto dock=CalculateDesktopMenuMouseLayout(w,h,hud,false,true);
        const HudMenuReadablePanel text{true,hud.Left,hud.Top,
            dock.Left-hud.Left-12*hud.Scale,h-hud.Top-20,40};
        const auto choice=CalculateQuickRaceChoicePanel(w,h,hud,dock,text,false,MenuPage::QuickRace);
        Require(!SettingsRectsOverlap(layout.Panel,choice.Panel),"mode cards stay visible");
        Require(!SettingsRectsOverlap(layout.Panel,{text.X,text.Y,text.Width,text.Height}),"text stays readable");
        for(const auto& control:layout.Controls)
        {
            for(const auto& button:dock.Buttons)
                Require(!SettingsRectsOverlap(control.Bounds,button.Bounds),"no navigation collisions");
            Require(ContainsSettingsRect({0,0,w,h},control.Bounds),"all controls inside viewport");
        }
    }
    Require(shown>100,"nontrivial layout coverage");
}
}
int main()
{
    ButtonsAndPermissions();
    MalformedPlans();
    ResponsiveCoverage();
    std::cout<<"Practice trial panel tests passed\n";
}
