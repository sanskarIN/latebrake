#include "UI/HudTelemetryPriority.h"
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

namespace
{
using namespace latebrake;
void Require(bool ok,const char* why)
{
    if(!ok){std::cerr<<"HudTelemetryPriority: "<<why<<'\n';std::exit(EXIT_FAILURE);}
}

void Modes()
{
    Require(NextHudTelemetryMode(HudTelemetryMode::Focused)==HudTelemetryMode::Balanced &&
        NextHudTelemetryMode(HudTelemetryMode::Balanced)==HudTelemetryMode::Detailed &&
        NextHudTelemetryMode(HudTelemetryMode::Detailed)==HudTelemetryMode::Focused,
        "three modes");
    Require(NextHudTelemetryMode(static_cast<HudTelemetryMode>(99))==HudTelemetryMode::Balanced,
        "invalid mode fallback");
    for(int mask=0;mask<16;++mask)
        Require(CanCycleHudTelemetryMode(mask&1,mask&2,mask&4,mask&8)==(mask==7),
            "local focused Settings only");
    for(unsigned i=0;i<static_cast<unsigned>(HudTelemetryKind::Count);++i)
    {
        const auto kind=static_cast<HudTelemetryKind>(i);
        Require(HudTelemetryAdmitted(HudTelemetryMode::Detailed,kind),"detailed valid");
        Require(HudTelemetryAdmitted(HudTelemetryMode::Focused,kind)==HudTelemetryEssential(kind),
            "focused essentials");
        if(HudTelemetryEssential(kind))
            Require(HudTelemetryAdmitted(HudTelemetryMode::Balanced,kind),"balanced essentials");
    }
    Require(!HudTelemetryAdmitted(HudTelemetryMode::Detailed,HudTelemetryKind::Count),"invalid kind");
}

void LateAlertsAndStableOrder()
{
    HudTelemetryQueue q;
    Require(q.Add(0,HudTelemetryKind::Decorative),"title");
    Require(q.Add(1,HudTelemetryKind::Analysis),"diagnostics");
    Require(q.Add(2,HudTelemetryKind::Vehicle),"engine");
    Require(q.Add(3,HudTelemetryKind::Clock),"clock");
    Require(q.Add(4,HudTelemetryKind::Progress),"gate");
    Require(q.Add(5,HudTelemetryKind::Speed),"speed");
    Require(q.Add(6,HudTelemetryKind::Urgent),"wrong way");
    Require(q.Add(7,HudTelemetryKind::Urgent),"traffic");
    Require(q.Add(8,HudTelemetryKind::Blocking),"track unavailable");
    Require(q.Add(9,HudTelemetryKind::Navigation),"route");
    Require(q.Add(10,HudTelemetryKind::Pace),"split");
    std::vector<int> drawn;
    const auto r=DrawSelectedHudTelemetry(q.Select(HudTelemetryMode::Detailed,7),
        [&](int i){drawn.push_back(i);return true;});
    Require(drawn==std::vector<int>({8,3,4,5,6,7,9}),"late alerts before optional");
    Require(r.Drawn==7 && r.EssentialDrawn==7 && r.OptionalDrawn==0 &&
        r.CapacityHidden==4,"exact budget");
    drawn.clear();
    DrawSelectedHudTelemetry(q.Select(HudTelemetryMode::Detailed,1),
        [&](int i){drawn.push_back(i);return true;});
    Require(drawn==std::vector<int>({8}),"blocking row on tiny screen");
    const auto focused=q.Select(HudTelemetryMode::Focused,80);
    Require(focused.EssentialCount==7 && focused.SuppressedByMode==4 &&
        focused.EligibleOptionalCount==0,"focused does not hide alerts");
    const auto balanced=q.Select(HudTelemetryMode::Balanced,80);
    Require(balanced.EssentialCount==7 && balanced.SuppressedByMode==2 &&
        balanced.EligibleOptionalCount==2,"balanced limits diagnostics");
}

void ActualFontBackfill()
{
    HudTelemetryQueue q;
    for(int i=0;i<40;++i)Require(q.Add(i,HudTelemetryKind::Vehicle),"optional");
    Require(q.Add(40,HudTelemetryKind::Clock),"late clock");
    Require(q.Add(41,HudTelemetryKind::Urgent),"late alert");
    Require(q.Add(42,HudTelemetryKind::Progress),"late gate");
    std::vector<int> drawn;
    auto r=DrawSelectedHudTelemetry(q.Select(HudTelemetryMode::Balanced,80),[&](int i)
    {
        if(i==40 || i<5)return false;
        drawn.push_back(i);return true;
    });
    Require(r.EssentialDrawn==2 && r.OptionalDrawn==10 && r.Unfittable==6 &&
        r.Drawn==12 && r.OptionalHidden==25,"actual successful fits count");
    Require(drawn[0]==42 && drawn[1]==41 && drawn[2]==5 && drawn.back()==14,
        "backfill after invalid fonts");
    drawn.clear();
    r=DrawSelectedHudTelemetry(q.Select(HudTelemetryMode::Detailed,3),[&](int i)
    {
        if(i==40 || i==0)return false;
        drawn.push_back(i);return true;
    });
    Require(drawn==std::vector<int>({42,41,1}) && r.Unfittable==2,"successful rows only");
}

void BoundsAndInvalidSelections()
{
    HudTelemetryQueue q;
    Require(!q.Add(-1,HudTelemetryKind::Clock) &&
        !q.Add(160,HudTelemetryKind::Clock) &&
        !q.Add(0,HudTelemetryKind::Count),"malformed candidates");
    for(int i=0;i<HudTelemetryCandidateCapacity;++i)
        Require(q.Add(i,HudTelemetryKind::Analysis),"bounded collection");
    Require(!q.Add(3,HudTelemetryKind::Urgent),"duplicate or full");
    Require(!q.Select(HudTelemetryMode::Detailed,0).Valid &&
        !q.Select(HudTelemetryMode::Detailed,81).Valid,"bad row budget");
    const auto s=q.Select(HudTelemetryMode::Detailed,80);
    Require(s.Valid && s.CandidateCount==160 && s.Refused==4,"bounded selection");
    auto r=DrawSelectedHudTelemetry(s,[](int){return true;});
    Require(r.Drawn==80 && r.CapacityHidden==80,"absolute maximum");
    auto bad=s;bad.CandidateCount=161;
    Require(DrawSelectedHudTelemetry(bad,[](int){return true;}).Drawn==0,"overflow");
    bad=s;bad.OptionalBudget=81;
    Require(DrawSelectedHudTelemetry(bad,[](int){return true;}).Drawn==0,"invalid optional");
    bad=s;bad.Candidates[1]=bad.Candidates[0];
    int calls=0;
    Require(DrawSelectedHudTelemetry(bad,[&](int){++calls;return true;}).Drawn==0 &&
        calls==0,"duplicate selection fails before side effects");
    bad=s;bad.Candidates[159].Kind=HudTelemetryKind::Clock;
    Require(DrawSelectedHudTelemetry(bad,[&](int){++calls;return true;}).Drawn==0 &&
        calls==0,"out-of-order selection fails before draw");
}

void RandomViewportAndMode()
{
    std::mt19937 rng(20771);
    for(int iteration=0;iteration<4000;++iteration)
    {
        const double width=240+rng()%3000,height=240+rng()%1800;
        const bool touch=rng()%2!=0;
        const auto plan=CalculateHudVerticalPlan(width,height,CalculateHudLayout(width,height),touch);
        if(!plan.Valid)continue;
        HudTelemetryQueue q;
        std::array<HudTelemetryKind,100> kinds{};
        for(int i=0;i<100;++i)
        {
            kinds[static_cast<std::size_t>(i)]=static_cast<HudTelemetryKind>(
                rng()%static_cast<unsigned>(HudTelemetryKind::Count));
            Require(q.Add(i,kinds[static_cast<std::size_t>(i)]),"random candidate");
        }
        const auto mode=static_cast<HudTelemetryMode>(rng()%3);
        const auto s=q.Select(mode,plan.MaxTelemetryLines);
        int previous=-1,lastIndex=-1;
        const auto r=DrawSelectedHudTelemetry(s,[&](int i)
        {
            if(i%7==0)return false;
            const auto kind=kinds[static_cast<std::size_t>(i)];
            Require(HudTelemetryAdmitted(mode,kind),"mode admission");
            const int priority=HudTelemetryPriority(kind);
            Require(priority>=previous,"priority order");
            if(priority==previous)Require(i>lastIndex,"stable order");
            previous=priority;lastIndex=i;
            return true;
        });
        Require(r.Drawn<=plan.MaxTelemetryLines &&
            r.OptionalDrawn<=HudTelemetryOptionalBudget(mode) &&
            r.Drawn==r.EssentialDrawn+r.OptionalDrawn,"viewport and mode budget");
        Require(r.Drawn+r.Unfittable+r.CapacityHidden+r.OptionalHidden==s.CandidateCount,
            "candidate conservation");
    }
}
}

int main()
{
    Modes();LateAlertsAndStableOrder();ActualFontBackfill();
    BoundsAndInvalidSelections();RandomViewportAndMode();
    std::cout<<"HUD priority, font backfill, mode and viewport tests passed\n";
}
